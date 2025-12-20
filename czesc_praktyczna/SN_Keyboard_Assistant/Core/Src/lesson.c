#include "main.h"          // For LED and button GPIO definitions and HAL functions
#include "lesson.h"
#include "grove_lcd16x2_i2c.h"
#include "button.h"
#include "notes.h"         // Provides NoteName_ToMidi and NOTE_OK
#include <string.h>
#include <stdio.h>

/* The Grove LCD instance is created in main.c */
extern GroveLCD_t lcd;

// LED blink duration (milliseconds)
#define LED_BLINK_DURATION 100

// Song definition: each step is a chord represented by note names (null-terminated list for each chord)
static const char *SONG_STEPS_NAMES[][MAX_CHORD_NOTES + 1] = {
    {"C4", NULL},                  // Step 1: single note C4
    {"C4", "E4", "G4", NULL},      // Step 2: C4-E4-G4 chord (C major triad)
    {"F4", "A4", "C5", NULL},      // Step 3: F4-A4-C5 chord (F major triad)
    {"G4", "B4", "D5", NULL},      // Step 4: G4-B4-D5 chord (G major triad)
    {"C4", "E4", "G4", NULL}       // Step 5: C4-E4-G4 chord (repeat C major)
};
#define LESSON_STEPS_COUNT  (sizeof(SONG_STEPS_NAMES) / sizeof(SONG_STEPS_NAMES[0]))

// MIDI note numbers for each step (to be filled in Lesson_Init)
static uint8_t songStepsMidi[LESSON_STEPS_COUNT][MAX_CHORD_NOTES];
static uint8_t stepNoteCount[LESSON_STEPS_COUNT];

// Lesson state variables
static uint8_t currentStep = 0;
static uint8_t lessonCompleted = 0;
static uint32_t totalNoteOnCount = 0;
static uint32_t correctNoteOnCount = 0;
static uint8_t stepNotesPlayedFlags[MAX_CHORD_NOTES];  // Flags to mark played notes in the current chord

// LED blink timing control
static uint32_t greenLedOffTime = 0;
static uint32_t redLedOffTime = 0;

/**
 * @brief Draw current lesson step (or its chord) on the Grove 16x2 LCD.
 */
static void Lesson_DisplayStep(void) {
    GroveLCD_Clear(&lcd);

    char line[17];
    snprintf(line, sizeof(line), "Step %d:", currentStep + 1);
    GroveLCD_SetCursor(&lcd, 0, 0);
    GroveLCD_Print(&lcd, line);

    GroveLCD_SetCursor(&lcd, 1, 0);
    line[0] = '\0';
    for (uint8_t i = 0; i < stepNoteCount[currentStep]; ++i) {
        strncat(line, SONG_STEPS_NAMES[currentStep][i], sizeof(line) - strlen(line) - 1);
        if (i < stepNoteCount[currentStep] - 1) {
            strncat(line, " ", sizeof(line) - strlen(line) - 1);
        }
    }
    GroveLCD_Print(&lcd, line);
}

/**
 * @brief Draw lesson summary on the Grove 16x2 LCD.
 */
static void Lesson_DisplaySummary(void) {
    GroveLCD_Clear(&lcd);

    char line1[17];
    char line2[17];
    uint32_t accuracyPercent = 0;
    if (totalNoteOnCount > 0) {
        accuracyPercent = (correctNoteOnCount * 100U) / totalNoteOnCount;
    }

    // Shortened to fit 16 chars reliably
    snprintf(line1, sizeof(line1), "Acc %lu/%lu",
             (unsigned long)correctNoteOnCount,
             (unsigned long)totalNoteOnCount);
    snprintf(line2, sizeof(line2), "%lu%% Reset",
             (unsigned long)accuracyPercent);

    GroveLCD_SetCursor(&lcd, 0, 0);
    GroveLCD_Print(&lcd, line1);
    GroveLCD_SetCursor(&lcd, 1, 0);
    GroveLCD_Print(&lcd, line2);
}

