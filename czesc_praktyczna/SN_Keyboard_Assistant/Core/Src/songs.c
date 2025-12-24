#include "songs.h"
#include <stdint.h>

/* Note length icon indices (0=whole,1=half,2=quarter,3=eighth,4=sixteenth) */
#define LEN_WHOLE      0
#define LEN_HALF       1
#define LEN_QUARTER    2
#define LEN_EIGHTH     3
#define LEN_SIXTEENTH  4

/* "Twinkle Twinkle Little Star" (first phrase) */
static SongStep twinkleSteps[] = {
    { 2, { { 'C', ACC_NONE, 60, LEN_QUARTER }, { 'C', ACC_NONE, 60, LEN_QUARTER } } },
    { 2, { { 'G', ACC_NONE, 67, LEN_QUARTER }, { 'G', ACC_NONE, 67, LEN_QUARTER } } },
    { 2, { { 'A', ACC_NONE, 69, LEN_QUARTER }, { 'A', ACC_NONE, 69, LEN_QUARTER } } },
    { 1, { { 'G', ACC_NONE, 67, LEN_HALF    } } }
};

/* "Mary Had a Little Lamb" (first phrases) */
static SongStep marySteps[] = {
    { 2, { { 'E', ACC_NONE, 64, LEN_QUARTER }, { 'D', ACC_NONE, 62, LEN_QUARTER } } },
    { 2, { { 'C', ACC_NONE, 60, LEN_QUARTER }, { 'D', ACC_NONE, 62, LEN_QUARTER } } },
    { 3, { { 'E', ACC_NONE, 64, LEN_QUARTER }, { 'E', ACC_NONE, 64, LEN_QUARTER }, { 'E', ACC_NONE, 64, LEN_HALF } } }
};

/*
 * Exported songs registry.
 * IMPORTANT: direct initialization is a constant initializer.
 */
Song songs[] = {
    { "Twinkle Twinkle",
      (uint8_t)(sizeof(twinkleSteps) / sizeof(twinkleSteps[0])),
      twinkleSteps },

    { "Mary Had a Lamb",
      (uint8_t)(sizeof(marySteps) / sizeof(marySteps[0])),
      marySteps }
};

const uint8_t SONG_COUNT = (uint8_t)(sizeof(songs) / sizeof(songs[0]));
