#include <stdint.h>
#include <MKL25Z4.h>
#include <math.h>
#include <cmsis_os2.h>

#include "sound.h"
#include "misc.h"
#include "delay.h"
#include "gpio_defs.h"
#include "timers.h"
#include "DMA.h"
#include "threads.h"
#include "debug.h"

int16_t SineTable[NUM_STEPS];
uint16_t Waveform[2][NUM_WAVEFORM_SAMPLES];
uint8_t write_buffer_num= 0; // Number of waveform buffer currently being written 

VOICE_T Voice[NUM_VOICES];

// RTOS Objects
osThreadId_t t_Sound_Manager, t_Refill_Sound_Buffer;

const osThreadAttr_t Sound_Manager_attr = {
  .priority = osPriorityAboveNormal1            
};
const osThreadAttr_t Refill_Sound_Buffer_attr = {
  .priority = osPriorityAboveNormal2       
};

osMutexId_t Voice_mutex;

const osMutexAttr_t Voice_mutex_attr = {
  "Voice_mutex",     // human readable mutex name
  osMutexPrioInherit    // attr_bits
};

// Functions
void DAC_Init(void) {
  // Init DAC output
	
	SIM->SCGC6 |= MASK(SIM_SCGC6_DAC0_SHIFT); 
	SIM->SCGC5 |= MASK(SIM_SCGC5_PORTE_SHIFT); 
	
	PORTE->PCR[DAC_POS] &= ~PORT_PCR_MUX_MASK;	
	PORTE->PCR[DAC_POS] |= PORT_PCR_MUX(0);	// Select analog 
		
	// Disable buffer mode
	DAC0->C1 = 0;
	DAC0->C2 = 0;
	
	// Enable DAC, select VDDA as reference voltage
	DAC0->C0 = MASK(DAC_C0_DACEN_SHIFT) | MASK(DAC_C0_DACRFS_SHIFT);
}

/*
	Code for driving DAC
*/
void Play_Sound_Sample(uint16_t val) {
	DAC0->DAT[0].DATH = DAC_DATH_DATA1(val >> 8);
	DAC0->DAT[0].DATL = DAC_DATL_DATA0(val);
}

void SineTable_Init(void) {
	unsigned n;
	
	for (n=0; n<NUM_STEPS; n++) {
		SineTable[n] = (MAX_DAC_CODE/2)*sinf(n*(2*3.1415927/NUM_STEPS));
	}
}

/* Fill waveform buffers with silence. */
void Init_Waveform(void) {
	uint32_t i;
	
	for (i=0; i<NUM_WAVEFORM_SAMPLES; i++) {
		Waveform[0][i] = (MAX_DAC_CODE/2);
		Waveform[1][i] = (MAX_DAC_CODE/2);
	}
}

void Init_Voices(void) {
	uint16_t i;
	
	for (i=0; i<NUM_VOICES; i++) {
		Voice[i].Volume = 0;
		Voice[i].Decay = 0;
		Voice[i].Duration = 0;
		Voice[i].Period = 0;
		Voice[i].Counter = 0;
		Voice[i].CounterIncrement = 0;
		Voice[i].Type = VW_UNINIT;
		Voice[i].Updated = 0;
	}
}

/* Initialize sound hardware, sine table, and waveform buffer. */
void Sound_Init(void) {
	SineTable_Init();	
	Init_Waveform();
	Init_Voices();
	write_buffer_num = 0; // Start writing to waveform buffer 0
	
	DAC_Init();
	DMA_Init();
	TPM2_Init();
	Configure_TPM2_for_DMA(AUDIO_SAMPLE_PERIOD_US); 

	SIM->SOPT2 |= (SIM_SOPT2_TPMSRC(1) | SIM_SOPT2_PLLFLLSEL_MASK);

	SIM->SCGC5 |= SIM_SCGC5_PORTE_MASK; 
	
	PORTE->PCR[AMP_ENABLE_POS] &= ~PORT_PCR_MUX_MASK;	
	PORTE->PCR[AMP_ENABLE_POS] |= PORT_PCR_MUX(1);	// Select GPIO
	PTE->PDDR |= MASK(AMP_ENABLE_POS); // set to output
	PTE->PSOR = MASK(AMP_ENABLE_POS);  // enable audio amp

}

void Sound_Create_OS_Objects(void) {
	Voice_mutex = osMutexNew(&Voice_mutex_attr);

	t_Sound_Manager = osThreadNew(Thread_Sound_Manager, NULL, &Sound_Manager_attr);
	t_Refill_Sound_Buffer = osThreadNew(Thread_Refill_Sound_Buffer, NULL, &Refill_Sound_Buffer_attr);
}

void Sound_Enable_Amp(void) {
	PTE->PSOR = MASK(AMP_ENABLE_POS);  // enable audio amp
}

