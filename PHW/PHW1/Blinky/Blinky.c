/*----------------------------------------------------------------------------
 * Name:    Blinky.c
 * Purpose: LED Flasher
 *----------------------------------------------------------------------------
 * This file is part of the uVision/ARM development tools.
 * This software may only be used under the terms of a valid, current,
 * end user licence from KEIL for a compatible version of KEIL software
 * development tools. Nothing else gives you the right to use this software.
 *
 * This software is supplied "AS IS" without warranties of any kind.
 *
 * Copyright (c) 2015 Keil - An ARM Company. All rights reserved.
 *----------------------------------------------------------------------------*/

// agdean@ncsu.edu: Modified 9/8/2023 to support Arm Compiler 6, CMSIS-RTOS2 with RTX5

#include "MKL25Z4.h"                    // Device header
#include "cmsis_os2.h"                   // ARM::CMSIS:RTOS:Keil RTX
#include "Board_LED.h"                  // ::Board Support:LED

/*----------------------------------------------------------------------------
 *      Thread 1 'Thread_LED': Sample thread
 *---------------------------------------------------------------------------*/

void Thread_LED (void *argument) {
  uint32_t led_max    = LED_GetCount();
  uint32_t led_num    = 0;

  while (1) {
    LED_On(led_num);                                               // Turn specified LED on
    osDelay(500);
    LED_Off(led_num);                                               // Turn specified LED off
    osDelay(500);

    led_num++;                                                      // Change LED number
    if (led_num >= led_max) {
      led_num = 0;                                                  // Restart with first LED
    }
	}
}

/*----------------------------------------------------------------------------
 * main: blink LED and check button state
 *----------------------------------------------------------------------------*/
int main (void) {

  SystemCoreClockUpdate();
  LED_Initialize();                                                 // initalize LEDs
  osKernelInitialize ();                                            // initialize CMSIS-RTOS
	osThreadNew(Thread_LED, NULL, NULL);
  osKernelStart();                                                 // start thread execution
}
