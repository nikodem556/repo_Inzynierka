#ifndef APP_H
#define APP_H

#include <stdint.h>
#include <stdbool.h>
#include "button.h"
#include "lesson.h"
#include "songs.h"
#include "chords.h"

// Application state definitions for the menu/lesson state machine
typedef enum {
    APP_STATE_WELCOME = 0,    // Welcome screen (startup)
    APP_STATE_MENU_MAIN,      // Main menu
    APP_STATE_MENU_SONGS,     // Song selection list
    APP_STATE_MENU_CHORDPACKS,// Chord pack selection list
    APP_STATE_VIEW_LEGEND,    // Note symbols legend screen
    APP_STATE_LESSON_SONG,    // In a song lesson
    APP_STATE_LESSON_CHORD    // In a chord lesson
} AppState;

// Initialize the application state machine and hardware
void App_Init(void);

// Process inputs and update the state machine (to be called in main loop)
void App_Update(void);

#endif // APP_H
