/*
 * I2C_checks.h
 *
 *  Created on: Feb 23, 2025
 *      Author: Alperen Akkünücü
 */

#ifndef INC_I2C_CHECKS_H_
#define INC_I2C_CHECKS_H_



HAL_StatusTypeDef I2C_checks_config_ambient_sensor(void);
HAL_StatusTypeDef I2C_checks_read_ambient_sensor(void);

void I2C_checks_start(void);
void I2C_checks_get_ID(void);
void I2C_checks_read_thermocouples(void);
void I2C_checks_config_thermocouple(void);

#endif /* INC_I2C_CHECKS_H_ */
