/*
 * GPIO_func.c
 *
 *  Created on: May 25, 2024
 *      Author: Alperen Akkünücü
 */
#include <stdint.h>
#include <stdbool.h>

#include "stm32g4xx.h"
#include "main.h"

#include "modbus_func.h"
#include "GPIO_func.h"


enum contactors {contactor_0, contactor_1, contactor_2, contactor_3, contactor_4,
				 contactor_5, contactor_6, contactor_7, contactor_8,
				 contactor_9};

//Select signal path for the ADC
//When LPF path is selected the harmonics in the signal are not filtered
//When Notch path is selected the harmonics in the signal are filtered
void GPIO_func_SEL_Signal_Path(enum signal_path path){
	if(path == Notch){
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9|GPIO_PIN_10, GPIO_PIN_RESET);
	}

	else if(path == LPF){
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_RESET);
	}
}

void GPIO_func_configure_contactors(uint16_t contactor_reg){

	if((contactor_reg & 0x0001) == 1)
		HAL_GPIO_WritePin(GPIOC, Contactor_0_Pin, GPIO_PIN_SET);
	else
		HAL_GPIO_WritePin(GPIOC, Contactor_0_Pin, GPIO_PIN_RESET);

	if((contactor_reg>>1 & 0x0001) == 1)
		HAL_GPIO_WritePin(GPIOC, Contactor_1_Pin, GPIO_PIN_SET);
	else
		HAL_GPIO_WritePin(GPIOC, Contactor_1_Pin, GPIO_PIN_RESET);

	if((contactor_reg>>2 & 0x0001) == 1)
		HAL_GPIO_WritePin(GPIOC, Contactor_2_Pin, GPIO_PIN_SET);
	else
		HAL_GPIO_WritePin(GPIOC, Contactor_2_Pin, GPIO_PIN_RESET);

	if((contactor_reg>>3 & 0x0001) == 1)
		HAL_GPIO_WritePin(GPIOC, Contactor_3_Pin, GPIO_PIN_SET);
	else
		HAL_GPIO_WritePin(GPIOC, Contactor_3_Pin, GPIO_PIN_RESET);

	if((contactor_reg>>4 & 0x0001) == 1)
		HAL_GPIO_WritePin(GPIOC, Contactor_4_Pin, GPIO_PIN_SET);
	else
		HAL_GPIO_WritePin(GPIOC, Contactor_4_Pin, GPIO_PIN_RESET);

	if((contactor_reg>>5 & 0x0001) == 1)
		HAL_GPIO_WritePin(GPIOC, Contactor_5_Pin, GPIO_PIN_SET);
	else
		HAL_GPIO_WritePin(GPIOC, Contactor_5_Pin, GPIO_PIN_RESET);

	if((contactor_reg>>6 & 0x0001) == 1)
		HAL_GPIO_WritePin(GPIOC, Contactor_6_Pin, GPIO_PIN_SET);
	else
		HAL_GPIO_WritePin(GPIOC, Contactor_6_Pin, GPIO_PIN_RESET);

	if((contactor_reg>>7 & 0x0001) == 1)
		HAL_GPIO_WritePin(GPIOC, Contactor_7_Pin, GPIO_PIN_SET);
	else
		HAL_GPIO_WritePin(GPIOC, Contactor_7_Pin, GPIO_PIN_RESET);

	if((contactor_reg>>8 & 0x0001) == 1)
		HAL_GPIO_WritePin(GPIOC, Contactor_8_Pin, GPIO_PIN_SET);
	else
		HAL_GPIO_WritePin(GPIOC, Contactor_8_Pin, GPIO_PIN_RESET);

	if((contactor_reg>>9 & 0x0001) == 1)
		HAL_GPIO_WritePin(GPIOC, Contactor_9_Pin, GPIO_PIN_SET);
	else
		HAL_GPIO_WritePin(GPIOC, Contactor_9_Pin, GPIO_PIN_RESET);


}

void GPIO_set_main_contactor(uint8_t val){

	if(val == 0){
		HAL_GPIO_WritePin(GPIOB, Contactor_0_Pin, GPIO_PIN_RESET);
		MODBUS_clear_bit(CONTACTOR_ADDR_L, MAIN_CONTACTOR_POS);
	}

	else{
		HAL_GPIO_WritePin(GPIOB, Contactor_0_Pin, GPIO_PIN_SET);
		MODBUS_set_bit(CONTACTOR_ADDR_L, MAIN_CONTACTOR_POS);
	}

}



