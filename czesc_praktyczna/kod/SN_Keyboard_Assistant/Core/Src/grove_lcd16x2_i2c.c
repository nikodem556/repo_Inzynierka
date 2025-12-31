#include "grove_lcd16x2_i2c.h"

/**
 * @file grove_lcd16x2_i2c.c
 * @brief Grove 16x2 LCD (I2C) driver implementation.
 *
 * This driver communicates with the Grove LCD backpack using I2C "memory write"
 * transactions:
 * - GROVE_LCD_REG_CMD  (0x80): command register
 * - GROVE_LCD_REG_DATA (0x40): data register
 *
 * The command set is HD44780-like (clear, home, entry mode, display control,
 * function set, set DDRAM address, set CGRAM address).
 */

/* Grove LCD "registers" */
#define GROVE_LCD_REG_CMD   (0x80)
#define GROVE_LCD_REG_DATA  (0x40)

/* HD44780-like commands */
#define LCD_CMD_CLEAR           (0x01)
#define LCD_CMD_HOME            (0x02)
#define LCD_CMD_ENTRYMODE       (0x04)
#define LCD_CMD_DISPLAYCTRL     (0x08)
#define LCD_CMD_FUNCTIONSET     (0x20)
#define LCD_CMD_SET_DDRAM       (0x80)
#define LCD_CMD_SET_CGRAM       (0x40)

/* Entry mode flags */
#define LCD_ENTRY_INC           (0x02)
#define LCD_ENTRY_SHIFT_OFF     (0x00)

/* Display control flags */
#define LCD_DISPLAY_ON          (0x04)
#define LCD_DISPLAY_OFF         (0x00)
#define LCD_CURSOR_ON           (0x02)
#define LCD_CURSOR_OFF          (0x00)
#define LCD_BLINK_ON            (0x01)
#define LCD_BLINK_OFF           (0x00)

/* Function set flags */
#define LCD_2LINE               (0x08)
#define LCD_5x8DOTS             (0x00)

/**
 * @brief Convert 7-bit I2C address into HAL format (left-shift by 1).
 */
static inline uint16_t lcd_hal_addr(const GroveLCD_t *lcd)
{
    return (uint16_t)(lcd->addr_7bit << 1);
}

/**
 * @brief Low-level helper: write one command byte to the LCD command register.
 */
static HAL_StatusTypeDef lcd_write_cmd(GroveLCD_t *lcd, uint8_t cmd)
{
    return HAL_I2C_Mem_Write(
        lcd->hi2c,
        lcd_hal_addr(lcd),
        GROVE_LCD_REG_CMD,
        I2C_MEMADD_SIZE_8BIT,
        &cmd,
        1,
        lcd->timeout_ms
    );
}

/**
 * @brief Low-level helper: write one data byte to the LCD data register.
 */
static HAL_StatusTypeDef lcd_write_data(GroveLCD_t *lcd, uint8_t data)
{
    return HAL_I2C_Mem_Write(
        lcd->hi2c,
        lcd_hal_addr(lcd),
        GROVE_LCD_REG_DATA,
        I2C_MEMADD_SIZE_8BIT,
        &data,
        1,
        lcd->timeout_ms
    );
}

HAL_StatusTypeDef GroveLCD_Init(GroveLCD_t *lcd, I2C_HandleTypeDef *hi2c, uint8_t addr_7bit)
{
    if (lcd == NULL || hi2c == NULL)
        return HAL_ERROR;

    /* Store I2C handle, address and a conservative default timeout. */
    lcd->hi2c = hi2c;
    lcd->addr_7bit = addr_7bit;
    lcd->timeout_ms = 50;

    /* Power-up delay */
    HAL_Delay(50);

    /*
     * Initialization sequence (HD44780-like).
     * Delays are conservative for reliability across modules.
     */
    HAL_StatusTypeDef st;

    st = lcd_write_cmd(lcd, LCD_CMD_HOME);
    if (st != HAL_OK) return st;
    HAL_Delay(5);

    st = lcd_write_cmd(lcd, LCD_CMD_FUNCTIONSET | LCD_2LINE | LCD_5x8DOTS); // 0x28
    if (st != HAL_OK) return st;
    HAL_Delay(1);

    st = lcd_write_cmd(lcd, LCD_CMD_DISPLAYCTRL | LCD_DISPLAY_ON | LCD_CURSOR_OFF | LCD_BLINK_OFF); // 0x0C
    if (st != HAL_OK) return st;
    HAL_Delay(1);

    st = lcd_write_cmd(lcd, LCD_CMD_CLEAR);
    if (st != HAL_OK) return st;
    HAL_Delay(3);

    st = lcd_write_cmd(lcd, LCD_CMD_ENTRYMODE | LCD_ENTRY_INC | LCD_ENTRY_SHIFT_OFF); // 0x06
    if (st != HAL_OK) return st;
    HAL_Delay(1);

    return HAL_OK;
}

