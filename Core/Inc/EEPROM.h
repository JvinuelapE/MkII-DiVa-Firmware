/*
 * EEPROM.h
 *
 *  Created on: Jun 18, 2024
 *      Author: Alperen Akkünücü
 */

#ifndef INC_EEPROM_H_
#define INC_EEPROM_H_

#define EEPROM_ADDR 0xA0

#define VOLTAGE_OFFSET_ADDR 0
#define CURRENT_OFFSET_ADDR 4
#define VRG1_OFFSET_ADDR 8
#define VRG2_OFFSET_ADDR 12
#define VOLTAGE_GAIN_ADDR 16
#define CURRENT_GAIN_ADDR 20
#define PHASE_CORR_ADDR 24
#define PHASE_CORR_DFT_ADDR 28
#define OV_LIMIT_ADDR 32
#define UV_LIMIT_ADDR 36
#define OC_LIMIT_ADDR 40
#define UC_LIMIT_ADDR 44
#define PHASE_LIMIT_ADDR 48
#define VRG1_GAIN_ADDR 52
#define VRG2_GAIN_ADDR 56
#define RTU_ID_EEPROM_ADDR 60

/*
 * This is the address offset between MODBUS server register and EEPROM address field.
 * Example:
 * VOLTAGE_OFFSET is located at the address 0 in EEPROM
 * VOLTAGE_OFFSET_L is located at address 38 in MODBUS server registers
 * The address offset is 38
 *
 * The order of the values should be kept the same between EEPROM and MODBUS registers
 *
 * The formula to convert MOSBUS server address to EEPROM address is the following
 * (MODBUS_ADDR-38)*2
 *
 * The formula to convert EEPROM address to MODBUS server address it the following
 * (EEPROM_ADDR/2)+38
 */
#define ADDR_OFFSET 38


HAL_StatusTypeDef EEPROM_store_float(float value, uint32_t address);
HAL_StatusTypeDef EEPROM_read_float(float *value, uint32_t address);
HAL_StatusTypeDef EEPROM_read_int(int32_t *value, uint32_t address);
uint32_t EEPROM_convert_MODBUS_ADDR(uint32_t MODBUS_address);


#endif /* INC_EEPROM_H_ */
