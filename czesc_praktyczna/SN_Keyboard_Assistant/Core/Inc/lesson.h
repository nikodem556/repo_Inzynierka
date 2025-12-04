/*
 * lesson.h
 *
 *  Created on: 5 gru 2025
 *      Author: nikod
 */

#ifndef LESSON_H
#define LESSON_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Initialize lesson system.
 *
 * - Converts textual note names (e.g. "C4", "D5") to MIDI note numbers.
 * - Resets current step and LEDs.
 * - Must be called once at startup (after GPIO init).
 */
void Lesson_Init(void);

/**
 * @brief Reset lesson state back to the first step.
 *
 * - Sets current step to 0.
 * - Turns off all LEDs and lights the LED for the first expected note.
 */
void Lesson_Reset(void);

/**
 * @brief Handle a NOTE ON event from the MIDI keyboard.
 *
 * Call this from your main loop whenever you detect a NOTE ON message.
 *
 * @param note  MIDI note number (0..127).
 * @param vel   Velocity (0..127) - can be used later if needed.
 */
void Lesson_OnNoteOn(uint8_t note, uint8_t vel);

/**
 * @brief Check if the current lesson sequence has been completed.
 *
 * @retval true  Lesson finished (all notes played correctly).
 * @retval false Lesson still in progress.
 */
bool Lesson_IsFinished(void);

/* -------------------------------------------------------------------------
 * Hardware abstraction for LEDs
 * -------------------------------------------------------------------------
 * You need to IMPLEMENT these functions in your project (e.g. in lesson.c
 * or a separate lesson_hw.c) using HAL_GPIO_WritePin or similar.
 *
 * The idea:
 *  - There are N "note LEDs" (one per lesson step).
 *  - There is 1 extra "error LED" to signal a wrong note.
 * -------------------------------------------------------------------------*/

/**
 * @brief Turn off all LEDs used by the lesson system.
 */
void Lesson_Leds_AllOff(void);

/**
 * @brief Turn on LED for a given lesson step index (0-based).
 *
 * Example mapping:
 *  step 0 -> LED0
 *  step 1 -> LED1
 *  step 2 -> LED2
 *  step 3 -> LED3
 *
 * @param stepIndex Index of the lesson step (0..N-1).
 */
void Lesson_Led_NoteOn(uint8_t stepIndex);

/**
 * @brief Turn off LED for a given lesson step index (0-based).
 */
void Lesson_Led_NoteOff(uint8_t stepIndex);

/**
 * @brief Visual feedback for a wrong note.
 *
 * Example implementation:
 *  - Turn on "error LED" for a short time, then turn it off.
 *  - Or blink it a few times.
 */
void Lesson_Led_ErrorFeedback(void);

/**
 * @brief Visual feedback when the whole lesson is finished.
 *
 * Example implementation:
 *  - Blink all note LEDs.
 *  - Or turn on a "success LED".
 */
void Lesson_Led_SuccessFeedback(void);

#endif /* LESSON_H */
