/*----------------------------------------------------------------------------
 *----------------------------------------------------------------------------*/
#include <MKL25Z4.H>
#include <stdio.h>

#include "gpio_defs.h"

#include <cmsis_os2.h>
#include "threads.h"

#include "LCD.h"
#include "LCD_driver.h"
#include "font.h"

#include "ADC.h"
#include "LEDs.h"
#include "timers.h"
#include "sound.h"
#include "DMA.h"
#include "I2C.h"
#include "mma8451.h"
#include "delay.h"
#include "profile.h"
#include "math.h"
#include "OL_HBLED.h"
#include "sound.h"
	

/*----------------------------------------------------------------------------
  MAIN function
 *----------------------------------------------------------------------------*/
int main (void) {
	int HBLED_current = 0;
	char buffer[30];
	
	Init_Debug_Signals();
	Init_RGB_LEDs();
	Control_RGB_LEDs(0,0,1);			

	Sound_Init();	
	Play_Tone();
	Sound_Disable_Amp();
	
	ADC_Init(0,0);
	Init_OL_HBLED();
	HBLED_current = Test_OL_HBLED();

	LCD_Init();
	if (!LCD_Text_Init(1)) {
		Control_RGB_LEDs(1, 0, 0);
		while (1)
			;
	}
	LCD_Erase();
	Graphics_Test();
	
	LCD_Erase();
	LCD_Text_PrintStr_RC(0,0, "Test Code");
	sprintf(buffer, "I_HBLED = %d mA", HBLED_current);
	LCD_Text_PrintStr_RC(1,0, buffer);
	
#if 0
	// LCD_TS_Calibrate();
	LCD_TS_Test();
#endif

	LCD_Text_PrintStr_RC(2,0, "Accel...");
	i2c_init();											// init I2C peripheral
	if (!init_mma()) {							// init accelerometer
		Control_RGB_LEDs(1,0,0);			// accel initialization failed, so turn on red error light
		while (1)
			;
	}
	LCD_Text_PrintStr_RC(2,9, "Done");

	Delay(250);

	osKernelInitialize();
	Create_OS_Objects();
	osKernelStart();	
}
