#include <MKL25Z4.H>
#include "fault.h"
#include <cmsis_os2.h>
#include "control.h"
#include "LEDs.h"
#include "threads.h"

osThreadId_t t_Fault;
const osThreadAttr_t Fault_attr = {
  .priority = osPriorityAboveNormal         
};

// Defines names of all possible different types of fault injection tests.
typedef enum {
	TR_None,
	TR_Setpoint_High,
	TR_Setpoint_Zero,
	TR_PID_FX_Gains,
	TR_LCD_mutex,
	TR_Disable_All_IRQs,
	TR_Disable_ADC_IRQ,
	TR_Disable_PeriphClocks,
	TR_osKernelLock,
	TR_High_Priority_Thread,
	TR_Change_MCU_Clock,
	TR_Slow_TPM,
	TR_Stack_Overflow,
	TR_Fill_Queue, 
	TR_End
} Fault_Test_E;
	


// Uncomment the tests you want to run
Fault_Test_E Fault_Tests[] = {
//	TR_Setpoint_High,
	TR_Setpoint_Zero,
//	TR_PID_FX_Gains,
//	TR_LCD_mutex,
//	TR_Disable_All_IRQs,
//	TR_Disable_ADC_IRQ,
//	TR_Stack_Overflow,
//	TR_Fill_Queue, 
//	TR_Disable_PeriphClocks,
//	TR_osKernelLock,
//	TR_High_Priority_Thread,
//	TR_Change_MCU_Clock,
//	TR_Slow_TPM,
	TR_End // Required to indicate end of list of tests to run 
};	

int Fault_Recursion_Test(int level) {
	return Fault_Recursion_Test(level+1);
}

void Fault_Fill_Queue(void ) {
	ADC_Request_t req;
	osStatus_t status;

	req.Channel = 0;
	req.ResponseQueue = (osMessageQueueId_t) NULL;
	do {
		req.Channel++;
		status = osMessageQueuePut(ADC_RequestQueue, &req, 0, 0);
	} while (1);
}

void Test_Fault(int t) {
	int n;
	
	DEBUG_START(DBG_FAULT);
	switch (t) {
		case TR_None:
			break;
		case TR_Setpoint_High:
			// Manually change current setpoint.
			g_set_current = 1000;
			break;
		case TR_Setpoint_Zero:
			// Manually change current setpoint.
			g_set_current = 0;
			break;
		case TR_PID_FX_Gains:
			// Corrupt controller gains
			plantPID_FX.pGain = 123;
			plantPID_FX.iGain = 456;
			plantPID_FX.dGain = 789;
			break;
		case TR_LCD_mutex:
			// Take LCD_mutex, don't return it
			osMutexAcquire(LCD_mutex, osWaitForever);
			break;
		case TR_Fill_Queue:
			// Fill ADC request queue with garbage
			Fault_Fill_Queue();
			break;
		case TR_Disable_PeriphClocks:
			SIM->SCGC6 = 0;
			break;
		case TR_Disable_All_IRQs:
			// Disable Interrupts
			__disable_irq();
			break;
		case TR_Disable_ADC_IRQ:
			// Disable ADC interrupt
			NVIC_DisableIRQ(ADC0_IRQn);
			break;
		case TR_osKernelLock:
			// Lock kernel - don't let other tasks run
			// See details at https://www.keil.com/pack/doc/CMSIS/RTOS2/html/group__CMSIS__RTOS__KernelCtrl.html#ga948609ee930d9b38336b9e1c2a4dfe12
			osKernelLock();
			break;
		case TR_Change_MCU_Clock:
			// Change MCU clock frequency
			MCG->C6 = 0x4A;
			break;
		case TR_Slow_TPM:
			TPM0->MOD = 2345;
			break;
		case TR_Stack_Overflow:
			n = Fault_Recursion_Test(0);
			break;
		case TR_High_Priority_Thread:
			// Raise own priority very high, then go into infinite loop
			osThreadSetPriority(osThreadGetId(), osPriorityRealtime);
			while (1)
				DEBUG_TOGGLE(DBG_FAULT);
			break;
		case TR_End:
			break;
	}
	osDelay(2); // To make DBG_FAULT pulse visible
	DEBUG_STOP(DBG_FAULT);
}

void Thread_Fault_Injector(void * arg) {
	int test_num=0;
	Fault_Test_E test;
	char msg[24];
	
	// Any first run initialization goes here
	osDelay(FAULT_PERIOD + FLASH_DURATION_MS/(2*THREAD_BUS_PERIOD_MS));
	
	while (1) {
		test = Fault_Tests[test_num];
		if (test != TR_End) {
			Control_RGB_LEDs(1, 0, 0);
			sprintf(msg, "Test %02d", test_num);
			LCD_Text_Set_Colors(&yellow, &red);
			LCD_Text_PrintStr_RC(15, 4, "           ");
			LCD_Text_PrintStr_RC(15, 6, msg);
			Test_Fault(test);
			test_num++;
			Control_RGB_LEDs(0, 0, 0);
		} else {
			// Reached end of tests in table
			Control_RGB_LEDs(0, 1, 0);
			while (1)
				osDelay(FAULT_PERIOD);
		}
		osDelay(FAULT_PERIOD);
	}
}

void Fault_Init(void){
	t_Fault = osThreadNew(Thread_Fault_Injector, NULL, &Fault_attr);
}

