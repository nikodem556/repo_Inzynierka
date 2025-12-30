#ifndef BUTTON_H
#define BUTTON_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @file button.h
 * @brief Debounced button input with edge (press) detection.
 *
 * The module provides:
 * - periodic debouncing via Button_Update()
 * - one-shot press events via Button_WasPressed()
 */

/* Logical button identifiers used across the application. */
typedef enum {
    BUTTON_RESET = 0,
    BUTTON_NEXT,
    BUTTON_OK,
    BUTTON_COUNT
} ButtonType;

/**
 * @brief Initialize the button module.
 *
 * GPIO is typically configured in CubeMX, but this function also configures the
 * pins as a safety net (input + pull-up).
 */
void Button_Init(void);

/**
 * @brief Update debouncing and press-event detection.
 *
 * Must be called periodically (e.g., every main loop iteration) for correct
 * behavior.
 */
void Button_Update(void);

/**
 * @brief Return true once per physical press (edge-triggered).
 *
 * The event is latched and cleared on read.
 * Button_Update() must be called regularly for correct behavior.
 *
 * @param button Button identifier.
 * @return true if a new press event occurred since last call; otherwise false.
 */
bool Button_WasPressed(ButtonType button);

#endif // BUTTON_H
