/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"
#include "usb_host.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "usb_host.h"
#include "usbh_midi.h"
#include "notes.h"
#include "lesson.h"
#include "grove_lcd16x2_i2c.h"
#include "button.h"
#include "app.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

/* USER CODE BEGIN PV */
extern USBH_HandleTypeDef hUsbHostFS;   // declared in usb_host.c
extern ApplicationTypeDef Appli_state;  // declared in usb_host.c
static ApplicationTypeDef prevState = APPLICATION_IDLE;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
void MX_USB_HOST_Process(void);

/* USER CODE BEGIN PFP */
int _write(int file, char *ptr, int len);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
GroveLCD_t lcd;

/* Slot 0: Whole note */
static const uint8_t CH_WHOLE[8] = {
    0b00000, 0b00110, 0b01001, 0b01001, 0b01001, 0b00110, 0b00000, 0b00000
};

/* Slot 1: Half note */
static const uint8_t CH_HALF[8] = {
    0b00001, 0b00001, 0b00111, 0b01001, 0b01001, 0b00111, 0b00000, 0b00000
};

/* Slot 2: Quarter note */
static const uint8_t CH_QUARTER[8] = {
    0b00001, 0b00001, 0b00111, 0b01111, 0b01111, 0b00111, 0b00000, 0b00000
};

/* Slot 3: Eighth note */
static const uint8_t CH_EIGHTH[8] = {
    0b00011,
    0b00101,
    0b00011,
    0b00001,
    0b01111,
    0b01111,
    0b00110,
    0b00000
};

/* Slot 4: Sixteenth note */
static const uint8_t CH_SIXTEENTH[8] = {
    0b00011,
    0b00101,
    0b00011,
    0b00101,
    0b01111,
    0b01111,
    0b00110,
    0b00000
};

/* Slot 5: Sharp (#) */
static const uint8_t CH_SHARP[8] = {
    0b00100,
    0b01110,
    0b00100,
    0b01110,
    0b00100,
    0b00000,
    0b00000,
    0b00000
};

/* Slot 6: Flat (b) */
static const uint8_t CH_FLAT[8] = {
    0b00100,
    0b00100,
    0b00110,
    0b00101,
    0b00110,
    0b00000,
    0b00000,
    0b00000
};
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USB_HOST_Init();
  MX_I2C1_Init();

  /* USER CODE BEGIN 2 */
  printf("\r\n==== SN_Keyboard_assistant started (SWV printf active) ====\r\n");

  /* LCD init */
  GroveLCD_Init(&lcd, &hi2c1, GROVE_LCD_I2C_ADDR_7BIT_DEFAULT);

  /* Upload custom chars into CGRAM slots 0..6 */
  GroveLCD_CreateChar(&lcd, 0, CH_WHOLE);
  GroveLCD_CreateChar(&lcd, 1, CH_HALF);
  GroveLCD_CreateChar(&lcd, 2, CH_QUARTER);
  GroveLCD_CreateChar(&lcd, 3, CH_EIGHTH);
  GroveLCD_CreateChar(&lcd, 4, CH_SIXTEENTH);
  GroveLCD_CreateChar(&lcd, 5, CH_SHARP);
  GroveLCD_CreateChar(&lcd, 6, CH_FLAT);

  /* Start UI state machine (shows Welcome screen) */
  App_Init();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USB Host background processing */
    MX_USB_HOST_Process();

    /* Log application state changes */
    if (Appli_state != prevState)
    {
      switch (Appli_state)
      {
        case APPLICATION_START:
          printf("State: APPLICATION_START (device connected)\r\n");
          break;
        case APPLICATION_READY:
          printf("State: APPLICATION_READY (MIDI class active)\r\n");
          break;
        case APPLICATION_DISCONNECT:
          printf("State: APPLICATION_DISCONNECT (device disconnected)\r\n");
          break;
        default:
          printf("State: %d\r\n", Appli_state);
          break;
      }
      prevState = Appli_state;
    }

    /* Read MIDI events and forward NOTE ON to Lesson only when lesson is active */
    if (Appli_state == APPLICATION_READY || Appli_state == APPLICATION_START)
    {
      uint8_t midi_event[4];
      if (USBH_MIDI_GetEvent(&hUsbHostFS, midi_event) == USBH_OK)
      {
        uint8_t status  = midi_event[1] & 0xF0;
        uint8_t note    = midi_event[2];
        uint8_t vel     = midi_event[3];

        if (status == 0x90 && vel != 0)  /* NOTE ON */
        {
          if (Lesson_IsActive())
          {
            /* Lesson_HandleInput treats 0..127 as MIDI notes */
            Lesson_HandleInput(note);
          }
        }
        else
        {
          /* NOTE OFF ignored */
        }
      }
    }

    /* Update debouncing and run UI/menu logic */
    Button_Update();
    App_Update();


    /* USER CODE END WHILE */
    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure LSE Drive Capability */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

  /** Initializes the RCC Oscillators according to the specified parameters */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSE|RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 16;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Enable MSI Auto calibration */
  HAL_RCCEx_EnableMSIPLLMode();
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x00B07CB4;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0|GPIO_PIN_1, GPIO_PIN_RESET);

  /*Configure GPIO pins : PC0 PC1 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PA1 PA4 (buttons) */
  GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PB0 (button) */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/* USER CODE BEGIN 4 */
int __io_putchar(int ch)
{
    return ITM_SendChar(ch);
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
