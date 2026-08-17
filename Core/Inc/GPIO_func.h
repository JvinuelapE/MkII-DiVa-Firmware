/*
 * GPIO_func.h
 *
 *  Created on: May 25, 2024
 *      Author: Alperen Akkünücü
 */

#ifndef INC_GPIO_FUNC_H_
#define INC_GPIO_FUNC_H_

enum signal_path {LPF, Notch};

void GPIO_func_SEL_Signal_Path(enum signal_path path);
void GPIO_func_configure_contactors(uint16_t contactor_reg);
void GPIO_set_main_contactor(uint8_t val);

#endif /* INC_GPIO_FUNC_H_ */
