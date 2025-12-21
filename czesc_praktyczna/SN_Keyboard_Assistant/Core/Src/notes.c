#include "notes.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>

/// Static helper: Map an uppercase note root string to a semitone number (0=C, 1=C#/Db, ..., 11=B).
static int8_t NoteName_MapRootToSemitone(const char *root)
{
    if (root == NULL || *root == '\0') {
        return -1;
    }

    // --- New notation (preferred): English with # and b ---
    // IMPORTANT:
    //  - "B" means B natural (semitone 11)
    //  - Flats are written as "Bb", "Db", etc.
    //  - Input is already uppercased in NoteName_ToMidi(), so 'b' becomes 'B'
    if (strcmp(root, "C") == 0) return 0;
    if (strcmp(root, "D") == 0) return 2;
    if (strcmp(root, "E") == 0) return 4;
    if (strcmp(root, "F") == 0) return 5;
    if (strcmp(root, "G") == 0) return 7;
    if (strcmp(root, "A") == 0) return 9;
    if (strcmp(root, "B") == 0) return 11;  // B natural

    // Keep compatibility with older Polish/German notation:
    if (strcmp(root, "H") == 0) return 11;  // H = B natural

    // Two-character accidentals: "C#", "DB", "BB" (Bb), etc.
    if (strlen(root) == 2)
    {
        char base = root[0];
        char acc  = root[1];

        int8_t baseSemi = -1;
        switch (base)
        {
            case 'C': baseSemi = 0;  break;
            case 'D': baseSemi = 2;  break;
            case 'E': baseSemi = 4;  break;
            case 'F': baseSemi = 5;  break;
            case 'G': baseSemi = 7;  break;
            case 'A': baseSemi = 9;  break;
            case 'B': baseSemi = 11; break;  // base B
            case 'H': baseSemi = 11; break;  // base H (compat)
            default:  baseSemi = -1; break;
        }
        if (baseSemi < 0) return -1;

        if (acc == '#') {
            return (int8_t)((baseSemi + 1) % 12);
        }

        // Flat: user typed 'b' -> becomes 'B' after toupper()
        if (acc == 'B') {
            return (int8_t)((baseSemi + 11) % 12); // -1 mod 12
        }
    }

    // --- Legacy compatibility: CIS/DES etc. ---
    if (strcmp(root, "CIS") == 0 || strcmp(root, "DES") == 0)  return 1;
    if (strcmp(root, "DIS") == 0 || strcmp(root, "ES")  == 0)  return 3;
    if (strcmp(root, "FIS") == 0 || strcmp(root, "GES") == 0)  return 6;
    if (strcmp(root, "GIS") == 0 || strcmp(root, "AS")  == 0)  return 8;

    // Old "AIS" and also "BES/HES" are Bb (semitone 10)
    if (strcmp(root, "AIS") == 0 || strcmp(root, "BES") == 0 || strcmp(root, "HES") == 0) return 10;

    return -1;
}



NoteParseStatus NoteName_ToMidi(const char *name, uint8_t *outNote) {
    if (name == NULL || outNote == NULL) {
        return NOTE_ERR_INVALID_FORMAT;
    }

    // Make an uppercase copy of the input string (to handle case-insensitivity)
    char temp[8];
    size_t len = strlen(name);
    if (len < 2 || len >= sizeof(temp)) {
        // Need at least 2 characters (note + octave), and not exceed buffer
        return NOTE_ERR_INVALID_FORMAT;
    }
    for (size_t i = 0; i < len; ++i) {
        temp[i] = toupper((unsigned char)name[i]);
    }
    temp[len] = '\0';

    // The last character should be the octave digit (0–9)
    char octaveChar = temp[len - 1];
    if (octaveChar < '0' || octaveChar > '9') {
        return NOTE_ERR_INVALID_FORMAT;
    }
    int octave = octaveChar - '0';

    // Everything before the last char is the note root (e.g. "C", "CIS", "DES", "HES", etc.)
    temp[len - 1] = '\0';  // isolate the root part
    const char *root = temp;
    if (*root == '\0') {
        // No note root present (string was just a digit or empty)
        return NOTE_ERR_INVALID_FORMAT;
    }

    int8_t semitone = NoteName_MapRootToSemitone(root);
    if (semitone < 0) {
        return NOTE_ERR_INVALID_NOTE;  // root not recognized
    }

    // Compute MIDI note number: MIDI = 12 * (octave + 1) + semitone
    // (C4 => octave=4, semitone=0, yields 12*(4+1)+0 = 60)
    int midi = 12 * (octave + 1) + semitone;
    if (midi < 0 || midi > 127) {
        return NOTE_ERR_INVALID_OCTAVE;  // out of MIDI range (e.g., "A9" would be >127)
    }

    *outNote = (uint8_t)midi;
    return NOTE_OK;
}

NoteParseStatus NoteNameArray_ToMidi(const char *names[], size_t count, uint8_t outNotes[]) {
    if (names == NULL || outNotes == NULL) {
        return NOTE_ERR_INVALID_FORMAT;
    }
    for (size_t i = 0; i < count; ++i) {
        NoteParseStatus status = NoteName_ToMidi(names[i], &outNotes[i]);
        if (status != NOTE_OK) {
            return status;  // return the first error encountered
        }
    }
    return NOTE_OK;
}

NoteParseStatus Midi_ToNoteName(uint8_t midiNote, char *outName, size_t maxLen) {
    if (outName == NULL || maxLen == 0) {
        return NOTE_ERR_INVALID_FORMAT;
    }
    if (midiNote > 127) {
        return NOTE_ERR_INVALID_NOTE;  // MIDI note out of range
    }

    // Determine octave and semitone from MIDI number
    int semitone = midiNote % 12;
    int octave   = midiNote / 12 - 1;

    static const char *semitoneNames[12] = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };

    // Compose the note name string
    // (Ensure the buffer is large enough for up to 3 chars of name + possible '-' + octave digit + null)
    char buf[8];
    const char *noteName = semitoneNames[semitone];
    if (octave >= 0) {
        // Format like "C4"
        snprintf(buf, sizeof(buf), "%s%d", noteName, octave);
    } else {
        // Negative octave (e.g., MIDI 0 = C-1)
        snprintf(buf, sizeof(buf), "%s%d", noteName, octave);
    }
    // Check length against maxLen (including null terminator)
    size_t outLen = strlen(buf) + 1;
    if (outLen > maxLen) {
        return NOTE_ERR_INVALID_FORMAT;  // buffer too small to hold the output
    }
    strcpy(outName, buf);
    return NOTE_OK;
}
