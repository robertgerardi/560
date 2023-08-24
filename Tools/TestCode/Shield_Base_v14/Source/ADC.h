#ifndef ADC_H
#define ADC_H
#include <cmsis_os2.h>
#include "config.h"

#define V_3V3 (3.280f)

#define ADC_MCU_THERMO_CHANNEL (26)
#define ADC_MCU_THERMO_SLOPE_MV_C (1.715f)
#define ADC_MCU_THERMO_MV_TEMP25 (719)

#define ADC_LED_THERMO_CHANNEL (12)
#define ADC_LED_THERMO_C0 (96.231f) // tweaked from 99.231f
#define ADC_LED_THERMO_C1 (-85.471f)
#define ADC_LED_THERMO_C2 (37.394f)
#define ADC_LED_THERMO_C3 (-7.8359f)

#define ADC_LED_I_SENSE_CHANNEL (8)
#define ADC_LED_I_SENSE_MUX (0)

void ADC_Init(int, int);
void ADC_Create_OS_Objects(void);
extern osMutexId_t ADC_mutex;

extern int ADC_Read_Temperature(int channel, int mux);
extern int ADC_Read_Current(int channel, int mux, int num_samples);

#endif
