#include "main.h"    // For GPIO definitions and HAL functions
#include "button.h"

// Debounce configuration
#define DEBOUNCE_DELAY_MS  20  // minimum stable time in ms to confirm state change

// Static variables to track button state (debounce logic)
static uint8_t stableState = 0;       // Last stable (debounced) state: 1 = pressed, 0 = not pressed
static uint8_t lastReading = 0;       // Last raw reading of the button (before debounce)
static uint32_t lastChangeTime = 0;   // Timestamp of last state change (for debounce timing)
static uint8_t pressedEventFlag = 0;  // Flag indicating a press event occurred

void Button_Init(void) {
    // Read the initial raw state of the button
    GPIO_PinState initial = HAL_GPIO_ReadPin(RESET_BTN_GPIO_Port, RESET_BTN_Pin);
    // Determine if initial state is pressed or not pressed based on active level
    if (initial == (RESET_BTN_ACTIVE_LEVEL ? GPIO_PIN_SET : GPIO_PIN_RESET)) {
        stableState = 1;  // button is pressed initially
    } else {
        stableState = 0;  // button is not pressed initially
    }
    lastReading = stableState;
    lastChangeTime = HAL_GetTick();
    pressedEventFlag = 0;
}

void Button_Update(void) {
    // Read current raw state of the button pin
    GPIO_PinState rawState = HAL_GPIO_ReadPin(RESET_BTN_GPIO_Port, RESET_BTN_Pin);
    uint8_t rawPressed = (rawState == (RESET_BTN_ACTIVE_LEVEL ? GPIO_PIN_SET : GPIO_PIN_RESET)) ? 1 : 0;

    if (rawPressed != lastReading) {
        // Button state has changed (could be bounce), reset the debounce timer
        lastChangeTime = HAL_GetTick();
        lastReading = rawPressed;
    }
    // If the state remains changed for longer than the debounce delay, accept the new state
    if (rawPressed != stableState && (HAL_GetTick() - lastChangeTime) >= DEBOUNCE_DELAY_MS) {
        // Update the debounced stable state
        stableState = rawPressed;
        if (stableState == 1) {
            // Button has transitioned from released to pressed
            pressedEventFlag = 1;
        }
    }
}

uint8_t Button_WasPressedEvent(void) {
    if (pressedEventFlag) {
        // A press event was detected since last check
        pressedEventFlag = 0;
        return 1;
    }
    return 0;
}
