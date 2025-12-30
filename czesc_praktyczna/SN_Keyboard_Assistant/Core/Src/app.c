/*
 * app.c
 *
 * Top-level user interface and navigation logic.
 * The module reacts to button presses and drives:
 * - LCD screens (welcome/menu/lists/legend)
 * - lesson engine start/stop and button forwarding during lessons
 *
 * NOTE: This file only contains UI/state-machine code. USB/MIDI parsing happens in main.c.
 */

#include "app.h"
#include "grove_lcd16x2_i2c.h"

/* LCD instance lives in main.c (initialized there via GroveLCD_Init). */
extern GroveLCD_t lcd;

/* Current application state and menu indices (kept static inside this module). */
static AppState appState;
static uint8_t mainMenuIndex = 0;
static uint8_t songListIndex = 0;
static uint8_t chordPackIndex = 0;

/* Forward declarations for LCD screen rendering functions. */
static void DisplayWelcomeScreen(void);
static void DisplayMainMenu(void);
static void DisplayNotesLegend(void);
static void DisplaySongsList(void);
static void DisplayChordPacksList(void);

/**
 * @brief Clears a single LCD row by overwriting it with spaces.
 * @param row LCD row index (0 or 1 for a 16x2 display).
 */
static void LCD_ClearRow(uint8_t row)
{
    GroveLCD_SetCursor(&lcd, row, 0);
    GroveLCD_Print(&lcd, "                "); /* 16 spaces */
}

/**
 * @brief Writes one of the LCD custom characters (CGRAM slot 0..7).
 * @param slot Custom character slot index.
 */
static void LCD_WriteCustom(uint8_t slot)
{
    GroveLCD_WriteChar(&lcd, (char)slot);
}

void App_Init(void)
{
    /* Initialize the button module (debouncing and edge detection). */
    Button_Init();

    /* Enter initial state and render the welcome screen. */
    appState = APP_STATE_WELCOME;
    DisplayWelcomeScreen();
}

void App_Update(void)
{
    /* -------- OK button: select/confirm -------- */
    if (Button_WasPressed(BUTTON_OK))
    {
        switch (appState)
        {
            case APP_STATE_WELCOME:
                /* From welcome -> main menu */
                appState = APP_STATE_MENU_MAIN;
                mainMenuIndex = 0;
                DisplayMainMenu();
                break;

            case APP_STATE_MENU_MAIN:
                /* Enter selected submenu */
                if (mainMenuIndex == 0) {
                    appState = APP_STATE_VIEW_LEGEND;
                    DisplayNotesLegend();
                } else if (mainMenuIndex == 1) {
                    appState = APP_STATE_MENU_SONGS;
                    songListIndex = 0;
                    DisplaySongsList();
                } else {
                    appState = APP_STATE_MENU_CHORDPACKS;
                    chordPackIndex = 0;
                    DisplayChordPacksList();
                }
                break;

            case APP_STATE_MENU_SONGS:
                /* Start a song lesson for the currently selected song */
                appState = APP_STATE_LESSON_SONG;
                Lesson_StartSong(&songs[songListIndex]);
                break;

            case APP_STATE_MENU_CHORDPACKS:
                /* Start a chord exercise for the currently selected pack */
                appState = APP_STATE_LESSON_CHORD;
                Lesson_StartChordExercise(&chordPacks[chordPackIndex]);
                break;

            case APP_STATE_VIEW_LEGEND:
                /* OK does nothing here (RESET goes back) */
                break;

            case APP_STATE_LESSON_SONG:
            case APP_STATE_LESSON_CHORD:
                /* Forward button input to the lesson engine */
                Lesson_HandleInput(LESSON_INPUT_BTN_OK);

                /* If the lesson ended, return to the list from which we started */
                if (!Lesson_IsActive())
                {
                    if (appState == APP_STATE_LESSON_SONG) {
                        appState = APP_STATE_MENU_SONGS;
                        DisplaySongsList();
                    } else {
                        appState = APP_STATE_MENU_CHORDPACKS;
                        DisplayChordPacksList();
                    }
                }
                break;

            default:
                break;
        }
    }

    /* -------- NEXT button: navigate down / next item -------- */
    if (Button_WasPressed(BUTTON_NEXT))
    {
        switch (appState)
        {
            case APP_STATE_MENU_MAIN:
                /* Cycle through 3 main-menu items */
                mainMenuIndex = (mainMenuIndex + 1) % 3;
                DisplayMainMenu();
                break;

            case APP_STATE_MENU_SONGS:
                /* Cycle through songs (if any) */
                if (SONG_COUNT > 0) {
                    songListIndex = (songListIndex + 1) % SONG_COUNT;
                    DisplaySongsList();
                }
                break;

            case APP_STATE_MENU_CHORDPACKS:
                /* Cycle through chord packs (if any) */
                if (CHORD_PACK_COUNT > 0) {
                    chordPackIndex = (chordPackIndex + 1) % CHORD_PACK_COUNT;
                    DisplayChordPacksList();
                }
                break;

            case APP_STATE_VIEW_LEGEND:
                /* Single screen -> ignore */
                break;

            case APP_STATE_LESSON_SONG:
            case APP_STATE_LESSON_CHORD:
                /* Forward button input to the lesson engine */
                Lesson_HandleInput(LESSON_INPUT_BTN_NEXT);

                /* If the lesson ended, return to the list */
                if (!Lesson_IsActive())
                {
                    if (appState == APP_STATE_LESSON_SONG) {
                        appState = APP_STATE_MENU_SONGS;
                        DisplaySongsList();
                    } else {
                        appState = APP_STATE_MENU_CHORDPACKS;
                        DisplayChordPacksList();
                    }
                }
                break;

            default:
                break;
        }
    }

    /* -------- RESET button: back / cancel -------- */
    if (Button_WasPressed(BUTTON_RESET))
    {
        switch (appState)
        {
            case APP_STATE_MENU_MAIN:
                /* RESET is ignored in the main menu */
                break;

            case APP_STATE_MENU_SONGS:
            case APP_STATE_MENU_CHORDPACKS:
            case APP_STATE_VIEW_LEGEND:
                /* Back to main menu */
                appState = APP_STATE_MENU_MAIN;
                DisplayMainMenu();
                break;

            case APP_STATE_LESSON_SONG:
            case APP_STATE_LESSON_CHORD:
                /* Forward reset/cancel to the lesson engine */
                Lesson_HandleInput(LESSON_INPUT_BTN_RESET);

                /* If the lesson ended, return to the list */
                if (!Lesson_IsActive())
                {
                    if (appState == APP_STATE_LESSON_SONG) {
                        appState = APP_STATE_MENU_SONGS;
                        DisplaySongsList();
                    } else {
                        appState = APP_STATE_MENU_CHORDPACKS;
                        DisplayChordPacksList();
                    }
                }
                break;

            default:
                break;
        }
    }

    /* Periodic lesson update (e.g., non-blocking LED timing, timeouts). */
    Lesson_Update();
}

