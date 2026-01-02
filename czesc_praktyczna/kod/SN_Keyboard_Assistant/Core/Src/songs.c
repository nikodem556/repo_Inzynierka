#include "songs.h"

/*
 * songs.c
 *
 * This module provides a small registry of built-in songs for the lesson mode.
 * Each song is defined as an array of SongStep items. A step can require up to
 * 3 notes (NoteEntry), and each note carries an LCD icon index describing the
 * note duration (whole/half/quarter/etc.).
 *
 * NOTE: This file contains only constant data definitions (no runtime logic).
 */

/* Note length icon indices (match LCD CGRAM slots configured in main.c) */
#define LEN_WHOLE      0
#define LEN_HALF       1
#define LEN_QUARTER    2
#define LEN_EIGHTH     3
#define LEN_SIXTEENTH  4

/* "Twinkle Twinkle Little Star" (first phrase) */
static SongStep twinkleSteps[] = {
    { 2, { { 'C', ACC_NONE, 60, LEN_QUARTER }, { 'C', ACC_NONE, 60, LEN_QUARTER } } },
    { 2, { { 'G', ACC_NONE, 67, LEN_QUARTER }, { 'G', ACC_NONE, 67, LEN_QUARTER } } },
    { 2, { { 'A', ACC_NONE, 69, LEN_QUARTER }, { 'A', ACC_NONE, 69, LEN_QUARTER } } },
    { 1, { { 'G', ACC_NONE, 67, LEN_HALF    } } }
};

/* "Mary Had a Little Lamb" (first phrases) */
static SongStep marySteps[] = {
    { 2, { { 'E', ACC_NONE, 64, LEN_QUARTER }, { 'D', ACC_NONE, 62, LEN_QUARTER } } },
    { 2, { { 'C', ACC_NONE, 60, LEN_QUARTER }, { 'D', ACC_NONE, 62, LEN_QUARTER } } },
    { 3, { { 'E', ACC_NONE, 64, LEN_QUARTER }, { 'E', ACC_NONE, 64, LEN_QUARTER }, { 'E', ACC_NONE, 64, LEN_HALF } } }
};

/* "Chromatic Study" – longer exercise with sharps and flats */
static SongStep chromaticSteps[] = {
    /* C – D */
    { 2, { { 'C', ACC_NONE, 60, LEN_QUARTER },
           { 'D', ACC_NONE, 62, LEN_QUARTER } } },

    /* E – F */
    { 2, { { 'E', ACC_NONE, 64, LEN_QUARTER },
           { 'F', ACC_NONE, 65, LEN_QUARTER } } },

    /* F# – G – A */
    { 3, { { 'F', ACC_SHARP, 66, LEN_EIGHTH },
           { 'G', ACC_NONE,  67, LEN_EIGHTH },
           { 'A', ACC_NONE,  69, LEN_QUARTER } } },

    /* Bb – A */
    { 2, { { 'B', ACC_FLAT, 70, LEN_QUARTER },
           { 'A', ACC_NONE, 69, LEN_QUARTER } } },

    /* G – F# */
    { 2, { { 'G', ACC_NONE,  67, LEN_QUARTER },
           { 'F', ACC_SHARP, 66, LEN_QUARTER } } },

    /* Step with both SHARP and FLAT */
    { 2, { { 'F', ACC_SHARP, 66, LEN_QUARTER },
           { 'B', ACC_FLAT,  70, LEN_QUARTER } } },

    /* Final C */
    { 1, { { 'C', ACC_NONE, 72, LEN_HALF } } }
};

/*
 * Exported songs registry.
 * IMPORTANT: direct initialization is a constant initializer.
 */
Song songs[] = {
    { "Twinkle Twinkle",
      (uint8_t)(sizeof(twinkleSteps) / sizeof(twinkleSteps[0])),
      twinkleSteps },

    { "Mary Had a Lamb",
      (uint8_t)(sizeof(marySteps) / sizeof(marySteps[0])),
      marySteps },

    { "Chroma Study",
          (uint8_t)(sizeof(chromaticSteps) / sizeof(chromaticSteps[0])),
          chromaticSteps }

};

const uint8_t SONG_COUNT = (uint8_t)(sizeof(songs) / sizeof(songs[0]));
