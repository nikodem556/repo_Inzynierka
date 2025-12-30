#ifndef APP_H
#define APP_H

#include <stdint.h>
#include <stdbool.h>
#include "button.h"
#include "lesson.h"
#include "songs.h"
#include "chords.h"

/*
 * app.h / app.c
 *
 * This module implements the top-level application state machine:
 * - Welcome screen
 * - Main menu
 * - Lists (songs / chord packs)
 * - Legend screen (custom LCD symbols)
 * - Lesson runtime (song lesson or chord exercise)
 *
 * The UI is controlled via three buttons: RESET, NEXT, OK.
 */

/* Application state definitions for the menu/lesson state machine */
typedef enum {
    APP_STATE_WELCOME = 0,     /* Welcome screen shown on startup */
    APP_STATE_MENU_MAIN,       /* Main menu (3 entries) */
    APP_STATE_MENU_SONGS,      /* Song selection list */
    APP_STATE_MENU_CHORDPACKS, /* Chord pack selection list */
    APP_STATE_VIEW_LEGEND,     /* Note symbols legend screen */
    APP_STATE_LESSON_SONG,     /* Active song lesson */
    APP_STATE_LESSON_CHORD     /* Active chord exercise */
} AppState;

/* Initialize the application state machine (must be called once at startup). */
void App_Init(void);

/* Process button inputs and update the state machine (call periodically in main loop). */
void App_Update(void);

#endif /* APP_H */
