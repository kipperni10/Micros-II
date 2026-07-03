/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "stm32f4xx_hal.h"

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
#define BTN_SEL_Pin GPIO_PIN_0
#define BTN_SEL_GPIO_Port GPIOC
#define BTN_FUNC_Pin GPIO_PIN_1
#define BTN_FUNC_GPIO_Port GPIOC
#define POT_ADC_Pin GPIO_PIN_0
#define POT_ADC_GPIO_Port GPIOA
#define IR_OBS_Pin GPIO_PIN_1
#define IR_OBS_GPIO_Port GPIOA
#define PEDAL_ACC_Pin GPIO_PIN_4
#define PEDAL_ACC_GPIO_Port GPIOA
#define DIG_1_Pin GPIO_PIN_6
#define DIG_1_GPIO_Port GPIOA
#define DIG_0_Pin GPIO_PIN_7
#define DIG_0_GPIO_Port GPIOA
#define PEDAL_BRAKE_Pin GPIO_PIN_0
#define PEDAL_BRAKE_GPIO_Port GPIOB
#define SEG_C_Pin GPIO_PIN_10
#define SEG_C_GPIO_Port GPIOB
#define SEG_F_Pin GPIO_PIN_7
#define SEG_F_GPIO_Port GPIOC
#define SEG_D_Pin GPIO_PIN_8
#define SEG_D_GPIO_Port GPIOA
#define SEG_E_Pin GPIO_PIN_9
#define SEG_E_GPIO_Port GPIOA
#define FAN_PWM_Pin GPIO_PIN_10
#define FAN_PWM_GPIO_Port GPIOA
#define TMS_Pin GPIO_PIN_13
#define TMS_GPIO_Port GPIOA
#define TCK_Pin GPIO_PIN_14
#define TCK_GPIO_Port GPIOA
#define FAN_TACH_Pin GPIO_PIN_3
#define FAN_TACH_GPIO_Port GPIOB
#define SEG_B_Pin GPIO_PIN_4
#define SEG_B_GPIO_Port GPIOB
#define SEG_A_Pin GPIO_PIN_5
#define SEG_A_GPIO_Port GPIOB
#define SEG_G_Pin GPIO_PIN_6
#define SEG_G_GPIO_Port GPIOB
#define RELAY_Pin GPIO_PIN_8
#define RELAY_GPIO_Port GPIOB
#define DIG_2_Pin GPIO_PIN_9
#define DIG_2_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
