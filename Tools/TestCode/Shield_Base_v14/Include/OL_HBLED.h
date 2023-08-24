#include <MKL25Z4.H>
#ifndef OL_HBLED_H
#define OL_HBLED_H

// Open-Loop HBLED Driver code

#define R_SENSE (2.2f)

#define TEST_LOOP_COUNT 40

// Switching parameters
#define TPM_HBLED (TPM0)
#define PWM_HBLED_CHANNEL (4)
#define PWM_PERIOD (15) // for fast 200 kHz switching to reduce ripple

// 48 MHz/(8*PWM_PERIOD*2) = 200 kHz

#define LIM_DUTY_CYCLE (PWM_PERIOD)

void Init_OL_HBLED(void);
int Test_OL_HBLED(void);
void Set_OL_HBLED_Pulse_Width(uint16_t pw);

#endif // OL_HBLED_H
