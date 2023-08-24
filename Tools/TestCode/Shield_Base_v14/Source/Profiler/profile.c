#include <stdio.h>
#include <stdint.h>
#include "region.h"
#include "timers.h"
#include "profile.h"
#include "debug.h"

#ifdef PROFILER_LCD_SUPPORT
	#include "LCD.h"
	#include "string.h"
	#include "font.h"
	#include "ST7789.h"
#endif

#ifdef USING_RTOS
#include <cmsis_os2.h>
#endif

/*
register unsigned int _psp __asm("psp");
register unsigned int _msp __asm("msp");
*/

volatile unsigned int adx_lost=0, num_lost=0; 
volatile unsigned long profile_samples=0;
volatile unsigned int profiling_enabled = 0;

volatile unsigned int PC_val = 0;

#ifdef USING_RTOS
osThreadId_t * threadID = NULL;
#endif

void Init_Profiling(void) {
	unsigned i;
	
	profile_samples = 0;
	num_lost = 0;
	adx_lost = 0;
	
	// Clear region counts
  for (i=0; i<NumProfileRegions; i++) {
	  RegionCount[i]=0;
  }
	// Initialize and start timer
	PIT_Init(SAMPLE_FREQ_HZ_TO_TICKS(PROFILER_SAMPLE_FREQ_HZ));
	PIT_Start();
}

void Disable_Profiling(void) {
  profiling_enabled = 0;
}

void Enable_Profiling(void) {
  profiling_enabled = 1;
}

int Profiling_Is_Enabled(void) {
  return profiling_enabled;
}

#ifdef USING_RTOS
void Profiler_Select_Thread(osThreadId_t * th) {
	threadID = th;
}
#endif

void Process_Profile_Sample(void) {
#if 0 // DISABLED	
	unsigned int s, e;
  unsigned int i;

	if (osThreadGetId() == threadID) {
		PC_val = *((unsigned int *) (/* CUR_SP */ __current_sp() + FRAME_SIZE + HW_RET_ADX_OFFSET));
		profile_samples++;

		/* look up function in table and increment counter  */
		for (i=0; i<NumProfileRegions; i++) {
			s = RegionTable[i].Start & ~1; // Convert Thumb instruction address by zeroing LS bit
			e = RegionTable[i].End;
			if ((PC_val >= s) && (PC_val <= e)) {
				RegionCount[i]++;
				break; // break out of the for loop
			}
		}
		if (i == NumProfileRegions) {
			adx_lost = PC_val;
			num_lost++;
		}
	}
#endif
}

void Sort_Profile_Regions(void) {
    unsigned int i, j, temp;

    // Copy unsorted region numbers into table
    for (i = 0; i < NumProfileRegions; i++) {
        SortedRegions[i] = i;
    }
    // Sort those region numbers
    for (i = 0; i < NumProfileRegions; ++i) {
        for (j = i + 1; j < NumProfileRegions; ++j) {
            if (RegionCount[SortedRegions[i]] < RegionCount[SortedRegions[j]]) {
                temp = SortedRegions[i];
                SortedRegions[i] = SortedRegions[j];
                SortedRegions[j] = temp;
            }
        }
    }
}

#ifdef PROFILER_LCD_SUPPORT
#define BUF_LEN 20
void Display_Profile(void) {
	PT_T p, pt;
	unsigned i, row;
	char buffer[BUF_LEN];
	unsigned need_to_block_and_erase_at_end=0;

	LCD_Erase();
	p.X = COL_TO_X(0);
	row = 0;
	p.Y = ROW_TO_Y(row);
	snprintf(buffer, BUF_LEN, "%4lu Total Samples", profile_samples); 
	LCD_Text_Set_Colors(&white, &dark_yellow);
	LCD_Text_PrintStr(&p, buffer);
	LCD_Text_Set_Colors(&yellow, &black);
	row++;
	
	for (i=0; i<NumProfileRegions; i++) {
		if (RegionCount[SortedRegions[i]] > 0) {
			p.X = COL_TO_X(0);
			p.Y = ROW_TO_Y(row);
			snprintf(buffer, BUF_LEN, "%4u %s", RegionCount[SortedRegions[i]], RegionTable[SortedRegions[i]].Name);
			LCD_Text_PrintStr(&p, buffer);		
			row++;
			need_to_block_and_erase_at_end = 1;
			if (row >= LCD_MAX_ROWS) {
				LCD_TS_Blocking_Read(&pt);
				row = 0;
				LCD_Erase();
				need_to_block_and_erase_at_end = 0;
			}
		}
	}
	if (need_to_block_and_erase_at_end) {
		LCD_TS_Blocking_Read(&pt);
		LCD_Erase();
	}
}

void New_Display_Profile(void) {
	PT_T p, tp;
	int i, row;
	char buffer[BUF_LEN];
	
	p.X = 0;
	row = 0;
	p.Y = ROW_TO_Y(row);
	snprintf(buffer, BUF_LEN, "%4lu Total Samples", profile_samples); 
	LCD_Text_PrintStr(&p, buffer);
	row++;
	i = 0;
	LCD_TS_Blocking_Read(&tp);
	
	while (1) {
			for (row = 0; row < LCD_MAX_ROWS-1; row++) {
				if ((i < NumProfileRegions) && (RegionCount[i] > 0)) {
					p.X = COL_TO_X(0);
					p.Y = ROW_TO_Y(row);
					snprintf(buffer, BUF_LEN, "%4u %s", RegionCount[i], RegionTable[i].Name);
					LCD_Text_PrintStr(&p, buffer);
				}
				i++;
				p.X = COL_TO_X(0);
				p.Y = ROW_TO_Y(row);
				LCD_Text_PrintStr(&p, "Previous      Next"); 
				LCD_TS_Blocking_Read(&tp);
				if (tp.X < LCD_WIDTH/2) {
					// to go back
					i -= 2*(LCD_MAX_ROWS-1);
					if (i < 0)
						i = 0;
				} else {
					// go forward, no change to i needed
					if (i>= NumProfileRegions)
							i = NumProfileRegions-1;
				}
				LCD_Erase();
			}
		}
	}

#endif

	
#ifdef PROFILER_SERIAL_SUPPORT
void Serial_Print_Sorted_Profile(void) {
	int i;
	printf("%d total samples, %d samples lost (last was 0x%x)\r\n", profile_samples, num_lost, adx_lost);
	for (i=0; i<NumProfileRegions; i++) {
		if (RegionCount[SortedRegions[i]] > 0) // just print out sampled regions
			printf("%d: \t%s\r\n", RegionCount[SortedRegions[i]], RegionTable[SortedRegions[i]].Name);
	}
}
#endif

