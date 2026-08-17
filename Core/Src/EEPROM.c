/*
 * EEPROM.c
 *
 *  Created on: Jun 18, 2024
 *      Author: Alperen Akkünücü
 */
#include "stm32g4xx.h"
#include "EEPROM.h"

extern I2C_HandleTypeDef hi2c3;



HAL_StatusTypeDef EEPROM_store_float(float value, uint32_t address){
	HAL_StatusTypeDef ret;
	ret = HAL_I2C_Mem_Write(&hi2c3, EEPROM_ADDR, address, I2C_MEMADD_SIZE_8BIT, (uint8_t*)&value, sizeof(float), 1000);
	return ret;
}


HAL_StatusTypeDef EEPROM_read_float(float *value, uint32_t address){
	uint8_t temp_data[4];
	uint32_t data_unsigned;
	float data_temp;
	HAL_StatusTypeDef ret;

	ret = HAL_I2C_Mem_Read(&hi2c3, EEPROM_ADDR, address, I2C_MEMADD_SIZE_8BIT, temp_data, sizeof(float), 1000);
	data_unsigned = (temp_data[3]<<24) | (temp_data[2]<<16) | (temp_data[1]<<8) | temp_data[0];
	data_temp =  *((float *)&data_unsigned);
	*value = data_temp;

	return ret;
}

HAL_StatusTypeDef EEPROM_read_int(int32_t *value, uint32_t address){
	uint8_t temp_data[4];
	uint32_t data_unsigned;
	int32_t data_temp;
	HAL_StatusTypeDef ret;

	ret = HAL_I2C_Mem_Read(&hi2c3, EEPROM_ADDR, address, I2C_MEMADD_SIZE_8BIT, temp_data, sizeof(float), 1000);
	data_unsigned = (temp_data[3]<<24) | (temp_data[2]<<16) | (temp_data[1]<<8) | temp_data[0];
	data_temp =  *((int32_t *)&data_unsigned);
	*value = data_temp;

	return ret;
}


/*
 * Converts modbus server register address to EEPROM address
 */
uint32_t EEPROM_convert_MODBUS_ADDR(uint32_t MODBUS_address){
	uint32_t address;

	address = (MODBUS_address - ADDR_OFFSET)*2;

	return address;
}
