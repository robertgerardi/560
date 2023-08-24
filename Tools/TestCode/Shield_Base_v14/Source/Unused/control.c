#include <MKL25Z4.H>
#include <stdio.h>
#include <stdint.h>

#include "gpio_defs.h"
#include "debug.h"

#include "control.h"

#include "timers.h"
#include "delay.h"
#include "LEDs.h"
#include "UI.h"

#include "FX.h"

// volatile __align(4) int16_t g_duty_cycle=5;  // global to give debugger access
volatile int g_duty_cycle=5;  // global to give debugger access

volatile int g_enable_flash=0;
volatile int g_peak_set_current=FLASH_CURRENT_MA; // Peak flash current
volatile int g_flash_duration=FLASH_DURATION_MS;
volatile int g_flash_period=FLASH_PERIOD_MS; 

volatile int g_enable_control=1;
volatile int g_set_current=0; 				// Default starting LED current
volatile uint16_t g_set_current_sample=0; 	// Default starting LED current DAC val

volatile int g_measured_current;
volatile int error;

int32_t pGain_8 = PGAIN_8; // proportional gain numerator scaled by 2^8

volatile __align(256) uint16_t g_set_sample[NUM_CURR_SAMPLES];
volatile __align(256) uint16_t g_meas_sample[NUM_CURR_SAMPLES];
volatile int g_trigger_pt=TRIG_ARMED;
volatile uint8_t g_disp_buffer=0;

SPid plantPID = {0, // dState
	0, // iState
	LIM_DUTY_CYCLE, // iMax
	-LIM_DUTY_CYCLE, // iMin
	P_GAIN_FL, // pGain
	I_GAIN_FL, // iGain
	D_GAIN_FL  // dGain
};

SPidFX plantPID_FX = {FL_TO_FX(0), // dState
	FL_TO_FX(0), // iState
	FL_TO_FX(LIM_DUTY_CYCLE), // iMax
	FL_TO_FX(-LIM_DUTY_CYCLE), // iMin
	P_GAIN_FX, // pGain
	I_GAIN_FX, // iGain
	D_GAIN_FX  // dGain
};

float UpdatePID(SPid * pid, float error, float position){
	float pTerm, dTerm, iTerm;

	// calculate the proportional term
	pTerm = pid->pGain * error;
	// calculate the integral state with appropriate limiting
	pid->iState += error;
	if (pid->iState > pid->iMax) 
		pid->iState = pid->iMax;
	else if (pid->iState < pid->iMin) 
		pid->iState = pid->iMin;
	iTerm = pid->iGain * pid->iState; // calculate the integral term
	dTerm = pid->dGain * (position - pid->dState);
	pid->dState = position;

	return pTerm + iTerm - dTerm;
}

FX16_16 UpdatePID_FX(SPidFX * pid, FX16_16 error_FX, FX16_16 position_FX){
	FX16_16 pTerm, dTerm, iTerm, diff, ret_val;

	// calculate the proportional term
	pTerm = Multiply_FX(pid->pGain, error_FX);

	// calculate the integral state with appropriate limiting
	pid->iState = Add_FX(pid->iState, error_FX);
	if (pid->iState > pid->iMax) 
		pid->iState = pid->iMax;
	else if (pid->iState < pid->iMin) 
		pid->iState = pid->iMin;
	
	iTerm = Multiply_FX(pid->iGain, pid->iState); // calculate the integral term
	diff = Subtract_FX(position_FX, pid->dState);
	dTerm = Multiply_FX(pid->dGain, diff);
	pid->dState = position_FX;

	ret_val = Add_FX(pTerm, iTerm);
	ret_val = Subtract_FX(ret_val, dTerm);
	return ret_val;
}

