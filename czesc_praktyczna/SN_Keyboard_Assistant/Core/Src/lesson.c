#include "lesson.h"
#include "grove_lcd16x2_i2c.h"
#include "main.h"   /* GPIO macros and HAL_GetTick() */

#include <stdio.h>  /* snprintf() */

/*
 * lesson.c
 *
 * Lesson engine responsible for verifying played notes and providing feedback.
 *
 * Two modes are supported:
 * 1) Song mode:
 *    - Each step defines one to three exact MIDI notes to be played (match by MIDI number).
 *    - The display shows note names with accidentals and octave, plus duration icons.
 *
 * 2) Chord mode:
 *    - Each step defines one chord (up to three notes).
 *    - Matching is done by pitch class (note % 12), allowing any octave.
 *
 * Feedback:
 * - Green LED blinks on correct input, red LED blinks on wrong input.
 * - After finishing all steps, a summary screen is shown (OK/Total and percentage).
 *
 * Notes:
 * - This file does not read USB/MIDI directly; it only processes inputs passed in
 *   via Lesson_HandleInput().
 * - LCD instance is created and initialized in main.c (exported as extern).
 */

/* LCD instance lives in main.c */
extern GroveLCD_t lcd;

/* --- Tunables --- */
#define LED_BLINK_MS   (120U)

/* --- Lesson internal state --- */
typedef enum {
    LESSON_STATE_RUNNING = 0,
    LESSON_STATE_SUMMARY
} LessonState;

static Song *currentSong = NULL;
static ChordPack *currentChordPack = NULL;
static uint8_t currentStepIndex = 0;
static uint8_t totalSteps = 0;
static bool lessonActive = false;
static LessonState lessonState = LESSON_STATE_RUNNING;

/* Step hit slots (max 3 notes per step/chord in this project) */
static bool stepHit[3] = { false, false, false };

/* Session statistics */
static uint32_t correctPlayed = 0;
static uint32_t wrongPlayed = 0;
static uint32_t totalPlayed = 0;

/* LED blink state (non-blocking) */
static bool greenLedOn = false;
static bool redLedOn = false;
static uint32_t greenLedTick = 0;
static uint32_t redLedTick = 0;

/* Forward declarations */
static void DisplaySongStep(const SongStep *step);
static void DisplayChordStep(const Chord *chord);
static void ResetStepHit(void);
static uint8_t GetCurrentSlotCount(void);
static uint8_t CountMissingSlots(void);
static void AddMissingSlotsAsCorrect(void);
static void EnterSummary(void);
static void ShowSummary(void);
static void LedBlinkGreen(void);
static void LedBlinkRed(void);
static int8_t NoteToPitchClass(char letter, Accidental accidental);
static bool IsStepComplete(void);
static void AdvanceOrSummary(void);

/* --- Local LCD helpers --- */

/* Clears one LCD row by overwriting it with spaces. */
static void LCD_ClearRow(uint8_t row)
{
    GroveLCD_SetCursor(&lcd, row, 0);
    GroveLCD_Print(&lcd, "                ");
}

/* Writes one custom character (CGRAM slot 0..7) to the LCD. */
static void LCD_WriteCustom(uint8_t slot)
{
    GroveLCD_WriteChar(&lcd, (char)slot);
}

/*
 * Convert MIDI note number to octave character.
 * MIDI convention: C4=60 -> octave 4. Formula: octave = (midi/12) - 1.
 */
static char MidiToOctaveChar(int8_t midi)
{
    if (midi < 0) return '?';
    int octave = (midi / 12) - 1;
    if (octave < 0 || octave > 9) return '?';
    return (char)('0' + octave);
}

/* --- Step/slot helpers --- */

/* Resets per-step note hit flags. */
static void ResetStepHit(void)
{
    stepHit[0] = false;
    stepHit[1] = false;
    stepHit[2] = false;
}

