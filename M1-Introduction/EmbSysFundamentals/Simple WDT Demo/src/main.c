/*----------------------------------------------------------------------------
 *----------------------------------------------------------------------------*/
#include <MKL25Z4.H>
#include <stdio.h>
#include "gpio_defs.h"
#include "config.h"
#include "LEDs.h"
#include "delay.h"
#include "COP_WDT.h"

/*----------------------------------------------------------------------------
  MAIN function
 *----------------------------------------------------------------------------*/
int main (void) {

	Init_COP_WDT();
	Service_COP_WDT();
	Init_RGB_LEDs();
	
	// Show system is starting up by flashing LEDs
	// Green flashes mean regular reset
	// Red flashes mean WDT reset
	Flash_Reset_Cause(); 				
	
	// Do slower and slower loops to until WDT triggers on 12th iteration (6th blue flash)
	for (int n = 1; n<100; n++) {
		Toggle_RGB_LEDs(0, 0, 1);	// Flash blue LED
		Service_COP_WDT();
		Delay(n*40);							// delay longer each time
	}
}

