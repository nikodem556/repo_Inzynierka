#include "button.h"
#include "stm32l4xx_hal.h"
#include "main.h"

/**
 * @file button.c
 * @brief Debounce and edge detection for three GPIO buttons.
 *
 * Electrical assumptions:
 * - Buttons are wired with pull-ups (released = HIGH, pressed = LOW).
 * - The code treats LOW as "pressed".
 *
 * Timing:
 * - Debouncing is implemented using a simple time threshold (DEBOUNCE_MS).
 * - A press event is generated only on a stable transition: released -> pressed.
 */


/* Debounce time threshold (milliseconds) */
#define DEBOUNCE_MS  30U

/**
 * @brief Internal per-button state for debouncing and event latching.
 *
 * stable_level:
 *   - 1 = released, 0 = pressed (active-low, pull-up)
 * last_raw_level:
 *   - last sampled raw level (used to detect changes)
 * last_change_ms:
 *   - timestamp of the last raw-level change (HAL_GetTick())
 * pressed_event:
 *   - latched one-shot flag set when a new press is detected
 */
typedef struct {
    uint8_t stable_level;
    uint8_t last_raw_level;
    uint32_t last_change_ms;
    uint8_t pressed_event;
} BtnState_t;

static BtnState_t g_btn[BUTTON_COUNT];

/**
 * @brief Read raw GPIO level and normalize to logical level.
 *
 * Returns:
 * - 1 for released (GPIO HIGH)
 * - 0 for pressed  (GPIO LOW)
 */
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
    /*
     * If pins are already configured in CubeMX, this is optional.
     * Keeping it here is fine as a safety net (input + pull-up).
     */
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

    /* Initialize software state from the current raw levels. */
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

        /* Track raw-level changes and reset debounce timer on any transition. */
        if (raw != g_btn[i].last_raw_level) {
            g_btn[i].last_raw_level = raw;
            g_btn[i].last_change_ms = now;
        }

        /* If the raw level has been stable long enough, accept it as stable. */
        if ((now - g_btn[i].last_change_ms) >= DEBOUNCE_MS)
        {
            if (raw != g_btn[i].stable_level)
            {
                /* Stable edge detected */
                uint8_t prev = g_btn[i].stable_level;
                g_btn[i].stable_level = raw;

                /* Press event = released->pressed transition (1->0) */
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

    /* One-shot: return true once and clear the latched flag. */
    if (g_btn[button].pressed_event) {
        g_btn[button].pressed_event = 0;
        return true;
    }
    return false;
}
