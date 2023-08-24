#include <stdint.h>
#include <stdio.h>
#include <cmsis_os2.h>
#include <MKL25Z4.h>
#include <string.h>
#include <stdlib.h>
#include "logger.h"

#include "debug.h"
#include "profile.h"
#include "ADC.h"
#include "UI.h"
#include "MMA8451.h"

volatile int g_sampling_period = 4;
volatile int g_samples_wanted = LOG_SAMPLES;
volatile int g_enable_fake_data = 0;
volatile int g_enable_temperature = 0;
volatile int g_enable_accel = 1;
volatile int g_samples_used = 0;
volatile int g_mode=M_RECORDING;
volatile int g_redraw_scope = 0;
volatile uint32_t next_logger_tick;

DATA_LOG_T Acc_log[3];

void Logger_Mode_Handler(UI_FIELD_T * fld, int v) {
	if (fld->Val != NULL) {
		if (v > 0) { // start recording
			*fld->Val = M_RECORDING;
		} else {
			*fld->Val = M_PAUSED;
		}
	}
}

int Logger_Init_Log(DATA_LOG_T * log){

#if 0
	strncpy(log->Name, "FakeDat", 8);
	log->SamplingPeriod_ms = g_sampling_period;
	log->SampleSize_bytes = 4;
	log->Data = temporary_data_buffer; // (int * ) malloc(g_samples_wanted*log->SampleSize_bytes);
	if (!log->Data)
		return 0;
	log->NumSamplesFree = g_samples_wanted;
	log->NumSamplesUsed = 0;
#endif
	return 1;
}

void Log_Data(void) {
	int n = g_samples_used;
	
	if (g_samples_used < LOG_SAMPLES) {
		if (g_enable_accel) {
			read_full_xyz();
			Acc_log[0].Data[n] = acc_X;
			Acc_log[1].Data[n] = acc_Y;
			Acc_log[2].Data[n] = acc_Z - COUNTS_PER_G; // Remove gravity offset
		}
	}
}

void Thread_Logger(void * arg) {
	int prev_mode=M_PAUSED;
	
	uint32_t tick;
	
	tick = osKernelGetTickCount();
		
	while (1) {
		DEBUG_START(DBG_TLOGGER);
		switch (g_mode) {
			case M_RECORDING:
				if (prev_mode == M_PAUSED) { // Start recording now!
					tick = osKernelGetTickCount();
					g_samples_used = 0;
				}
				// Read and log the data
				Log_Data();				
				g_samples_used++;
				if (g_samples_used >= g_samples_wanted) {
					g_mode = M_PAUSED;
					g_redraw_scope = 1;
					tick += 100;
					Disable_Profiling(); // REMOVE
				} else {
					tick += g_sampling_period;
				}
				break;
			case M_PAUSED:
			default:
				tick += 100;
				g_mode = M_PAUSED;
				break;
		}
		prev_mode = g_mode;
		DEBUG_STOP(DBG_TLOGGER);
		osDelayUntil(tick);
	}
}
