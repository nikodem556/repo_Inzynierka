/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "stm32l4xx_hal.h"

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
#define TMS_Pin GPIO_PIN_13
#define TMS_GPIO_Port GPIOA
#define TCK_Pin GPIO_PIN_14
#define TCK_GPIO_Port GPIOA
#define SWO_Pin GPIO_PIN_3
#define SWO_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* LED pins (used for visual feedback in the application). */
#define GREEN_LED_GPIO_Port     GPIOC
#define GREEN_LED_Pin           GPIO_PIN_0   /* e.g. PC0 */
#define RED_LED_GPIO_Port       GPIOC
#define RED_LED_Pin             GPIO_PIN_1   /* e.g. PC1 */

/* Reset button pin (default active-low with pull-up). */
#define RESET_BTN_GPIO_Port     GPIOB
#define RESET_BTN_Pin           GPIO_PIN_0   /* e.g. PB0 */
#define RESET_BTN_ACTIVE_LEVEL  0            /* 0 = active-low, 1 = active-high */

/* Compile-time guards: fail early if any required mapping is missing. */
#ifndef GREEN_LED_GPIO_Port
#error "LED, Button, or LCD pin macros not defined in main.h (check GREEN_LED/RED_LED, RESET_BTN, LCD pins)."
#endif
#ifndef GREEN_LED_Pin
#error "GREEN_LED_Pin not defined"
#endif
#ifndef RED_LED_GPIO_Port
#error "RED_LED_GPIO_Port not defined"
#endif
#ifndef RED_LED_Pin
#error "RED_LED_Pin not defined"
#endif
#ifndef RESET_BTN_GPIO_Port
#error "RESET_BTN_GPIO_Port not defined"
#endif
#ifndef RESET_BTN_Pin
#error "RESET_BTN_Pin not defined"
#endif
#ifndef RESET_BTN_ACTIVE_LEVEL
#error "RESET_BTN_ACTIVE_LEVEL not defined"
#endif


/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
