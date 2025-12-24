#include "lesson.h"
#include "grove_lcd16x2_i2c.h"
#include <stdint.h>
#include <stdbool.h>

/* LCD instance lives in main.c */
extern GroveLCD_t lcd;

/* Internal state variables */
static Song *currentSong = NULL;
static ChordPack *currentChordPack = NULL;
static uint8_t currentStepIndex = 0;
static uint8_t totalSteps = 0;
static bool lessonActive = false;

/* Forward declarations */
static void DisplaySongStep(const SongStep *step);
static void DisplayChordStep(const Chord *chord);

static void LCD_ClearRow(uint8_t row)
{
    GroveLCD_SetCursor(&lcd, row, 0);
    GroveLCD_Print(&lcd, "                ");
}

static void LCD_WriteCustom(uint8_t slot)
{
    GroveLCD_WriteChar(&lcd, (char)slot);
}

static char MidiToOctaveChar(int8_t midi)
{
    if (midi < 0) return '?';
    int octave = (midi / 12) - 1; /* MIDI convention: C4=60 */
    if (octave < 0 || octave > 9) return '?';
    return (char)('0' + octave);
}

/* Start song lesson */
void Lesson_StartSong(Song *song)
{
    currentSong = song;
    currentChordPack = NULL;
    currentStepIndex = 0;
    totalSteps = (song != NULL) ? song->stepCount : 0;
    lessonActive = (song != NULL && totalSteps > 0);

    if (lessonActive) {
        DisplaySongStep(&song->steps[0]);
    }
}

/* Start chord exercise */
void Lesson_StartChordExercise(ChordPack *pack)
{
    currentChordPack = pack;
    currentSong = NULL;
    currentStepIndex = 0;
    totalSteps = (pack != NULL) ? pack->chordCount : 0;
    lessonActive = (pack != NULL && totalSteps > 0);

    if (lessonActive) {
        DisplayChordStep(&pack->chords[0]);
    }
}

bool Lesson_IsActive(void)
{
    return lessonActive;
}

void Lesson_HandleInput(uint8_t input)
{
    if (!lessonActive) return;

    /* MIDI note input (0..127) */
    if (input <= 0x7F)
    {
        if (currentSong != NULL)
        {
            SongStep *step = &currentSong->steps[currentStepIndex];

            /* Auto-advance only for single-note steps */
            if (step->noteCount == 1)
            {
                uint8_t expected = (uint8_t)step->notes[0].midiNote;
                if (expected == input)
                {
                    if (currentStepIndex < (totalSteps - 1)) {
                        currentStepIndex++;
                        DisplaySongStep(&currentSong->steps[currentStepIndex]);
                    } else {
                        lessonActive = false;
                    }
                }
            }
        }
        /* Chord mode: ignore MIDI here (user presses OK to advance) */
        return;
    }

    /* Button commands */
    if (input == LESSON_INPUT_BTN_OK)
    {
        if (currentStepIndex < (totalSteps - 1))
        {
            currentStepIndex++;
            if (currentSong) DisplaySongStep(&currentSong->steps[currentStepIndex]);
            else DisplayChordStep(&currentChordPack->chords[currentStepIndex]);
        }
        else {
            lessonActive = false;
        }
    }
    else if (input == LESSON_INPUT_BTN_NEXT)
    {
        if (currentStepIndex > 0)
        {
            currentStepIndex--;
            if (currentSong) DisplaySongStep(&currentSong->steps[currentStepIndex]);
            else DisplayChordStep(&currentChordPack->chords[currentStepIndex]);
        }
        else {
            /* At step 0 -> exit lesson to list */
            lessonActive = false;
        }
    }
    else if (input == LESSON_INPUT_BTN_RESET)
    {
        if (currentStepIndex != 0)
        {
            currentStepIndex = 0;
            if (currentSong) DisplaySongStep(&currentSong->steps[0]);
            else DisplayChordStep(&currentChordPack->chords[0]);
        }
        else {
            /* At step 0 -> exit lesson to list */
            lessonActive = false;
        }
    }
}

/* --- LCD rendering --- */

static void DisplaySongStep(const SongStep *step)
{
    GroveLCD_Clear(&lcd);
    LCD_ClearRow(0);
    LCD_ClearRow(1);

    uint8_t col = 0;
    uint8_t startCol[3] = {0};

    /* Row 0: print notes as e.g. C#4, Db4, E4 */
    for (uint8_t i = 0; i < step->noteCount; i++)
    {
        startCol[i] = col;

        /* Letter */
        GroveLCD_SetCursor(&lcd, 0, col);
        GroveLCD_WriteChar(&lcd, step->notes[i].letter);
        col++;

        /* Accidental */
        if (step->notes[i].accidental == ACC_SHARP) {
            LCD_WriteCustom(5);
            col++;
        } else if (step->notes[i].accidental == ACC_FLAT) {
            LCD_WriteCustom(6);
            col++;
        }

        /* Octave (from midiNote if present) */
        char oct = MidiToOctaveChar(step->notes[i].midiNote);
        GroveLCD_WriteChar(&lcd, oct);
        col++;

        /* Space between notes */
        if (i < (step->noteCount - 1) && col < 16) {
            GroveLCD_WriteChar(&lcd, ' ');
            col++;
        }

        if (col >= 16) break;
    }

    /* Row 1: duration icons under first character of each note */
    for (uint8_t i = 0; i < step->noteCount; i++)
    {
        if (startCol[i] < 16) {
            GroveLCD_SetCursor(&lcd, 1, startCol[i]);
            LCD_WriteCustom(step->notes[i].lengthIcon); /* 0..4 */
        }
    }
}

static void DisplayChordStep(const Chord *chord)
{
    GroveLCD_Clear(&lcd);
    LCD_ClearRow(0);
    LCD_ClearRow(1);

    /* Line 0: chord name */
    GroveLCD_SetCursor(&lcd, 0, 0);
    GroveLCD_Print(&lcd, "Chord:");
    GroveLCD_Print(&lcd, chord->name);

    /* Line 1: chord notes (no durations) */
    GroveLCD_SetCursor(&lcd, 1, 0);

    for (uint8_t i = 0; i < chord->noteCount; i++)
    {
        GroveLCD_WriteChar(&lcd, chord->notes[i].letter);

        if (chord->notes[i].accidental == ACC_SHARP) {
            LCD_WriteCustom(5);
        } else if (chord->notes[i].accidental == ACC_FLAT) {
            LCD_WriteCustom(6);
        }

        if (i < (chord->noteCount - 1)) {
            GroveLCD_WriteChar(&lcd, ' ');
        }
    }
}
