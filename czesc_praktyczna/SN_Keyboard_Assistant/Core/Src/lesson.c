/*
 * lesson.c
 *
 *  Created on: 5 gru 2025
 *      Author: nikod
 */


#include "lesson.h"
#include "notes.h"    // for NoteNameArray_ToMidi and Midi_ToNoteName
#include "main.h"     // for HAL GPIO definitions (GPIO_TypeDef, HAL_Delay, etc)


/* Configuration macros: Lesson notes and LED pins */

// Define the lesson note sequence (as note name strings).
// You can change these to alter the lesson without changing code logic.
#define LESSON_STEP1_NAME   "C4"
#define LESSON_STEP2_NAME   "D5"
#define LESSON_STEP3_NAME   "E4"
#define LESSON_STEP4_NAME   "CIS4"

// Define LED ports and pins corresponding to each lesson step:
#define LESSON_LED1_PORT    GPIOA
#define LESSON_LED1_PIN     GPIO_PIN_0  // e.g., PA0
#define LESSON_LED2_PORT    GPIOA
#define LESSON_LED2_PIN     GPIO_PIN_1  // e.g., PA1
#define LESSON_LED3_PORT    GPIOA
#define LESSON_LED3_PIN     GPIO_PIN_4  // e.g., PA4
#define LESSON_LED4_PORT    GPIOB
#define LESSON_LED4_PIN     GPIO_PIN_0  // e.g., PB0

// Define an LED for error feedback (lights on wrong note)
#define LESSON_ERROR_LED_PORT  GPIOC
#define LESSON_ERROR_LED_PIN   GPIO_PIN_1  // e.g., PC1

// Internal arrays for step LED ports/pins (for easier iteration)
static GPIO_TypeDef * const noteLedPorts[] = {
    LESSON_LED1_PORT, LESSON_LED2_PORT, LESSON_LED3_PORT, LESSON_LED4_PORT
};
static const uint16_t noteLedPins[] = {
    LESSON_LED1_PIN, LESSON_LED2_PIN, LESSON_LED3_PIN, LESSON_LED4_PIN
};

// Determine number of steps in the lesson (size of lessonNames array defined below)
#define LESSON_STEP_COUNT  (sizeof(noteLedPins) / sizeof(noteLedPins[0]))

// Internal lesson note data
static const char *lessonNames[] = {
    LESSON_STEP1_NAME, LESSON_STEP2_NAME, LESSON_STEP3_NAME, LESSON_STEP4_NAME
};
static uint8_t lessonNotes[LESSON_STEP_COUNT];  // MIDI note numbers for each lesson step

// Internal state tracking
static size_t currentStep = 0;
static bool lessonFinished = false;

/* Static helper functions for LED control */

/// Turn **all** LEDs (step LEDs and the error LED) off.
static void Lesson_Leds_AllOff(void) {
    for (size_t i = 0; i < LESSON_STEP_COUNT; ++i) {
        HAL_GPIO_WritePin(noteLedPorts[i], noteLedPins[i], GPIO_PIN_RESET);
    }
    HAL_GPIO_WritePin(LESSON_ERROR_LED_PORT, LESSON_ERROR_LED_PIN, GPIO_PIN_RESET);
}

/// Turn on the LED for a specific lesson step index.
static void Lesson_Led_NoteOn(size_t stepIndex) {
    if (stepIndex < LESSON_STEP_COUNT) {
        HAL_GPIO_WritePin(noteLedPorts[stepIndex], noteLedPins[stepIndex], GPIO_PIN_SET);
    }
}

/// Turn off the LED for a specific lesson step index.
static void Lesson_Led_NoteOff(size_t stepIndex) {
    if (stepIndex < LESSON_STEP_COUNT) {
        HAL_GPIO_WritePin(noteLedPorts[stepIndex], noteLedPins[stepIndex], GPIO_PIN_RESET);
    }
}

/// Briefly flash the error LED to indicate a wrong note (blocking delay).
static void Lesson_Led_ErrorFeedback(void) {
    // Turn on error LED, wait, then turn it off
    HAL_GPIO_WritePin(LESSON_ERROR_LED_PORT, LESSON_ERROR_LED_PIN, GPIO_PIN_SET);
    HAL_Delay(100);  // ~100 ms blink; in a real application, consider using a timer or non-blocking method
    HAL_GPIO_WritePin(LESSON_ERROR_LED_PORT, LESSON_ERROR_LED_PIN, GPIO_PIN_RESET);
}

