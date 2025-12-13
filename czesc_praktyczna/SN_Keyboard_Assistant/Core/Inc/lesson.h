#ifndef LESSON_H
#define LESSON_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>


/** Maximum number of notes in a chord (lesson step). */
#define MAX_CHORD_NOTES 3

/**
 * @brief Initialize the lesson system and load the song data.
 *        Should be called once at startup.
 */
void Lesson_Init(void);

/**
 * @brief Reset the lesson state to the beginning of the song.
 *        Can be called when the reset button is pressed.
 */
void Lesson_Reset(void);

/**
 * @brief Handle a MIDI Note On event in the lesson.
 * @param midiNote  MIDI note number (0-127) of the Note On event.
 */
void Lesson_OnNoteOn(uint8_t midiNote);

/**
 * @brief Periodic update for the lesson system.
 *        Should be called in the main loop to handle tasks like LED blink timing.
 */
void Lesson_Tick(void);

#endif // LESSON_H
