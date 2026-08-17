/*
 * modbus.h
 *
 *  Created on: May 12, 2024
 *      Author: Alperen Akkünücü
 */

#ifndef INC_MODBUS_FUNC_H_
#define INC_MODBUS_FUNC_H_


#define COILS_ADDR_MAX 2
#define REGS_ADDR_MAX 128



/*
 * RMS value of the measured voltage,
 * gain corrected
 */
#define VOLTAGE_RMS_H 1
#define VOLTAGE_RMS_L 0

/*
 * RMS value of the measured current,
 * gain corrected
 */
#define CURRENT_RMS_H 3
#define CURRENT RMS_L 2

/*
 * Frequency of the voltage,
 * measured by comparators
 */
#define VOLTAGE_FREQ_H 5
#define VOLTAGE_FREQ_L 4

/*
 * Frequency of the current,
 * measured by comparators
 */
#define CURRENT_FREQ_H 7
#define CURRENT_FREQ_L 6

/*
 * Phase measurement,
 * measured by the comparators,
 * phase correction applied
 */
#define PHASE_H 9
#define PHASE_L 8

/*
 * Phase measurement,
 * measured by the DFT algorithm,
 * phase correction applied
 */
#define DFT_PHASE_MAIN_TONE_H 11
#define DFT_PHASE_MAIN_TONE_L 10

/*
 * Maximum Voltage value in ADC samples
 * not used at the moment
 */
#define VOLTAGE_MAX_H 13
#define VOLTAGE_MIN_L 12

/*
 * Maximum current value in ADC samples
 * Not used at the moment
 */
#define CURRENT_MAX_H 15
#define CURRENT_MIN_L 14


/*
 * Contactor register
 * only CONTACTOR_ADDR_L is used
 * CONTACTOR_ADDR_H reserved for future use
 */
#define CONTACTOR_ADDR_H 17
#define CONTACTOR_ADDR_L 16

#define MAIN_CONTACTOR_POS 0
/*
 * Register for power stats 18-24
 */

/*
 * Apparent power,
 * calculated by multiplying VOLTAGE_RMS and CURRENT_RMS
 */
#define APPARENT_PWR_ADDR_H 19
#define APPARENT_PWR_ADDR_L 18

/*
 * Real power,
 * calculated by multiplying VOLTAGE_RMS, CURRENT_RMS and cos(PHASE)
 */
#define REAL_PWR_ADDR_H 21
#define REAL_PWR_ADDR_L 20

/*
 * Reactive power,
 * calculated by multiplying VOLTAGE_RMS, CURRENT_RMS and sin(PHASE)
 */
#define REACTIVE_PWR_ADDR_H 23
#define REACTIVE_PWR_ADDR_L 22

/*
 * Power factor,
 * calculated by dividing REAL_PWR by REACTIVE_PWR
 */
#define PWR_FACTOR_ADDR_H 25
#define PWR_FACTOR_PWR_ADDR_L 24

/*
 * Status Registers
 * Status registers hold the information on the fault states
 */
#define STATUS_REGISTER_H 27
#define STATUS_REGISTER_L 26

#define OV_REGISTER_NUM 26
#define OV_BIT_POS 0
#define OC_REGISTER_NUM 26
#define OC_BIT_POS 1
#define NO_COMM_REGISTER_NUM 26
#define NO_COMM_BIT_POS 2


/*
 * Ambient temperature results from TMP102
 */
#define AMBIENT_TEMP_H 29
#define AMBIENT_TEMP_L 28

/*
 * Vrg1 and Vrg2 RMS values
 */
#define VRG1_RMS_H 31
#define VRG1_RMS_L 30
#define VRG2_RMS_H 33
#define VRG2_RMS_L 32

/*
 * Vrg1 and Vrg2 Absolute Phase values
 */
#define VRG1_PHASE_H 35
#define VRG1_PHASE_L 34
#define VRG2_PHASE_H 37
#define VRG2_PHASE_L 36

/*
 * Calibration and over/under voltage/current limits
 */


/*
 * offset values for channels
 */
#define VOLTAGE_OFFSET_H 39
#define VOLTAGE_OFFSET_L 38
#define CURRENT_OFFSET_H 41
#define CURRENT_OFFSET_L 40
#define VRG1_OFFSET_H 43
#define VRG1_OFFSET_L 42
#define VRG2_OFFSET_H 45
#define VRG2_OFFSET_L 44

/*
 * Voltage gain,
 * stored in EEPROM,
 * applied to VOLTAGE_RMS
 */
#define VOLTAGE_GAIN_ADDR_H 47
#define VOLTAGE_GAIN_ADDR_L 46

/*
 * Current gain,
 * stored in EEPROM,
 * applied to CURRENT_RMS
 */
#define CURRENT_GAIN_ADDR_H 49
#define CURRENT_GAIN_ADDR_L 48

/*
 * Phase correction factor for the comparator phase measurement
 * stored in EEPROM
 * applied to PHASE
 */
#define PHASE_CORR_ADDR_H 51
#define PHASE_CORR_ADDR_L 50

