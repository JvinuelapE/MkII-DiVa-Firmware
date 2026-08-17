/*
 * comparator.c
 *
 *  Created on: Mar 23, 2024
 *      Author: Alperen Akkünücü
 */



#include "comparator.h"
#include "stm32g4xx.h"
#include <string.h>
//#include <math.h>

extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
extern DMA_HandleTypeDef hdma_tim3_ch3;
extern DMA_HandleTypeDef hdma_tim2_ch1;

static uint16_t voltage_capture[NUM_CAPTURES], voltage_capture_locked[NUM_CAPTURES];
static uint16_t current_capture[NUM_CAPTURES], current_capture_locked[NUM_CAPTURES];
static int difference[NUM_CAPTURES];
static comparator_measurement measurement;

static uint16_t voltage_capture_index;
static uint16_t current_capture_index;

static float phase_corr;

/**
  * @brief  Configure timer 2 and timer 3
  *         Configure DMA to store capture data
*/
void comparator_config(void){

	//Disable Timers for the configurations
	__HAL_TIM_DISABLE(&htim3);
	__HAL_TIM_DISABLE(&htim2);

	//disable DMA channels before configuring them
	__HAL_DMA_DISABLE(&hdma_tim3_ch3);
	__HAL_DMA_DISABLE(&hdma_tim2_ch1);

	//Set timer2 and timer 3 to stop when the MCU stops in debugging mode
	DBGMCU->APB1FZR1 = DBGMCU->APB1FZR1 | DBGMCU_APB1FZR1_DBG_TIM2_STOP_Msk;
	DBGMCU->APB1FZR1 = DBGMCU->APB1FZR1 | DBGMCU_APB1FZR1_DBG_TIM3_STOP_Msk;

	//set the counter timers to zero
	TIM2->CNT = 0;
	TIM3->CNT = 0;

	//Enable input capture channel. Timer3_CH3: Voltage Timer2_CH1: Current
	TIM_CCxChannelCmd(htim3.Instance, TIM_CHANNEL_3, TIM_CCx_ENABLE);
	TIM_CCxChannelCmd(htim2.Instance, TIM_CHANNEL_1, TIM_CCx_ENABLE);

	//Set peripheral address for the DMA transfers
	//Timer3 Compare/Capture channel 3
	//Timer2 Compare/Capture channel 1
	hdma_tim3_ch3.Instance->CPAR = (uint32_t)&htim3.Instance->CCR3;
	hdma_tim2_ch1.Instance->CPAR = (uint32_t)&htim2.Instance->CCR1;

	//Set memory address for the DMA transfers
	hdma_tim3_ch3.Instance->CMAR = (uint32_t)voltage_capture;
	hdma_tim2_ch1.Instance->CMAR = (uint32_t)current_capture; //Weird bug in the first sample!!

	//Set number of transfers
	hdma_tim3_ch3.Instance->CNDTR = NUM_CAPTURES;
	hdma_tim2_ch1.Instance->CNDTR = NUM_CAPTURES;

	//Disable half-transfer interrupt for the DMA channels
	__HAL_DMA_DISABLE_IT(&hdma_tim3_ch3, DMA_IT_HT);
	__HAL_DMA_DISABLE_IT(&hdma_tim2_ch1, DMA_IT_HT);
	__HAL_DMA_DISABLE_IT(&hdma_tim2_ch1, (DMA_IT_TC | DMA_IT_TE) );

	//Enable error and transfer complete interrupts for the DMA channels
	__HAL_DMA_ENABLE_IT(&hdma_tim3_ch3, (DMA_IT_TC | DMA_IT_TE) );
	//__HAL_DMA_ENABLE_IT(&hdma_tim2_ch1, (DMA_IT_TC | DMA_IT_TE) );

	//Enable the DMA channels
	__HAL_DMA_ENABLE(&hdma_tim3_ch3);
	__HAL_DMA_ENABLE(&hdma_tim2_ch1);

	//Enable DMA transfer from Timer captures
	__HAL_TIM_ENABLE_DMA(&htim3, TIM_DMA_CC3);
	__HAL_TIM_ENABLE_DMA(&htim2, TIM_DMA_CC1);

	//Enable Timer 3 which enables timer 2 synchronously
	__HAL_TIM_ENABLE(&htim3);

}


