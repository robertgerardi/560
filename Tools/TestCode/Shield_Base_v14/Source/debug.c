#include <MKL25Z4.H>
#include "debug.h"

#if USE_NEW_DEBUG_PINS
const uint32_t DBG_Bit[8] = {(0), (2), (3), (4), (8), (9), (10), (11)};
const FGPIO_MemMapPtr DBG_PT[8] = {FPTD, FPTD, FPTD, FPTD, FPTB, FPTB, FPTB, FPTB};
const PORT_MemMapPtr DBG_PORT[8] = {PORTD, PORTD, PORTD, PORTD, PORTB, PORTB, PORTB, PORTB};
#endif
	
void Init_Debug_Signals(void) {
	int i;
	
	// Enable clock to ports B and D
	SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK | SIM_SCGC5_PORTD_MASK;
	
	for (i=0; i<8; i++) {
		DBG_PORT[i]->PCR[DBG_Bit[i]] &= ~PORT_PCR_MUX_MASK; // Make pin GPIO          
		DBG_PORT[i]->PCR[DBG_Bit[i]] |= PORT_PCR_MUX(1);          
		DBG_PT[i]->PDDR |= MASK(DBG_Bit[i]);	 // Make an output
		DBG_PT[i]->PCOR = MASK(DBG_Bit[i]); // Clear output
	}

#if DEBUG_WIGGLE_ON_STARTUP
	// Walking debug signals test code
	for (int j=0; j<10; j++) {
		for (i=0; i<8; i++) {
			DEBUG_START(i);
		}
		for (i=0; i<8; i++) {
			DEBUG_STOP(i);
		}
		for (i=0; i<8; i++) {
			DEBUG_TOGGLE(i);
		}
		for (i=0; i<8; i++) {
			DEBUG_TOGGLE(i);
		}
	}
#endif
}	