void Control_HBLED(void) {
	uint16_t res;
	FX16_16 change_FX, error_FX;
	static int curr_sample=0;
	static uint16_t prev_set_current_sample=0;
	int buff_size, threshold;
	
	DEBUG_START(DBG_CONTROLLER);
	
#if USE_ADC_INTERRUPT
	// already completed conversion, so don't wait
#else
	while (!(ADC0->SC1[0] & ADC_SC1_COCO_MASK))
		; // wait until end of conversion
#endif
	res = ADC0->R[0];

	g_measured_current = (res*1500)>>16; // Extra Credit: Make this code work: V_REF_MV*MA_SCALING_FACTOR)/(ADC_FULL_SCALE*R_SENSE)

	// reset the display trigger if we've been re-armed.
	if (g_trigger_pt == TRIG_PEND) {
		g_disp_buffer=0;
		curr_sample=0;
	}

	// only collect data if not currently updating display
	if (!g_disp_buffer) {
		// capture data in circular buffers until they're full
		g_set_sample[curr_sample] =g_set_current_sample;
		g_meas_sample[curr_sample]=res;

		// Arm the trigger if pending and set point == zero
		//if ((g_trigger_pt == TRIG_PEND) && (g_set_current_sample < prev_set_current_sample))
		if ((g_trigger_pt == TRIG_PEND) && (g_set_current_sample == 0))
			g_trigger_pt=TRIG_ARMED;

#if 0 // original code
		// Trigger on set point low-to-high if armed.
		if ((g_trigger_pt == TRIG_ARMED) && 													/* Check if we're armed */
				(g_set_current_sample > prev_set_current_sample) &&     	/* check for rising edge */
				(g_set_current > (g_peak_set_current/2) ) ) 							/* Check for threshold */
				g_trigger_pt=curr_sample;
#else
		threshold = g_peak_set_current>>3;
		// Trigger on set point low-to-high past threshold if armed.
		if ((g_trigger_pt == TRIG_ARMED) && 													/* Check if we're armed */
				(g_set_current_sample >= threshold) &&
				(prev_set_current_sample < threshold) ) 							/* Check for threshold */
				g_trigger_pt=curr_sample;
#endif
		
		// Start buffer display if we're not already displaying we've triggered AND we've a full buffer.
		buff_size=((curr_sample + g_trigger_pt + g_holdoff) % NUM_CURR_SAMPLES) + 1;
		if ((g_trigger_pt >= 0) && (buff_size==NUM_CURR_SAMPLES))	
			g_disp_buffer=1;
		curr_sample=(curr_sample+1)%NUM_CURR_SAMPLES;
	}

	prev_set_current_sample = g_set_current_sample;
	
	if (g_enable_control) {
		switch (control_mode) {
			case OpenLoop:
					// don't do anything!
				break;
			case BangBang:
				if (g_measured_current < g_set_current)
					g_duty_cycle = LIM_DUTY_CYCLE;
				else
					g_duty_cycle = 0;
				break;
			case Incremental:
				if (g_measured_current < g_set_current)
					g_duty_cycle += INC_STEP;
				else
					g_duty_cycle -= INC_STEP;
				break;
			case Proportional:
				g_duty_cycle += (pGain_8*(g_set_current - g_measured_current))/256; //  - 1;
			break;
			case PID:
				g_duty_cycle += UpdatePID(&plantPID, g_set_current - g_measured_current, g_measured_current);
				break;
			case PID_FX:
				error_FX = INT_TO_FX(g_set_current - g_measured_current);
				change_FX = UpdatePID_FX(&plantPID_FX, error_FX, INT_TO_FX(g_measured_current));
				g_duty_cycle += FX_TO_INT(change_FX);
			break;
			default:
				break;
		}
	
		// Update PWM controller with duty cycle
		if (g_duty_cycle < 0)
			g_duty_cycle = 0;
		else if (g_duty_cycle > LIM_DUTY_CYCLE)
			g_duty_cycle = LIM_DUTY_CYCLE;
		PWM_Set_Value(TPM0, PWM_HBLED_CHANNEL, g_duty_cycle);
	} // if g_enable_control
	
	// Increment sample index and wrap at NUM_CURR_SAMPLES
	DEBUG_STOP(DBG_CONTROLLER);
}

#if USE_ADC_INTERRUPT
#if !USE_ADC_SERVER        // moved to ADC.c to keep everything together.
void ADC0_IRQHandler() {
	FPTB->PSOR = MASK(DBG_IRQ_ADC);
	Control_HBLED();
	FPTB->PCOR = MASK(DBG_IRQ_ADC);
}
#endif
#endif

void Set_DAC(unsigned int code) {
	// Force 16-bit write to DAC
	uint16_t * dac0dat = (uint16_t *)&(DAC0->DAT[0].DATL);
	*dac0dat = (uint16_t) code;
}

void Set_DAC_mA(unsigned int current) {
	unsigned int code = MA_TO_DAC_CODE(current);
	// Force 16-bit write to DAC
	uint16_t * dac0dat = (uint16_t *)&(DAC0->DAT[0].DATL);
	*dac0dat = (uint16_t) code;
	g_set_current_sample=(0xFFFF) & ((code)<<8);		// normalize 10-bit DAC values to 16-bit ADC values;
}

