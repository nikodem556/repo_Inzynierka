#include "notes.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>

/**
 * @file notes.c
 * @brief Implementation of note name parsing and MIDI note formatting.
 *
 * Supported input conventions (case-insensitive):
 * - Single-letter roots: C, D, E, F, G, A, B (in this project: B = B natural)
 * - 'H' is also accepted as an alternative name for B natural (common in Polish/German sources);
 * - note that this project still treats "B" as B natural.
 * - Accidentals: '#' for sharp, 'b' for flat (e.g. C#4, Bb3)
 * - Legacy names: CIS/DES, DIS/ES, FIS/GES, GIS/AS, AIS/BES/HES
 *
 * Octave is given as the last character (0..9). MIDI convention: C4 = 60.
 */


/**
 * @brief Map an uppercase note root string to a semitone number.
 *
 * @param root Uppercase note root without octave (e.g. "C", "CIS", "DB", "F#").
 * @return Semitone number in range 0..11 on success, or -1 on failure.
 */
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

/**
 * @brief Parse a note name string and convert it to a MIDI note number.
 *
 * The input is case-insensitive. The last character must be an octave digit (0..9).
 * Returns a status code describing the first error encountered.
 */
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

    // Everything before the last char is the note root
    temp[len - 1] = '\0';  // isolate the root part
    const char *root = temp;
    if (*root == '\0') {
        return NOTE_ERR_INVALID_FORMAT;
    }

    int8_t semitone = NoteName_MapRootToSemitone(root);
    if (semitone < 0) {
        return NOTE_ERR_INVALID_NOTE;
    }

    // Compute MIDI note number: MIDI = 12 * (octave + 1) + semitone
    int midi = 12 * (octave + 1) + semitone;
    if (midi < 0 || midi > 127) {
        return NOTE_ERR_INVALID_OCTAVE;
    }

    *outNote = (uint8_t)midi;
    return NOTE_OK;
}

/**
 * @brief Convert an array of note name strings into MIDI note numbers.
 *
 * Stops and returns immediately on the first parsing error.
 */
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

/**
 * @brief Convert a MIDI note number (0..127) into a textual note name (e.g. 60 -> "C4").
 *
 * Output uses English names with sharps: C, C#, D, ... , B.
 */
NoteParseStatus Midi_ToNoteName(uint8_t midiNote, char *outName, size_t maxLen) {
    if (outName == NULL || maxLen == 0) {
        return NOTE_ERR_INVALID_FORMAT;
    }
    if (midiNote > 127) {
        return NOTE_ERR_INVALID_NOTE;
    }

    // Determine octave and semitone from MIDI number
    int semitone = midiNote % 12;
    int octave   = midiNote / 12 - 1;

    static const char *semitoneNames[12] = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };

    // Compose the note name string
    char buf[8];
    const char *noteName = semitoneNames[semitone];
    snprintf(buf, sizeof(buf), "%s%d", noteName, octave);

    // Check length against maxLen (including null terminator)
    size_t outLen = strlen(buf) + 1;
    if (outLen > maxLen) {
        return NOTE_ERR_INVALID_FORMAT;
    }
    strcpy(outName, buf);
    return NOTE_OK;
}
