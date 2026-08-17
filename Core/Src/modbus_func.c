/*
 * modbus_func.c
 *
 *  Created on: May 12, 2024
 *      Author: Alperen Akkünücü
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "main.h"
#include "modbus_func.h"
#include "calculation.h"
#include "ADC_acquisition.h"
#include "comparator.h"
#include "GPIO_func.h"
#include "nanomodbus.h"
#include "stm32g4xx.h"
#include "EEPROM.h"

uint32_t MODBUS_read_receive_buf(uint8_t *buf, uint32_t count);

#define MODBUS_RECEIVE_BUF_LEN 128

extern TIM_HandleTypeDef htim15;
extern TIM_HandleTypeDef htim7;
extern UART_HandleTypeDef huart2;

static uint8_t UART_data;
static uint8_t MODBUS_receive_buf[MODBUS_RECEIVE_BUF_LEN], *MODBUS_read_pointer, *MODBUS_write_pointer;
nmbs_bitfield server_coils = {0};
static uint16_t server_registers[REGS_ADDR_MAX] = {0};

static nmbs_t nmbs;
volatile static nmbs_error err;

//debugging
static uint16_t timeout_counter = 0, other_err_counter = 0, err_none;
volatile uint8_t flag_read_holding_regs = 0;

// --- Delayed Reboot Variables ---
static bool pending_reboot = false;
static uint16_t reboot_timer_ms = 0;
// --------------------------------

uint8_t MODBUS_buffer_peek(uint32_t pos);
bool MODBUS_get_bit(uint16_t reg, uint8_t bit_pos);

int32_t read_serial(uint8_t* buf, uint16_t count, int32_t byte_timeout_ms, void* arg) {
	uint32_t i = 0;

	i = MODBUS_read_receive_buf(buf, count);

	return i;

}


int32_t write_serial(const uint8_t* buf, uint16_t count, int32_t byte_timeout_ms, void* arg) {
	HAL_StatusTypeDef ret = HAL_OK;

	do{
		ret = HAL_UART_Transmit_DMA(&huart2, buf, count);
	}while(ret == HAL_BUSY);


	//HAL_UART_Transmit(&huart2, buf, count, byte_timeout_ms);
	return count;
}

void onError() {
  // Set the led ON on error
  while (true) {
    ;
  }
}


nmbs_error handle_read_coils(uint16_t address, uint16_t quantity, nmbs_bitfield coils_out, uint8_t unit_id, void *arg) {
  if (address + quantity  > COILS_ADDR_MAX + 1)
    return NMBS_EXCEPTION_ILLEGAL_DATA_ADDRESS;

  // Read our coils values into coils_out
  for (int i = 0; i < quantity; i++) {
    bool value = nmbs_bitfield_read(server_coils, address + i);
    nmbs_bitfield_write(coils_out, i, value);
  }

  return NMBS_ERROR_NONE;
}


nmbs_error handle_write_multiple_coils(uint16_t address, uint16_t quantity, const nmbs_bitfield coils, uint8_t unit_id, void *arg) {
  if (address + quantity > COILS_ADDR_MAX + 1)
    return NMBS_EXCEPTION_ILLEGAL_DATA_ADDRESS;

  // Write coils values to our server_coils
  for (int i = 0; i < quantity; i++) {
    nmbs_bitfield_write(server_coils, address + i, nmbs_bitfield_read(coils, i));
  }

  return NMBS_ERROR_NONE;
}


nmbs_error handler_read_holding_registers(uint16_t address, uint16_t quantity, uint16_t* registers_out, uint8_t unit_id, void *arg) {

flag_read_holding_regs = 1;
  if (address + quantity > REGS_ADDR_MAX + 1)
    return NMBS_EXCEPTION_ILLEGAL_DATA_ADDRESS;

  // Read our registers values into registers_out
  for (int i = 0; i < quantity; i++)
    registers_out[i] = server_registers[address + i];

  return NMBS_ERROR_NONE;
}


nmbs_error handle_write_multiple_registers(uint16_t address, uint16_t quantity, const uint16_t* registers, uint8_t unit_id, void *arg) {
  if (address + quantity > REGS_ADDR_MAX + 1)
    return NMBS_EXCEPTION_ILLEGAL_DATA_ADDRESS;

  // Write registers values to our server_registers
  for (int i = 0; i < quantity; i++)
    server_registers[address + i] = registers[i];

  // --- UPDATED: Expanded boundary to include RTU_ID_ADDR_H ---
  if(address>= VOLTAGE_OFFSET_L &&  !((address+quantity-1)>RTU_ID_ADDR_H) ){

	  /*
		Check if the data alignment and number of data is correct.
		If the start address is an odd number then the data misaligned
		If the number of data is not an even number, there is a missing data
		Disregard coming data in both cases.
	   */

	    if( (address%2 == 0) && (quantity%2 == 0) ){
		  uint32_t address_counter = address, quantity_counter = quantity;

		  while(quantity_counter>0){
			  uint32_t EEPROM_address;
			  float value;

			  EEPROM_address = EEPROM_convert_MODBUS_ADDR(address);
			  value = MODBUS_convert_16_bit_to_float(&server_registers[address]);
			  EEPROM_store_float(value, EEPROM_address);

			  quantity_counter = quantity_counter - 2;
			  address_counter = address_counter + 2;
		  }
		// Because RTU_ID_ADDR_L (68) > VOLTAGE_GAIN_ADDR_L (46), 
        // writing a new ID will automatically trigger this 500ms reboot sequence!
		if (address >= VOLTAGE_GAIN_ADDR_L) {
			pending_reboot = true;
			reboot_timer_ms = 0; 
		}
	  }
  }
  return NMBS_ERROR_NONE;
}