/// Flash all note LEDs in unison to indicate lesson success (blocking pattern).
static void Lesson_Led_SuccessFeedback(void) {
    // Blink all step LEDs together a few times
    for (int i = 0; i < 3; ++i) {
        // Turn all note LEDs on
        for (size_t j = 0; j < LESSON_STEP_COUNT; ++j) {
            HAL_GPIO_WritePin(noteLedPorts[j], noteLedPins[j], GPIO_PIN_SET);
        }
        HAL_Delay(200);
        // Turn all note LEDs off
        for (size_t j = 0; j < LESSON_STEP_COUNT; ++j) {
            HAL_GPIO_WritePin(noteLedPorts[j], noteLedPins[j], GPIO_PIN_RESET);
        }
        HAL_Delay(200);
    }
    // Ensure error LED is off as well (just in case)
    HAL_GPIO_WritePin(LESSON_ERROR_LED_PORT, LESSON_ERROR_LED_PIN, GPIO_PIN_RESET);
}

/* Public API function implementations */

void Lesson_Init(void) {
    // Convert lesson note name strings to MIDI note numbers using NoteNameArray_ToMidi
    NoteParseStatus convStatus = NoteNameArray_ToMidi(lessonNames, LESSON_STEP_COUNT, lessonNotes);
    if (convStatus != NOTE_OK) {
        printf("Lesson_Init: Error parsing lesson note names (status = %d)\r\n", convStatus);
        // If conversion failed, mark lesson as finished to ignore input (to avoid undefined behavior)
        lessonFinished = true;
        Lesson_Leds_AllOff();
        return;
    }

    // Initialize state
    currentStep = 0;
    lessonFinished = false;
    // Initialize LEDs: all off, then turn on the LED for the first expected note
    Lesson_Leds_AllOff();
    if (LESSON_STEP_COUNT > 0) {
        Lesson_Led_NoteOn(0);
    }
    printf("Lesson_Init: Ready - first note is %s (MIDI %u)\r\n",
           lessonNames[0], (unsigned)lessonNotes[0]);
}

void Lesson_Reset(void) {
    // Reset state to the beginning of the lesson
    currentStep = 0;
    lessonFinished = false;
    // Turn off all LEDs and light the first step's LED
    Lesson_Leds_AllOff();
    if (LESSON_STEP_COUNT > 0) {
        Lesson_Led_NoteOn(0);
    }
    printf("Lesson_Reset: Back to step 1 (note %s)\r\n", lessonNames[0]);
}

void Lesson_OnNoteOn(uint8_t note, uint8_t vel) {
    if (lessonFinished) {
        // Lesson already completed – optionally, we could flash a success indicator again.
        // For now, just ignore further notes.
        return;
    }

    // Get the expected MIDI note for the current step
    uint8_t expectedNote = lessonNotes[currentStep];
    if (note == expectedNote) {
        // Correct note played
        char noteNameBuf[8];
        Midi_ToNoteName(note, noteNameBuf, sizeof(noteNameBuf));
        printf("Lesson: Correct! Note %s (MIDI %u) was the expected note.\r\n",
               noteNameBuf, (unsigned)note);

        // Turn off LED for this step
        Lesson_Led_NoteOff(currentStep);
        // Advance to next step
        currentStep++;
        if (currentStep >= LESSON_STEP_COUNT) {
            // All notes completed
            lessonFinished = true;
            printf("Lesson: All %d notes correct! Lesson completed.\r\n", (int)LESSON_STEP_COUNT);
            // All LEDs off and success indication
            Lesson_Leds_AllOff();
            Lesson_Led_SuccessFeedback();
        } else {
            // Turn on LED for the next expected note
            Lesson_Led_NoteOn(currentStep);
            // (Optional debug) print next expected note name
            printf("Lesson: Next note to play is %s (MIDI %u)\r\n",
                   lessonNames[currentStep], (unsigned)lessonNotes[currentStep]);
        }
    } else {
        // Wrong note played
        char expName[8], gotName[8];
        Midi_ToNoteName(expectedNote, expName, sizeof(expName));
        Midi_ToNoteName(note, gotName, sizeof(gotName));
        printf("Lesson: Wrong note! Expected %s (MIDI %u) but got %s (MIDI %u).\r\n",
               expName, (unsigned)expectedNote, gotName, (unsigned)note);

        // Indicate error (flash error LED). The current step LED stays on.
        Lesson_Led_ErrorFeedback();
        // Note: currentStep remains the same, user should try the same note again.
    }
}

bool Lesson_IsFinished(void) {
    return lessonFinished;
}
