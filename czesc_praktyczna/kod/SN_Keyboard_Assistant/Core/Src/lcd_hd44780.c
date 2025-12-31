#include "main.h"
#include "lcd_hd44780.h"
#include "stm32l4xx_hal.h"

/* --- proste opóźnienie w us (dla LCD wystarczy) --- */
static void lcd_delay_us(volatile uint32_t us)
{
    while (us--)
    {
        /* ~1us-ish przy typowych zegarach; LCD nie jest wymagający */
        for (volatile uint32_t i = 0; i < 8; i++) { __NOP(); }
    }
}

/* Impuls E: min ~450ns, dajemy kilka us */
static void lcd_pulse_enable(void)
{
    HAL_GPIO_WritePin(LCD_E_GPIO_Port, LCD_E_Pin, GPIO_PIN_SET);
    lcd_delay_us(2);
    HAL_GPIO_WritePin(LCD_E_GPIO_Port, LCD_E_Pin, GPIO_PIN_RESET);
    lcd_delay_us(50);  // czas na “zlapanie” danych
}

/* >>> NAJWAŻNIEJSZA POPRAWKA: bit0->D4 ... bit3->D7 <<< */
static void lcd_write4(uint8_t nibble)
{
    HAL_GPIO_WritePin(LCD_D4_GPIO_Port, LCD_D4_Pin, (nibble & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LCD_D5_GPIO_Port, LCD_D5_Pin, (nibble & 0x02) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LCD_D6_GPIO_Port, LCD_D6_Pin, (nibble & 0x04) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LCD_D7_GPIO_Port, LCD_D7_Pin, (nibble & 0x08) ? GPIO_PIN_SET : GPIO_PIN_RESET);

    lcd_pulse_enable();
}

static void lcd_send_byte(uint8_t byte, uint8_t isData)
{
    HAL_GPIO_WritePin(LCD_RS_GPIO_Port, LCD_RS_Pin, isData ? GPIO_PIN_SET : GPIO_PIN_RESET);

    lcd_write4((byte >> 4) & 0x0F);
    lcd_write4(byte & 0x0F);

    /* typowe czasy wykonania wg HD44780: ~37us, a clear/home ~1.52ms */
    if (!isData && (byte == 0x01 || byte == 0x02))
        HAL_Delay(2);
    else
        lcd_delay_us(50);
}

void LCD_Init(void)
{
    /* WAŻNE: wywołaj po HAL_Init() i po MX_GPIO_Init() */
    HAL_Delay(50); // >40ms po starcie zasilania LCD

    HAL_GPIO_WritePin(LCD_RS_GPIO_Port, LCD_RS_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LCD_E_GPIO_Port,  LCD_E_Pin,  GPIO_PIN_RESET);

    /* Sekwencja wejścia w 4-bit */
    lcd_write4(0x03); HAL_Delay(5);
    lcd_write4(0x03); lcd_delay_us(150);
    lcd_write4(0x03); lcd_delay_us(150);
    lcd_write4(0x02); lcd_delay_us(150);

    /* Konfiguracja */
    lcd_send_byte(0x28, 0); // 4-bit, 2 linie, 5x8
    lcd_send_byte(0x0C, 0); // display on
    lcd_send_byte(0x06, 0); // entry mode inc
    lcd_send_byte(0x01, 0); // clear
}

void LCD_Clear(void)
{
    lcd_send_byte(0x01, 0);
}

void LCD_SetCursor(uint8_t row, uint8_t col)
{
    if (row > 1) row = 1;
    if (col > 15) col = 15;

    uint8_t addr = (row == 0) ? 0x00 : 0x40;
    addr += col;

    lcd_send_byte(0x80 | addr, 0);
}

void LCD_Print(const char *str)
{
    if (!str) return;
    while (*str)
        lcd_send_byte((uint8_t)*str++, 1);
}
