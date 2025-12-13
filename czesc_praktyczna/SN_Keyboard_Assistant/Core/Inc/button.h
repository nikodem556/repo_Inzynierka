#ifndef BUTTON_H
#define BUTTON_H

#include <stdint.h>

/**
 * @brief Initialize the button debounce module (capture initial state).
 */
void Button_Init(void);

/**
 * @brief Poll the button input and update its debounced state.
 *        Call this function frequently (e.g., on every main loop iteration).
 */
void Button_Update(void);

/**
 * @brief Check if a debounced button press event has occurred since the last check.
 * @return 1 if the button was pressed (released-to-pressed transition) since last call, 0 otherwise.
 * @note  When this returns 1, the internal flag is reset until the next press event.
 */
uint8_t Button_WasPressedEvent(void);

#endif /* BUTTON_H */
