#ifndef CHORDS_H
#define CHORDS_H

#include <stdint.h>
#include "songs.h"  // reuse NoteEntry and Accidental definitions

// Structure for a chord (as a lesson step in chords exercise)
typedef struct {
    const char *name;     // Chord name (e.g., "Am", "F#", etc.)
    uint8_t noteCount;
    NoteEntry notes[3];
} Chord;

// Chord pack definition (a collection of chords under a category)
typedef struct {
    const char *packName;
    uint8_t chordCount;
    Chord *chords;
} ChordPack;

// Expose chord packs list and count
extern ChordPack chordPacks[];
extern const uint8_t CHORD_PACK_COUNT;

#endif // CHORDS_H
