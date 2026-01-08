#ifndef NOTES_H
#define NOTES_H

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>


/**
 * @brief Status code returned by note name parser.
 */
typedef enum
{
    NOTE_OK = 0,
    NOTE_ERR_INVALID_FORMAT,
    NOTE_ERR_INVALID_NOTE,
    NOTE_ERR_INVALID_OCTAVE
} NoteParseStatus;

/**
 * @brief  Convert a single note name (e.g. "C4", "CIS4", "H3") to MIDI note number.
 *
 * Format:
 *   - Note names are CASE-INSENSITIVE ("C4", "c4", "Cis4" all work).
 *   - Supported roots include:
 *       C, CIS, DES, D, DIS, ES, E, F, FIS, GES, G, GIS, AS,
 *       A, AIS, B, BES, H, HES
 *	 - Naming convention used in this project: "B" means B natural; Bb can be written as "Bb" or "BES"/"HES".
 *   - Last character is the octave digit (0..9), e.g. "C4".
 *
 * MIDI convention used:
 *   - C4 = 60, A4 = 69 (standard MIDI note numbers).
 *
 * @param  name      Null-terminated string with note name, e.g. "C4".
 * @param  outNote   Pointer where resulting MIDI note (0..127) will be stored.
 * @retval NOTE_OK on success, otherwise error code.
 */
NoteParseStatus NoteName_ToMidi(const char *name, uint8_t *outNote);

/**
 * @brief  Convert an array of note names to an array of MIDI note numbers.
 *
 * @param  names      Array of pointers to note name strings.
 * @param  count      Number of elements in the names array.
 * @param  outNotes   Output array for MIDI note numbers (must have at least 'count' elements).
 * @retval NOTE_OK if ALL notes were parsed successfully,
 *         otherwise the first error code encountered.
 */
NoteParseStatus NoteNameArray_ToMidi(const char *names[], size_t count, uint8_t outNotes[]);

/**
 * @brief  Convert MIDI note number back to note name string (e.g. 60 -> "C4").
 *
 * @param  midiNote   MIDI note number (0..127).
 * @param  outName    Output buffer for note name (null-terminated).
 * @param  maxLen     Maximum length of outName buffer in bytes.
 *                    Must be at least 3 ("C4\0") and preferably >= 6 ("CIS4\0").
 *
 * @retval NOTE_OK on success, otherwise error code.
 */
NoteParseStatus Midi_ToNoteName(uint8_t midiNote, char *outName, size_t maxLen);

#endif /* NOTES_H */
