/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : usb_host.h
  * @version        : v2.0_Cube
  * @brief          : Header for usb_host.c file.
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
#ifndef __USB_HOST__H__
#define __USB_HOST__H__

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx.h"
#include "stm32l4xx_hal.h"

/* USER CODE BEGIN INCLUDE */
/*
 * usb_host.h / usb_host.c
 *
 * CubeMX-generated USB Host glue code.
 * - Exposes init and background-process functions
 * - Defines a simple application state (Appli_state) used by main.c
 */
/* USER CODE END INCLUDE */

/** @addtogroup USBH_OTG_DRIVER
  * @{
  */

/** @defgroup USBH_HOST USBH_HOST
  * @brief Host file for Usb otg low level driver.
  * @{
  */

/** @defgroup USBH_HOST_Exported_Variables USBH_HOST_Exported_Variables
  * @brief Public variables.
  * @{
  */

/**
  * @}
  */

/**
 * @brief Application-level USB state (updated by USBH_UserProcess callback).
 *
 * main.c uses this state to know whether:
 * - a device is connected (START),
 * - the class is active (READY),
 * - the device is disconnected (DISCONNECT).
 */
typedef enum {
  APPLICATION_IDLE = 0,
  APPLICATION_START,
  APPLICATION_READY,
  APPLICATION_DISCONNECT
}ApplicationTypeDef;

/** @defgroup USBH_HOST_Exported_FunctionsPrototype USBH_HOST_Exported_FunctionsPrototype
  * @brief Declaration of public functions for Usb host.
  * @{
  */

/* Exported functions -------------------------------------------------------*/

/** @brief USB Host initialization function (init + class register + start). */
void MX_USB_HOST_Init(void);

/** @brief USB Host background process (must be called periodically). */
void MX_USB_HOST_Process(void);

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* __USB_HOST__H__ */
