#include <stdint.h>
#include <stdio.h>
#include <cmsis_os2.h>
#include <MKL25Z4.h>

#include "ADC.h"
#include "LCD.h"
#include "touchscreen.h"
#include "LCD_driver.h"
#include "font.h"
#include "threads.h"
#include "MMA8451.h"
#include "sound.h"
#include "DMA.h"
#include "gpio_defs.h"
#include "debug.h"
#include "OL_HBLED.h"

#include "ST7789.h"
#include "T6963.h"

void Thread_Read_TS(void * arg); // 
void Thread_Read_Accelerometer(void * arg); // 
void Thread_Update_Screen(void * arg); // 

osThreadId_t t_Read_TS, t_Read_Accelerometer, t_US;

// Thread priority options: osPriority[RealTime|High|AboveNormal|Normal|BelowNormal|Low|Idle]

const osThreadAttr_t Read_TS_attr = {
  .priority = osPriorityNormal            
};
const osThreadAttr_t Read_Accelerometer_attr = {
  .priority = osPriorityBelowNormal,      
	.stack_size = READ_ACCEL_STK_SZ
};
const osThreadAttr_t Update_Screen_attr = {
  .priority = osPriorityNormal            
};

void Create_OS_Objects(void) {
		t_Read_TS = osThreadNew(Thread_Read_TS, NULL, &Read_TS_attr); 
	t_Read_Accelerometer = osThreadNew(Thread_Read_Accelerometer, NULL, &Read_Accelerometer_attr);
	t_US = osThreadNew(Thread_Update_Screen, NULL, &Update_Screen_attr);
	LCD_Create_OS_Objects();
	Sound_Create_OS_Objects();
	ADC_Create_OS_Objects();
}

