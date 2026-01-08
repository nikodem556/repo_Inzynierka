#ifndef LCD_HD44780_H
#define LCD_HD44780_H

#include <stdint.h>

/**
 * @brief Initialize the HD44780 16x2 LCD (4-bit mode).
 */
void LCD_Init(void);

/**
 * @brief Clear the LCD display and return cursor to home position.
 */
void LCD_Clear(void);

/**
 * @brief Set the cursor position.
 * @param row  LCD row (0 or 1)
 * @param col  LCD column (0-15)
 */
void LCD_SetCursor(uint8_t row, uint8_t col);

/**
 * @brief Print a null-terminated string to the LCD at the current cursor position.
 */
void LCD_Print(const char *str);

#endif /* LCD_HD44780_H */
