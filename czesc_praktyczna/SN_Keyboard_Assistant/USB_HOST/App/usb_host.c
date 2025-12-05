/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file            : usb_host.c
  * @version         : v2.0_Cube
  * @brief           : This file implements the USB Host
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

/* Includes ------------------------------------------------------------------*/

#include "usb_host.h"
#include "usbh_core.h"
#include "usbh_hid.h"

/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "usbh_midi.h"
/* USER CODE END Includes */

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/

/* USER CODE END PV */

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

/* USER CODE END PFP */

/* USB Host core handle declaration */
USBH_HandleTypeDef hUsbHostFS;
ApplicationTypeDef Appli_state = APPLICATION_IDLE;

/*
 * -- Insert your variables declaration here --
 */
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/*
 * user callback declaration
 */
static void USBH_UserProcess(USBH_HandleTypeDef *phost, uint8_t id);

/*
 * -- Insert your external function declaration here --
 */
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/**
  * Init USB host library, add supported class and start the library
  * @retval None
  */
void MX_USB_HOST_Init(void)
{
  /* USER CODE BEGIN USB_HOST_Init_PreTreatment */

  /* USER CODE END USB_HOST_Init_PreTreatment */



	  USBH_StatusTypeDef usb_status;
	  /* Initialize Host Library */
	  usb_status = USBH_Init(&hUsbHostFS, USBH_UserProcess, HOST_FS);
	  if (usb_status != USBH_OK)
	  {
	    printf("USBH_Init failed, status %d\r\n", usb_status);
	    Error_Handler();
	  }
	  else
	  {
	    printf("USBH_Init OK\r\n");
	  }

	  /* Register MIDI class */
	  usb_status = USBH_RegisterClass(&hUsbHostFS, USBH_MIDI_CLASS);
	  if (usb_status != USBH_OK)
	  {
	    printf("USBH_RegisterClass failed, status %d\r\n", usb_status);
	    Error_Handler();
	  }
	  else
	  {
	    printf("USBH_RegisterClass (MIDI) OK\r\n");
	  }

	  /* Start Host Process */
	  usb_status = USBH_Start(&hUsbHostFS);
	  if (usb_status != USBH_OK)
	  {
	    printf("USBH_Start failed, status %d\r\n", usb_status);
	    Error_Handler();
	  }
	  else
	  {
	    printf("USBH_Start OK\r\n");
	  }

	  /* USER CODE BEGIN USB_HOST_Init_PostTreatment */
	  /* USER CODE END USB_HOST_Init_PostTreatment */
	}

/**
  * @brief  USB Host Background Task
  */
void MX_USB_HOST_Process(void)
{
  USBH_Process(&hUsbHostFS);
}

/*
 * user callback definition
 */
static void USBH_UserProcess(USBH_HandleTypeDef *phost, uint8_t id)
{
  switch (id)
  {
    case HOST_USER_SELECT_CONFIGURATION:
      printf("USBH_UserProcess: HOST_USER_SELECT_CONFIGURATION\r\n");
      break;

    case HOST_USER_CONNECTION:
      Appli_state = APPLICATION_START;
      printf("USBH_UserProcess: HOST_USER_CONNECTION -> APPLICATION_START\r\n");
      break;

    case HOST_USER_CLASS_ACTIVE:
      Appli_state = APPLICATION_READY;
      printf("USBH_UserProcess: HOST_USER_CLASS_ACTIVE -> APPLICATION_READY\r\n");
      break;

    case HOST_USER_DISCONNECTION:
      Appli_state = APPLICATION_DISCONNECT;
      printf("USBH_UserProcess: HOST_USER_DISCONNECTION -> APPLICATION_DISCONNECT\r\n");
      break;

    default:
      printf("USBH_UserProcess: Unknown id=%d\r\n", id);
      break;
  }
  /* USER CODE END CALL_BACK_1 */
}

/**
  * @}
  */

/**
  * @}
  */
