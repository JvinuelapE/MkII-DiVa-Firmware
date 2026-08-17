/*
 * ADC_acquisition.c
 *
 *  Created on: Mar 2, 2024
 *      Author: Alperen Akkünücü
 */

/* Includes ------------------------------------------------------------------*/
#include "arm_math.h"
#include "main.h"
#include "ADC_acquisition.h"
#include "stm32g4xx.h"

//SPI handles
extern SPI_HandleTypeDef hspi1;	//Current Channel ADC
extern SPI_HandleTypeDef hspi2; //Vrg ADC
extern SPI_HandleTypeDef hspi3; //Voltage Channel ADC


//DMA handles
extern DMA_HandleTypeDef hdma_spi1_rx;
extern DMA_HandleTypeDef hdma_spi2_rx;
extern DMA_HandleTypeDef hdma_spi3_rx;

extern DMA_HandleTypeDef hdma_tim4_up;	//timer to trigger SPI transfer for the Current Channel (SPI1)
extern DMA_HandleTypeDef hdma_tim15_up; //timer to trigger SPI transfer for the Voltage Channel (SPI3)
extern DMA_HandleTypeDef hdma_tim20_up; //timer to trigger SPI transfer for Vrg (SPI2)
//Timer handles
extern TIM_HandleTypeDef htim6;
extern TIM_HandleTypeDef htim4;  //Current
extern TIM_HandleTypeDef htim15; //Voltage
extern TIM_HandleTypeDef htim20; //Vrg

//UART Handle
extern UART_HandleTypeDef huart2;

static uint16_t dummy_SPI_data_tim4 = 0xFF;
static uint16_t dummy_SPI_data_tim15 = 0xFF;
static uint16_t dummy_SPI_data_tim20 = 0xFF;
static int16_t hex_voltage_samples[NUM_SAMPLES], hex_current_samples[NUM_SAMPLES], hex_Vrg_samples[NUM_SAMPLES];
static ADC_measurement measurement;

//calibration values
static gain_data gain;
static offset_data offset;
static float DFT_phase_correction;

void ADC_convert_samples(uint8_t Vrg_channel);

