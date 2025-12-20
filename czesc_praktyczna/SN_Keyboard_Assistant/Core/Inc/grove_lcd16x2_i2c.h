#ifndef GROVE_LCD16X2_I2C_H
#define GROVE_LCD16X2_I2C_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32l4xx_hal.h"
#include <stdint.h>
#include <stddef.h>

/*
 * Grove 16x2 LCD (I2C)
 * - Default 7-bit address: 0x3E
 * - Command register: 0x80
 * - Data register:    0x40
 */

#define GROVE_LCD_I2C_ADDR_7BIT_DEFAULT   (0x3E)

typedef struct
{
    I2C_HandleTypeDef *hi2c;
    uint8_t addr_7bit;          // 7-bit I2C address (e.g. 0x3E)
    uint32_t timeout_ms;        // HAL I2C timeout
} GroveLCD_t;

/* --- Public API --- */

HAL_StatusTypeDef GroveLCD_Init(GroveLCD_t *lcd, I2C_HandleTypeDef *hi2c, uint8_t addr_7bit);
HAL_StatusTypeDef GroveLCD_Clear(GroveLCD_t *lcd);
HAL_StatusTypeDef GroveLCD_Home(GroveLCD_t *lcd);

HAL_StatusTypeDef GroveLCD_SetCursor(GroveLCD_t *lcd, uint8_t row, uint8_t col);

HAL_StatusTypeDef GroveLCD_WriteChar(GroveLCD_t *lcd, char c);
HAL_StatusTypeDef GroveLCD_Print(GroveLCD_t *lcd, const char *s);

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
