/*----------------------------------------------------------------------------
 *----------------------------------------------------------------------------*/
#include <MKL25Z4.H>
// #include <stdio.h>
#include "gpio_defs.h"
#include "LEDs.h"
#include "switches.h"

extern volatile unsigned switch_pressed;
extern void init_debug_signals(void);

/*----------------------------------------------------------------------------
  MAIN function
 *----------------------------------------------------------------------------*/
int main (void) {
	
	init_switch();
	init_RGB_LEDs();
	init_debug_signals();
	__enable_irq();
	
	while (1) {
		DEBUG_PORT->PTOR = MASK(DBG_MAIN_POS);
		control_RGB_LEDs(count&1, count&2, count&4);
	}
}

// *******************************ARM University Program Copyright © ARM Ltd 2013*************************************   
