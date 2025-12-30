#ifndef CHORDS_H
#define CHORDS_H

#include <stdint.h>
#include "songs.h"  /* Reuse NoteEntry and Accidental definitions */

/*
 * chords.h / chords.c
 *
 * This module defines chord data used by the chord exercise mode.
 * Chords are grouped into packs (categories) that can be selected in the UI.
 *
 * NOTE:
 * - Chord notes reuse NoteEntry from songs.h (letter + accidental + midiNote placeholder).
 * - In chord mode, lesson.c matches played notes by pitch class (note % 12),
 *   so the octave is not important.
 */

/* Single chord definition (one exercise step) */
typedef struct {
    const char *name;     /* Chord name (e.g., "Am", "F#", "Bb") */
    uint8_t noteCount;    /* Number of valid tones in notes[] */
    NoteEntry notes[3];   /* Chord tones (up to 3) */
} Chord;

/* Chord pack (collection of chords) */
typedef struct {
    const char *packName; /* Pack/category name shown in the UI */
    uint8_t chordCount;   /* Number of chords in the pack */
    Chord *chords;        /* Pointer to chord array */
} ChordPack;

/* Expose chord packs list and count */
extern ChordPack chordPacks[];
extern const uint8_t CHORD_PACK_COUNT;

#endif /* CHORDS_H */
