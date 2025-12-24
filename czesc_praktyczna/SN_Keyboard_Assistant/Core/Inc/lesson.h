#ifndef LESSON_H
#define LESSON_H

#include <stdint.h>
#include <stdbool.h>
#include "songs.h"
#include "chords.h"

// Special codes for non-note inputs to Lesson_HandleInput
#define LESSON_INPUT_BTN_OK    0xF1  // Forward/OK
#define LESSON_INPUT_BTN_NEXT  0xF2  // Backward/NEXT (used as "previous step")
#define LESSON_INPUT_BTN_RESET 0xF3  // Reset

// Initialize and start a song lesson
void Lesson_StartSong(Song *song);

// Initialize and start a chord exercise
void Lesson_StartChordExercise(ChordPack *pack);

// Handle an input event for the lesson (MIDI note or button code)
// If input is a MIDI note (0-127), pass the note value directly (uint8_t).
// If input is a special button code (LESSON_INPUT_BTN_*), pass that constant.
void Lesson_HandleInput(uint8_t input);

// Check if a lesson is currently active (in progress)
bool Lesson_IsActive(void);

#endif // LESSON_H