void Thread_Read_TS(void * arg) {
	PT_T p, pp;
	COLOR_T c;
	char buffer[16];
	 
	c.R = 255;
	c.G = 0;
	c.B = 0;
	
	// LCD_TS_Calibrate();
	
	LCD_Text_PrintStr_RC(LCD_MAX_ROWS-4, 0, "Mute");
	LCD_Text_PrintStr_RC(LCD_MAX_ROWS-4, 12, "Unmute");

	while (1) {
		DEBUG_START(DBG_TREADTS_POS);
		if (LCD_TS_Read(&p)) { 
#if 1
			if (p.Y > 240) { 
				if (p.X < LCD_WIDTH/2) {
					Sound_Disable_Amp();
				} else {
					Sound_Enable_Amp();
				}
			} else {		
				// Now draw on screen
				if ((pp.X == 0) && (pp.Y == 0)) {
					pp = p;
				}
				osMutexAcquire(LCD_mutex, osWaitForever);
				LCD_Draw_Line(&p, &pp, &c);
				osMutexRelease(LCD_mutex);
				pp = p;
			}
		} else {
			pp.X = 0;
			pp.Y = 0;
		}
#else
			sprintf(buffer,"X: %06d", p.X);
			osMutexAcquire(LCD_mutex, osWaitForever);
			LCD_Text_PrintStr_RC(0, 0, buffer);
			osMutexRelease(LCD_mutex);

			sprintf(buffer,"Y: %06d", p.Y);
			osMutexAcquire(LCD_mutex, osWaitForever);
			LCD_Text_PrintStr_RC(1, 0, buffer);
			osMutexRelease(LCD_mutex);

#endif
		DEBUG_STOP(DBG_TREADTS_POS);
		osDelay(THREAD_READ_TS_PERIOD_MS);
	}
}

 void Thread_Read_Accelerometer(void * arg) {
	char buffer[16]="               ";

	// Clear part of display which will be used
	LCD_Text_PrintStr_RC(THREAD_READ_ACCELEROMETER_START_ROW, 0, buffer);
	LCD_Text_PrintStr_RC(THREAD_READ_ACCELEROMETER_START_ROW+1, 0, buffer);
	 
	while (1) {
		DEBUG_START(DBG_TREADACC_POS);

		read_full_xyz();
		convert_xyz_to_roll_pitch();
	 
		sprintf(buffer, "Roll:  %6.2f  ", roll);
		osMutexAcquire(LCD_mutex, osWaitForever);
		LCD_Text_PrintStr_RC(THREAD_READ_ACCELEROMETER_START_ROW, 0, buffer);
		osMutexRelease(LCD_mutex);

		sprintf(buffer, "Pitch: %6.2f  ", pitch);
		osMutexAcquire(LCD_mutex, osWaitForever);
		LCD_Text_PrintStr_RC(THREAD_READ_ACCELEROMETER_START_ROW+1, 0, buffer);
		osMutexRelease(LCD_mutex);
		DEBUG_STOP(DBG_TREADACC_POS);
		osDelay(THREAD_READ_ACCELEROMETER_PERIOD_MS);
	}
}

 void Thread_Update_Screen(void * arg) {
	int16_t paddle_pos=LCD_WIDTH/2;
	PT_T p1, p2;
	COLOR_T paddle_color; 
	int PCB_Temp=0, MCU_Temp=0;
	char buffer[24]="                        ";
	int HBLED_DC;
	 
	paddle_color.R = 100;
	paddle_color.G = 10;
	paddle_color.B = 100;
	 
	while (1) {	
		DEBUG_START(DBG_TUPDATESCR_POS);

		if ((roll < -2.0) || (roll > 2.0)) {
			p1.X = paddle_pos;
			p1.Y = PADDLE_Y_POS;
			p2.X = p1.X + PADDLE_WIDTH;
			p2.Y = p1.Y + PADDLE_HEIGHT;
			osMutexAcquire(LCD_mutex, osWaitForever);
			LCD_Fill_Rectangle(&p1, &p2, &black); 		
			osMutexRelease(LCD_mutex);
			
			paddle_pos += roll;
			paddle_pos = MAX(0, paddle_pos);
			paddle_pos = MIN(paddle_pos, LCD_WIDTH-1-PADDLE_WIDTH);
			
			p1.X = paddle_pos;
			p1.Y = PADDLE_Y_POS;
			p2.X = p1.X + PADDLE_WIDTH;
			p2.Y = p1.Y + PADDLE_HEIGHT;
			paddle_color.R = 100+roll;
			paddle_color.B = 100-roll;
			osMutexAcquire(LCD_mutex, osWaitForever);
			LCD_Fill_Rectangle(&p1, &p2, &white); 		
			p1.X++;
			p2.X--;
			p1.Y++;
			p2.Y--;
			LCD_Fill_Rectangle(&p1, &p2, &paddle_color); 		
			osMutexRelease(LCD_mutex);
		}

		int abs_roll;
		if (roll < 0)
			abs_roll = -roll;
		else
			abs_roll = roll;
		HBLED_DC = LIM_DUTY_CYCLE - abs_roll/4;
		if (HBLED_DC > LIM_DUTY_CYCLE)
			HBLED_DC = LIM_DUTY_CYCLE;
		if (HBLED_DC < 0)
			HBLED_DC = 0;
		Set_OL_HBLED_Pulse_Width((uint16_t) HBLED_DC);

		sprintf(buffer, "HBLED DC: %d/%d ", LIM_DUTY_CYCLE-HBLED_DC, LIM_DUTY_CYCLE);
		osMutexAcquire(LCD_mutex, osWaitForever);
		LCD_Text_PrintStr_RC(THREAD_UPDATE_SCREEN_START_ROW, 0, buffer);
		osMutexRelease(LCD_mutex);
		
		int i_uA = ADC_Read_Current(ADC_LED_I_SENSE_CHANNEL, ADC_LED_I_SENSE_MUX, 10);
		sprintf(buffer, "I_HBLED: %d.%03d mA ", i_uA/1000, i_uA % 1000);
		osMutexAcquire(LCD_mutex, osWaitForever);
		LCD_Text_PrintStr_RC(THREAD_UPDATE_SCREEN_START_ROW+1, 0, buffer);
		osMutexRelease(LCD_mutex);
		
		PCB_Temp = ADC_Read_Temperature(ADC_LED_THERMO_CHANNEL, 0);
		sprintf(buffer, "LED Temp: %d.%03d ", PCB_Temp/1000, PCB_Temp % 1000);
		osMutexAcquire(LCD_mutex, osWaitForever);
		LCD_Text_PrintStr_RC(THREAD_UPDATE_SCREEN_START_ROW+2, 0, buffer);
		osMutexRelease(LCD_mutex);
		MCU_Temp = ADC_Read_Temperature(ADC_MCU_THERMO_CHANNEL, 0);
		sprintf(buffer, "MCU Temp: %d.%03d ", MCU_Temp/1000, MCU_Temp % 1000);
		osMutexAcquire(LCD_mutex, osWaitForever);
		LCD_Text_PrintStr_RC(THREAD_UPDATE_SCREEN_START_ROW+3, 0, buffer);
		osMutexRelease(LCD_mutex);
		
		

		DEBUG_STOP(DBG_TUPDATESCR_POS);
		osDelay(THREAD_UPDATE_SCREEN_PERIOD_MS);
	}
}