HAL_StatusTypeDef GroveLCD_Clear(GroveLCD_t *lcd)
{
    /* Clear requires a longer execution time on the LCD controller. */
    HAL_StatusTypeDef st = lcd_write_cmd(lcd, LCD_CMD_CLEAR);
    HAL_Delay(3);
    return st;
}

HAL_StatusTypeDef GroveLCD_Home(GroveLCD_t *lcd)
{
    /* Home requires a longer execution time on the LCD controller. */
    HAL_StatusTypeDef st = lcd_write_cmd(lcd, LCD_CMD_HOME);
    HAL_Delay(3);
    return st;
}

HAL_StatusTypeDef GroveLCD_SetCursor(GroveLCD_t *lcd, uint8_t row, uint8_t col)
{
    /*
     * Standard 16x2 DDRAM layout:
     * row 0 -> 0x00..0x0F
     * row 1 -> 0x40..0x4F
     */
    uint8_t base = (row == 0) ? 0x00 : 0x40;
    uint8_t addr = (uint8_t)(base + (col & 0x0F));
    return lcd_write_cmd(lcd, (uint8_t)(LCD_CMD_SET_DDRAM | addr));
}

HAL_StatusTypeDef GroveLCD_WriteChar(GroveLCD_t *lcd, char c)
{
    return lcd_write_data(lcd, (uint8_t)c);
}

HAL_StatusTypeDef GroveLCD_Print(GroveLCD_t *lcd, const char *s)
{
    if (lcd == NULL || s == NULL) return HAL_ERROR;

    HAL_StatusTypeDef st = HAL_OK;
    while (*s)
    {
        st = lcd_write_data(lcd, (uint8_t)*s++);
        if (st != HAL_OK) return st;
    }
    return HAL_OK;
}

HAL_StatusTypeDef GroveLCD_CreateChar(GroveLCD_t *lcd, uint8_t slot, const uint8_t pattern[8])
{
    if (lcd == NULL || pattern == NULL) return HAL_ERROR;
    if (slot > 7) return HAL_ERROR;

    /* 1) Set CGRAM address: 0x40 | (slot * 8) */
    uint8_t cmd = (uint8_t)(LCD_CMD_SET_CGRAM | ((slot & 0x07) << 3));
    HAL_StatusTypeDef st = lcd_write_cmd(lcd, cmd);
    if (st != HAL_OK) return st;

    /* 2) Write 8 rows (only lower 5 bits are used by the character generator) */
    for (uint8_t i = 0; i < 8; i++)
    {
        uint8_t row = (uint8_t)(pattern[i] & 0x1F);
        st = lcd_write_data(lcd, row);
        if (st != HAL_OK) return st;
    }

    /* 3) Return to DDRAM (recommended after CGRAM write) */
    st = lcd_write_cmd(lcd, LCD_CMD_SET_DDRAM);
    HAL_Delay(1);
    return st;
}

/* --- Optional helpers (display/cursor/blink) --- */

HAL_StatusTypeDef GroveLCD_DisplayOn(GroveLCD_t *lcd)
{
    return lcd_write_cmd(lcd, LCD_CMD_DISPLAYCTRL | LCD_DISPLAY_ON | LCD_CURSOR_OFF | LCD_BLINK_OFF);
}

HAL_StatusTypeDef GroveLCD_DisplayOff(GroveLCD_t *lcd)
{
    return lcd_write_cmd(lcd, LCD_CMD_DISPLAYCTRL | LCD_DISPLAY_OFF);
}

HAL_StatusTypeDef GroveLCD_CursorOn(GroveLCD_t *lcd)
{
    return lcd_write_cmd(lcd, LCD_CMD_DISPLAYCTRL | LCD_DISPLAY_ON | LCD_CURSOR_ON | LCD_BLINK_OFF);
}

HAL_StatusTypeDef GroveLCD_CursorOff(GroveLCD_t *lcd)
{
    return lcd_write_cmd(lcd, LCD_CMD_DISPLAYCTRL | LCD_DISPLAY_ON | LCD_CURSOR_OFF | LCD_BLINK_OFF);
}

HAL_StatusTypeDef GroveLCD_BlinkOn(GroveLCD_t *lcd)
{
    return lcd_write_cmd(lcd, LCD_CMD_DISPLAYCTRL | LCD_DISPLAY_ON | LCD_CURSOR_OFF | LCD_BLINK_ON);
}

HAL_StatusTypeDef GroveLCD_BlinkOff(GroveLCD_t *lcd)
{
    return lcd_write_cmd(lcd, LCD_CMD_DISPLAYCTRL | LCD_DISPLAY_ON | LCD_CURSOR_OFF | LCD_BLINK_OFF);
}
