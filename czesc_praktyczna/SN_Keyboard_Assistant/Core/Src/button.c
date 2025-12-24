#include "button.h"
#include "stm32l4xx_hal.h"

/* GPIO mapping (your current pins) */
#define BTN_NEXT_PORT   GPIOA
#define BTN_NEXT_PIN    GPIO_PIN_1

#define BTN_OK_PORT     GPIOA
#define BTN_OK_PIN      GPIO_PIN_4

#define BTN_RESET_PORT  GPIOB
#define BTN_RESET_PIN   GPIO_PIN_0

/* Simple debounce parameters */
#define DEBOUNCE_MS  30U

typedef struct {
    uint8_t stable_level;       /* 1=released, 0=pressed (pull-up -> active low) */
    uint8_t last_raw_level;
    uint32_t last_change_ms;
    uint8_t pressed_event;      /* latched: 1 if a new press was detected */
} BtnState_t;

static BtnState_t g_btn[BUTTON_COUNT];

static uint8_t read_raw(ButtonType b)
{
    GPIO_PinState ps = GPIO_PIN_SET;

    switch (b) {
        case BUTTON_NEXT:  ps = HAL_GPIO_ReadPin(BTN_NEXT_PORT, BTN_NEXT_PIN); break;
        case BUTTON_OK:    ps = HAL_GPIO_ReadPin(BTN_OK_PORT, BTN_OK_PIN);     break;
        case BUTTON_RESET: ps = HAL_GPIO_ReadPin(BTN_RESET_PORT, BTN_RESET_PIN); break;
        default:           ps = GPIO_PIN_SET; break;
    }

    /* pull-up: released=1, pressed=0 */
    return (ps == GPIO_PIN_RESET) ? 0U : 1U;
}

void Button_Init(void)
{
    /* If you already configure pins in CubeMX, this is optional.
       Keeping it here is fine as a safety net. */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    GPIO_InitStruct.Pin = BTN_NEXT_PIN;
    HAL_GPIO_Init(BTN_NEXT_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = BTN_OK_PIN;
    HAL_GPIO_Init(BTN_OK_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = BTN_RESET_PIN;
    HAL_GPIO_Init(BTN_RESET_PORT, &GPIO_InitStruct);

    uint32_t now = HAL_GetTick();
    for (uint8_t i = 0; i < BUTTON_COUNT; i++) {
        uint8_t raw = read_raw((ButtonType)i);
        g_btn[i].stable_level   = raw;
        g_btn[i].last_raw_level = raw;
        g_btn[i].last_change_ms = now;
        g_btn[i].pressed_event  = 0;
    }
}

void Button_Update(void)
{
    uint32_t now = HAL_GetTick();

    for (uint8_t i = 0; i < BUTTON_COUNT; i++)
    {
        ButtonType b = (ButtonType)i;
        uint8_t raw = read_raw(b);

        if (raw != g_btn[i].last_raw_level) {
            g_btn[i].last_raw_level = raw;
            g_btn[i].last_change_ms = now;
        }

        /* If raw has been stable long enough -> accept as stable */
        if ((now - g_btn[i].last_change_ms) >= DEBOUNCE_MS)
        {
            if (raw != g_btn[i].stable_level)
            {
                /* Edge detected on stable signal */
                uint8_t prev = g_btn[i].stable_level;
                g_btn[i].stable_level = raw;

                /* Released->Pressed = 1->0 */
                if (prev == 1U && raw == 0U) {
                    g_btn[i].pressed_event = 1U;
                }
            }
        }
    }
}

bool Button_WasPressed(ButtonType button)
{
    if (button >= BUTTON_COUNT) return false;

    if (g_btn[button].pressed_event) {
        g_btn[button].pressed_event = 0;
        return true;
    }
    return false;
}
