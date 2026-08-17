/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>

#include "stm32g4xx.h"
#include "arm_math.h"
#include "calculation.h"
#include "comparator.h"
#include "ADC_acquisition.h"
#include "modbus_func.h"
#include "EEPROM.h"
#include "GPIO_func.h"
#include "main.h"

static float measurement_data[14]; // Expanded to 14 to hold Vrg phases

static power power_stats;
static DFT_results DFT_main_tone;
static limits measurement_limits;
static enum Vrg_CH Vrg = Vrg1;

volatile float phase;

void calculate_fault_conditions(void);
void calculate_multiplex_Vrg(void);

//Calculate the measurement data from samples and captures
void calculate_data(void){
	ADC_measurement ADC_measurement;
	comparator_measurement comparator_measurement;
	float phase_in_radians;
	enum Vrg_CH Vrg_current;

	Vrg_current = Vrg;
	//we are switching the Vrg channel right away,
	//After running calculate_multiplex_Vrg function,
	//if Vrg is Vrg1 ADC_measurement.Vrg_RMS holds data for the Vrg2 visa versa
	calculate_multiplex_Vrg();

	comparator_calc(); //run the comparator algorithm
	ADC_acquisition_RMS_calc(Vrg_current); //calculate the RMS values from the captured samples
	ADC_acquisition_peak_calc(); //calculate the peak to peak from the captured samples

	ADC_measurement = ADC_acquisition_get_measurement();
	comparator_measurement = comparator_get_measurement();

	//store the RMS, frequency and phase measurements
	measurement_data[0] = ADC_measurement.voltage_RMS;
	measurement_data[1] = ADC_measurement.current_RMS;
	measurement_data[2] = comparator_measurement.voltage_freq_Hz;
	measurement_data[3] = comparator_measurement.current_freq_Hz;
	measurement_data[4] = comparator_measurement.phase_degree;

	//store the max/min measurements
	measurement_data[5] = ADC_measurement.voltage_pos_peak;
	measurement_data[6] = ADC_measurement.voltage_neg_peak;
	measurement_data[7] = ADC_measurement.current_pos_peak;
	measurement_data[8] = ADC_measurement.current_neg_peak;


	//since we switched Vrg channels
	//whatever Vrg says it's the opposite
	if(Vrg == Vrg2){
		measurement_data[9] = ADC_measurement.Vrg1_RMS; //hold Vrg1 measurements
	}
	if(Vrg == Vrg1){
		measurement_data[10] = ADC_measurement.Vrg2_RMS; //holds Vrg2 measurements
	}

	phase_in_radians = comparator_measurement.phase_degree*DEG2RAD;

	//calculate the power stats
	power_stats.apparent_power = ADC_measurement.voltage_RMS*ADC_measurement.current_RMS;
	power_stats.real_power = power_stats.apparent_power*arm_cos_f32(phase_in_radians);
	power_stats.reactive_power = power_stats.apparent_power*arm_sin_f32(phase_in_radians);
	power_stats.power_factor = power_stats.real_power /power_stats.apparent_power;


	//DFT calculation
	uint32_t freq_ind = 29; // 85 kHz Fundamental
	float temp_dft[7];      // Safe catcher array for 7 outputs
	
	// 1. Calculate Fundamental (85 kHz)
	ADC_acquisition_DFT(freq_ind, Vrg_current, temp_dft);
	
	// --- CAPTURE ALL 7 OUTPUTS ---
	DFT_main_tone.voltage_amplitude = temp_dft[0];
	DFT_main_tone.current_amplitude = temp_dft[1];
	DFT_main_tone.voltage_phase = temp_dft[2];
	DFT_main_tone.current_phase = temp_dft[3];
	DFT_main_tone.phase_difference = temp_dft[4];       // Main Phase Shift!
	DFT_main_tone.Vrg_phase_difference = temp_dft[5];   // Delta V Phase Shift!

	// 2. Calculate Harmonics Safely (3rd, 5th, and 7th only)
	uint32_t multipliers[3] = {3, 5, 7};
	
	for(int h = 0; h < 3; h++) {
		ADC_acquisition_DFT(freq_ind * multipliers[h], Vrg_current, temp_dft);
		
		float v_amp = temp_dft[0];
		float i_amp = temp_dft[1];
		float vrg_amp = temp_dft[6]; 

		if(h == 0) { DFT_main_tone.v_h3 = v_amp; DFT_main_tone.i_h3 = i_amp; if(Vrg == Vrg2) DFT_main_tone.vrg1_h3 = vrg_amp; if(Vrg == Vrg1) DFT_main_tone.vrg2_h3 = vrg_amp; }
		if(h == 1) { DFT_main_tone.v_h5 = v_amp; DFT_main_tone.i_h5 = i_amp; if(Vrg == Vrg2) DFT_main_tone.vrg1_h5 = vrg_amp; if(Vrg == Vrg1) DFT_main_tone.vrg2_h5 = vrg_amp; }
		if(h == 2) { DFT_main_tone.v_h7 = v_amp; DFT_main_tone.i_h7 = i_amp; if(Vrg == Vrg2) DFT_main_tone.vrg1_h7 = vrg_amp; if(Vrg == Vrg1) DFT_main_tone.vrg2_h7 = vrg_amp; }
	}

	// --- ASSIGN DELTA V PHASES AFTER THE DFT RUNS ---
	if(Vrg == Vrg2){
		measurement_data[11] = DFT_main_tone.Vrg_phase_difference; // Anchor Delta V to Current
	}
	if(Vrg == Vrg1){
		measurement_data[12] = DFT_main_tone.Vrg_phase_difference; // Anchor Delta V to Current
	}
	// ------------------------------------------------

	calculate_fault_conditions();
}

