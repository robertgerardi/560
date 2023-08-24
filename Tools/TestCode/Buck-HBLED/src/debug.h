#ifndef DEBUG_H
#define DEBUG_H

#include <stdint.h>
#include <MKL25Z4.H>

#define USE_NEW_DEBUG_PINS 1


#define MASK(x) (1UL << (x))

#if USE_NEW_DEBUG_PINS // New flexible debug code
extern const uint32_t DBG_Bit[8];
extern const FGPIO_MemMapPtr DBG_PT[8];

#define DBG_0 0
#define DBG_1 1
#define DBG_2 2
#define DBG_3 3
#define DBG_4 4
#define DBG_5 5
#define DBG_6 6
#define DBG_7 7


#define DBG_LED_ON 			1  
#define DBG_CONTROLLER	2 
#define DBG_IRQ_TPM			3 
#define DBG_IRQ_ADC			4 
#define DBG_IRQ_PIT			5 
#define DBG_IRQ_DMA			6
#define DBG_EXTRG				7   

#define DEBUG_START(x)	{DBG_PT[x]->PSOR = MASK(DBG_Bit[x]);}
#define DEBUG_STOP(x)		{DBG_PT[x]->PCOR = MASK(DBG_Bit[x]);}
#define DEBUG_TOGGLE(x)	{DBG_PT[x]->PTOR = MASK(DBG_Bit[x]);}
#else
// Original debug code // Debug Signals on port B
#define DBG_0_PT FPTB
#define DBG_0 16 // NULLish device, doesn't leave FRDM-KL25Z
#define DBG_1_PT FPTB
#define DBG_1 1 	
#define DBG_2_PT FPTB
#define DBG_2 2	  
#define DBG_3_PT FPTB
#define DBG_3 3		
#define DBG_4_PT FPTB
#define DBG_4 8		
#define DBG_5_PT FPTB
#define DBG_5 9		
#define DBG_6_PT FPTB
#define DBG_6 10  
#define DBG_7_PT FPTB
#define DBG_7 11

#if 1 // normal
#define DBG_LED_ON 	DBG_1  
#define DBG_CONTROLLER	DBG_2 
#define DBG_IRQ_TPM	DBG_3 
#define DBG_IRQ_ADC	DBG_4 
#define DBG_IRQ_PIT	DBG_5 
#define DBG_IRQ_DMA	DBG_6
#define DBG_EXTRG		DBG_7   
#else // Disable debug signals (crosstalk investigation)
#define DBG_LED_ON 	DBG_0  
#define DBG_CONTROLLER	DBG_0
#define DBG_IRQ_TPM	DBG_0 
#define DBG_IRQ_ADC	DBG_0 
#define DBG_IRQ_PIT	DBG_0 
#define DBG_IRQ_DMA	DBG_0
#define DBG_EXTRG		DBG_0   
#endif

#define DEBUG_START(x)	{FPTB->PSOR = MASK(x);}
#define DEBUG_STOP(x)		{FPTB->PCOR = MASK(x);}
#define DEBUG_TOGGLE(x)	{FPTB->PTOR = MASK(x);}
#endif

void Init_Debug_Signals(void);

#endif // DEBUG_H
