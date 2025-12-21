#include "main.h"
#include "lesson.h"
#include "grove_lcd16x2_i2c.h"
#include "notes.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>

/* The Grove LCD instance is created in main.c */
extern GroveLCD_t lcd;

#define LED_BLINK_DURATION 100

/* Duration type per NOTE (icon slots defined in main.c) */
typedef enum {
    DUR_WHOLE,        // slot 0
    DUR_HALF,         // slot 1
    DUR_QUARTER,      // slot 2
    DUR_EIGHTH,       // slot 3
    DUR_SIXTEENTH     // slot 4
} NoteDur_t;

static uint8_t Dur_ToIconSlot(NoteDur_t d)
{
    switch (d) {
        case DUR_WHOLE:      return 0;
        case DUR_HALF:       return 1;
        case DUR_QUARTER:    return 2;
        case DUR_EIGHTH:     return 3;
        case DUR_SIXTEENTH:  return 4;
        default:             return 2;
    }
}

/* Custom symbol slots (as you defined in main.c) */
#define SLOT_SHARP 5
#define SLOT_FLAT  6

/* Example scenario using sharps/flats in the NOTE TEXT */
static const char *SONG_STEPS_NAMES[][MAX_CHORD_NOTES + 1] = {
    {"C#4", NULL},                 // uses sharp
    {"Db4", "F4", "Ab4", NULL},     // uses flats
    {"Bb3", "D4", "F4", NULL},      // Bb major
    {"G4", "B4", "D5", NULL}        // G major (B natural)
};
#define LESSON_STEPS_COUNT  (sizeof(SONG_STEPS_NAMES) / sizeof(SONG_STEPS_NAMES[0]))

/* Duration per NOTE in each step */
static const NoteDur_t SONG_STEPS_DUR[LESSON_STEPS_COUNT][MAX_CHORD_NOTES] = {
    /* Step 1 */ { DUR_EIGHTH,     DUR_EIGHTH,     DUR_EIGHTH     }, // only first used
    /* Step 2 */ { DUR_QUARTER,    DUR_QUARTER,    DUR_QUARTER    },
    /* Step 3 */ { DUR_HALF,       DUR_HALF,       DUR_HALF       },
    /* Step 4 */ { DUR_SIXTEENTH,  DUR_SIXTEENTH,  DUR_SIXTEENTH  }
};

/* MIDI note numbers for each step (to be filled in Lesson_Init) */
static uint8_t songStepsMidi[LESSON_STEPS_COUNT][MAX_CHORD_NOTES];
static uint8_t stepNoteCount[LESSON_STEPS_COUNT];

/* Lesson state variables */
static uint8_t currentStep = 0;
static uint8_t lessonCompleted = 0;
static uint32_t totalNoteOnCount = 0;
static uint32_t correctNoteOnCount = 0;
static uint8_t stepNotesPlayedFlags[MAX_CHORD_NOTES];

/* LED blink timing control */
static uint32_t greenLedOffTime = 0;
static uint32_t redLedOffTime = 0;

static void LCD_ClearRow(uint8_t row)
{
    GroveLCD_SetCursor(&lcd, row, 0);
    GroveLCD_Print(&lcd, "                ");
}

/*
 * Print a note name using custom sharp/flat symbols:
 * Accepts formats like:
 * - "C4"
 * - "C#4"
 * - "Db4"
 * - "Ab3"
 * - "F#5"
 *
 * Output on LCD:
 * - "C" + [sharp-icon] + "4"
 * - "D" + [flat-icon]  + "4"
 */
static void LCD_PrintNotePretty(const char *name)
{
    if (name == NULL || name[0] == '\0') return;

    char base = (char)toupper((unsigned char)name[0]);
    GroveLCD_WriteChar(&lcd, base);

    if (name[1] == '#') {
        GroveLCD_WriteChar(&lcd, (char)SLOT_SHARP);
        if (name[2] != '\0') GroveLCD_WriteChar(&lcd, name[2]); // octave
        return;
    }

    if (name[1] == 'b' || name[1] == 'B') {
        GroveLCD_WriteChar(&lcd, (char)SLOT_FLAT);
        if (name[2] != '\0') GroveLCD_WriteChar(&lcd, name[2]); // octave
        return;
    }

    // No accidental: octave is name[1]
    if (name[1] != '\0') GroveLCD_WriteChar(&lcd, name[1]);
}

/* How many LCD columns a note will occupy when printed with LCD_PrintNotePretty() */
static uint8_t LCD_NotePrintedWidth(const char *name)
{
    if (name == NULL || name[0] == '\0') return 0;
    if (name[1] == '#' || name[1] == 'b' || name[1] == 'B') return 3; // base + symbol + octave
    return 2; // base + octave
}

/**
 * Row 0: notes/chord (pretty printed with custom ♯/♭)
 * Row 1: duration icon under the FIRST character of each note
 */