void Sound_Disable_Amp(void) {
	PTE->PCOR = MASK(AMP_ENABLE_POS);  // disable audio amp
}
/* Simple audio test function using busy-waiting. */
void Play_Tone(void) {
	int i, d=MAX_DAC_CODE>>5, n;
	float p = 40000;
	
	
	for (i=0; i<60; i++) {
		n = 20000.0/p;
		while (n--) {
			Play_Sound_Sample((MAX_DAC_CODE>>1)+d);
			ShortDelay(p);
			Play_Sound_Sample((MAX_DAC_CODE>>1)-d);
			ShortDelay(p);
		}
		// Delay(40);
		p *= 0.9;
	}
}


int16_t Sound_Generate_Next_Sample (VOICE_T *voice) {
	uint16_t lfsr;
	uint16_t bit;
	int16_t sample;

	switch (voice->Type) {
		case VW_NOISE:
			lfsr = voice->Counter;
			// source code from http://en.wikipedia.org/wiki/Linear_feedback_shift_register
			/* taps: 16 14 13 11; characteristic polynomial: x^16 + x^14 + x^13 + x^11 + 1 */
			bit  = ((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5) ) & 1;
			lfsr =  (lfsr >> 1) | (bit << 15);
			voice->Counter = lfsr;
			sample = (lfsr >> 4) - (MAX_DAC_CODE/2); // scale to get 12-bit value
			break;
		case VW_SQUARE:
			if (voice->Counter < voice->Period/2) {
				sample = MAX_DAC_CODE/2 - 1;
			} else {
				sample = -MAX_DAC_CODE/2;
			}
			voice->Counter++;
			if (voice->Counter == voice->Period) {
				voice->Counter = 0;
			}
			break;
		case VW_SINE:
			sample = SineTable[((voice->Counter)/256)]; // & (NUM_STEPS-1)]; 
			voice->Counter += voice->CounterIncrement;
			if (voice->Counter >= voice->Period * voice->CounterIncrement){ // interesting bug with >
				voice->Counter = 0;
			}
			break;
		default:
			sample = 0;
			break;
	}
	return sample;
}

void Play_Waveform_with_DMA(void) {
	Configure_DMA_For_Playback(Waveform[0], Waveform[1], NUM_WAVEFORM_SAMPLES, 1);
	Start_DMA_Playback();
}