/* --- Screen rendering functions --- */

static void DisplayWelcomeScreen(void)
{
    GroveLCD_Clear(&lcd);
    LCD_ClearRow(0);
    LCD_ClearRow(1);

    GroveLCD_SetCursor(&lcd, 0, 3);
    GroveLCD_Print(&lcd, "Welcome to");
    GroveLCD_SetCursor(&lcd, 1, 4);
    GroveLCD_Print(&lcd, "KeyGuide");
}

static void DisplayMainMenu(void)
{
    GroveLCD_Clear(&lcd);
    LCD_ClearRow(0);
    LCD_ClearRow(1);

    GroveLCD_SetCursor(&lcd, 0, 0);

    /* Print the currently selected main-menu entry */
    if (mainMenuIndex == 0) GroveLCD_Print(&lcd, "Notes symbol");
    else if (mainMenuIndex == 1) GroveLCD_Print(&lcd, "Songs");
    else GroveLCD_Print(&lcd, "Basic chords");

    /* Optional header label on the right side */
    GroveLCD_SetCursor(&lcd, 0, 12);
    GroveLCD_Print(&lcd, "MENU");

    /* Hint line (must fit in 16 columns) */
    GroveLCD_SetCursor(&lcd, 1, 0);
    GroveLCD_Print(&lcd, "NEXT=Down OK=Sel");
}

static void DisplayNotesLegend(void)
{
    GroveLCD_Clear(&lcd);
    LCD_ClearRow(0);
    LCD_ClearRow(1);

    /* Example: show sharp + flat and some duration icons */
    GroveLCD_SetCursor(&lcd, 0, 0);
    GroveLCD_Print(&lcd, "A");
    LCD_WriteCustom(5);          /* sharp */
    GroveLCD_Print(&lcd, "  B");
    LCD_WriteCustom(6);          /* flat */

    GroveLCD_SetCursor(&lcd, 1, 0);
    /* Show duration icons 0..4 */
    LCD_WriteCustom(0);
    LCD_WriteCustom(1);
    LCD_WriteCustom(2);
    LCD_WriteCustom(3);
    LCD_WriteCustom(4);
    GroveLCD_Print(&lcd, "  RESET=Back");
}

static void DisplaySongsList(void)
{
    GroveLCD_Clear(&lcd);
    LCD_ClearRow(0);
    LCD_ClearRow(1);

    GroveLCD_SetCursor(&lcd, 0, 0);
    GroveLCD_Print(&lcd, "Songs");

    GroveLCD_SetCursor(&lcd, 1, 0);
    if (SONG_COUNT > 0) {
        GroveLCD_Print(&lcd, "> ");
        /* No explicit truncation; LCD will stop at 16 characters. */
        GroveLCD_Print(&lcd, songs[songListIndex].title);
    } else {
        GroveLCD_Print(&lcd, "<no songs>");
    }
}

static void DisplayChordPacksList(void)
{
    GroveLCD_Clear(&lcd);
    LCD_ClearRow(0);
    LCD_ClearRow(1);

    GroveLCD_SetCursor(&lcd, 0, 0);
    GroveLCD_Print(&lcd, "Chord packs");

    GroveLCD_SetCursor(&lcd, 1, 0);
    if (CHORD_PACK_COUNT > 0) {
        GroveLCD_Print(&lcd, "> ");
        GroveLCD_Print(&lcd, chordPacks[chordPackIndex].packName);
    } else {
        GroveLCD_Print(&lcd, "<no packs>");
    }
}
