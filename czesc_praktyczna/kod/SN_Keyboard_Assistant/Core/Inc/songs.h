#ifndef SONGS_H
#define SONGS_H

#include <stdint.h>

/*
 * songs.h
 *
 * This header defines the data structures used to describe built-in songs.
 * A song consists of multiple steps (SongStep). Each step holds up to 3 notes.
 *
 * The lesson engine (lesson.c) uses these structures to display required notes
 * and verify user input from a MIDI keyboard.
 */

/* Accidental indicator for notes */
typedef enum { ACC_NONE = 0, ACC_SHARP, ACC_FLAT } Accidental;

/* Structure describing a single note within a step */
typedef struct {
    char letter;            /* Note letter A..G */
    Accidental accidental;  /* ACC_NONE / ACC_SHARP / ACC_FLAT */
    int8_t midiNote;        /* MIDI note number (0..127), or -1 if not applicable */
    uint8_t lengthIcon;     /* LCD custom char slot for note length icon (0..4) */
} NoteEntry;

/* A step in a song lesson (up to 3 notes, each with a duration) */
typedef struct {
    uint8_t noteCount;      /* Number of valid entries in notes[] */
    NoteEntry notes[3];     /* Required notes for this step */
} SongStep;

/* Song definition (title + step list) */
typedef struct {
    const char *title;      /* Song title shown in the UI */
    uint8_t stepCount;      /* Number of steps in the song */
    SongStep *steps;        /* Pointer to the step array */
} Song;

/* Expose the song list and count */
extern Song songs[];
extern const uint8_t SONG_COUNT;

#endif /* SONGS_H */