//load the server registers
void calculate_load_server_registers(void){
	uint8_t i = 0;
	uint16_t *p_measurement;

	//load RMS, frequency and phase measurements to server registers
	p_measurement = (uint16_t *)measurement_data;
	for(i=VOLTAGE_RMS_L; i<(PHASE_H+1);i++){
		MODBUS_set_server_register(i, *p_measurement );
		p_measurement++;
	}

	//load power stats to the server registers
	p_measurement = (uint16_t *)&power_stats;
	for(i=APPARENT_PWR_ADDR_L; i<(PWR_FACTOR_ADDR_H+1);i++){
		MODBUS_set_server_register(i, *p_measurement );
		p_measurement++;
	}

	//load peak values to the server registers
	p_measurement = (uint16_t *)&measurement_data[5];
	for(i=VOLTAGE_MIN_L;i<(CURRENT_MAX_H+1);i++){
		MODBUS_set_server_register(i, *p_measurement );
		p_measurement++;
	}

	//load DFT phase to the server registers
	p_measurement = (uint16_t *)&DFT_main_tone.phase_difference;
	for(i=DFT_PHASE_MAIN_TONE_L;i<(DFT_PHASE_MAIN_TONE_H+1);i++){
		MODBUS_set_server_register(i, *p_measurement );
		p_measurement++;
	}

	//load the Vrg measurements to the server registers
	if(Vrg == Vrg2){
		p_measurement = (uint16_t *)&measurement_data[9]; // RMS
		for(i=VRG1_RMS_L;i<(VRG1_RMS_H+1);i++){
			MODBUS_set_server_register(i, *p_measurement );
			p_measurement++;
		}
		p_measurement = (uint16_t *)&measurement_data[11]; // Phase
		for(i=VRG1_PHASE_L;i<(VRG1_PHASE_H+1);i++){
			MODBUS_set_server_register(i, *p_measurement );
			p_measurement++;
		}
	}

	if(Vrg == Vrg1){
		p_measurement = (uint16_t *)&measurement_data[10]; // RMS
		for(i=VRG2_RMS_L;i<(VRG2_RMS_H+1);i++){
			MODBUS_set_server_register(i, *p_measurement );
			p_measurement++;
		}
		p_measurement = (uint16_t *)&measurement_data[12]; // Phase
		for(i=VRG2_PHASE_L;i<(VRG2_PHASE_H+1);i++){
			MODBUS_set_server_register(i, *p_measurement );
			p_measurement++;
		}
	}

	// Load Harmonics to Modbus Server (Addresses 70 to 117) ---
	uint16_t *p_harmonics = (uint16_t *)&DFT_main_tone.v_h3;
	
	for(i = HARMONICS_START_ADDR; i <= HARMONICS_END_ADDR; i++){
		MODBUS_set_server_register(i, *p_harmonics);
		p_harmonics++;
	}

}

