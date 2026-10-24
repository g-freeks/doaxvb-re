#ifndef DOAXBV_RECOMP_BUTTON_HINT_H
#define DOAXBV_RECOMP_BUTTON_HINT_H

#include <stdbool.h>
#include <stdint.h>

#include "overlay_text.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Which button does what in the game being played: a key panel for a
   volleyball match a person plays and for the Hopping game, one of the
   extra overlays (RECOMP_EXTRA_OVERLAYS=1). */
typedef enum RecompHintGame {
    RECOMP_HINT_NONE,
    RECOMP_HINT_VOLLEYBALL,
    RECOMP_HINT_HOPPING
} RecompHintGame;

/* One controller's layout as Controller Settings stores it. Digital gives
   A, B, X and Y one action each; analog gives A and B one each, its
   strength set by how hard the button is pressed. */
enum {
    RECOMP_ACTION_NONE = 0,
    RECOMP_ACTION_ATTACK = 1,
    RECOMP_ACTION_RECEIVE = 2,
    RECOMP_ACTION_ATTACK_STRONG = 3,
    RECOMP_ACTION_RECEIVE_STRONG = 4,
    RECOMP_ACTION_ATTACK_SOFT = 5,
    RECOMP_ACTION_RECEIVE_SOFT = 6
};

typedef struct RecompPadLayout {
    bool analog;
    uint8_t digital[4]; /* A, B, X, Y */
    uint8_t analog_ab[2]; /* A, B */
} RecompPadLayout;

/* What to show for a game with a layout: nothing for RECOMP_HINT_NONE;
   otherwise the Back reminder, and the key rows only when expanded. */
void recomp_button_hint_fill(RecompOverlayKeys *keys, RecompHintGame game, const RecompPadLayout *layout,
    bool expanded);

/* Once a presented frame: Back shows or hides the rows (hidden at first);
   publishes the panel when what it shows changes and, with
   RECOMP_AUTO_LOG=1, an [auto:hint] line. Read-only. */
void recomp_button_hint_frame(void);

#ifdef __cplusplus
}
#endif

#endif
