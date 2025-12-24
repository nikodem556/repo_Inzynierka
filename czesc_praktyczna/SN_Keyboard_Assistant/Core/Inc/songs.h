#ifndef SONGS_H
#define SONGS_H

#include <stdint.h>

// Accidental indicator for notes
typedef enum { ACC_NONE = 0, ACC_SHARP, ACC_FLAT } Accidental;

// Structure for a single note (within a step or chord)
typedef struct {
    char letter;         // Note letter A-G
    Accidental accidental; // Accidental: sharp or flat (or none)
    int8_t midiNote;     // MIDI note number (0-127), or -1 if not applicable
    uint8_t lengthIcon;  // LCD custom char slot for note length icon (0-4 for whole, half, quarter, etc.)
} NoteEntry;

// A step in a song lesson (up to 3 notes, each with a duration)
typedef struct {
    uint8_t noteCount;
    NoteEntry notes[3];
} SongStep;

// Song definition
typedef struct {
    const char *title;    // Song title
    uint8_t stepCount;
    SongStep *steps;
} Song;

// Expose the song list and count
extern Song songs[];
extern const uint8_t SONG_COUNT;

#endif // SONGS_H
