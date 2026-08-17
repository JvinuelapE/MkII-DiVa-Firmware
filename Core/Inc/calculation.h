/*
 * communication.h
 *
 *  Created on: Apr 7, 2024
 *      Author: Alperen Akkünücü
 */

#ifndef INC_CALCULATION_H_
#define INC_CALCULATION_H_


#define DEG2RAD M_PI/180.0
#define RAD2DEF 180/M_PI

typedef struct power power;
struct power{
	float apparent_power;
	float real_power;
	float reactive_power;
	float power_factor;
};



typedef struct DFT_results DFT_results;
struct DFT_results{
	float voltage_amplitude;
	float current_amplitude;
	float voltage_phase;
	float current_phase;
	float phase_difference;
	float Vrg_phase_difference;

	float v_h3, i_h3, vrg1_h3, vrg2_h3;
	float v_h5, i_h5, vrg1_h5, vrg2_h5;
	float v_h7, i_h7, vrg1_h7, vrg2_h7;
	float v_h9, i_h9, vrg1_h9, vrg2_h9;
	float v_h11, i_h11, vrg1_h11, vrg2_h11;
	float v_h13, i_h13, vrg1_h13, vrg2_h13;
};


typedef struct limits limits;
struct limits{
	float OV_limit;
	float UV_limit;
	float OC_limit;
	float UC_limit;
	float phase_limit;
};

enum Vrg_CH {Vrg1 = 1, Vrg2 = 2};

void calculate_data(void);
void calculate_load_server_registers(void);
void calculate_read_calibration_data_EEPROM(void);
void calculate_read_calibration_data_MODBUS(void);

void calculate_set_dummy_calibration_data(void);

#endif /* INC_CALCULATION_H_ */
