#include "main.h"           // For GPIO port/pin definitions and HAL functions
#include "lcd_hd44780.h"
#include <string.h>         // For strlen if needed (not strictly required here)

/**
 * @brief Pulse the LCD Enable line (E) to latch the data.
 */
static void lcd_pulse_enable(void) {
    HAL_GPIO_WritePin(LCD_E_GPIO_Port, LCD_E_Pin, GPIO_PIN_SET);
    // E pulse width >= 450 ns. Use a short delay to ensure the LCD latches the data.
    HAL_Delay(1);  // 1 ms delay (safe margin for pulse width)
    HAL_GPIO_WritePin(LCD_E_GPIO_Port, LCD_E_Pin, GPIO_PIN_RESET);
    // Small delay after falling edge of E
    HAL_Delay(1);
}

/**
 * @brief Send a 4-bit nibble to the LCD (assumes RS is already set appropriately).
 */
static void lcd_write4(uint8_t nibble) {
    // Set each data line (D4-D7) according to the bits of the nibble
	HAL_GPIO_WritePin(LCD_D7_GPIO_Port, LCD_D7_Pin, (nibble & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LCD_D6_GPIO_Port, LCD_D6_Pin, (nibble & 0x02) ? GPIO_PIN_SET : GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LCD_D5_GPIO_Port, LCD_D5_Pin, (nibble & 0x04) ? GPIO_PIN_SET : GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LCD_D4_GPIO_Port, LCD_D4_Pin, (nibble & 0x08) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    // Pulse Enable to latch this nibble
    lcd_pulse_enable();
}

/**
 * @brief Send a full byte to the LCD.
 * @param byte    The data/command value to send.
 * @param isData  Set to 1 if sending display data, 0 if sending a command.
 */
static void lcd_send_byte(uint8_t byte, uint8_t isData) {
    // Set RS (Register Select): LOW for command, HIGH for data
    HAL_GPIO_WritePin(LCD_RS_GPIO_Port, LCD_RS_Pin, isData ? GPIO_PIN_SET : GPIO_PIN_RESET);
    // Send high nibble (bits 7-4)
    lcd_write4((byte >> 4) & 0x0F);
    // Send low nibble (bits 3-0)
    lcd_write4(byte & 0x0F);
    // Delay for command execution time:
    if (!isData && (byte == 0x01 || byte == 0x02)) {
        // Clear display (0x01) and Return home (0x02) require >1.5 ms
        HAL_Delay(2);
    } else {
        // Most other commands and data writes need ~40 µs, use 1 ms for safety
        HAL_Delay(1);
    }
}

void LCD_Init(void) {
    // The GPIO pins for LCD must be configured as outputs (done in CubeMX configuration).
    HAL_Delay(15);  // wait >15 ms after LCD power-up

    // Initialization sequence for 4-bit mode:
    // Step 1: Send 0x3 three times (LCD default to 8-bit mode) to prepare switching to 4-bit.
    HAL_GPIO_WritePin(LCD_RS_GPIO_Port, LCD_RS_Pin, GPIO_PIN_RESET);  // RS = 0 (command)
    HAL_GPIO_WritePin(LCD_E_GPIO_Port, LCD_E_Pin, GPIO_PIN_RESET);
    lcd_write4(0x03);
    HAL_Delay(5);
    lcd_write4(0x03);
    HAL_Delay(1);
    lcd_write4(0x03);
    HAL_Delay(1);
    // Step 2: Send 0x2 to switch to 4-bit mode
    lcd_write4(0x02);
    HAL_Delay(1);
    // Step 3: Send function set, display control, entry mode, and clear commands
    lcd_send_byte(0x28, 0);  // Function set: 4-bit mode, 2 lines, 5x8 font (0x28)
    lcd_send_byte(0x0C, 0);  // Display ON, Cursor OFF, Blink OFF (0x0C)
    lcd_send_byte(0x06, 0);  // Entry mode: increment cursor, no display shift (0x06)
    lcd_send_byte(0x01, 0);  // Clear display (0x01)
    // LCD is now initialized (display cleared, cursor at home position).
}

void LCD_Clear(void) {
    lcd_send_byte(0x01, 0);  // Clear display command
    // (Delay handled inside lcd_send_byte)
}

void LCD_SetCursor(uint8_t row, uint8_t col) {
    if (row > 1) row = 1;
    if (col > 15) col = 15;
    uint8_t address = (row == 0) ? 0x00 : 0x40;  // DDRAM offset for line 0 or 1
    address += col;
    lcd_send_byte(0x80 | address, 0);  // 0x80 sets DDRAM address
}

void LCD_Print(const char *str) {
    if (str == NULL) return;
    while (*str) {
        lcd_send_byte((uint8_t)*str++, 1);  // Send each character (RS=1 for data)
    }
}
