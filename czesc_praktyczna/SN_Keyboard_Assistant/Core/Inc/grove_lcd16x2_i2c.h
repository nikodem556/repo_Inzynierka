#ifndef GROVE_LCD16X2_I2C_H
#define GROVE_LCD16X2_I2C_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32l4xx_hal.h"
#include <stdint.h>
#include <stddef.h>

/**
 * @file grove_lcd16x2_i2c.h
 * @brief Grove 16x2 LCD driver over I2C (HD44780-compatible command set).
 *
 * Device basics (common for Grove 16x2 RGB/LCD adapters):
 * - Default 7-bit I2C address: 0x3E
 * - Command "register": 0x80
 * - Data "register":    0x40
 *
 * The driver uses HAL_I2C_Mem_Write() to write a single command or data byte.
 */

#define GROVE_LCD_I2C_ADDR_7BIT_DEFAULT   (0x3E)

/**
 * @brief Driver context for a single LCD instance.
 */
typedef struct
{
    I2C_HandleTypeDef *hi2c;   /**< HAL I2C handle used for communication */
    uint8_t addr_7bit;         /**< 7-bit I2C address (e.g. 0x3E) */
    uint32_t timeout_ms;       /**< HAL I2C timeout in milliseconds */
} GroveLCD_t;

/* --- Public API --- */

/**
 * @brief Initialize the LCD in 16x2 mode, clear screen, set entry mode.
 *
 * @param lcd       Driver context.
 * @param hi2c      HAL I2C handle.
 * @param addr_7bit 7-bit I2C address.
 * @return HAL status.
 */
HAL_StatusTypeDef GroveLCD_Init(GroveLCD_t *lcd, I2C_HandleTypeDef *hi2c, uint8_t addr_7bit);

/** @brief Clear display and return cursor to home position. */
HAL_StatusTypeDef GroveLCD_Clear(GroveLCD_t *lcd);

/** @brief Return cursor to home position (DDRAM address 0). */
HAL_StatusTypeDef GroveLCD_Home(GroveLCD_t *lcd);

/**
 * @brief Set cursor position for 16x2 layout.
 *
 * row 0 -> DDRAM 0x00..0x0F
 * row 1 -> DDRAM 0x40..0x4F
 */
HAL_StatusTypeDef GroveLCD_SetCursor(GroveLCD_t *lcd, uint8_t row, uint8_t col);

/** @brief Write a single character at the current cursor position. */
HAL_StatusTypeDef GroveLCD_WriteChar(GroveLCD_t *lcd, char c);

/** @brief Write a null-terminated string starting at the current cursor position. */
HAL_StatusTypeDef GroveLCD_Print(GroveLCD_t *lcd, const char *s);

/**
 * @brief Define a custom character in CGRAM (slot 0..7).
 *
 * pattern[] contains 8 rows, only the lower 5 bits in each row are used.
 */
HAL_StatusTypeDef GroveLCD_CreateChar(GroveLCD_t *lcd, uint8_t slot, const uint8_t pattern[8]);

/* Optional helpers */
HAL_StatusTypeDef GroveLCD_DisplayOn(GroveLCD_t *lcd);
HAL_StatusTypeDef GroveLCD_DisplayOff(GroveLCD_t *lcd);
HAL_StatusTypeDef GroveLCD_CursorOn(GroveLCD_t *lcd);
HAL_StatusTypeDef GroveLCD_CursorOff(GroveLCD_t *lcd);
HAL_StatusTypeDef GroveLCD_BlinkOn(GroveLCD_t *lcd);
HAL_StatusTypeDef GroveLCD_BlinkOff(GroveLCD_t *lcd);

#ifdef __cplusplus
}
#endif

#endif /* GROVE_LCD16X2_I2C_H */
