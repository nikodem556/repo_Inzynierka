#include "chords.h"

/*
 * chords.c
 *
 * This file contains only constant data definitions:
 * - Two chord packs: basic and advanced
 * - Exported registry chordPacks[] and CHORD_PACK_COUNT
 *
 * Each chord is defined as up to 3 tones. The tones use NoteEntry:
 *   - letter + accidental define the pitch class
 *   - midiNote is not used in chord mode (kept as a placeholder value)
 *   - lengthIcon is not used in chord mode (set to 0)
 */

/* Placeholder used for NoteEntry.midiNote in chord mode (not used by matching logic). */
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
 * Exported chord packs registry.
 * Each entry points to a static chord array defined above.
 */
ChordPack chordPacks[] = {
    { "Basic chords",
      (uint8_t)(sizeof(basicChords) / sizeof(basicChords[0])),
      basicChords },

    { "Advanced chords",
      (uint8_t)(sizeof(advancedChords) / sizeof(advancedChords[0])),
      advancedChords }
};

/* Total number of chord packs available in the application. */
const uint8_t CHORD_PACK_COUNT = (uint8_t)(sizeof(chordPacks) / sizeof(chordPacks[0]));
