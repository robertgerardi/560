#ifndef TIMERS_H
#define TIMERS_H
#include "MKL25Z4.h"

void PWM_Configure_For_SMPS(void);

void PWM_Init(TPM_Type * TPM, uint8_t channel_num, uint16_t period, uint16_t duty);
void PWM_Set_Value(TPM_Type * TPM, uint8_t channel_num, uint16_t value);

void Init_PIT(unsigned period);
void Start_PIT(void);
void Stop_PIT(void);

#endif
// *******************************ARM University Program Copyright � ARM Ltd 2013*************************************   
