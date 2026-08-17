/*
 * comparator.h
 *
 *  Created on: Mar 23, 2024
 *      Author: Alperen Akkünücü
 */

#ifndef INC_COMPARATOR_H_
#define INC_COMPARATOR_H_

#define NUM_CAPTURES 25
#define MCU_PERIOD 1/120000000 //clock frequency 120MHz

typedef struct comparator_measurement comparator_measurement;

struct comparator_measurement{
	float voltage_freq_Hz;
	float current_freq_Hz;
	float phase_degree;
};

void comparator_config(void);

void comparator_calc(void);

void comparator_stop(void);

void comparator_start(void);

void comparator_lock_captures(void);

comparator_measurement comparator_get_measurement(void);

void comparator_set_phase_correction(float phase);


#endif /* INC_COMPARATOR_H_ */