void comparator_calc(void){

	uint16_t voltage_capture_index, current_capture_index, i, j, k, l;

	voltage_capture_index = NUM_CAPTURES - hdma_tim3_ch3.Instance->CNDTR;
	current_capture_index = NUM_CAPTURES - hdma_tim2_ch1.Instance->CNDTR;


	//calculate the difference between the captured values
	//Skipping the first captures because it's inaccurate from time to time
	j = voltage_capture_index;
	k = current_capture_index;
	for(i=1;i<NUM_CAPTURES;i++){
		j++;
		k++;
		if(j >= NUM_CAPTURES)
			j = 0;
		if(k >= NUM_CAPTURES)
			k = 0;
		difference[i] = voltage_capture[j] - current_capture[k];

	}


	//Calculate the frequency of the voltage channel
	//Use the same buffer as the voltage_capture to save memory
	j = voltage_capture_index;
	for(i=0;i<NUM_CAPTURES-1;i++){
		//check if the 16-bit number wraps up
		k = j + 1;
		if(k>=NUM_CAPTURES) //check for wrap around
			k = 0;
		if(voltage_capture[j] > voltage_capture[k]){
			voltage_capture[j] = 65535 - voltage_capture[j] + voltage_capture[k];
		}
		else
			voltage_capture[j] = voltage_capture[k] - voltage_capture[j];

		j++;
		if(j >= NUM_CAPTURES)
			j = 0;
	}

	//calculate average freq
	j = voltage_capture_index;
	l = 0; //valid capture counter
	float voltage_freq_count = 0; //average voltage frequency in count
	for(i=0;i<NUM_CAPTURES-1;i++){
		if(voltage_capture[j] < 4000 && voltage_capture[j] > 0){
			voltage_freq_count = voltage_freq_count + voltage_capture[j];

			j++;
			l++;
			if(j>=NUM_CAPTURES)
				j=0;
		}
		else{
			j++;
			if(j>=NUM_CAPTURES)
				j=0;
		}


	}
	voltage_freq_count = voltage_freq_count/(l); //letter "l" not 1


	//Calculate the frequency of the current channel
	//Use the same buffer as the current_capture to save memory
	j = current_capture_index;
	for(i=0;i<NUM_CAPTURES-1;i++){
		//check if the 16-bit number wraps up
		k = j + 1;
		if(k>=NUM_CAPTURES) //check for wrap around
			k = 0;
		if(current_capture[j] > current_capture[k]){
			current_capture[j] = 65535 - current_capture[j] + current_capture[k];
		}
		else
			current_capture[j] = current_capture[k] - current_capture[j];

		j++;
		if(j >= NUM_CAPTURES)
			j = 0;
	}

	//calculate average freq
	j = current_capture_index;
	l = 0;
	float current_freq_count = 0; //average voltage frequency in count
	for(i=0;i<NUM_CAPTURES-1;i++){
		if(current_capture[j] < 4000 && current_capture[j] > 0){
			current_freq_count = current_freq_count + current_capture[j];

			j++;
			l++;
			if(j>=NUM_CAPTURES)
				j=0;
		}
		else{
			j++;
			if(j>=NUM_CAPTURES)
				j=0;
		}
	}
	current_freq_count = current_freq_count/(l);


	//calculate average difference for phase calculation.
	j=0;
	int phase_count = 0;
	for(i=1;i<NUM_CAPTURES;i++){
		//filter the large differences, they are invalid
		if(difference[i] < 4000 && difference[i] > -4000){
			phase_count = phase_count + difference[i];
		}
		else
			j++;
	}
	if( (NUM_CAPTURES-j)>0 && (NUM_CAPTURES-j)>1 ){
		phase_count = phase_count/(NUM_CAPTURES-j-1);
		measurement.phase_degree = (float)phase_count/voltage_freq_count*360;

	}

	measurement.phase_degree = measurement.phase_degree - phase_corr;
	//correct phase measurement, commented out for now.
	/*
	if(measurement.phase_degree > 180)
		measurement.phase_degree = -1*(360-measurement.phase_degree);
	else if(measurement.phase_degree < -180)
		measurement.phase_degree = 360+measurement.phase_degree;
	*/

	//convert the period to frequency
	measurement.voltage_freq_Hz = 1/(voltage_freq_count*MCU_PERIOD);
	measurement.current_freq_Hz = 1/(current_freq_count*MCU_PERIOD);
}

void comparator_stop(void){

	//Disable Timers for the configurations
	__HAL_TIM_DISABLE(&htim2);
	__HAL_TIM_DISABLE(&htim3);

	//disable DMA before configuration
	__HAL_DMA_DISABLE(&hdma_tim3_ch3);
	__HAL_DMA_DISABLE(&hdma_tim2_ch1);

	TIM_CCxChannelCmd(htim3.Instance, TIM_CHANNEL_3, TIM_CCx_DISABLE);
	TIM_CCxChannelCmd(htim2.Instance, TIM_CHANNEL_1, TIM_CCx_DISABLE);
}

void comparator_start(void){

	TIM2->CNT = 0;
	TIM3->CNT = 0;

	//Enable input capture channel. Timer3_CH3: Voltage Timer2_CH1: Current
	TIM_CCxChannelCmd(htim3.Instance, TIM_CHANNEL_3, TIM_CCx_ENABLE);
	TIM_CCxChannelCmd(htim2.Instance, TIM_CHANNEL_1, TIM_CCx_ENABLE);

	//Set number of transfers
	hdma_tim3_ch3.Instance->CNDTR = NUM_CAPTURES;
	hdma_tim2_ch1.Instance->CNDTR = NUM_CAPTURES;

	/*
	//Disable half-transfer interrupt for the DMA channels
	__HAL_DMA_DISABLE_IT(&hdma_tim3_ch3, DMA_IT_HT);
	__HAL_DMA_DISABLE_IT(&hdma_tim2_ch1, DMA_IT_HT);

	//Enable error and transfer complete interrupts for the DMA channels
	__HAL_DMA_ENABLE_IT(&hdma_tim3_ch3, (DMA_IT_TC | DMA_IT_TE) );
	__HAL_DMA_ENABLE_IT(&hdma_tim2_ch1, (DMA_IT_TC | DMA_IT_TE) );
	*/
	__HAL_DMA_ENABLE(&hdma_tim3_ch3);
	__HAL_DMA_ENABLE(&hdma_tim2_ch1);

	//Enable Timer 3 which enables timer 2 synchronously
	__HAL_TIM_ENABLE(&htim3);

}

void comparator_lock_captures(void){
	uint16_t i = 0;

	//Lock samples for processing later
	for(i=0;i<NUM_CAPTURES;i++){
		voltage_capture_locked[i] = voltage_capture[i];
		current_capture_locked[i] = current_capture[i];
	}

	//Lock index
	voltage_capture_index = NUM_CAPTURES - hdma_tim3_ch3.Instance->CNDTR;
	current_capture_index = NUM_CAPTURES - hdma_tim2_ch1.Instance->CNDTR;

}

comparator_measurement comparator_get_measurement(void){
	return measurement;
}

void comparator_set_phase_correction(float phase){
	phase_corr = phase;
}