void ADC_acquisition_config_DMA(void){

	//stop timers
	HAL_TIM_Base_Stop_IT(&htim6);
	//stop timer and disable DMA channel
	HAL_TIM_Base_Stop_DMA(&htim4);
	HAL_TIM_Base_Stop_DMA(&htim15);
	HAL_TIM_Base_Stop_DMA(&htim20);

	//reset counter value
	htim6.Instance->CNT = 0;
	htim4.Instance->CNT = 0;
	htim15.Instance->CNT = 0;
	htim20.Instance->CNT = 0;
	//set the period
	htim4.Instance->ARR = MCU_FREQ_HZ/SAMPLING_RATE - 1;
	htim15.Instance->ARR = MCU_FREQ_HZ/SAMPLING_RATE - 1;
	htim20.Instance->ARR = MCU_FREQ_HZ/SAMPLING_RATE - 1;

	//Set timer4 and timer 15 to stop when the MCU stops in debugging mode
	DBGMCU->APB1FZR1 = DBGMCU->APB1FZR1 | DBGMCU_APB1FZR1_DBG_TIM4_STOP_Msk;
	DBGMCU->APB2FZ = DBGMCU->APB2FZ | DBGMCU_APB2FZ_DBG_TIM15_STOP_Msk;
	DBGMCU->APB2FZ = DBGMCU->APB2FZ | DBGMCU_APB2FZ_DBG_TIM20_STOP_Msk;

	//disable SPI
	__HAL_SPI_DISABLE(&hspi1);
	__HAL_SPI_DISABLE(&hspi2);
	__HAL_SPI_DISABLE(&hspi3);

	//dummy read to clear SPI's data register
	(void)hspi3.Instance->DR;
	(void)hspi1.Instance->DR;
	(void)hspi2.Instance->DR;

	//disable DMA before configuration
	__HAL_DMA_DISABLE(&hdma_spi1_rx);
	__HAL_DMA_DISABLE(&hdma_spi2_rx);
	__HAL_DMA_DISABLE(&hdma_spi3_rx);
	__HAL_DMA_DISABLE(&hdma_tim4_up);
	__HAL_DMA_DISABLE(&hdma_tim15_up);
	__HAL_DMA_DISABLE(&hdma_tim20_up);

	//set memory address
	hdma_spi3_rx.Instance->CMAR = (uint32_t)hex_voltage_samples;
	hdma_spi2_rx.Instance->CMAR = (uint32_t)hex_Vrg_samples;
	hdma_spi1_rx.Instance->CMAR = (uint32_t)hex_current_samples;
	hdma_tim4_up.Instance->CMAR = (uint32_t)&dummy_SPI_data_tim4;
	hdma_tim15_up.Instance->CMAR = (uint32_t)&dummy_SPI_data_tim15;
	hdma_tim20_up.Instance->CMAR = (uint32_t)&dummy_SPI_data_tim20;

	//set peripheral address
	hdma_spi3_rx.Instance->CPAR = (uint32_t)&hspi3.Instance->DR;
	hdma_spi2_rx.Instance->CPAR = (uint32_t)&hspi2.Instance->DR;
	hdma_spi1_rx.Instance->CPAR = (uint32_t)&hspi1.Instance->DR;
	hdma_tim4_up.Instance->CPAR = (uint32_t)&hspi3.Instance->DR;
	hdma_tim20_up.Instance->CPAR = (uint32_t)&hspi2.Instance->DR;
	hdma_tim15_up.Instance->CPAR = (uint32_t)&hspi1.Instance->DR;

	//set number of transfers
	hdma_spi1_rx.Instance->CNDTR = NUM_SAMPLES;
	hdma_spi2_rx.Instance->CNDTR = NUM_SAMPLES;
	hdma_spi3_rx.Instance->CNDTR = NUM_SAMPLES;
	hdma_tim4_up.Instance->CNDTR = NUM_SAMPLES;
	hdma_tim15_up.Instance->CNDTR = NUM_SAMPLES;
	hdma_tim20_up.Instance->CNDTR = NUM_SAMPLES;

	//enable error and transfer complete interrupt.
    __HAL_DMA_DISABLE_IT(&hdma_spi3_rx, DMA_IT_HT); // disable half transfer interrupt
    __HAL_DMA_ENABLE_IT(&hdma_spi3_rx, (DMA_IT_TC | DMA_IT_TE)); //enable transfer error and transfer complete

    /* Enable the Peripheral */
    __HAL_DMA_ENABLE(&hdma_spi3_rx);
    __HAL_DMA_ENABLE(&hdma_spi2_rx);
    __HAL_DMA_ENABLE(&hdma_spi1_rx);
    __HAL_DMA_ENABLE(&hdma_tim4_up);
    __HAL_DMA_ENABLE(&hdma_tim15_up);
    __HAL_DMA_ENABLE(&hdma_tim20_up);


    /* Enable Rx DMA Request */
    SET_BIT(hspi3.Instance->CR2, SPI_CR2_RXDMAEN);
    SET_BIT(hspi2.Instance->CR2, SPI_CR2_RXDMAEN);
    SET_BIT(hspi1.Instance->CR2, SPI_CR2_RXDMAEN);

    /* Enable timer DMA request upon update*/
    SET_BIT(htim20.Instance->DIER, TIM_DIER_UDE);
    SET_BIT(htim15.Instance->DIER, TIM_DIER_UDE);
    SET_BIT(htim4.Instance->DIER, TIM_DIER_UDE);

    /* Enable the SPI Error Interrupt Bit */
    //__HAL_SPI_ENABLE_IT(&hspi3, (SPI_IT_ERR));

    //enable SPI
    __HAL_SPI_ENABLE(&hspi1);
    __HAL_SPI_ENABLE(&hspi2);
    __HAL_SPI_ENABLE(&hspi3);


}

void ADC_acquisition_start(void){
	//reset counter value
	//htim6.Instance->CNT = 0;
	//start timer
	//HAL_TIM_Base_Start_IT(&htim6);
	htim4.Instance->CNT = 0;
	htim20.Instance->CNT = 0;
	htim15.Instance->CNT = 0;

	//__HAL_TIM_ENABLE(&htim4);
	__HAL_TIM_ENABLE(&htim15);
}