static void Lesson_DisplayStep(void)
{
    GroveLCD_Clear(&lcd);

    uint8_t startCol[MAX_CHORD_NOTES] = {0};
    uint8_t noteWidth[MAX_CHORD_NOTES] = {0};

    uint8_t col = 0;

    /* Row 0: print notes with spacing */
    GroveLCD_SetCursor(&lcd, 0, 0);
    LCD_ClearRow(0);
    GroveLCD_SetCursor(&lcd, 0, 0);

    for (uint8_t i = 0; i < stepNoteCount[currentStep]; ++i)
    {
        const char *name = SONG_STEPS_NAMES[currentStep][i];
        uint8_t w = LCD_NotePrintedWidth(name);
        if (w == 0) continue;

        /* +1 for space between notes except first */
        uint8_t needed = w + ((i > 0) ? 1 : 0);
        if ((uint8_t)(col + needed) > 16) break;

        if (i > 0) {
            GroveLCD_WriteChar(&lcd, ' ');
            col += 1;
        }

        startCol[i] = col;
        noteWidth[i] = w;

        LCD_PrintNotePretty(name);
        col += w;
    }

    /* Row 1: duration icons aligned under note start */
    LCD_ClearRow(1);

    for (uint8_t i = 0; i < stepNoteCount[currentStep]; ++i)
    {
        if (noteWidth[i] == 0) continue;

        uint8_t iconCol = startCol[i];  // under first char
        if (iconCol > 15) iconCol = 15;

        uint8_t slot = Dur_ToIconSlot(SONG_STEPS_DUR[currentStep][i]);
        GroveLCD_SetCursor(&lcd, 1, iconCol);
        GroveLCD_WriteChar(&lcd, (char)slot);
    }
}

static void Lesson_DisplaySummary(void)
{
    GroveLCD_Clear(&lcd);

    uint32_t accuracyPercent = 0;
    if (totalNoteOnCount > 0)
        accuracyPercent = (correctNoteOnCount * 100U) / totalNoteOnCount;

    char line0[17];
    char line1[17];

    snprintf(line0, sizeof(line0), "Acc %lu/%lu",
             (unsigned long)correctNoteOnCount,
             (unsigned long)totalNoteOnCount);

    snprintf(line1, sizeof(line1), "%lu%%  Reset",
             (unsigned long)accuracyPercent);

    GroveLCD_SetCursor(&lcd, 0, 0);
    GroveLCD_Print(&lcd, "                ");
    GroveLCD_SetCursor(&lcd, 1, 0);
    GroveLCD_Print(&lcd, "                ");

    GroveLCD_SetCursor(&lcd, 0, 0);
    GroveLCD_Print(&lcd, line0);
    GroveLCD_SetCursor(&lcd, 1, 0);
    GroveLCD_Print(&lcd, line1);
}

void Lesson_Init(void)
{
    for (uint8_t step = 0; step < LESSON_STEPS_COUNT; ++step)
    {
        uint8_t count = 0;
        for (uint8_t i = 0; i < MAX_CHORD_NOTES && SONG_STEPS_NAMES[step][i] != NULL; ++i)
        {
            uint8_t midiVal;
            if (NoteName_ToMidi(SONG_STEPS_NAMES[step][i], &midiVal) == NOTE_OK)
                songStepsMidi[step][count++] = midiVal;
        }
        stepNoteCount[step] = count;
    }

    HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(RED_LED_GPIO_Port,   RED_LED_Pin,   GPIO_PIN_RESET);

    currentStep = 0;
    lessonCompleted = 0;
    totalNoteOnCount = 0;
    correctNoteOnCount = 0;
    memset(stepNotesPlayedFlags, 0, sizeof(stepNotesPlayedFlags));

    Lesson_DisplayStep();
}

void Lesson_Reset(void)
{
    currentStep = 0;
    lessonCompleted = 0;
    totalNoteOnCount = 0;
    correctNoteOnCount = 0;

    memset(stepNotesPlayedFlags, 0, sizeof(stepNotesPlayedFlags));

    HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(RED_LED_GPIO_Port,   RED_LED_Pin,   GPIO_PIN_RESET);
    greenLedOffTime = 0;
    redLedOffTime = 0;

    Lesson_DisplayStep();
}

void Lesson_OnNoteOn(uint8_t midiNote)
{
    if (lessonCompleted)
        return;

    totalNoteOnCount++;

    uint8_t correct = 0;

    for (uint8_t i = 0; i < stepNoteCount[currentStep]; ++i)
    {
        if (midiNote == songStepsMidi[currentStep][i] && stepNotesPlayedFlags[i] == 0)
        {
            stepNotesPlayedFlags[i] = 1;
            correctNoteOnCount++;
            correct = 1;

            HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, GPIO_PIN_SET);
            greenLedOffTime = HAL_GetTick() + LED_BLINK_DURATION;
            break;
        }
    }

    if (!correct)
    {
        HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, GPIO_PIN_SET);
        redLedOffTime = HAL_GetTick() + LED_BLINK_DURATION;
    }

    uint8_t chordComplete = 1;
    for (uint8_t i = 0; i < stepNoteCount[currentStep]; ++i)
    {
        if (stepNotesPlayedFlags[i] == 0)
        {
            chordComplete = 0;
            break;
        }
    }

    if (chordComplete)
    {
        currentStep++;

        if (currentStep >= LESSON_STEPS_COUNT)
        {
            lessonCompleted = 1;
            Lesson_DisplaySummary();
        }
        else
        {
            memset(stepNotesPlayedFlags, 0, sizeof(stepNotesPlayedFlags));
            Lesson_DisplayStep();
        }
    }
}

void Lesson_Tick(void)
{
    uint32_t now = HAL_GetTick();

    if (greenLedOffTime && now >= greenLedOffTime)
    {
        HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, GPIO_PIN_RESET);
        greenLedOffTime = 0;
    }

    if (redLedOffTime && now >= redLedOffTime)
    {
        HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, GPIO_PIN_RESET);
        redLedOffTime = 0;
    }
}
