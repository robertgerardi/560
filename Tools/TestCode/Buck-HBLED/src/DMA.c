#include <stdint.h>
#include <MKL25Z4.h>

#include "timers.h"
#include "LEDS.h"
#include "gpio_defs.h"
#include "debug.h"
#include "HBLED.h"

uint32_t Reload_DMA_Byte_Count=0;


void DMA_Init(uint32_t divider) {
	static uint32_t src=0, dst=0xffffffff;
	
	SIM->SCGC7 |= SIM_SCGC7_DMA_MASK;
	SIM->SCGC6 |= SIM_SCGC6_DMAMUX_MASK;
	
	// byte count
	Reload_DMA_Byte_Count = divider*4;

	// Disable DMA channel in order to allow changes
	DMAMUX0->CHCFG[0] = 0;

	// Select TPM0 overflow as trigger via mux
	// Don't use periodic triggering
	DMAMUX0->CHCFG[0] = DMAMUX_CHCFG_SOURCE(54);   

	// initialize source and destination pointers
	DMA0->DMA[0].SAR = DMA_SAR_SAR((uint32_t) &src);
	DMA0->DMA[0].DAR = DMA_DAR_DAR((uint32_t) &dst);


	// Generate DMA interrupt when done
	// Don't increment source or dest
	// Transfer bytes	
	// Enable peripheral request, 
	// Normally don't disable it
	DMA0->DMA[0].DCR = DMA_DCR_EINT_MASK | 
											DMA_DCR_ERQ(1) | 
											DMA_DCR_CS(1)	|
											DMA_DCR_D_REQ(1) | // Try allowing one-shot for debugging
											DMA_DCR_SSIZE(0) | DMA_DCR_DSIZE(0);


#if USE_DMA_INTERRUPT
	// Configure NVIC for DMA ISR
	NVIC_SetPriority(DMA0_IRQn, 0); // 0, 1, 2 or 3
	NVIC_ClearPendingIRQ(DMA0_IRQn); 
	NVIC_EnableIRQ(DMA0_IRQn);	
#endif

	// Clear flags
	DMA0->DMA[0].DSR_BCR = DMA_DSR_BCR_DONE(1);
	// byte count
	DMA0->DMA[0].DSR_BCR = DMA_DSR_BCR_BCR(Reload_DMA_Byte_Count);
	
	// Enable DMA
	DMAMUX0->CHCFG[0] |= DMAMUX_CHCFG_ENBL_MASK;
}

#if USE_DMA_INTERRUPT
void DMA0_IRQHandler(void) {
	DEBUG_START(DBG_IRQ_DMA); 
	// Start ADC Conversion
	ADC0->SC1[0] = ADC_SC1_AIEN(1) | ADC_SENSE_CHANNEL;
	// Clear done flag, Reload the byte count
	DMA0->DMA[0].DSR_BCR = DMA_DSR_BCR_DONE(1);
	DMA0->DMA[0].DSR_BCR = DMA_DSR_BCR_BCR(Reload_DMA_Byte_Count);
	DMA0->DMA[0].DCR |= DMA_DCR_ERQ(1);
	DEBUG_STOP(DBG_IRQ_DMA); 
}
#endif

// *******************************ARM University Program Copyright © ARM Ltd 2013*************************************   