void ADC_convert_samples(uint8_t Vrg_channel){

	uint16_t i;

	//Shift the MSB (sign bit) to the MSB
	for(i=0;i<NUM_SAMPLES;i++){
		hex_voltage_samples[i] = ((hex_voltage_samples[i])<<3);
		hex_current_samples[i] = (hex_current_samples[i]<<3);
		hex_Vrg_samples[i] = (hex_Vrg_samples[i]<<3);
	}
	//Correct for the shift that is made earlier.
	for(i=0;i<NUM_SAMPLES;i++){
		hex_voltage_samples[i] = ((hex_voltage_samples[i])/16);
		hex_current_samples[i] = (hex_current_samples[i]/16);
		hex_Vrg_samples[i] = (hex_Vrg_samples[i]/16);
	}
	//Subtract the offset
	for(i=0;i<NUM_SAMPLES;i++){
		hex_voltage_samples[i] = hex_voltage_samples[i] - offset.voltage_channel;
		hex_current_samples[i] = hex_current_samples[i] - offset.current_channel;
		if(Vrg_channel == 1)
			hex_Vrg_samples[i] = hex_Vrg_samples[i] - offset.Vrg1_channel;
		else if(Vrg_channel == 2)
			hex_Vrg_samples[i] = hex_Vrg_samples[i] - offset.Vrg2_channel;
	}
}

void ADC_acquisition_RMS_calc(uint8_t Vrg_channel){

	uint16_t i = 0;
	uint64_t voltage = 0, current = 0, Vrg = 0;

	ADC_convert_samples(Vrg_channel);

	for(i=0;i<NUM_SAMPLES;i++){
		voltage = voltage + hex_voltage_samples[i]*hex_voltage_samples[i];
		current = current + hex_current_samples[i]*hex_current_samples[i];
		Vrg = Vrg + hex_Vrg_samples[i]*hex_Vrg_samples[i];
	}

	voltage = voltage/NUM_SAMPLES;
	current = current/NUM_SAMPLES;
	Vrg = Vrg/NUM_SAMPLES;

	measurement.voltage_RMS = (sqrt((float)voltage)*REF/(DIVISION_FACTOR) )/gain.voltage_gain;
	measurement.current_RMS = (sqrt((float)current)*REF/(DIVISION_FACTOR) )/gain.current_gain;

	if(Vrg_channel == 1)
		measurement.Vrg1_RMS = (sqrt((float)Vrg)*REF/(DIVISION_FACTOR) )/gain.Vrg1_gain;
	if(Vrg_channel == 2)
		measurement.Vrg2_RMS = (sqrt((float)Vrg)*REF/(DIVISION_FACTOR) )/gain.Vrg2_gain;
}


//find the max and minimum values in the sample arrays
//for both voltage and current channels
void ADC_acquisition_peak_calc(void){

	int16_t max_voltage, max_current, min_voltage, min_current;
	uint32_t i;

	//initialize the samples with the first values in the arrays
	max_voltage = hex_voltage_samples[0];
	max_current = hex_current_samples[0];

	min_voltage = hex_voltage_samples[0];
	min_current = hex_current_samples[0];

	//search the arrays for the max
	for(i=0;i<NUM_SAMPLES;i++){
		if(max_voltage < hex_voltage_samples[i])
			max_voltage = hex_voltage_samples[i];

		if(max_current < hex_current_samples[i])
			max_current = hex_current_samples[i];
	}

	//search the array for the min
	for(i=0;i<NUM_SAMPLES;i++){
		if(min_voltage > hex_voltage_samples[i])
			min_voltage = hex_voltage_samples[i];

		if(min_current > hex_current_samples[i])
			min_current = hex_current_samples[i];
	}

	//convert the min/max values to float
	measurement.voltage_pos_peak = (float)max_voltage*REF/(DIVISION_FACTOR);
	measurement.current_pos_peak = (float)max_current*REF/(DIVISION_FACTOR);

	measurement.voltage_neg_peak = (float)min_voltage*REF/(DIVISION_FACTOR);
	measurement.current_neg_peak = (float)min_current*REF/(DIVISION_FACTOR);

}