// volatile uint16_t temp_freq = 604;

 void Thread_Sound_Manager(void * arg) {
	uint16_t lfsr=12345;
	uint16_t bit;
	unsigned int v=0;

  Play_Waveform_with_DMA();

	while (1) {
		DEBUG_START(DBG_TSNDMGR_POS);
		osMutexAcquire(Voice_mutex, osWaitForever);

		// make a new random sound periodically
		Voice[v].Volume = 0x4FFF; 
		Voice[v].Duration = 0.5*AUDIO_SAMPLE_FREQ;
		bit  = ((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5) ) & 1;
		lfsr =  (lfsr >> 1) | (bit << 15);
#if 1 // random frequencies
		Voice[v].Period = FREQ_TO_PERIOD((lfsr & 0x03FF) + 40); 
#else
		Voice[v].Period = 64; // FREQ_TO_PERIOD(temp_freq); 
#endif
		
		Voice[v].Decay = (lfsr>>4)&0x1f;
		Voice[v].Counter = 0; 
		Voice[v].CounterIncrement = (NUM_STEPS*256)/Voice[v].Period; 
#if SOUND_ONLY_SINES
		Voice[v].Type = VW_SINE;
#else
		switch ((lfsr & 0x30) >> 4) {
			case 0:
			case 1:
				Voice[v].Type = VW_SINE;
				break;
			case 2:
				Voice[v].Type = VW_SQUARE;
				break;
			default:
				Voice[v].Type = VW_NOISE;
				Voice[v].Counter = 1234; // Pseudonoise LFSR seed 
				Voice[v].Decay <<= 2; // Make noise decay faster  
				break;
		}
#endif
		Voice[v].Updated = 1;
		osMutexRelease(Voice_mutex);

		osThreadFlagsSet(t_Refill_Sound_Buffer, EV_START_NEW_SOUND);	
		v++;
		if (v>=NUM_VOICES)
			v = 0;
		DEBUG_STOP(DBG_TSNDMGR_POS);
		osDelay(THREAD_SOUND_PERIOD_TICKS);
	}
}

 void Thread_Refill_Sound_Buffer(void * arg) {
	uint32_t i;
	uint16_t v;
	int32_t sum, sample;
	uint32_t events;
#if USE_DOUBLE_BUFFER
	uint8_t initialized = 0;
#endif
	 
	while (1) {
		DEBUG_STOP(DBG_TREFILLSB_POS);
		events = osThreadFlagsWait(EV_REFILL_SOUND_BUFFER | EV_START_NEW_SOUND, osFlagsWaitAny, osWaitForever); // wait for trigger
		DEBUG_START(DBG_TREFILLSB_POS);
		if (events & EV_REFILL_SOUND_BUFFER) { // updates all samples, regardless of whether any voices have been added
			osMutexAcquire(Voice_mutex, osWaitForever);
			for (i=0; i<NUM_WAVEFORM_SAMPLES; i++) {
				sum = 0;
				for (v=0; v<NUM_VOICES; v++) {
					if (Voice[v].Duration > 0) {
						sample = Sound_Generate_Next_Sample(&(Voice[v]));
						sample = (sample*Voice[v].Volume)>>16;
						// update volume with decayed version
						Voice[v].Volume = (Voice[v].Volume * (((uint32_t) 65536) - Voice[v].Decay)) >> 16; 
						Voice[v].Duration--;
						sum += sample;
					} 
				}
				
#if DMA_ISR_BUG
			if (i == 10) {
				i += 50;
			}
#endif
				sum = sum + (MAX_DAC_CODE/2); // Center at 1/2 DAC range
				sum = MIN(sum, MAX_DAC_CODE-1);
				Waveform[write_buffer_num][i] = sum; 
			}
			osMutexRelease(Voice_mutex);
		} else if (events & EV_START_NEW_SOUND) { // One or more voices has been added, so update remaining buffer
			int32_t start_i = Get_DMA_Transfers_Completed(0);
			if ((start_i < 0) || (start_i > NUM_WAVEFORM_SAMPLES))
				while (1); // catch errror!
			osMutexAcquire(Voice_mutex, osWaitForever);
			for (i=start_i; i<NUM_WAVEFORM_SAMPLES; i++) {
				sum = 0;
				for (int v=0; v<NUM_VOICES; v++) {
					if ((Voice[v].Duration > 0) & Voice[v].Updated) {
						sample = Sound_Generate_Next_Sample(&(Voice[v]));
						sample = (sample*Voice[v].Volume)>>16;
							// update volume with decayed version
						Voice[v].Volume = (Voice[v].Volume * (((uint32_t) 65536) - Voice[v].Decay)) >> 16; 
						Voice[v].Duration--;
						sum += sample;
					}
				}
				sum += Waveform[write_buffer_num][i]; // Add to current waveform value
				sum = MIN(sum, MAX_DAC_CODE-1); // Clip upper limit
				sum = MAX(sum, 0); // Clip lower limit
				Waveform[write_buffer_num][i] = sum; // Save to buffer
			}
			// Clear updated flags for all voices
			for (v=0; v<NUM_VOICES; v++) {
				Voice[v].Updated = 0;
			}
			osMutexRelease(Voice_mutex);
		}
#if USE_DOUBLE_BUFFER
		write_buffer_num = 1 - write_buffer_num;
		if (!initialized) { // Fill up both buffers the first time
			initialized = 1; 
			for (i=0; i<NUM_WAVEFORM_SAMPLES; i++) {
				sum = 0;
				for (v=0; v<NUM_VOICES; v++) {
					if (Voice[v].Duration > 0) {
						sample = Sound_Generate_Next_Sample(&(Voice[v]));
						
						sample = (sample*Voice[v].Volume)>>16;
						sum += sample;
						// update volume with decayed version
						Voice[v].Volume = (Voice[v].Volume * (((uint32_t) 65536) - Voice[v].Decay)) >> 16; 
						Voice[v].Duration--;
					} 
				}
				sum = sum + (MAX_DAC_CODE/2);
				sum = MIN(sum, MAX_DAC_CODE-1);
				Waveform[write_buffer_num][i] = sum; 
			}
			write_buffer_num = 1 - write_buffer_num;
		}	
#endif			
	}
}

// Future expansion code below disabled
#if 0
const float startup_sound[] = {740, 880, 622.3, 740};
const float startup_chord[] = {659.3, 493.9, 329.6, 246.9};

 void Thread_Sequencer(void) {
	uint32_t p = 128;
	uint32_t n=0, v=0;
	
	while (1) {
		// Insert OS delay call here 
		if (n<4) {
			v = 0;
			Voice[v].Volume = 0xFFFF; 
			Voice[v].Duration = 10000;
			Voice[v].Period = FREQ_TO_PERIOD(startup_sound[n]); 
			Voice[v].Decay = 20;
			Voice[v].Counter = 0; 
			Voice[v].CounterIncrement = (NUM_STEPS*256)/Voice[v].Period; 
			Voice[v].Type = VW_SINE;

			Voice[1].Duration = 0;
			Voice[2].Duration = 0;
			Voice[3].Duration = 0;

			n++;
		} else if (n==4) {
			for (v=0; v<4; v++) {
				Voice[v].Volume = 0xE000; 
				Voice[v].Duration = 60000;
				Voice[v].Period = FREQ_TO_PERIOD(startup_chord[v]); 
				Voice[v].Decay = 5;
				Voice[v].Counter = 0; 
				Voice[v].CounterIncrement = (NUM_STEPS*256)/Voice[v].Period; 
				Voice[v].Type = VW_SINE;
			}
			n++;
		}
		os_evt_set(EV_REFILL_SOUND, t_Refill_Sound_Buffer);	
		Play_Waveform_with_DMA();
	}
}
#endif
