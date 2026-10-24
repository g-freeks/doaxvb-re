#include "button_hint.h"

#include "island_icons.h"

#include <stdio.h>
#include <string.h>

static int failures;

static void check(bool condition, const char *what)
{
    if (!condition) {
        fprintf(stderr, "button hint test failed: %s\n", what);
        ++failures;
    }
}

int main(void)
{
    RecompOverlayKeys keys;
    RecompPadLayout layout = {false, {3, 4, 5, 6}, {1, 2}};

    recomp_button_hint_fill(&keys, RECOMP_HINT_NONE, &layout);
    check(keys.count == 0u, "no game, no keys");

    /* The game's default digital layout: a button for each action, then
       the sticks. */
    recomp_button_hint_fill(&keys, RECOMP_HINT_VOLLEYBALL, &layout);
    check(keys.count == 6u, "digital shows four buttons and two stick rows");
    check(keys.key[0].image == RECOMP_ICON_BUTTON && strcmp(keys.key[0].action, "Attack (Strong)") == 0,
        "A attacks strongly");
    check(keys.key[3].image == RECOMP_ICON_BUTTON + 3u && strcmp(keys.key[3].action, "Receive (Soft)") == 0,
        "Y receives softly");
    check(keys.key[4].image == RECOMP_ICON_COUNT && strcmp(keys.key[4].name, "L") == 0,
        "a stick row has no button image");

    /* A remapped button and an unassigned one. */
    layout.digital[0] = 4;
    layout.digital[2] = 0;
    recomp_button_hint_fill(&keys, RECOMP_HINT_VOLLEYBALL, &layout);
    check(keys.count == 5u && strcmp(keys.key[0].action, "Receive (Strong)") == 0,
        "a remapped A follows the settings and an unassigned X is left out");
    check(keys.key[2].image == RECOMP_ICON_BUTTON + 3u, "the rows after it keep their buttons");

    /* Analog: A and B only. */
    layout.analog = true;
    recomp_button_hint_fill(&keys, RECOMP_HINT_VOLLEYBALL, &layout);
    check(keys.count == 4u && strcmp(keys.key[0].action, "Attack") == 0 &&
        strcmp(keys.key[1].action, "Receive") == 0, "analog shows A attack and B receive");

    recomp_button_hint_fill(&keys, RECOMP_HINT_HOPPING, NULL);
    check(keys.count == 2u && keys.key[0].image == RECOMP_ICON_BUTTON && strcmp(keys.key[0].action, "Jump") == 0,
        "the Hopping game jumps with A");

    /* Out-of-range action codes are not shown. */
    layout.analog = false;
    memset(layout.digital, 0xEE, sizeof layout.digital);
    recomp_button_hint_fill(&keys, RECOMP_HINT_VOLLEYBALL, &layout);
    check(keys.count == 2u, "unknown action codes are left out");

    if (failures == 0) printf("button hint test passed\n");
    return failures == 0 ? 0 : 1;
}
