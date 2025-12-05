#ifndef LESSON_H
#define LESSON_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

/**
 * Initialize the lesson system.
 * Converts the configured lesson note names to MIDI notes, resets internal state, and lights the first step's LED.
 * Should be called once after GPIO and USB Host initialization.
 */
void Lesson_Init(void);

/**
 * Reset the lesson to the beginning.
 * Turns off all LEDs, resets to the first lesson step, and lights the LED for the first note.
 */
void Lesson_Reset(void);

/**
 * Handle a NOTE ON event from the MIDI keyboard.
 * @param note MIDI note number of the Note On event.
 * @param vel  Velocity of the Note On (not used for logic, but can be logged or extended for dynamics).
 *
 * This function checks if the given note matches the current expected lesson note.
 * If correct, it advances the lesson (turning off the current LED and moving to the next note/LED, or finishing the lesson if it was the last note).
 * If wrong, it triggers an error indication (flashes the error LED) and the lesson stays on the same note.
 */
void Lesson_OnNoteOn(uint8_t note, uint8_t vel);

/**
 * Check if the lesson has been successfully finished.
 * @return true if all notes in the lesson have been played correctly, false otherwise.
 */
bool Lesson_IsFinished(void);

#endif // LESSON_H