void Init_DAC_HBLED(void) {
  // Enable clock to DAC and Port E
	SIM->SCGC6 |= SIM_SCGC6_DAC0_MASK;
	SIM->SCGC5 |= SIM_SCGC5_PORTE_MASK;
	
	// Select analog for pin
	PORTE->PCR[DAC_POS] &= ~PORT_PCR_MUX_MASK;
	PORTE->PCR[DAC_POS] |= PORT_PCR_MUX(0);	
		
	// Disable buffer mode
	DAC0->C1 = 0;
	DAC0->C2 = 0;
	
	// Enable DAC, select VDDA as reference voltage
	DAC0->C0 = DAC_C0_DACEN_MASK | DAC_C0_DACRFS_MASK;
	Set_DAC(0);
}

void Init_ADC_HBLED(void) {
	// Configure ADC to read Ch 8 (FPTB 0)
	SIM->SCGC6 |= SIM_SCGC6_ADC0_MASK; 
	ADC0->CFG1 = 0x0C; // 16 bit
	//	ADC0->CFG2 = ADC_CFG2_ADLSTS(3);
	ADC0->SC2 = ADC_SC2_REFSEL(0);

#if USE_ADC_HW_TRIGGER
	// Enable hardware triggering of ADC
	ADC0->SC2 |= ADC_SC2_ADTRG(1);
	// Select triggering by TPM0 Overflow
	SIM->SOPT7 = SIM_SOPT7_ADC0TRGSEL(8) | SIM_SOPT7_ADC0ALTTRGEN_MASK;
	// Select input channel 
	ADC0->SC1[0] &= ~ADC_SC1_ADCH_MASK;
	ADC0->SC1[0] |= ADC_SC1_ADCH(ADC_SENSE_CHANNEL);
#endif // USE_ADC_HW_TRIGGER

#if USE_ADC_INTERRUPT 

	// delay enabling the interrupt until the server thread is running.
#if !USE_ADC_SERVER
	// enable ADC interrupt
	ADC0->SC1[0] |= ADC_SC1_AIEN(1);
#endif
	// Configure NVIC for ADC interrupt
	NVIC_SetPriority(ADC0_IRQn, 2); 
	NVIC_ClearPendingIRQ(ADC0_IRQn); 
	NVIC_EnableIRQ(ADC0_IRQn);	
#endif // USE_ADC_INTERRUPT
}

void Update_Set_Current(void) {
	// Ramp curent up and down 
	static volatile int t=0;
	int phase;
	int t_ramp;
	
	if (g_enable_flash){
		t_ramp=g_flash_duration/2;
		t++;
		phase = (2*t)/g_flash_duration;
		switch (phase) {
			case 0:
				g_set_current = (t*g_peak_set_current)/t_ramp;
				break;
			case 1:
				g_set_current = ((2*t_ramp - t)*g_peak_set_current)/t_ramp;
				break;
			default:
				g_set_current = 0;
				break;
		}
		Set_DAC_mA(g_set_current);
		if (t >= g_flash_period)
			t = 0;
	}
}

void Init_Buck_HBLED(void) {
	Init_DAC_HBLED();
	Init_ADC_HBLED();
	
	// Configure driver for buck converter
	// Set up PTE31 to use for SMPS with TPM0 Ch 4
	SIM->SCGC5 |= SIM_SCGC5_PORTE_MASK;
	PORTE->PCR[31]  &= PORT_PCR_MUX(7);
	PORTE->PCR[31]  |= PORT_PCR_MUX(3);
	PWM_Init(TPM0, PWM_HBLED_CHANNEL, PWM_PERIOD, g_duty_cycle, 0, 0);
	
	// Set up PIT[1] to trigger ADC
}



void Control_DutyCycle_Handler(UI_FIELD_T * fld, int v) {
	int dc;
	if (fld->Val != NULL) {
		dc = g_duty_cycle + v/16;
		if (dc < 0)
			dc = 0;
		else if (dc > LIM_DUTY_CYCLE)
			dc = LIM_DUTY_CYCLE;
		*(fld->Val) = dc;
		PWM_Set_Value(TPM0, PWM_HBLED_CHANNEL, g_duty_cycle);
	}
}
