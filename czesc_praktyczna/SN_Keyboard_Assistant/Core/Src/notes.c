#include "notes.h"
#include <string.h>
#include <ctype.h>

/// Static helper: Map an uppercase note root string to a semitone number (0=C, 1=C♯/D♭, ..., 11=B/H).
static int8_t NoteName_MapRootToSemitone(const char *root) {
    // Note names mapping (German/English combined notation)
    if (strcmp(root, "C") == 0)   return 0;
    if (strcmp(root, "CIS") == 0 || strcmp(root, "DES") == 0)  return 1;   // C♯ or D♭
    if (strcmp(root, "D") == 0)   return 2;
    if (strcmp(root, "DIS") == 0 || strcmp(root, "ES") == 0)   return 3;   // D♯ or E♭ (Es = E♭)
    if (strcmp(root, "E") == 0)   return 4;
    if (strcmp(root, "F") == 0)   return 5;
    if (strcmp(root, "FIS") == 0 || strcmp(root, "GES") == 0)  return 6;   // F♯ or G♭
    if (strcmp(root, "G") == 0)   return 7;
    if (strcmp(root, "GIS") == 0 || strcmp(root, "AS") == 0)   return 8;   // G♯ or A♭ (As = A♭)
    if (strcmp(root, "A") == 0)   return 9;
    if (strcmp(root, "AIS") == 0) return 10;  // A♯ (B♭ in English)
    if (strcmp(root, "B") == 0 || strcmp(root, "BES") == 0 || strcmp(root, "HES") == 0)
        return 10;  // In German/Polish notation, "B" or "Bes"/"Hes" = B♭ (semitone 10)
    if (strcmp(root, "H") == 0)   return 11;  // H = B natural
    return -1; // not a valid note name
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

    // Note names for semitones 0–11 (using sharps and H for B natural)
    static const char *semitoneNames[12] = {
        "C", "CIS", "D", "DIS", "E", "F", "FIS", "G", "GIS", "A", "AIS", "H"
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
