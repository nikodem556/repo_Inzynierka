#include "chords.h"
#include <stdint.h>

/* If midiNote field exists but is not used in chord mode, keep a placeholder. */
#define NO_MIDI_NOTE   ((int16_t)-1)

/* Basic chords pack – common chords (C, G, Am, F, Dm, Em) */
static Chord basicChords[] = {
    { "C",  3, { {'C', ACC_NONE,  NO_MIDI_NOTE, 0}, {'E', ACC_NONE,  NO_MIDI_NOTE, 0}, {'G', ACC_NONE,  NO_MIDI_NOTE, 0} } },
    { "G",  3, { {'G', ACC_NONE,  NO_MIDI_NOTE, 0}, {'B', ACC_NONE,  NO_MIDI_NOTE, 0}, {'D', ACC_NONE,  NO_MIDI_NOTE, 0} } },
    { "Am", 3, { {'A', ACC_NONE,  NO_MIDI_NOTE, 0}, {'C', ACC_NONE,  NO_MIDI_NOTE, 0}, {'E', ACC_NONE,  NO_MIDI_NOTE, 0} } },
    { "F",  3, { {'F', ACC_NONE,  NO_MIDI_NOTE, 0}, {'A', ACC_NONE,  NO_MIDI_NOTE, 0}, {'C', ACC_NONE,  NO_MIDI_NOTE, 0} } },
    { "Dm", 3, { {'D', ACC_NONE,  NO_MIDI_NOTE, 0}, {'F', ACC_NONE,  NO_MIDI_NOTE, 0}, {'A', ACC_NONE,  NO_MIDI_NOTE, 0} } },
    { "Em", 3, { {'E', ACC_NONE,  NO_MIDI_NOTE, 0}, {'G', ACC_NONE,  NO_MIDI_NOTE, 0}, {'B', ACC_NONE,  NO_MIDI_NOTE, 0} } }
};

/* Advanced chords pack – chords with sharps/flats */
static Chord advancedChords[] = {
    { "F#",  3, { {'F', ACC_SHARP, NO_MIDI_NOTE, 0}, {'A', ACC_SHARP, NO_MIDI_NOTE, 0}, {'C', ACC_SHARP, NO_MIDI_NOTE, 0} } },
    { "Bb",  3, { {'B', ACC_FLAT,  NO_MIDI_NOTE, 0}, {'D', ACC_NONE,  NO_MIDI_NOTE, 0}, {'F', ACC_NONE,  NO_MIDI_NOTE, 0} } },
    { "Gm",  3, { {'G', ACC_NONE,  NO_MIDI_NOTE, 0}, {'B', ACC_FLAT,  NO_MIDI_NOTE, 0}, {'D', ACC_NONE,  NO_MIDI_NOTE, 0} } },
    { "Ab",  3, { {'A', ACC_FLAT,  NO_MIDI_NOTE, 0}, {'C', ACC_NONE,  NO_MIDI_NOTE, 0}, {'E', ACC_FLAT,  NO_MIDI_NOTE, 0} } },
    { "C#m", 3, { {'C', ACC_SHARP, NO_MIDI_NOTE, 0}, {'E', ACC_NONE,  NO_MIDI_NOTE, 0}, {'G', ACC_SHARP, NO_MIDI_NOTE, 0} } },
    { "E",   3, { {'E', ACC_NONE,  NO_MIDI_NOTE, 0}, {'G', ACC_SHARP, NO_MIDI_NOTE, 0}, {'B', ACC_NONE,  NO_MIDI_NOTE, 0} } }
};

/*
 * Exported chord packs.
 * IMPORTANT: direct initialization with pointers to static arrays is a constant initializer.
 */
ChordPack chordPacks[] = {
    { "Basic chords",
      (uint8_t)(sizeof(basicChords) / sizeof(basicChords[0])),
      basicChords },

    { "Advanced chords",
      (uint8_t)(sizeof(advancedChords) / sizeof(advancedChords[0])),
      advancedChords }
};

const uint8_t CHORD_PACK_COUNT = (uint8_t)(sizeof(chordPacks) / sizeof(chordPacks[0]));