/* Returns number of required notes for the current step (capped by 3 in logic). */
static uint8_t GetCurrentSlotCount(void)
{
    if (currentSong != NULL) {
        return currentSong->steps[currentStepIndex].noteCount;
    }
    if (currentChordPack != NULL) {
        return currentChordPack->chords[currentStepIndex].noteCount;
    }
    return 0;
}

/* Counts remaining (not yet hit) required notes for the current step. */
static uint8_t CountMissingSlots(void)
{
    uint8_t cnt = 0;
    uint8_t slots = GetCurrentSlotCount();
    if (slots > 3) slots = 3;

    for (uint8_t i = 0; i < slots; i++) {
        if (!stepHit[i]) cnt++;
    }
    return cnt;
}

/*
 * Treats remaining slots as correct (used for "skip" via OK button).
 * This updates stats and marks all required notes as hit.
 */
static void AddMissingSlotsAsCorrect(void)
{
    uint8_t missing = CountMissingSlots();
    if (missing == 0) return;

    correctPlayed += missing;
    totalPlayed += missing;

    uint8_t slots = GetCurrentSlotCount();
    if (slots > 3) slots = 3;
    for (uint8_t i = 0; i < slots; i++) {
        stepHit[i] = true;
    }
}

/* Returns true if all required notes for the step have been hit. */
static bool IsStepComplete(void)
{
    uint8_t slots = GetCurrentSlotCount();
    if (slots == 0) return false;
    if (slots > 3) slots = 3;

    for (uint8_t i = 0; i < slots; i++) {
        if (!stepHit[i]) return false;
    }
    return true;
}

/*
 * Advances to the next step if available; otherwise enters the summary screen.
 * This is called after completing a step (or after OK "skip").
 */
static void AdvanceOrSummary(void)
{
    if (currentStepIndex < (totalSteps - 1))
    {
        currentStepIndex++;
        ResetStepHit();

        if (currentSong) DisplaySongStep(&currentSong->steps[currentStepIndex]);
        else DisplayChordStep(&currentChordPack->chords[currentStepIndex]);
    }
    else
    {
        EnterSummary();
    }
}

/* --- LED helpers --- */

/* Turn on GREEN LED and record timestamp for non-blocking turn-off. */
static void LedBlinkGreen(void)
{
    HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, GPIO_PIN_SET);
    greenLedOn = true;
    greenLedTick = HAL_GetTick();
}

/* Turn on RED LED and record timestamp for non-blocking turn-off. */
static void LedBlinkRed(void)
{
    HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, GPIO_PIN_SET);
    redLedOn = true;
    redLedTick = HAL_GetTick();
}

/* --- Pitch class mapping for chord mode --- */

/*
 * Maps (letter + accidental) into pitch class 0..11.
 * 'H' is treated like 'B' for Polish/German notation.
 */
static int8_t NoteToPitchClass(char letter, Accidental accidental)
{
    int8_t base = -1;
    switch (letter)
    {
        case 'C': base = 0;  break;
        case 'D': base = 2;  break;
        case 'E': base = 4;  break;
        case 'F': base = 5;  break;
        case 'G': base = 7;  break;
        case 'A': base = 9;  break;
        case 'B': base = 11; break;
        case 'H': base = 11; break;
        default:  base = -1; break;
    }
    if (base < 0) return -1;

    if (accidental == ACC_SHARP) {
        base = (int8_t)((base + 1) % 12);
    } else if (accidental == ACC_FLAT) {
        base = (int8_t)((base + 11) % 12);
    }
    return base;
}

/* --- Lesson lifecycle --- */

void Lesson_StartSong(Song *song)
{
    currentSong = song;
    currentChordPack = NULL;
    currentStepIndex = 0;
    totalSteps = (song != NULL) ? song->stepCount : 0;
    lessonActive = (song != NULL && totalSteps > 0);

    /* Reset session statistics */
    correctPlayed = 0;
    wrongPlayed = 0;
    totalPlayed = 0;

    lessonState = LESSON_STATE_RUNNING;
    ResetStepHit();

    /* Reset LEDs */
    HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, GPIO_PIN_RESET);
    greenLedOn = false;
    redLedOn = false;
    greenLedTick = 0;
    redLedTick = 0;

    if (lessonActive) {
        DisplaySongStep(&song->steps[0]);
    }
}