/*
 * Phase correction factor for the comparator phase measurement
 * stored in EEPROM
 * applied to PHASE
 */
#define PHASE_CORR_DFT_ADDR_H 53
#define PHASE_CORR_DFT_ADDR_L 52

/*
 * Over voltage limit,
 * stored in EEPROM,
 * Applied to VOLTAGE_RMS
 */
#define OV_LIMIT_H 55
#define OV_LIMIT_L 54

/*
 * Under voltage limit,
 * stored in EEPROM,
 * Applied to VOLTAGE_RMS
 */
#define UV_LIMIT_H 57
#define UV_LIMIT_L 56

/*
 * Over current limit,
 * stored in EEPROM,
 * Applied to CURRENT_RMS
 */
#define OC_LIMIT_H 59
#define OC_LIMIT_L 58

/*
 * Under current limit
 * stored in EEPROM,
 * Applied to CURRENT_RMS
 */
#define UC_LIMIT_H 61
#define UC_LIMIT_L 60

/*
 * Phase limit
 * stored in EEPROM,
 * Applied to PHASE AND DFT_PHASE_MAIN_TONE
 */
#define PHASE_LIMIT_H 63
#define PHASE_LIMIT_L 62

/*
 * Vrg gains
 * stored in EEPROM,
 * Applied to Vrg1_RMS and Vrg2_RNS
 */
#define VRG1_GAIN_H 65
#define VRG1_GAIN_L 64
#define VRG2_GAIN_H 67
#define VRG2_GAIN_L 66

/*
 * RTU Server ID
 * stored in EEPROM as a float,
 * Used at boot to set Modbus slave address
 */
#define RTU_ID_ADDR_H 69
#define RTU_ID_ADDR_L 68

// --- Harmonic Registers ---
#define HARMONICS_START_ADDR  70
#define HARMONICS_END_ADDR    87  // Reduced to 3rd, 5th, and 7th only

// --- Emulation Mode Registers ---
#define SIM_MODE_ADDR         90
#define SIM_VOLTAGE_L         92
#define SIM_VOLTAGE_H         93
#define SIM_CURRENT_L         94
#define SIM_CURRENT_H         95

/*
 * Thermocouple Inputs 1 to 3
 * Read from MCP9600 via DG4052E Multiplexer
 */
#define THERMOCOUPLE_1_H 97
#define THERMOCOUPLE_1_L 96

#define THERMOCOUPLE_2_H 99
#define THERMOCOUPLE_2_L 98

#define THERMOCOUPLE_3_H 101
#define THERMOCOUPLE_3_L 100

/*
 * MODBUS function codes
 */
#define READ_COIL_FUNC_CODE 					0x01 	//Not implemented
#define READ_DISCRETE_INPUT_FUNC_CODE 			0x02 	//Not implemented
#define READ_HOLDING_REGISTERS_FUNC_CODE 		0x03	//Implemented
#define READ_INPUT_REGISTERS_FUNC_CODE 			0x04	//Not implemented
#define WRITE_SINGLE_COIL_FUNC_CODE 			0x05	//Not implemented
#define WRITE_SINGLE_REGISTER_FUNC_CODE 		0x06	//Implemented
#define DIAGNOSTICS_FUNC_CODE 					0x08	//Not implemented
#define GET_COMM_EVENT_COUNTER_FUNC_CODE 		0x0B	//Not implemented
#define WRITE_MULTIPLE_COILS_FUNC_CODE 			0x0F	//Not implemented
#define WRITE_MULTIPLE_REGISTERS_FUNC_CODE 		0x10	//Implemented
#define REPORT_SERVER_ID_FUNC_CODE 				0x11	//Not implemented
#define MASK_WRITE_REGISTER_FUNC_CODE 			0x16	//Not implemented
#define READ_WRITE_MULTIPLE_REGISTERS_FUNC_CODE 0x17	//Not implemented
#define READ_IDENTIFICATION_FUNC_CODE 			0x2B	//Not implemented
/*
 * MODBUS function code lengths
 */
#define READ_HOLDING_REGISTERS_FUNC_LEN 	8
#define	WRITE_SINGLE_REGISTER_FUNC_LEN		8
#define WRITE_MULTIPLE_REGISTERS_FUNC_LEN 	13


// Our RTU address
#define DEFAULT_RTU_ADDRESS 1

void MODBUS_init(void);
void MODBUS_poll(void);
void MODBUS_set_server_register(uint16_t address, uint16_t data);
void MODBUS_timeout_check(void);
uint32_t MODBUS_convert_EEPROM_ADDR(uint32_t EEPROM_address);
float MODBUS_convert_16_bit_to_float(uint16_t *value);
bool MODBUS_get_bit(uint16_t reg_num, uint8_t bit_pos);
void MODBUS_set_bit(uint16_t reg_num, uint8_t bit_pos);
void MODBUS_clear_bit(uint16_t reg_num, uint8_t bit_pos);
void MODBUS_reset_no_comm_timeout_timer(void);
void MODBUS_start_no_comm_timeout_timer(void);
uint16_t MODBUS_get_server_register(uint16_t address);

#endif /* INC_MODBUS_FUNC_H_ */
