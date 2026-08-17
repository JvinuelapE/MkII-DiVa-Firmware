/*
 * I2C_checks.c
 */

#include "stm32g4xx.h"
#include "main.h"        // Needed for Multiplex_SW_Pin and MUX pin definitions
#include "I2C_checks.h"
#include <stdbool.h>     // Fixes the 'bool' compiler error
#include "modbus_func.h" // Needed to push data to Modbus
#include <string.h>      // Needed for memcpy

extern I2C_HandleTypeDef hi2c3;

#define TEMPERATURE_LSB 0.0625f

const uint8_t THERMOCOUPLE_ADDR = 0x60;	 // MCP9600-E/MX chip
const uint8_t AMBIENT_TEMP_ADDR = 0x48;	 // TMP102 chip

const uint8_t THERMOCOUPLE_HOT_JUNCTION = 0x00;
const uint8_t DEVICE_ID = 0x20;
const uint8_t TEMP = 0x00;

static uint8_t device_ID[2] = {0xFF, 0xFF};

// --- Multiplexer Pin Definitions (Adjust if necessary) ---
#define MUX_PORT GPIOB
#define MUX_A0_PIN GPIO_PIN_6
#define MUX_A1_PIN GPIO_PIN_7

void I2C_checks_start(void){
	HAL_I2C_Master_Transmit_IT(&hi2c3, THERMOCOUPLE_ADDR<<1, (uint8_t *)&DEVICE_ID, 1);
}

void I2C_checks_get_ID(void){
	HAL_I2C_Master_Receive_IT(&hi2c3, THERMOCOUPLE_ADDR<<1, device_ID, 2);
}

// ---------------------------------------------------------
// 1. AMBIENT SENSOR (TMP102)
// ---------------------------------------------------------
HAL_StatusTypeDef I2C_checks_read_ambient_sensor(void){
	HAL_StatusTypeDef ret;
	uint8_t raw_data[2] = {0, 0};
	int16_t temp_code = 0;
	float temperature = 0.0f;
    uint16_t modbus_regs[2];

    // Point the sensor to the Temperature Register
	ret = HAL_I2C_Master_Transmit(&hi2c3, AMBIENT_TEMP_ADDR<<1, (uint8_t*)&TEMP, 1, 100);
	if(ret != HAL_OK) return ret;

    // Read the 2 bytes of data back
    ret = HAL_I2C_Master_Receive(&hi2c3, AMBIENT_TEMP_ADDR<<1, raw_data, 2, 100);
    if(ret != HAL_OK) return ret;

    // Combine bytes (12-bit resolution, left-justified)
    temp_code = ((int16_t)raw_data[0] << 4) | (raw_data[1] >> 4);

    // Handle negative temperatures (12-bit two's complement)
    if (temp_code > 0x7FF) {
        temp_code |= 0xF000;
    }

    temperature = temp_code * TEMPERATURE_LSB;

    // Push to Modbus registers 28 and 29
    memcpy(modbus_regs, &temperature, sizeof(float));
    MODBUS_set_server_register(AMBIENT_TEMP_L, modbus_regs[0]);
    MODBUS_set_server_register(AMBIENT_TEMP_H, modbus_regs[1]);

    return HAL_OK;
}

// ---------------------------------------------------------
// 2. THERMOCOUPLES (MCP9600 + DG4052E Mux)
// ---------------------------------------------------------
static void set_thermocouple_channel(uint8_t channel) {
    switch(channel) {
        case 0: // A0=0, A1=0
            HAL_GPIO_WritePin(MUX_PORT, MUX_A0_PIN, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(MUX_PORT, MUX_A1_PIN, GPIO_PIN_RESET);
            break;
        case 1: // A0=1, A1=0
            HAL_GPIO_WritePin(MUX_PORT, MUX_A0_PIN, GPIO_PIN_SET);
            HAL_GPIO_WritePin(MUX_PORT, MUX_A1_PIN, GPIO_PIN_RESET);
            break;
        case 2: // A0=0, A1=1
            HAL_GPIO_WritePin(MUX_PORT, MUX_A0_PIN, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(MUX_PORT, MUX_A1_PIN, GPIO_PIN_SET);
            break;
        case 3: // A0=1, A1=1
            HAL_GPIO_WritePin(MUX_PORT, MUX_A0_PIN, GPIO_PIN_SET);
            HAL_GPIO_WritePin(MUX_PORT, MUX_A1_PIN, GPIO_PIN_SET);
            break;
    }
}

void I2C_checks_config_thermocouple(void) {
    uint8_t config_data;
    
    // 1. Turn the Multiplexer ON (Active-LOW Enable)
    HAL_GPIO_WritePin(GPIOB, Multiplex_SW_Pin, GPIO_PIN_RESET);
    
    // 2. Set Thermocouple Type J (Register 0x05)
    config_data = 0x10; 
    HAL_I2C_Mem_Write(&hi2c3, THERMOCOUPLE_ADDR << 1, 0x05, I2C_MEMADD_SIZE_8BIT, &config_data, 1, 100);

    // 3. Set Device Config to Native 18-bit Mode (Takes ~320ms to calculate)
    config_data = 0x00; 
    HAL_I2C_Mem_Write(&hi2c3, THERMOCOUPLE_ADDR << 1, 0x06, I2C_MEMADD_SIZE_8BIT, &config_data, 1, 100);
}

void I2C_checks_read_thermocouples(void) {
    uint8_t raw_data[2];
    int16_t temp_code;
    float temperature;
    uint16_t modbus_regs[2];

    // --- ISOLATION TEST: Change this number (0, 1, 2, or 3) to find your physical channel! ---
    set_thermocouple_channel(0);
    
    // HUGE DELAY: Guarantee the 320ms 18-bit conversion is completely finished!
    HAL_Delay(400); 

    if (HAL_I2C_Mem_Read(&hi2c3, THERMOCOUPLE_ADDR << 1, THERMOCOUPLE_HOT_JUNCTION, I2C_MEMADD_SIZE_8BIT, raw_data, 2, 100) == HAL_OK) {
        
        // Calculate temperature from raw bytes
        temp_code = (int16_t)((raw_data[0] << 8) | raw_data[1]);
        temperature = temp_code * TEMPERATURE_LSB;
        
    } else {
        temperature = -99.0f; 
    }

    // Push value to TC1 Modbus registers
    memcpy(modbus_regs, &temperature, sizeof(float));
    MODBUS_set_server_register(THERMOCOUPLE_1_L, modbus_regs[0]);
    MODBUS_set_server_register(THERMOCOUPLE_1_H, modbus_regs[1]);
}