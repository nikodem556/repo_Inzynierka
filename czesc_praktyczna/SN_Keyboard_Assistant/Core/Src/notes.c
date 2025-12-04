/*
 * notes.c
 *
 *  Created on: 5 gru 2025
 *      Author: nikod
 */

#include "notes.h"
#include <string.h>
#include <ctype.h>

/**
 * @brief  Convert string to UPPERCASE in-place.
 *         Note: used to make note names case-insensitive.
 */
static void str_to_upper(char *s)
{
    while (*s)
    {
        *s = (char)toupper((unsigned char)*s);
        s++;
    }
}

/**
 * @brief  Map note root string (without octave) to semitone (0..11).
 *
 * Supported roots (case-insensitive before calling this function):
 *   C, CIS, DES,
 *   D, DIS, ES,
 *   E,
 *   F, FIS, GES,
 *   G, GIS, AS,
 *   A, AIS,
 *   B, BES,       // B (in PL) is B-flat
 *   H, HES        // H is B-natural
 *
 * You can extend this function if you want more aliases (e.g. "C#" -> CIS).
 *
 * @param  root  Note root string in uppercase.
 * @retval 0..11 for valid roots, or -1 if unknown.
 */
static int8_t note_root_to_semitone(const char *root)
{
    if (strcmp(root, "C") == 0)      return 0;
    if (strcmp(root, "CIS") == 0)    return 1;
    if (strcmp(root, "DES") == 0)    return 1;

    if (strcmp(root, "D") == 0)      return 2;
    if (strcmp(root, "DIS") == 0)    return 3;
    if (strcmp(root, "ES") == 0)     return 3;  // E-flat

    if (strcmp(root, "E") == 0)      return 4;

    if (strcmp(root, "F") == 0)      return 5;
    if (strcmp(root, "FIS") == 0)    return 6;
    if (strcmp(root, "GES") == 0)    return 6;

    if (strcmp(root, "G") == 0)      return 7;
    if (strcmp(root, "GIS") == 0)    return 8;
    if (strcmp(root, "AS") == 0)     return 8;  // A-flat

    if (strcmp(root, "A") == 0)      return 9;
    if (strcmp(root, "AIS") == 0)    return 10;
    if (strcmp(root, "B") == 0)      return 10; // B (PL) = Bb
    if (strcmp(root, "BES") == 0)    return 10;

    if (strcmp(root, "H") == 0)      return 11; // H = B natural
    if (strcmp(root, "HES") == 0)    return 10; // HES = Bb

    return -1; // unknown root
}

/**
 * @brief  Convert note name string to MIDI note number.
 *
 * Example valid inputs (case-insensitive):
 *   "C4", "c4", "Cis4", "cis4", "H3", "b3", "Bes3", "FIS5"
 */
NoteParseStatus NoteName_ToMidi(const char *name, uint8_t *outNote)
{
    if (name == NULL || outNote == NULL)
        return NOTE_ERR_INVALID_FORMAT;

    size_t len = strlen(name);
    if (len < 2 || len > 5)
        return NOTE_ERR_INVALID_FORMAT;

    // Copy to local buffer to safely modify (uppercase, etc.)
    char buf[8];
    if (len >= sizeof(buf))
        return NOTE_ERR_INVALID_FORMAT;

    strcpy(buf, name);
    str_to_upper(buf);

    // Last character must be an octave digit (0..9)
    char last = buf[len - 1];
    if (!isdigit((unsigned char)last))
        return NOTE_ERR_INVALID_OCTAVE;

    int octave = last - '0';   // assume octave 0..9
    if (octave < 0 || octave > 9)
        return NOTE_ERR_INVALID_OCTAVE;

    // Root is everything except the last character (octave)
    buf[len - 1] = '\0';
    const char *root = buf;

    int8_t semitone = note_root_to_semitone(root);
    if (semitone < 0)
        return NOTE_ERR_INVALID_NOTE;

    // Standard MIDI: C4 = 60, A4 = 69
    // Formula: midi = 12 * (octave + 1) + semitone
    int midi = 12 * (octave + 1) + semitone;
    if (midi < 0 || midi > 127)
        return NOTE_ERR_INVALID_NOTE;

    *outNote = (uint8_t)midi;
    return NOTE_OK;
}

/**
 * @brief  Convert array of note name strings to MIDI note numbers.
 */
NoteParseStatus NoteNameArray_ToMidi(const char *names[], size_t count, uint8_t outNotes[])
{
    if (names == NULL || outNotes == NULL)
        return NOTE_ERR_INVALID_FORMAT;

    for (size_t i = 0; i < count; i++)
    {
        NoteParseStatus st = NoteName_ToMidi(names[i], &outNotes[i]);
        if (st != NOTE_OK)
        {
            // You can add logging here for the failing index if needed.
            return st;
        }
    }

    return NOTE_OK;
}

/**
 * @brief  Convert MIDI note number back to note name string (e.g. 60 -> "C4").
 *
 * Uses the same convention: C4 = 60.
 * Root names use sharps in "IS" style (CIS, DIS, FIS, GIS, AIS, H).
 */
NoteParseStatus Midi_ToNoteName(uint8_t midiNote, char *outName, size_t maxLen)
{
    if (outName == NULL || maxLen < 3) // minimal "C4\0"
        return NOTE_ERR_INVALID_FORMAT;

    int note = midiNote;
    int octave = note / 12 - 1;   // inverse of: midi = 12*(octave+1) + semitone
    int semitone = note % 12;

    const char *root = NULL;

    switch (semitone)
    {
        case 0:  root = "C";   break;
        case 1:  root = "CIS"; break;
        case 2:  root = "D";   break;
        case 3:  root = "DIS"; break;
        case 4:  root = "E";   break;
        case 5:  root = "F";   break;
        case 6:  root = "FIS"; break;
        case 7:  root = "G";   break;
        case 8:  root = "GIS"; break;
        case 9:  root = "A";   break;
        case 10: root = "AIS"; break;
        case 11: root = "H";   break; // H = B natural
        default: return NOTE_ERR_INVALID_NOTE;
    }

    // root + octave-digit + '\0'
    size_t rootLen = strlen(root);
    if (rootLen + 2 > maxLen)  // root + 'd' + '\0'
        return NOTE_ERR_INVALID_FORMAT;

    memcpy(outName, root, rootLen);
    outName[rootLen] = (char)('0' + octave);
    outName[rootLen + 1] = '\0';

    return NOTE_OK;
}