void calculate_fault_conditions(void){
    float Voltage_RMS, Current_RMS;
    bool OC_condition, OV_condition, no_comm_timeout;
    uint16_t current_relays;

    // --- EMULATION OVERRIDE LOGIC ---
    uint16_t sim_mode_active = MODBUS_get_server_register(SIM_MODE_ADDR);

    if (sim_mode_active == 1) {
        // Read the injected fake values safely
        uint16_t sim_v_regs[2] = { MODBUS_get_server_register(SIM_VOLTAGE_L), MODBUS_get_server_register(SIM_VOLTAGE_H) };
        uint16_t sim_i_regs[2] = { MODBUS_get_server_register(SIM_CURRENT_L), MODBUS_get_server_register(SIM_CURRENT_H) };
        
        Voltage_RMS = MODBUS_convert_16_bit_to_float(sim_v_regs);
        Current_RMS = MODBUS_convert_16_bit_to_float(sim_i_regs);
    } else {
        // Use true physical measurements
        Voltage_RMS = measurement_data[0];
        Current_RMS = measurement_data[1];
    }
    // --------------------------------

    // Read current fault bits from Modbus register memory
    OC_condition = MODBUS_get_bit(OC_REGISTER_NUM, OC_BIT_POS);
    OV_condition = MODBUS_get_bit(OV_REGISTER_NUM, OV_BIT_POS);
    no_comm_timeout = MODBUS_get_bit(NO_COMM_REGISTER_NUM, NO_COMM_BIT_POS);

    // Grab the live state of the relays
    current_relays = MODBUS_get_server_register(CONTACTOR_ADDR_L);

    // 1. Detect and LATCH Over-Voltage
    if(Voltage_RMS > measurement_limits.OV_limit){
        MODBUS_set_bit(OV_REGISTER_NUM, OV_BIT_POS);
        OV_condition = true; // Force local flag true for this cycle
    }

    // 2. Detect and LATCH Over-Current
    if(Current_RMS > measurement_limits.OC_limit){
        MODBUS_set_bit(OC_REGISTER_NUM, OC_BIT_POS);
        OC_condition = true; // Force local flag true for this cycle
    }

    // 3. STRICT LATCHING ACTION: 
    // If ANY fault bit is active in Modbus, Relay 0 stays LOCKED ON.
    // It will NEVER turn off here, even if voltage/current go back to normal.
    if(OV_condition || OC_condition || no_comm_timeout) {
        current_relays |= (1 << 0); // Force Relay 0 ON
    } 
    else {
        // ONLY safe to turn off Relay 0 if Modbus Register 26 is completely 0 ( cleared by Python Reset )
        
    }

    // 4. Save state and trigger hardware
    MODBUS_set_server_register(CONTACTOR_ADDR_L, current_relays);
    GPIO_func_configure_contactors(current_relays);
}


