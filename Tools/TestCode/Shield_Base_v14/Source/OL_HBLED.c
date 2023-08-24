#include <MKL25Z4.H>
#include "OL_HBLED.h"
#include "timers.h"
#include "delay.h"
#include "ADC.h"

void Init_OL_HBLED(void) {
	// Configure driver for buck converter
	// Set up PTE31 to use for SMPS with TPM0 Ch 4
	SIM->SCGC5 |= SIM_SCGC5_PORTE_MASK;
	PORTE->PCR[31]  &= PORT_PCR_MUX(7);
	PORTE->PCR[31]  |= PORT_PCR_MUX(3);
	PWM_Init(TPM_HBLED, PWM_HBLED_CHANNEL, PWM_PERIOD, LIM_DUTY_CYCLE-1);
	
	// Also configure current sense input. 
	// Not really doing closed-loop control - just testing circuit operation
	// Select analog for pin B0 (ADC channel 8)
	PORTB->PCR[0] &= ~PORT_PCR_MUX_MASK;
	// PORTB->PCR[0] |= PORT_PCR_MUX(0);	// not needed, since 0
}



void Set_OL_HBLED_Pulse_Width(uint16_t pw) {
	if (pw > LIM_DUTY_CYCLE)
		pw = LIM_DUTY_CYCLE;
	PWM_Set_Value(TPM_HBLED, PWM_HBLED_CHANNEL, pw);
}

int Test_OL_HBLED(void) {
	// Returns current in mA through LED during testing
	int sum=0, res, d;
	
	// soft start
	for (int i=1; i<10; i++) {
		d = ((16 - i)*LIM_DUTY_CYCLE)>>4;
		Set_OL_HBLED_Pulse_Width(d);
		ShortDelay(500);
	}
	
	for (int i=0; i<TEST_LOOP_COUNT; i++) {
		// Measure voltage on current sense resistor
		// TODO: configure MUX select correctly, or make and use a standardized ADC conversion function?
		ADC0->CFG2 &= ~ADC_CFG2_MUXSEL_MASK;
		ADC0->CFG2 |= ADC_CFG2_MUXSEL(ADC_LED_I_SENSE_MUX);
		ADC0->SC1[0] = ADC_LED_I_SENSE_CHANNEL; // start conversion
		while (!(ADC0->SC1[0] & ADC_SC1_COCO_MASK))
			;
		res = (int) ADC0->R[0];
		// Accumulate
		sum += res;
		Delay(2);
	}
	// Drop to 0% duty cycle to turn off LED
	Set_OL_HBLED_Pulse_Width(15);

	return (sum*3300)/(2.2*65536*TEST_LOOP_COUNT);
}


