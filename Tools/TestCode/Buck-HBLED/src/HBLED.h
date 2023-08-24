#ifndef HBLED_H
#define HBLED_H
// Base configuration: USE_ASYNC_SAMPLING = 1, DEF_CONTROL_MODE = Incremental 

// <<< Use Configuration Wizard in Context Menu >>>

// <h>SMPS PWM Configuration
// Switching parameters
#define PWM_HBLED_CHANNEL (4)

// <o> PWM Period Divider <0-65535>
#define PWM_PERIOD (240) //32 48 div 10: 100 120 130 (fast!) 135 138 Bad <> OK 140 180 
// <i> 48 MHz input clock. PWM frequency = 48 MHz/(PWM_PERIOD*2). Timer is in count-up/down mode.
#define LIM_DUTY_CYCLE (PWM_PERIOD)

#define OUTPUT_TPM_TIMING_REFERENCE (1)
#define TPM_REF_CHANNEL (2)
// </h>

// <h>LED Parameters
// <o> Initial Duty Cycle (percent) <0-100>
#define DEF_DUTY_CYCLE_PCT (10)
#define DEF_DUTY_CYCLE ((DEF_DUTY_CYCLE_PCT * PWM_PERIOD)/100)

// <o> Initial Current Setpoint (mA) <1-500>
#define DEF_SET_CURRENT_MA (10)
// <e> Flash Behavior
// <i> Enable LED Flashing 
#define DEF_ENABLE_FLASH 1
// <o> Flash Period (PIT ticks) <1-1000>
#define FLASH_PERIOD (500)
// <o> Peak Flash Current (mA) <1-500>
#define FLASH_CURRENT_MA (100)
#define ENABLE_PRE_FLASH (0)
// <o> Flash Current Fade Slope [mA/ticks] <0-10000>
#define FLASH_FADE_SLOPE (30)
// </e> 
// </h>

// <h> Control System Selection 
// <o CTL_SYS_ARCH> Control System Architecture
//		<1=> Asynchronous sampling
//		<2=> Synchronous sampling, TPM0 OVF triggers ADC 
//		<3=> Synchronous sampling, Software f_ctl divider 
//		<4=> Synchronous sampling, DMA f_ctl divider
//		<5=> Synchronous sampling, EXTRG_IN triggers ADC
//		<6=> Under Development: Synchronous sampling, DMA f_ctl divider, EXTRG_IN triggers ADC


#define CTL_SYS_ARCH 4

#if CTL_SYS_ARCH == 1
#define 	USE_ASYNC_SAMPLING 1
#define 	USE_TPM0_INTERRUPT 0
#define 	USE_ADC_HW_TRIGGER 0
#define 	USE_ADC_INTERRUPT 1
#endif

#if CTL_SYS_ARCH == 2
#define 	USE_SYNC_NO_FREQ_DIV 1
#define 	USE_TPM0_INTERRUPT 0
#define 	USE_ADC_HW_TRIGGER 1
#define 	ADC_HW_TRIGGER_SRC 8 // Select TPM0 Overflow
#define 	USE_ADC_INTERRUPT 1
#endif

#if CTL_SYS_ARCH == 3
#define 	USE_SYNC_SW_CTL_FREQ_DIV 1
#define 	USE_TPM0_INTERRUPT 1
#define 	USE_ADC_HW_TRIGGER 0
#define 	USE_ADC_INTERRUPT 1
#endif

#if CTL_SYS_ARCH == 4
#define 	USE_SYNC_DMA_CTL_FREQ_DIV 1
#define 	USE_TPM0_INTERRUPT 0
#define 	USE_DMA_INTERRUPT 	1
#define 	USE_ADC_HW_TRIGGER 0
#define 	USE_ADC_INTERRUPT 1
#endif

#if CTL_SYS_ARCH == 5 // No freq. division
#define 	USE_SYNC_DMA_CTL_FREQ_DIV 0
#define 	USE_DMA_INTERRUPT 	0
#define 	USE_TPM0_INTERRUPT 	0
#define 	USE_ADC_HW_TRIGGER 	1
#define 	ADC_HW_TRIGGER_SRC 	0 // Trigger ADC with EXTRG_IN pin
#define 	USE_EXTRG_IN 				1
#define 	USE_ADC_INTERRUPT 	1
#endif