void Lesson_Init(void) {
    // Convert note name strings to MIDI note numbers for each step
    for (uint8_t step = 0; step < LESSON_STEPS_COUNT; ++step) {
        uint8_t count = 0;
        for (uint8_t i = 0; i < MAX_CHORD_NOTES && SONG_STEPS_NAMES[step][i] != NULL; ++i) {
            uint8_t midiVal;
            if (NoteName_ToMidi(SONG_STEPS_NAMES[step][i], &midiVal) == NOTE_OK) {
                songStepsMidi[step][count++] = midiVal;
            }
        }
        stepNoteCount[step] = count;
    }

    // Initialize LED states
    HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(RED_LED_GPIO_Port,   RED_LED_Pin,   GPIO_PIN_RESET);

    // Initialize lesson state variables
    currentStep = 0;
    lessonCompleted = 0;
    totalNoteOnCount = 0;
    correctNoteOnCount = 0;
    memset(stepNotesPlayedFlags, 0, sizeof(stepNotesPlayedFlags));

    // Display the first step
    Lesson_DisplayStep();
}

void Lesson_Reset(void) {
    // Reset lesson progress and counters
    currentStep = 0;
    lessonCompleted = 0;
    totalNoteOnCount = 0;
    correctNoteOnCount = 0;

    // Reset played-notes flags for the first chord
    memset(stepNotesPlayedFlags, 0, sizeof(stepNotesPlayedFlags));

    // Turn off LEDs
    HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(RED_LED_GPIO_Port,   RED_LED_Pin,   GPIO_PIN_RESET);
    greenLedOffTime = 0;
    redLedOffTime = 0;

    // Refresh the LCD to show the first step again
    Lesson_DisplayStep();
}

void Lesson_OnNoteOn(uint8_t midiNote) {
    if (lessonCompleted) {
        // Ignore inputs if lesson already completed (waiting for reset)
        return;
    }

    // Count every Note On event
    totalNoteOnCount++;

    uint8_t correct = 0;

    // Check if the incoming note is part of the current chord and not yet played
    for (uint8_t i = 0; i < stepNoteCount[currentStep]; ++i) {
        if (midiNote == songStepsMidi[currentStep][i] && stepNotesPlayedFlags[i] == 0) {
            // Correct note hit
            stepNotesPlayedFlags[i] = 1;
            correctNoteOnCount++;
            correct = 1;

            // Blink green LED for a correct note
            HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, GPIO_PIN_SET);
            greenLedOffTime = HAL_GetTick() + LED_BLINK_DURATION;
            break;
        }
    }

    if (!correct) {
        // Wrong note
        HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, GPIO_PIN_SET);
        redLedOffTime = HAL_GetTick() + LED_BLINK_DURATION;
    }

    // Check if the current chord is now fully played
    uint8_t chordComplete = 1;
    for (uint8_t i = 0; i < stepNoteCount[currentStep]; ++i) {
        if (stepNotesPlayedFlags[i] == 0) {
            chordComplete = 0;
            break;
        }
    }

    if (chordComplete) {
        // Advance to next step (chord)
        currentStep++;

        if (currentStep >= LESSON_STEPS_COUNT) {
            // Lesson finished
            lessonCompleted = 1;
            Lesson_DisplaySummary();
        } else {
            // Prepare for the next chord
            memset(stepNotesPlayedFlags, 0, sizeof(stepNotesPlayedFlags));
            Lesson_DisplayStep();
        }
    }
}

void Lesson_Tick(void) {
    uint32_t now = HAL_GetTick();

    // Turn off the green LED if its blink interval has passed
    if (greenLedOffTime && now >= greenLedOffTime) {
        HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, GPIO_PIN_RESET);
        greenLedOffTime = 0;
    }

    // Turn off the red LED if its blink interval has passed
    if (redLedOffTime && now >= redLedOffTime) {
        HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, GPIO_PIN_RESET);
        redLedOffTime = 0;
    }
}