void calculate_read_calibration_data_EEPROM(void){
	uint8_t i = 0;
	float calibration_data[6];
	float offset_data[4];
	uint16_t *p_measurement_limit;

	//read from EEPROM
	EEPROM_read_float(&offset_data[0], VOLTAGE_OFFSET_ADDR);
	EEPROM_read_float(&offset_data[1], CURRENT_OFFSET_ADDR);
	EEPROM_read_float(&offset_data[2], VRG1_OFFSET_ADDR);
	EEPROM_read_float(&offset_data[3], VRG2_OFFSET_ADDR);
	EEPROM_read_float(&calibration_data[0], VOLTAGE_GAIN_ADDR);
	EEPROM_read_float(&calibration_data[1], CURRENT_GAIN_ADDR);
	EEPROM_read_float(&calibration_data[2], VRG1_GAIN_ADDR);
	EEPROM_read_float(&calibration_data[3], VRG2_GAIN_ADDR);
	EEPROM_read_float(&calibration_data[4], PHASE_CORR_ADDR);
	EEPROM_read_float(&calibration_data[5], PHASE_CORR_DFT_ADDR);

	EEPROM_read_float(&measurement_limits.OV_limit, OV_LIMIT_ADDR);
	EEPROM_read_float(&measurement_limits.UV_limit, UV_LIMIT_ADDR);
	EEPROM_read_float(&measurement_limits.OC_limit, OC_LIMIT_ADDR);
	EEPROM_read_float(&measurement_limits.UC_limit, UC_LIMIT_ADDR);
	EEPROM_read_float(&measurement_limits.phase_limit, PHASE_LIMIT_ADDR);

	// --- FIXED EEPROM MODBUS MAPPING ---
	// 1. Load Offset Data
	uint16_t *p_offset = (uint16_t *)offset_data;
	for(i = VOLTAGE_OFFSET_L; i < VRG2_OFFSET_H + 1; i++){
		MODBUS_set_server_register(i, *p_offset);
		p_offset++;
	}

	// 2. Load Calibration (Gain) Data
	uint16_t *p_cal = (uint16_t *)calibration_data;

	MODBUS_set_server_register(VOLTAGE_GAIN_ADDR_L, p_cal[0]);
	MODBUS_set_server_register(VOLTAGE_GAIN_ADDR_H, p_cal[1]);

	MODBUS_set_server_register(CURRENT_GAIN_ADDR_L, p_cal[2]);
	MODBUS_set_server_register(CURRENT_GAIN_ADDR_H, p_cal[3]);

	MODBUS_set_server_register(VRG1_GAIN_L, p_cal[4]);
	MODBUS_set_server_register(VRG1_GAIN_H, p_cal[5]);

	MODBUS_set_server_register(VRG2_GAIN_L, p_cal[6]);
	MODBUS_set_server_register(VRG2_GAIN_H, p_cal[7]);

	MODBUS_set_server_register(PHASE_CORR_ADDR_L, p_cal[8]);
	MODBUS_set_server_register(PHASE_CORR_ADDR_H, p_cal[9]);

	MODBUS_set_server_register(PHASE_CORR_DFT_ADDR_L, p_cal[10]);
	MODBUS_set_server_register(PHASE_CORR_DFT_ADDR_H, p_cal[11]);
	// -----------------------------------

	p_measurement_limit = (uint16_t *)(&measurement_limits);
	//
	for(i=OV_LIMIT_L; i<PHASE_LIMIT_H+1; i++){
		MODBUS_set_server_register(i, *p_measurement_limit);
		p_measurement_limit++;
	}
	//Set the gains for internal calculations
	ADC_acquisition_set_gains(calibration_data);

	//set phase correction for internal calculation
	comparator_set_phase_correction(calibration_data[4]);

	//set phase correction for internal calculation
	ADC_acquistion_set_phase_correction(calibration_data[5]);

	//set offset correction for internal calculation
	ADC_acquisition_set_offset(offset_data);
}

void calculate_set_dummy_calibration_data(void){

	uint8_t i = 0;
	float calibration_data[4];
	uint16_t *p_calibration_data, *p_measurement_limit;

	calibration_data[0] = 1;
	calibration_data[1] = 1;
	calibration_data[2] = 0;
	calibration_data[3] = 0;

	p_calibration_data = (uint16_t *)calibration_data;
	//load server register for the MODBUS
	for(i=VOLTAGE_GAIN_ADDR_L; i<PHASE_CORR_DFT_ADDR_H+1;i++){
		MODBUS_set_server_register(i, *p_calibration_data );
		p_calibration_data++;
	}

	p_measurement_limit = (uint16_t *)(&measurement_limits);
	//
	for(i=OV_LIMIT_L; i<PHASE_LIMIT_H+1; i++){
		MODBUS_set_server_register(i, *p_measurement_limit);
		p_measurement_limit++;
	}
	//Set the gains for internal calculations
	ADC_acquisition_set_gains(calibration_data);

	//set phase correction for internal calculation
	comparator_set_phase_correction(calibration_data[2]);

	//set phase correction for internal calculation
	ADC_acquistion_set_phase_correction(calibration_data[3]);

}

void calculate_multiplex_Vrg(void){
	if(Vrg == Vrg1){
		Vrg = Vrg2;
		HAL_GPIO_WritePin(Multiplex_SW_GPIO_Port, Multiplex_SW_Pin, 1);
	}
	else if(Vrg == Vrg2){
		Vrg = Vrg1;
		HAL_GPIO_WritePin(Multiplex_SW_GPIO_Port, Multiplex_SW_Pin, 0);
	}
}