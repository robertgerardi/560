#ifndef TIMERS_H
#define TIMERS_H
#include "MKL25Z4.h"

#define USE_PIT (0)

#define LCD_UPDATE_PERIOD 10

void PIT_Init(unsigned period);
void PIT_Start(void);
void PIT_Stop(void);

void TPM_Init(unsigned period_ms);

void TPM2_Init(void);
void TPM2_Start(void);
void Configure_TPM2_for_DMA(uint32_t period_us);

void PWM_Init(TPM_Type * TPM, uint8_t channel_num, uint16_t period, uint16_t duty);
void PWM_Set_Value(TPM_Type * TPM, uint8_t channel_num, uint16_t value);


void LPTMR_Init(void);
void LPTMR_Start(void);
void LPTMR_Stop(void);

#endif
// *******************************ARM University Program Copyright � ARM Ltd 2013*************************************   