void ADC_acquisition_DFT(uint32_t freq_ind, uint8_t Vrg_channel, float *results){

	//Variables to hold DFT calculations
	float voltage_real = 0, voltage_complex = 0,
		  current_real = 0, current_complex = 0,
		  Vrg_real = 0, Vrg_complex = 0,             // <-- NEW
		  *p_results, cos_term, sine_term, trig_term;

	//Variables to hold results
	float voltage_amplitude, current_amplitude,
		  voltage_phase, current_phase, phase_difference,
		  Vrg_phase, Vrg_phase_difference;           // <-- NEW
		  
	uint32_t i;
	p_results = results;

	trig_term = 2*M_PI*freq_ind/NUM_SAMPLES;
	
	//calculate the real part of the DFT
	for(i=0;i<NUM_SAMPLES;i++){
		cos_term = arm_cos_f32(i*trig_term);
		voltage_real = hex_voltage_samples[i]*cos_term + voltage_real ;
		current_real = hex_current_samples[i]*cos_term + current_real ;
		Vrg_real = hex_Vrg_samples[i]*cos_term + Vrg_real ;          // <-- NEW
	}

	//calculate the imaginary part of the DFT
	for(i=0;i<NUM_SAMPLES;i++){
		sine_term = arm_sin_f32(i*trig_term);
		voltage_complex = hex_voltage_samples[i]*sine_term + voltage_complex ;
		current_complex = hex_current_samples[i]*sine_term + current_complex ;
		Vrg_complex = hex_Vrg_samples[i]*sine_term + Vrg_complex ;    // <-- NEW
	}
	
	//negate results
	voltage_complex = -1*voltage_complex;
	current_complex = -1*current_complex;
	Vrg_complex = -1*Vrg_complex;                                     // <-- NEW

	voltage_amplitude = sqrt(voltage_real*voltage_real + voltage_complex*voltage_complex);
	current_amplitude = sqrt(current_real*current_real + current_complex*current_complex);
	
	float Vrg_amplitude = sqrt(Vrg_real*Vrg_real + Vrg_complex*Vrg_complex);

	// --- NEW: SCALE RAW DFT TO PHYSICAL RMS ---
	// 1. Peak Amplitude = 2 * Magnitude / NUM_SAMPLES
	// 2. RMS Amplitude = Peak / sqrt(2) = Magnitude * sqrt(2) / NUM_SAMPLES
	// 3. Physical Value = RMS * REF / (DIVISION_FACTOR * Hardware_Gain)
	float dft_to_rms = 1.41421356f / NUM_SAMPLES;
	float adc_to_volts = (float)REF / DIVISION_FACTOR;
	float base_scale = dft_to_rms * adc_to_volts;

	voltage_amplitude = (voltage_amplitude * base_scale) / gain.voltage_gain;
	current_amplitude = (current_amplitude * base_scale) / gain.current_gain;

	if(Vrg_channel == 1){
		Vrg_amplitude = (Vrg_amplitude * base_scale) / gain.Vrg1_gain;
	} else {
		Vrg_amplitude = (Vrg_amplitude * base_scale) / gain.Vrg2_gain;
	}

	voltage_phase = atan2(voltage_complex,voltage_real)*RAD2DEF; //in degrees
	current_phase = atan2(current_complex,current_real)*RAD2DEF; //in degrees
	Vrg_phase = atan2(Vrg_complex, Vrg_real)*RAD2DEF;            // <-- NEW
	
	// Calculate the Phase Shifts!
	phase_difference = current_phase - voltage_phase;
	Vrg_phase_difference = current_phase - Vrg_phase;            // <-- NEW: Anchor Delta V to Current

	//store the values in results.
	*p_results = voltage_amplitude;
	p_results++;
	*p_results = current_amplitude;
	p_results++;
	*p_results = voltage_phase;
	p_results++;
	*p_results = current_phase;
	p_results++;
	*p_results = phase_difference - DFT_phase_correction; 
	p_results++;
	*p_results = Vrg_phase_difference;
	p_results++;
	*p_results = Vrg_amplitude;
}


ADC_measurement ADC_acquisition_get_measurement(void){
	return measurement;
}

void ADC_acquisition_stop_sampling(void){
	//HAL_TIM_Base_Stop_IT(&htim6); //stop timer 6 to stop ADC acquisition
	HAL_TIM_Base_Stop_DMA(&htim15); //stop timer 15 to stop ADC acquisition
	//HAL_TIM_Base_Stop_DMA(&htim4); //stop timer 15 to stop ADC acquisition
}

void ADC_acquisition_set_gains(float *gains){
	float *p_gain;

	p_gain = gains;

	gain.voltage_gain = *p_gain;
	p_gain++;
	gain.current_gain = *p_gain;
	p_gain++;
	gain.Vrg1_gain = *p_gain;
	p_gain++;
	gain.Vrg2_gain = *p_gain;
}

void ADC_acquistion_set_phase_correction(float phase_corr){

	DFT_phase_correction = phase_corr;

}

void ADC_acquisition_set_offset(float *offsets){
	float *p_offsets;

	p_offsets = offsets;

	offset.voltage_channel = (int32_t)*p_offsets;
	p_offsets++;
	offset.current_channel = (int32_t)*p_offsets;
	p_offsets++;
	offset.Vrg1_channel = (int32_t)*p_offsets;
	p_offsets++;
	offset.Vrg2_channel = (int32_t)*p_offsets;

}

