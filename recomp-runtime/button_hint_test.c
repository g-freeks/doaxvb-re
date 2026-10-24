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

    recomp_button_hint_fill(&keys, RECOMP_HINT_NONE, &layout, true);
    check(keys.count == 0u && keys.tip[0] == '\0', "no game, nothing shown");

    /* Hidden at first: only the reminder of how to show the rows. */
    recomp_button_hint_fill(&keys, RECOMP_HINT_VOLLEYBALL, &layout, false);
    check(keys.count == 0u && strcmp(keys.tip_key, "Back") == 0 && strcmp(keys.tip, "Controls") == 0,
        "collapsed shows the Back reminder only");

    /* The game's default digital layout: a button for each action, then
       the sticks, and the reminder says how to hide them. */
    recomp_button_hint_fill(&keys, RECOMP_HINT_VOLLEYBALL, &layout, true);
    check(keys.count == 6u, "digital shows four buttons and two stick rows");
    check(strcmp(keys.tip, "Hide controls") == 0, "expanded says how to hide");
    /* As on the pad: Y, X, B, A from the top, Y and B indented. */
    check(keys.key[0].image == RECOMP_ICON_BUTTON + 3u && strcmp(keys.key[0].action, "Receive (Soft)") == 0 &&
        keys.key[0].indent, "Y comes first, indented, and receives softly");
    check(keys.key[1].image == RECOMP_ICON_BUTTON + 2u && !keys.key[1].indent, "X second, flush");
    check(keys.key[2].image == RECOMP_ICON_BUTTON + 1u && keys.key[2].indent, "B third, indented");
    check(keys.key[3].image == RECOMP_ICON_BUTTON && strcmp(keys.key[3].action, "Attack (Strong)") == 0 &&
        !keys.key[3].indent, "A last, flush, and attacks strongly");
    check(keys.key[4].image == RECOMP_ICON_COUNT && strcmp(keys.key[4].name, "L") == 0 && !keys.key[4].indent,
        "a stick row has no button image and is not indented");

    /* The Hopping game follows the same layout's strengths. */
    recomp_button_hint_fill(&keys, RECOMP_HINT_HOPPING, &layout, true);
    check(keys.count == 5u && strcmp(keys.key[0].action, "Short jump") == 0 &&
        keys.key[0].image == RECOMP_ICON_BUTTON + 3u && strcmp(keys.key[3].action, "Long jump") == 0 &&
        keys.key[3].image == RECOMP_ICON_BUTTON, "digital hopping: in pad order, strong long, soft short");

    /* A remapped button and an unassigned one. */
    layout.digital[0] = 4;
    layout.digital[2] = 0;
    recomp_button_hint_fill(&keys, RECOMP_HINT_VOLLEYBALL, &layout, true);
    check(keys.count == 5u && strcmp(keys.key[2].action, "Receive (Strong)") == 0 &&
        keys.key[2].image == RECOMP_ICON_BUTTON, "a remapped A follows the settings");
    check(keys.key[0].image == RECOMP_ICON_BUTTON + 3u && keys.key[1].image == RECOMP_ICON_BUTTON + 1u,
        "an unassigned X is left out and the others keep their places");

    /* Analog: A and B only, and hopping on A by pressure. */
    layout.analog = true;
    recomp_button_hint_fill(&keys, RECOMP_HINT_VOLLEYBALL, &layout, true);
    check(keys.count == 4u && strcmp(keys.key[0].action, "Receive") == 0 && keys.key[0].indent &&
        strcmp(keys.key[1].action, "Attack") == 0 && !keys.key[1].indent, "analog shows B receive over A attack");
    recomp_button_hint_fill(&keys, RECOMP_HINT_HOPPING, &layout, true);
    check(keys.count == 2u && keys.key[0].image == RECOMP_ICON_BUTTON && strcmp(keys.key[0].action, "Jump") == 0,
        "analog hopping jumps with A");

    /* Out-of-range action codes are not shown. */
    layout.analog = false;
    memset(layout.digital, 0xEE, sizeof layout.digital);
    recomp_button_hint_fill(&keys, RECOMP_HINT_VOLLEYBALL, &layout, true);
    check(keys.count == 2u, "unknown action codes are left out");

    /* The reminder alone keeps the overlay from being empty. */
    RecompOverlayText text;
    recomp_overlay_clear(&text);
    recomp_button_hint_fill(&text.keys, RECOMP_HINT_HOPPING, &layout, false);
    check(!recomp_overlay_empty(&text), "a reminder alone is shown");

    if (failures == 0) printf("button hint test passed\n");
    return failures == 0 ? 0 : 1;
}
