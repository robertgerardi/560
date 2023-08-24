#include <MKL25Z4.H>
#include <stdio.h>
#include <stdint.h>
#include <cmsis_os2.h>
#include "gpio_defs.h"
#include "debug.h"

#include "config.h"
#include "LCD_driver.h"
#include "timers.h"
#include "delay.h"
#include "LEDs.h"
#include "ADC.h"
#include "OL_HBLED.h"

osMutexId_t ADC_mutex;

const osMutexAttr_t ADC_mutex_attr = {
  "ADC_mutex",     // human readable mutex name
  osMutexPrioInherit    // attr_bits
};


/***************************************
 * ADC0_IRQHandler:
 *
 ****************************************/
void ADC0_IRQHandler() {
//	DEBUG_START(DBG_IRQ_ADC);

//	res.Sample=ADC0->R[0];        // first read the value in case we trigger right away.

//	DEBUG_STOP(DBG_IRQ_ADC);
}

void ADC_Create_OS_Objects(void) {
	ADC_mutex = osMutexNew(&ADC_mutex_attr);
}

int ADC_Read_Channel(int channel, int mux, int use_mutex) {
	int res;
	if (use_mutex) 
		osMutexAcquire(ADC_mutex,osWaitForever);

	// Select correct mux here
	ADC0->CFG2 &= ~ADC_CFG2_MUXSEL_MASK;
	ADC0->CFG2 |= ADC_CFG2_MUXSEL(mux);
	
	// Wait a little before starting conversion
	Delay(1);
	
	ADC0->SC1[0] = channel; // start conversion with specified channel
	while (!(ADC0->SC1[0] & ADC_SC1_COCO_MASK))
		;
	res = ADC0->R[0];
		
	if (use_mutex)
		osMutexRelease(ADC_mutex);
	
	return res;
}

int ADC_Read_Temperature(int channel, int mux) {
	int res;
	int i_mv_temp, i_mc_temp;
	volatile float fv_temp, fc_temp;
	
	res = ADC_Read_Channel(channel, mux, 1);
	switch (channel) {
		case ADC_MCU_THERMO_CHANNEL:
			i_mv_temp = (res * V_3V3)/65536;
			i_mc_temp = 25000 - (i_mv_temp - ADC_MCU_THERMO_MV_TEMP25)/ADC_MCU_THERMO_SLOPE_MV_C;
			break;
		case ADC_LED_THERMO_CHANNEL: // AD: See Thermistor Calculations v2.xlsx in Git Shield Manual directory
			fv_temp = (res * V_3V3)/65536;
			fc_temp = ADC_LED_THERMO_C0 + fv_temp*(ADC_LED_THERMO_C1 + fv_temp*(ADC_LED_THERMO_C2 
								+ fv_temp*ADC_LED_THERMO_C3));
			i_mc_temp = fc_temp * 1000;
			break;
		default:
			i_mc_temp = 0;
			break;
	} 
	return i_mc_temp;
}

int ADC_Read_Current(int channel, int mux, int num_samples) {
// Read current num_samples times and average the samples to compensate for asynchronous sampling
	
	int res=0;
	int current_uA=0;
	int n = num_samples;
	
	while (n-- > 0) {
		res += ADC_Read_Channel(channel, mux, 1);
	}
	switch (channel) {
		case ADC_LED_I_SENSE_CHANNEL:
			res /= num_samples;
			current_uA = res * (V_3V3*1E6)/(R_SENSE * 65536); 
			break;
		default:
			break;
	}
	return current_uA;
	}	

#if 1
void ADC_Init(int use_interrupt, int channel) {
	SIM->SCGC6 |= SIM_SCGC6_ADC0_MASK; 
	ADC0->CFG1 = 0x0D; // 0000 1101
	// 0 Normal power
	// 00 ADIV clock divide ratio 1
	// 0  ADLSMP short sample time
	// 11 MODE 16 bit 
	// 01 ADICLK Bus clock/2

	ADC0->CFG2 = 4; // 0000 0100 
	// 0 MUXSEL ADxxa channels selected
	// 0 ADACKEN Don't use asynch clock output
	// 1 ADHSC Use high-speed configuration
	// 00 ADLSTS (not used)

	ADC0->SC2 = ADC_SC2_REFSEL(0);

	if (use_interrupt) {
		// Configure NVIC for ADC interrupt
		NVIC_SetPriority(ADC0_IRQn, 2); 
		NVIC_ClearPendingIRQ(ADC0_IRQn); 
		NVIC_EnableIRQ(ADC0_IRQn);	

		SIM->SCGC6 |= SIM_SCGC6_TPM0_MASK;
		ADC0->SC1[0] = ADC_SC1_AIEN(1)|ADC_SC1_ADCH(channel & ADC_SC1_ADCH_MASK);
		// AIEN: ADC Interrupt Enable
	}
}
#endif

