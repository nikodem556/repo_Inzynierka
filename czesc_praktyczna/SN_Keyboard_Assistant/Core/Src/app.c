#include "app.h"
#include "grove_lcd16x2_i2c.h"
#include <stdint.h>
#include <string.h>

/* LCD instance lives in main.c */
extern GroveLCD_t lcd;

/* Current application state and menu indices */
static AppState appState;
static uint8_t mainMenuIndex = 0;
static uint8_t songListIndex = 0;
static uint8_t chordPackIndex = 0;

/* Forward declarations */
static void DisplayWelcomeScreen(void);
static void DisplayMainMenu(void);
static void DisplayNotesLegend(void);
static void DisplaySongsList(void);
static void DisplayChordPacksList(void);

static void LCD_ClearRow(uint8_t row)
{
    GroveLCD_SetCursor(&lcd, row, 0);
    GroveLCD_Print(&lcd, "                "); // 16 spaces
}

static void LCD_WriteCustom(uint8_t slot)
{
    GroveLCD_WriteChar(&lcd, (char)slot);
}

void App_Init(void)
{
    /* Buttons init (if you have it in your button module) */
    Button_Init();

    /* LCD is initialized in main.c with GroveLCD_Init() */
    appState = APP_STATE_WELCOME;
    DisplayWelcomeScreen();
}

void App_Update(void)
{
    /* OK button */
    if (Button_WasPressed(BUTTON_OK))
    {
        switch (appState)
        {
            case APP_STATE_WELCOME:
                appState = APP_STATE_MENU_MAIN;
                mainMenuIndex = 0;
                DisplayMainMenu();
                break;

            case APP_STATE_MENU_MAIN:
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
                appState = APP_STATE_LESSON_SONG;
                Lesson_StartSong(&songs[songListIndex]);
                break;

            case APP_STATE_MENU_CHORDPACKS:
                appState = APP_STATE_LESSON_CHORD;
                Lesson_StartChordExercise(&chordPacks[chordPackIndex]);
                break;

            case APP_STATE_VIEW_LEGEND:
                /* OK does nothing here (RESET goes back) */
                break;

            case APP_STATE_LESSON_SONG:
            case APP_STATE_LESSON_CHORD:
                Lesson_HandleInput(LESSON_INPUT_BTN_OK);

                if (!Lesson_IsActive())
                {
                    /* Lesson ended -> go back to corresponding list */
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

    /* NEXT button */
    if (Button_WasPressed(BUTTON_NEXT))
    {
        switch (appState)
        {
            case APP_STATE_MENU_MAIN:
                mainMenuIndex = (mainMenuIndex + 1) % 3;
                DisplayMainMenu();
                break;

            case APP_STATE_MENU_SONGS:
                if (SONG_COUNT > 0) {
                    songListIndex = (songListIndex + 1) % SONG_COUNT;
                    DisplaySongsList();
                }
                break;

            case APP_STATE_MENU_CHORDPACKS:
                if (CHORD_PACK_COUNT > 0) {
                    chordPackIndex = (chordPackIndex + 1) % CHORD_PACK_COUNT;
                    DisplayChordPacksList();
                }
                break;

            case APP_STATE_VIEW_LEGEND:
                /* single screen -> ignore */
                break;

            case APP_STATE_LESSON_SONG:
            case APP_STATE_LESSON_CHORD:
                Lesson_HandleInput(LESSON_INPUT_BTN_NEXT);

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

    /* RESET button */
    if (Button_WasPressed(BUTTON_RESET))
    {
        switch (appState)
        {
            case APP_STATE_MENU_MAIN:
                /* ignored in main menu */
                break;

            case APP_STATE_MENU_SONGS:
            case APP_STATE_MENU_CHORDPACKS:
            case APP_STATE_VIEW_LEGEND:
                appState = APP_STATE_MENU_MAIN;
                DisplayMainMenu();
                break;

            case APP_STATE_LESSON_SONG:
            case APP_STATE_LESSON_CHORD:
                Lesson_HandleInput(LESSON_INPUT_BTN_RESET);

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
}

/* --- Screens --- */

static void DisplayWelcomeScreen(void)
{
    GroveLCD_Clear(&lcd);
    LCD_ClearRow(0);
    LCD_ClearRow(1);

    GroveLCD_SetCursor(&lcd, 0, 0);
    GroveLCD_Print(&lcd, "Welcome to KeyGuide");
    GroveLCD_SetCursor(&lcd, 1, 0);
    GroveLCD_Print(&lcd, "press OK");
}

static void DisplayMainMenu(void)
{
    GroveLCD_Clear(&lcd);
    LCD_ClearRow(0);
    LCD_ClearRow(1);

    GroveLCD_SetCursor(&lcd, 0, 0);

    if (mainMenuIndex == 0) GroveLCD_Print(&lcd, "Notes symbol");
    else if (mainMenuIndex == 1) GroveLCD_Print(&lcd, "Songs");
    else GroveLCD_Print(&lcd, "Basic chords");

    /* "MENU" in top-right if possible (col 12) */
    GroveLCD_SetCursor(&lcd, 0, 12);
    GroveLCD_Print(&lcd, "MENU");

    /* Short hint line (must fit 16 cols) */
    GroveLCD_SetCursor(&lcd, 1, 0);
    GroveLCD_Print(&lcd, "NEXT=Down OK=Sel");
}

static void DisplayNotesLegend(void)
{
    GroveLCD_Clear(&lcd);
    LCD_ClearRow(0);
    LCD_ClearRow(1);

    /* Example: show sharp + flat and some length icons */
    GroveLCD_SetCursor(&lcd, 0, 0);
    GroveLCD_Print(&lcd, "A");
    LCD_WriteCustom(5);          /* sharp */
    GroveLCD_Print(&lcd, "  B");
    LCD_WriteCustom(6);          /* flat */

    GroveLCD_SetCursor(&lcd, 1, 0);
    /* Show length icons 0..4 */
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
        /* (Not truncating here; LCD will just stop at 16 chars.) */
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
