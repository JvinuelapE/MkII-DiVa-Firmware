/*
 * ADC_acquisition.h
 *
 *  Created on: Mar 2, 2024
 *      Author: Alperen Akkünücü
 */

#ifndef INC_ADC_ACQUISITION_H_
#define INC_ADC_ACQUISITION_H_


#define NUM_SAMPLES 1024
#define SAMPLING_RATE 3000000
#define MCU_FREQ_HZ 120000000
#define REF 5
#define BIT_RESOLUTION 12
#define DIVISION_FACTOR 4096

#define DEG2RAD M_PI/180.0
#define RAD2DEF 180/M_PI

typedef struct ADC_measurement ADC_measurement;
struct ADC_measurement{
	float voltage_RMS;
	float current_RMS;
	float Vrg1_RMS;
	float Vrg2_RMS;
	float voltage_pos_peak;
	float current_pos_peak;
	float voltage_neg_peak;
	float current_neg_peak;
};

typedef struct gain_data gain_data;
struct gain_data{
	float voltage_gain;
	float current_gain;
	float Vrg1_gain;
	float Vrg2_gain;
};


typedef struct offset_data offset_data;
struct offset_data{
	int32_t voltage_channel;
	int32_t current_channel;
	int32_t Vrg1_channel;
	int32_t Vrg2_channel;
};
/*
 * Sets the DMA configurations, DMA interrupt and SPI settings for the ADCs
 */
void ADC_acquisition_config_DMA(void);
void ADC_acquisition_start(void);
void ADC_acquisition_RMS_calc(uint8_t Vrg_channel);
void ADC_acquisition_peak_calc(void);
void ADC_acquisition_DFT(uint32_t freq_ind, uint8_t Vrg_channel, float *results);
ADC_measurement ADC_acquisition_get_measurement(void);
void ADC_acquisition_stop_sampling(void);
void ADC_acquisition_set_gains(float *gains);
void ADC_acquistion_set_phase_correction(float phase_corr);
void ADC_acquisition_set_offset(float *offsets);

#endif /* INC_ADC_ACQUISITION_H_ */
