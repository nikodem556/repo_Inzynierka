#ifndef BUTTON_H
#define BUTTON_H

#include <stdint.h>
#include <stdbool.h>

/* Button identifiers */
typedef enum {
    BUTTON_RESET = 0,
    BUTTON_NEXT,
    BUTTON_OK,
    BUTTON_COUNT
} ButtonType;

/* Initialize button module (GPIO assumed configured in CubeMX, but safe to call). */
void Button_Init(void);

/*
 * Must be called periodically (e.g. every main loop iteration).
 * Updates debouncing and edge detection.
 */
void Button_Update(void);

/*
 * Returns true once per physical press (edge-triggered).
 * Button_Update() must be called regularly for correct behavior.
 */
bool Button_WasPressed(ButtonType button);

#endif // BUTTON_H