void Lesson_StartChordExercise(ChordPack *pack)
{
    currentChordPack = pack;
    currentSong = NULL;
    currentStepIndex = 0;
    totalSteps = (pack != NULL) ? pack->chordCount : 0;
    lessonActive = (pack != NULL && totalSteps > 0);

    /* Reset session statistics */
    correctPlayed = 0;
    wrongPlayed = 0;
    totalPlayed = 0;

    lessonState = LESSON_STATE_RUNNING;
    ResetStepHit();

    /* Reset LEDs */
    HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, GPIO_PIN_RESET);
    greenLedOn = false;
    redLedOn = false;
    greenLedTick = 0;
    redLedTick = 0;

    if (lessonActive) {
        DisplayChordStep(&pack->chords[0]);
    }
}

bool Lesson_IsActive(void)
{
    return lessonActive;
}

/* --- Summary --- */

/* Renders summary screen (correct/total and percent). */
static void ShowSummary(void)
{
    GroveLCD_Clear(&lcd);
    LCD_ClearRow(0);
    LCD_ClearRow(1);

    char line1[17];
    char line2[17];

    uint32_t percent = 0;
    if (totalPlayed > 0) {
        /* Rounded percentage */
        percent = (uint32_t)(((uint64_t)correctPlayed * 100ULL + (totalPlayed / 2U)) / totalPlayed);
    }

    snprintf(line1, sizeof(line1), "OK: %lu/%lu", (unsigned long)correctPlayed, (unsigned long)totalPlayed);
    snprintf(line2, sizeof(line2), "P: %lu%% any key", (unsigned long)percent);

    GroveLCD_SetCursor(&lcd, 0, 0);
    GroveLCD_Print(&lcd, line1);
    GroveLCD_SetCursor(&lcd, 1, 0);
    GroveLCD_Print(&lcd, line2);
}

/* Switches lesson state into summary and shows the summary screen. */
static void EnterSummary(void)
{
    lessonState = LESSON_STATE_SUMMARY;
    ShowSummary();
}

/* --- Input handling --- */

