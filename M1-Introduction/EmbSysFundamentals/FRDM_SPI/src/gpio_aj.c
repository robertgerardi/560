#include <MKL25Z4.H>
#include "user_defs.h"

void Init_GPIO(void)
{

	PTB->PDDR |= MASK(9);
	PTC->PDDR |= MASK(16)|MASK(17); //set PortC Pins 16 and 17 as GPIO Outputs
  PTE->PDDR |= MASK(5);
	
	PORTB->PCR[9] &= ~PORT_PCR_MUX_MASK;
	PORTB->PCR[9] |= PORT_PCR_MUX(1);
	PORTB->PCR[9] |= PORT_PCR_PS_MASK | PORT_PCR_PE_MASK ;
	
	PORTC->PCR[16] &= ~PORT_PCR_MUX_MASK;
	PORTC->PCR[16] |= PORT_PCR_MUX(1);
	
	PORTC->PCR[17] &= ~PORT_PCR_MUX_MASK;
	PORTC->PCR[17] |= PORT_PCR_MUX(1);
	
	PORTE->PCR[5]  &= ~PORT_PCR_MUX_MASK;
	PORTE->PCR[5] |= PORT_PCR_MUX(1);
	PORTE->PCR[5] |= PORT_PCR_PS_MASK | PORT_PCR_PE_MASK ;
	
}

