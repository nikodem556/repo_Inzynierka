#ifndef LESSON_H
#define LESSON_H

#include <stdint.h>
#include <stdbool.h>
#include "songs.h"
#include "chords.h"

/*
 * lesson.h / lesson.c
 *
 * This module implements the "lesson engine" used by the application:
 * - Song lesson mode: user must play the required notes for each step
 * - Chord exercise mode: user must play the chord tones (pitch-class matching)
 *
 * Inputs are provided through Lesson_HandleInput():
 * - MIDI notes (0..127)
 * - Special button codes (LESSON_INPUT_BTN_*)
 *
 * The module also provides a non-blocking update function (Lesson_Update())
 * for time-based feedback (LED blinking).
 */

/* Special codes for non-note inputs to Lesson_HandleInput() */
#define LESSON_INPUT_BTN_OK    0xF1  /* Forward / OK */
#define LESSON_INPUT_BTN_NEXT  0xF2  /* Backward / NEXT (previous step) */
#define LESSON_INPUT_BTN_RESET 0xF3  /* Reset */

/* Initialize and start a song lesson. */
void Lesson_StartSong(Song *song);

/* Initialize and start a chord exercise (chord pack). */
void Lesson_StartChordExercise(ChordPack *pack);

/*
 * Handle one input event.
 * - For MIDI NOTE ON: pass the note value directly (0..127).
 * - For buttons: pass one of LESSON_INPUT_BTN_* constants.
 */
void Lesson_HandleInput(uint8_t input);

/* Returns true if a lesson is currently active (running or summary screen). */
bool Lesson_IsActive(void);

/*
 * Periodic non-blocking update.
 * Used mainly for timing-based feedback (e.g. turning LEDs off after a blink).
 * Call regularly from the main loop (e.g. from App_Update()).
 */
void Lesson_Update(void);

#endif /* LESSON_H */