nmbs_error handle_write_single_register(uint16_t address, uint16_t value, uint8_t unit_id, void* arg){
  if (address> REGS_ADDR_MAX + 1)
	return NMBS_EXCEPTION_ILLEGAL_DATA_ADDRESS;

  //Write the value to the specified server register
  server_registers[address] = value;

  if(address == CONTACTOR_ADDR_L){
	  GPIO_func_configure_contactors(server_registers[CONTACTOR_ADDR_L]);
  }


  return NMBS_ERROR_NONE;

}

void MODBUS_init(void){

	nmbs_platform_conf platform_conf;
	platform_conf.transport = NMBS_TRANSPORT_RTU;
	platform_conf.read = read_serial;
	platform_conf.write = write_serial;
	platform_conf.arg = NULL;

	nmbs_callbacks callbacks = {0};
	callbacks.read_coils = handle_read_coils;
	callbacks.write_multiple_coils = handle_write_multiple_coils;
	callbacks.read_holding_registers = handler_read_holding_registers;
	callbacks.write_multiple_registers = handle_write_multiple_registers;
	callbacks.write_single_register = handle_write_single_register;

	// --- NEW: Load dynamic RTU ID from EEPROM ---
	float rtu_id_float = 0;
	uint8_t active_slave_id = DEFAULT_RTU_ADDRESS; 

	// Read the ID (Stored as a float to match your EEPROM architecture)
	EEPROM_read_float(&rtu_id_float, RTU_ID_EEPROM_ADDR);

	// Safely cast to an integer
	uint32_t id_check = (uint32_t)rtu_id_float;

	// Validate Modbus limits (1 to 247). If the EEPROM is blank/corrupt, 
	// it fails this check and safely falls back to DEFAULT_RTU_ADDRESS (1).
	if(id_check >= 1 && id_check <= 247){
		active_slave_id = (uint8_t)id_check;
	}

	// Load the float bits into the Modbus registers so Python can read them back
	uint32_t float_bits = *((uint32_t*)&rtu_id_float);
	server_registers[RTU_ID_ADDR_L] = (uint16_t)(float_bits & 0xFFFF);
	server_registers[RTU_ID_ADDR_H] = (uint16_t)((float_bits >> 16) & 0xFFFF);

	// Create the modbus server using the dynamic ID
	err = nmbs_server_create(&nmbs, active_slave_id, &platform_conf, &callbacks);
	// ---------------------------------------------

	if (err != NMBS_ERROR_NONE) {
	  onError();
	}

	nmbs_set_read_timeout(&nmbs, 5);
	nmbs_set_byte_timeout(&nmbs, 5);

	//initialize
	MODBUS_read_pointer = MODBUS_receive_buf;
	MODBUS_write_pointer = MODBUS_receive_buf;
	HAL_UART_Receive_IT(&huart2, &UART_data, 1);

	//dummy run
	MODBUS_poll();
	//reset counter value
	htim7.Instance->CNT = 0;
	//start timer
	HAL_TIM_Base_Start_IT(&htim7);
}

void MODBUS_poll(void){

	err = nmbs_server_poll(&nmbs);
//	// This will probably never happen, since we don't return < 0 in our platform funcs
//	if (err == NMBS_ERROR_TRANSPORT)
//	  break;
//
	//if there is an error flush the
	//received buffer
	if(err != NMBS_ERROR_NONE){
		MODBUS_read_pointer = MODBUS_receive_buf;
		MODBUS_write_pointer = MODBUS_receive_buf;
	}

	if(err == NMBS_ERROR_TIMEOUT){
		timeout_counter++;
	}
	else if(err == NMBS_ERROR_NONE){
		err_none++;
	}
	else{
		other_err_counter++;
	}

	MODBUS_reset_no_comm_timeout_timer();
	MODBUS_clear_bit(NO_COMM_REGISTER_NUM, NO_COMM_BIT_POS);
}


void MODBUS_set_server_register(uint16_t address, uint16_t data){

	server_registers[address] = data;

}

uint32_t MODBUS_read_receive_buf(uint8_t *buf, uint32_t count){

	uint8_t *data, i=0;

	data = buf;

	for(i=0;i<count;i++){
		if(MODBUS_read_pointer == MODBUS_write_pointer)//buffer is empty
			break;

		*data = *MODBUS_read_pointer;
		data++;
		MODBUS_read_pointer++;
		if(MODBUS_read_pointer >= MODBUS_receive_buf + MODBUS_RECEIVE_BUF_LEN )
			MODBUS_read_pointer = MODBUS_receive_buf;
	}

	return i;
}