#if CTL_SYS_ARCH == 6 // Freq. division
// Not implemented yet!!!
/* To Do:
	Modify/add DMA code 
	USE_EXTRG_IN == 0: DMA divides frequency by N, DMA ISR starts ADC
	USE_EXTRG_IN == 1: DMA repeatedly copies table items to PTOR to start ADC with HW Trigger
*/
#define 	USE_SYNC_DMA_CTL_FREQ_DIV 1
#define 	USE_DMA_INTERRUPT 	0
#define 	USE_TPM0_INTERRUPT 	0
#define 	USE_ADC_HW_TRIGGER 	1
#define 	ADC_HW_TRIGGER_SRC 	0 // Trigger ADC with EXTRG_IN pin
#define 	USE_EXTRG_IN 				1
#define 	USE_ADC_INTERRUPT 	1
#endif


// <h> Controller Parameters
// <o> Control Loop Frequency divider <1-65535>
#define CTL_FREQ_DIV_FACTOR (2)

#if (USE_SYNC_DMA_CTL_FREQ_DIV || USE_SYNC_SW_CTL_FREQ_DIV)
#define CTL_PERIOD (PWM_PERIOD*CTL_FREQ_DIV_FACTOR)
#else
#define CTL_PERIOD (PWM_PERIOD)
#endif 

// <o DEF_CONTROL_MODE> Controller Mode
//		<OpenLoop=> Open Loop
//		<BangBang=> Bang-Bang
// 		<Incremental=> Incremental
//		<Proportional=> Proportional
// 		<PID=> PID (Floating-point)
//		<PID_FX=> PID_FX (Fixed-point)
// <i> Select default control mode. 
#define DEF_CONTROL_MODE (PID_FX) 
// </h>

// <h> Controller Gains 
// <i> Gain factors scaled down by 1000 and then multplied by control period.
// <h> Incremental Controller
// <o> Incremental Step Size
#define INC_STEP (15*CTL_PERIOD/1000.0)
// </h>

// <h> Proportional Controller
// <o> Proportional Gain
// scaled by 2^8 
#define PGAIN_8 (80*CTL_PERIOD/1000.0)
// </h>

// <h> PID (floating-point) Gains 
// Guaranteed to be sub-optimal.
// <o> Proportional Gain
#define P_GAIN_FL (100*CTL_PERIOD/1000.0)
// <o> Integral Gain
#define I_GAIN_FL (1*CTL_PERIOD/1000.0)
// <o> Derivative Gain
#define D_GAIN_FL (0*CTL_PERIOD/1000.0)
// </h>

// <h> PID_FX (fixed-point) Gains 
// ** Guaranteed to be sub-optimal ** 
// <o> Proportional Gain
#define P_GAIN_FX (12000*CTL_PERIOD/1000.0) 
// <o> Integral Gain
#define I_GAIN_FX (1000*CTL_PERIOD/1000.0) 
// <o> Derivative Gain
#define D_GAIN_FX (0*CTL_PERIOD/1000.0) 
// </h>
// </h>

// </h>

// Hardware configuration
#define ADC_SENSE_CHANNEL (8)

#define R_SENSE (2.2f)
#define R_SENSE_MO ((int) (R_SENSE*1000))

#define V_REF (3.3f)
#define V_REF_MV ((int) (V_REF*1000))

#define ADC_FULL_SCALE (0x10000)
#define MA_SCALING_FACTOR (1000)

#define DAC_POS 30
#define DAC_RESOLUTION 4096

// <<< end of configuration section >>>

// #define MA_TO_DAC_CODE(i) (i*2.2*DAC_RESOLUTION/V_REF_MV) // Introduces timing delay and interesting bug!
#define MA_TO_DAC_CODE(i) ((i)*(2.2f*DAC_RESOLUTION/V_REF_MV))

#define MIN(a,b) ((a<b)?a:b)
#define MAX(a,b) ((a>b)?a:b)

#endif // HBLED_H
