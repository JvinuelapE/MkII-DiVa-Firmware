/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define Contactor_9_Pin GPIO_PIN_13
#define Contactor_9_GPIO_Port GPIOC
#define Contactor_0_Pin GPIO_PIN_0
#define Contactor_0_GPIO_Port GPIOC
#define Contactor_1_Pin GPIO_PIN_1
#define Contactor_1_GPIO_Port GPIOC
#define Contactor_2_Pin GPIO_PIN_2
#define Contactor_2_GPIO_Port GPIOC
#define Contactor_3_Pin GPIO_PIN_3
#define Contactor_3_GPIO_Port GPIOC
#define Multiplex_SW_Pin GPIO_PIN_2
#define Multiplex_SW_GPIO_Port GPIOB
#define Contactor_4_Pin GPIO_PIN_6
#define Contactor_4_GPIO_Port GPIOC
#define Contactor_5_Pin GPIO_PIN_7
#define Contactor_5_GPIO_Port GPIOC
#define Contactor_6_Pin GPIO_PIN_10
#define Contactor_6_GPIO_Port GPIOC
#define Contactor_7_Pin GPIO_PIN_11
#define Contactor_7_GPIO_Port GPIOC
#define Contactor_8_Pin GPIO_PIN_12
#define Contactor_8_GPIO_Port GPIOC

/* --- ADDED: Thermocouple Multiplexer Pin Defines --- */
#define TC_SW_A0_Pin GPIO_PIN_6
#define TC_SW_A0_GPIO_Port GPIOB
#define TC_SW_A1_Pin GPIO_PIN_7
#define TC_SW_A1_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */