/*----------------------------------------------------------------------------
 *      
 *----------------------------------------------------------------------------
 *      Name:    BLINKY.C
 *      Purpose: Bare Metal example program
 *----------------------------------------------------------------------------
 *      
 *      Copyright (c) 2006-2017 KEIL - An ARM Company. All rights reserved.
 *---------------------------------------------------------------------------*/

#include "MKL25Z4.h"                    // Device header

#define LED_NUM     3                   /* Number of user LEDs                */
const uint32_t led_mask[] = {1UL << 18, 1UL << 19, 1UL << 1};


#define LED_RED    0
#define LED_GREEN  1
#define LED_BLUE	 2
#define LED_A      0
#define LED_B      1
#define LED_C      2
#define LED_D      3
#define LED_CLK    4


/*----------------------------------------------------------------------------
  Function that initializes LEDs
 *----------------------------------------------------------------------------*/
void LED_Initialize(void) {

  SIM->SCGC5    |= (1UL <<  10) | (1UL <<  12);      /* Enable Clock to Port B & D */ 
  PORTB->PCR[18] = (1UL <<  8);                      /* Pin PTB18 is GPIO */
  PORTB->PCR[19] = (1UL <<  8);                      /* Pin PTB19 is GPIO */
  PORTD->PCR[1]  = (1UL <<  8);                      /* Pin PTD1  is GPIO */

  FPTB->PDOR = (led_mask[0] | led_mask[1] );          /* switch Red/Green LED off  */
  FPTB->PDDR = (led_mask[0] | led_mask[1] );          /* enable PTB18/19 as Output */

  FPTD->PDOR = led_mask[2];            /* switch Blue LED off  */
  FPTD->PDDR = led_mask[2];            /* enable PTD1 as Output */
}

volatile uint32_t msTicks;                            /* counts 1ms timeTicks */
/*----------------------------------------------------------------------------
  SysTick_Handler
 *----------------------------------------------------------------------------*/
void SysTick_Handler(void) {
  msTicks++;                        /* increment counter necessary in Delay() */
}

/*------------------------------------------------------------------------------
  delays number of tick Systicks (happens every 1 ms)
 *------------------------------------------------------------------------------*/
__INLINE static void Delay (uint32_t dlyTicks) {
  uint32_t curTicks;

  curTicks = msTicks;
  while ((msTicks - curTicks) < dlyTicks);
}

/*----------------------------------------------------------------------------
  Function that turns on Red LED
 *----------------------------------------------------------------------------*/
void LEDRed_On (void) {
  FPTD->PSOR   = led_mask[LED_BLUE];   /* Blue LED Off*/
  FPTB->PSOR   = led_mask[LED_GREEN];  /* Green LED Off*/
  FPTB->PCOR   = led_mask[LED_RED];    /* Red LED On*/
}

/*----------------------------------------------------------------------------
  Function that turns on Green LED
 *----------                 ------------------------------------------------------------------*/
void LEDGreen_On (void) {
  FPTB->PSOR   = led_mask[LED_RED];     /* Red LED Off*/
  FPTD->PSOR   = led_mask[LED_BLUE];    /* Blue LED Off*/
  FPTB->PCOR   = led_mask[LED_GREEN];   /* Green LED On*/
}

/*----------------------------------------------------------------------------
  Function that turns on Blue LED
 *----------------------------------------------------------------------------*/
void LEDBlue_On (void) {
  FPTB->PSOR   = led_mask[LED_GREEN];   /* Green LED Off*/
  FPTB->PSOR   = led_mask[LED_RED];     /* Red LED Off*/
  FPTD->PCOR   = led_mask[LED_BLUE];    /* Blue LED On*/
}

/*----------------------------------------------------------------------------
  Function that turns all LEDs off
 *----------------------------------------------------------------------------*/
void LED_Off (void) {
  FPTB->PSOR   = led_mask[LED_GREEN];   /* Green LED Off*/
  FPTB->PSOR   = led_mask[LED_RED];     /* Red LED Off*/
  FPTD->PSOR   = led_mask[LED_BLUE];    /* Blue LED Off*/
}

/*----------------------------------------------------------------------------
 *        Function 'phaseA': Red LED output
 *---------------------------------------------------------------------------*/
void phaseA (void) {
	  LEDRed_On();
		Delay(0x500);                     /* delay                  */
}

/*----------------------------------------------------------------------------
 *        Function 'phaseB': Blue LED B output
 *---------------------------------------------------------------------------*/
void phaseB (void) {
		LEDBlue_On();
    Delay(0x500);                          /* delay 1000ms                  */
}

/*----------------------------------------------------------------------------
 *         Function 'phaseC': Green LED output
 *---------------------------------------------------------------------------*/
void phaseC (void) {
    LEDGreen_On();
    Delay(0x500);                          /* delay 1000ms                  */
}

/*----------------------------------------------------------------------------
 *        Function 'phaseD': All LEDS Off output
 *---------------------------------------------------------------------------*/
void phaseD (void) {
  	LED_Off();
    Delay(0x500);                          /* delay 1000ms                  */
}

/*----------------------------------------------------------------------------
 *      Main: Initialize
 *---------------------------------------------------------------------------*/
int main (void) {

  SystemCoreClockUpdate();
  LED_Initialize();           /* Initialize the LEDs    */
	SysTick_Config(10000);			/* Turn the SysTick timer on	*/
 
  while(1){
		
	phaseA();			//Red LED on
	phaseB();			//Blue LED on
	phaseC();			//Green LED on
	phaseD();			//all LEDs OFF
	}
}