uint8_t MODBUS_buffer_peek(uint32_t pos){
	return *(MODBUS_receive_buf + pos);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){

	*MODBUS_write_pointer = UART_data;
	MODBUS_write_pointer++;

	//check for wrap around
	if(MODBUS_write_pointer == MODBUS_receive_buf + MODBUS_RECEIVE_BUF_LEN)
		MODBUS_write_pointer = MODBUS_receive_buf;

    HAL_UART_Receive_IT(&huart2, &UART_data, 1);


    //Parse MODBUS data
    //check the function code and length
    if( ((MODBUS_write_pointer - MODBUS_read_pointer) == READ_HOLDING_REGISTERS_FUNC_LEN) &&
    	  *(MODBUS_read_pointer+1) == READ_HOLDING_REGISTERS_FUNC_CODE )
    	MODBUS_poll();

    else if( ((MODBUS_write_pointer - MODBUS_read_pointer) == WRITE_SINGLE_REGISTER_FUNC_LEN) &&
      	  *(MODBUS_read_pointer+1) == WRITE_SINGLE_REGISTER_FUNC_CODE )
    	MODBUS_poll();

    else if( ((MODBUS_write_pointer - MODBUS_read_pointer) == WRITE_MULTIPLE_REGISTERS_FUNC_LEN) &&
        	  *(MODBUS_read_pointer+1)  == WRITE_MULTIPLE_REGISTERS_FUNC_CODE )
    	MODBUS_poll();


}


/*
 * This function is run every 1ms with timer 7 interrupt.
 * This is a timer callback function to check if unprocessed and unparsed data remains in
 * MODBUS_receive_buf. If there is any unprocessed data remaining longer than 1 millisecond
 * the buffer is flushed.
 */
void MODBUS_timeout_check(void){
	static uint16_t num_data = 0, num_data_prev = 0;

	//calculate the number of data in MODBUS_receive_buf
	if(MODBUS_write_pointer > MODBUS_read_pointer)
		num_data = MODBUS_write_pointer - MODBUS_read_pointer;
	else if(MODBUS_write_pointer < MODBUS_read_pointer){
		num_data = (MODBUS_write_pointer - MODBUS_receive_buf) +
					((MODBUS_receive_buf + MODBUS_RECEIVE_BUF_LEN)- MODBUS_read_pointer);
	}
	else
		num_data = 0;

	//if the previous number of data equal to the num_data
	//there is a timeout error, flush the buffer.
	if(num_data != 0 && num_data_prev == num_data){
		MODBUS_read_pointer = MODBUS_receive_buf;
		MODBUS_write_pointer = MODBUS_receive_buf;
		num_data_prev = 0;
		num_data = 0;
	}
	else
		num_data_prev = num_data;

	if (pending_reboot) {
        reboot_timer_ms++;
        if (reboot_timer_ms >= 500) {      // 500ms guarantees GUI readback finishes
            NVIC_SystemReset();            // ⚡ Reboot the STM32
        }
    }
}

/*
 * Converts EEPROM address to MODBUS server register address
 */
uint32_t MODBUS_convert_EEPROM_ADDR(uint32_t EEPROM_address){
	uint32_t address;

	address = EEPROM_ADDR/2 + ADDR_OFFSET;

	return address;
}

/*
 * Takes to two uint16_t (server registers) and converts them to
 * a float
 */
float MODBUS_convert_16_bit_to_float(uint16_t *value){

	uint16_t value_H, value_L;
	float value_float;
	uint32_t value_unsinged = 0;

	value_H = *(value+1);
	value_L = *value;
	value_unsinged =  (( (uint32_t)value_H<<16 & 0xFFFF0000) | ( (uint32_t)value_L & 0x0000FFFF));
	value_float = *((float *)&value_unsinged);

	return value_float;

}

bool MODBUS_get_bit(uint16_t reg_num, uint8_t bit_pos){
	uint16_t reg;

	reg = 0x0001 & (server_registers[reg_num]>>bit_pos);

	return (bool)reg;
}

void MODBUS_set_bit(uint16_t reg_num, uint8_t bit_pos){
	uint16_t reg_val, temp_val;

	temp_val = (0x0001<<bit_pos);
	reg_val = server_registers[reg_num] | temp_val;
	server_registers[reg_num] = reg_val;

}

void MODBUS_clear_bit(uint16_t reg_num, uint8_t bit_pos){
	uint16_t reg_val, temp_val;

	temp_val = ~(0x0001<<bit_pos);
	reg_val = server_registers[reg_num] & temp_val;
	server_registers[reg_num] = reg_val;

}


void MODBUS_reset_no_comm_timeout_timer(void){
	htim15.Instance->CNT = 0;
}

void MODBUS_start_no_comm_timeout_timer(void){
	  HAL_TIM_Base_Start_IT(&htim15);
}

uint16_t MODBUS_get_server_register(uint16_t address) {
    return server_registers[address];
}