void Lesson_HandleInput(uint8_t input)
{
    if (!lessonActive) return;

    /*
     * In summary screen:
     * - Ignore MIDI notes
     * - Any button ends the lesson and returns control to the UI
     */
    if (lessonState == LESSON_STATE_SUMMARY)
    {
        if (input <= 0x7F) {
            return;
        }

        lessonActive = false;
        lessonState = LESSON_STATE_RUNNING;
        ResetStepHit();
        HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, GPIO_PIN_RESET);
        greenLedOn = false;
        redLedOn = false;
        return;
    }

    /* --- MIDI note (0..127) --- */
    if (input <= 0x7F)
    {
        totalPlayed++;

        if (currentSong != NULL)
        {
            SongStep *step = &currentSong->steps[currentStepIndex];
            uint8_t slots = step->noteCount;
            if (slots > 3) slots = 3;

            bool matched = false;
            for (uint8_t i = 0; i < slots; i++)
            {
                if (!stepHit[i] && (uint8_t)step->notes[i].midiNote == input)
                {
                    stepHit[i] = true;
                    matched = true;
                    break;
                }
            }

            if (matched) {
                correctPlayed++;
                LedBlinkGreen();

                /* Auto-advance when all required notes are hit */
                if (IsStepComplete()) {
                    AdvanceOrSummary();
                }
            } else {
                wrongPlayed++;
                LedBlinkRed();
            }

            return;
        }

        if (currentChordPack != NULL)
        {
            Chord *chord = &currentChordPack->chords[currentStepIndex];
            uint8_t slots = chord->noteCount;
            if (slots > 3) slots = 3;

            uint8_t playedPC = (uint8_t)(input % 12U);
            bool matched = false;

            for (uint8_t i = 0; i < slots; i++)
            {
                if (stepHit[i]) continue;
                int8_t expectedPC = NoteToPitchClass(chord->notes[i].letter, chord->notes[i].accidental);
                if (expectedPC >= 0 && (uint8_t)expectedPC == playedPC)
                {
                    stepHit[i] = true;
                    matched = true;
                    break;
                }
            }

            if (matched) {
                correctPlayed++;
                LedBlinkGreen();

                /* Auto-advance when chord tones are all hit */
                if (IsStepComplete()) {
                    AdvanceOrSummary();
                }
            } else {
                wrongPlayed++;
                LedBlinkRed();
            }

            return;
        }

        return;
    }

    /* --- Buttons --- */
    if (input == LESSON_INPUT_BTN_OK)
    {
        /* Skip current step: count remaining slots as correct and move forward */
        AddMissingSlotsAsCorrect();
        AdvanceOrSummary();
    }
    else if (input == LESSON_INPUT_BTN_NEXT)
    {
        /* Go to previous step; if already at step 0 -> exit lesson */
        if (currentStepIndex > 0)
        {
            currentStepIndex--;
            ResetStepHit();
            if (currentSong) DisplaySongStep(&currentSong->steps[currentStepIndex]);
            else DisplayChordStep(&currentChordPack->chords[currentStepIndex]);
        }
        else
        {
            lessonActive = false;
            ResetStepHit();
            HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, GPIO_PIN_RESET);
            greenLedOn = false;
            redLedOn = false;
        }
    }
    else if (input == LESSON_INPUT_BTN_RESET)
    {
        /* Reset to step 0; if already at step 0 -> exit lesson */
        if (currentStepIndex != 0)
        {
            currentStepIndex = 0;
            ResetStepHit();
            if (currentSong) DisplaySongStep(&currentSong->steps[0]);
            else DisplayChordStep(&currentChordPack->chords[0]);
        }
        else
        {
            lessonActive = false;
            ResetStepHit();
            HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, GPIO_PIN_RESET);
            greenLedOn = false;
            redLedOn = false;
        }
    }
}

/*
 * Periodic non-blocking update for LED blink timing.
 * Turns LEDs off after LED_BLINK_MS.
 */
void Lesson_Update(void)
{
    uint32_t now = HAL_GetTick();

    if (greenLedOn && (uint32_t)(now - greenLedTick) >= LED_BLINK_MS) {
        HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, GPIO_PIN_RESET);
        greenLedOn = false;
    }

    if (redLedOn && (uint32_t)(now - redLedTick) >= LED_BLINK_MS) {
        HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, GPIO_PIN_RESET);
        redLedOn = false;
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

    /* Row 0: notes (e.g. C#4, Db4, E4) */
    for (uint8_t i = 0; i < step->noteCount; i++)
    {
        if (i >= 3) break;
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

        /* Octave (derived from MIDI note) */
        char oct = MidiToOctaveChar(step->notes[i].midiNote);
        GroveLCD_WriteChar(&lcd, oct);
        col++;

        /* Separator */
        if (i < (step->noteCount - 1) && col < 16) {
            GroveLCD_WriteChar(&lcd, ' ');
            col++;
        }

        if (col >= 16) break;
    }

    /* Row 1: duration icons under the first character of each note */
    for (uint8_t i = 0; i < step->noteCount; i++)
    {
        if (i >= 3) break;
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

    /* Row 0: chord name */
    GroveLCD_SetCursor(&lcd, 0, 0);
    GroveLCD_Print(&lcd, "Chord:");
    GroveLCD_Print(&lcd, chord->name);

    /* Row 1: chord tones (no durations) */
    GroveLCD_SetCursor(&lcd, 1, 0);

    for (uint8_t i = 0; i < chord->noteCount; i++)
    {
        if (i >= 3) break;

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
