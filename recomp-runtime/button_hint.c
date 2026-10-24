#include "button_hint.h"

#include "island_icons.h"

#include <string.h>

/* The game's names for the actions, as Controller Settings shows them, and
   what each does in a rally, from the community guides on GameFAQs. */
static const struct {
    const char *action;
    const char *detail;
} actions[] = {
    [RECOMP_ACTION_ATTACK] = {"Attack", "spike, serve; tap to tip"},
    [RECOMP_ACTION_RECEIVE] = {"Receive", "pass; hard to set"},
    [RECOMP_ACTION_ATTACK_STRONG] = {"Attack (Strong)", "spike, serve, block"},
    [RECOMP_ACTION_RECEIVE_STRONG] = {"Receive (Strong)", "set at the net"},
    [RECOMP_ACTION_ATTACK_SOFT] = {"Attack (Soft)", "tip over a block"},
    [RECOMP_ACTION_RECEIVE_SOFT] = {"Receive (Soft)", "pass to partner"},
};

static const char *const button_names[4] = {"A", "B", "X", "Y"};

static void add_button(RecompOverlayKeys *keys, unsigned button, uint8_t action)
{
    if (action == RECOMP_ACTION_NONE || action >= sizeof actions / sizeof actions[0] ||
        actions[action].action == NULL) {
        return;
    }
    recomp_overlay_add_key(keys, RECOMP_ICON_BUTTON + button, button_names[button], actions[action].action,
        actions[action].detail);
}

void recomp_button_hint_fill(RecompOverlayKeys *keys, RecompHintGame game, const RecompPadLayout *layout)
{
    memset(keys, 0, sizeof *keys);
    if (game == RECOMP_HINT_VOLLEYBALL && layout != NULL) {
        if (layout->analog) {
            for (unsigned button = 0; button < 2u; ++button) add_button(keys, button, layout->analog_ab[button]);
        } else {
            for (unsigned button = 0; button < 4u; ++button) add_button(keys, button, layout->digital[button]);
        }
        recomp_overlay_add_key(keys, RECOMP_ICON_COUNT, "L", "Move, aim", "serve: back low, forward jump");
        recomp_overlay_add_key(keys, RECOMP_ICON_COUNT, "R", "Partner", "forward net, back deep");
    } else if (game == RECOMP_HINT_HOPPING) {
        recomp_overlay_add_key(keys, RECOMP_ICON_BUTTON, "A", "Jump", "harder jumps farther");
        recomp_overlay_add_key(keys, RECOMP_ICON_BUTTON, "A", "Chain", "press as your foot lands");
    }
}

#ifdef RECOMP_FULL_PROGRAM
#include "kernel_abi.h"
#include "runtime.h"

#include <stdio.h>
#include "ui_log.h"

/* Guest state, for the verified USA executable. The match fields are in
   docs/title-idle.md, the message panel's in the game UI research notes;
   the controller settings were found by changing them on Controller
   Settings and comparing memory (docs/button-hint.md).

   Guest memory is never written. */
enum {
    SCENE = 0x005DECE0u,
    SCENE_MATCH = 0,
    SCENE_ISLAND = 1,
    PLAYER_IS_PERSON = 0x005DECE4u, /* four bytes */
    PLAYER_PORT = 0x005DECE8u, /* four bytes */
    RESULTS_OPEN = 0x005D89ECu,
    MATCH_OVER = 0x004D6FC8u,
    ISLAND_PLACE = 0x004D72E3u,
    PLACE_HOPPING = 16,
    /* The message panel's queued-record count (0x5C5BA0 + 0x431): nonzero
       while the game shows its own instructions, such as the Hopping
       game's before START. */
    MESSAGES_QUEUED = 0x005C5FD1u,
    /* A change shows once it has held this many frames, so a screen that
       queues its instructions a few frames after it opens does not flash
       the panel first. */
    SETTLE_FRAMES = 20,
    PAD_ANALOG = 0x0041655Du, /* a byte per port */
    PAD_LAYOUT = 0x00416570u, /* 8 bytes per port: +1 A B X Y digital, +5 A B analog */
    PAD_LAYOUT_STRIDE = 8,
    PORTS = 4
};

static unsigned frame;

static uint8_t byte_at(uint32_t address)
{
    return *recomp_memory(address, 1);
}

/* The game on screen and, for a match, the port of the person playing. */
static RecompHintGame current_game(unsigned *port)
{
    const uint8_t scene = byte_at(SCENE);

    *port = 0;
    if (byte_at(MESSAGES_QUEUED) != 0u) {
        return RECOMP_HINT_NONE;
    }
    if (scene == SCENE_ISLAND && byte_at(ISLAND_PLACE) == PLACE_HOPPING) {
        return RECOMP_HINT_HOPPING;
    }
    if (scene != SCENE_MATCH || byte_at(RESULTS_OPEN) != 0u || byte_at(MATCH_OVER) != 0u) {
        return RECOMP_HINT_NONE;
    }
    for (unsigned player = 0; player < PORTS; ++player) {
        if (byte_at(PLAYER_IS_PERSON + player) != 0u && byte_at(PLAYER_PORT + player) < PORTS) {
            *port = byte_at(PLAYER_PORT + player);
            return RECOMP_HINT_VOLLEYBALL;
        }
    }
    return RECOMP_HINT_NONE;
}

static void read_layout(unsigned port, RecompPadLayout *layout)
{
    const uint32_t record = PAD_LAYOUT + port * PAD_LAYOUT_STRIDE;

    layout->analog = byte_at(PAD_ANALOG + port) != 0u;
    for (unsigned i = 0; i < 4u; ++i) layout->digital[i] = byte_at(record + 1u + i);
    for (unsigned i = 0; i < 2u; ++i) layout->analog_ab[i] = byte_at(record + 5u + i);
}

static void log_keys(RecompHintGame game, unsigned port, const RecompPadLayout *layout, const RecompOverlayKeys *keys)
{
    fprintf(stderr, "[auto:hint] frame %u: ", frame);
    if (game == RECOMP_HINT_NONE) {
        fprintf(stderr, "none\n");
        return;
    }
    if (game == RECOMP_HINT_VOLLEYBALL) {
        fprintf(stderr, "volleyball port %u %s:", port, layout->analog ? "analog" : "digital");
    } else {
        fprintf(stderr, "hopping:");
    }
    for (unsigned i = 0; i < keys->count; ++i) {
        fprintf(stderr, "%s %s %s", i == 0u ? "" : ",", keys->key[i].name, keys->key[i].action);
    }
    fprintf(stderr, "\n");
}

void recomp_button_hint_frame(void)
{
    static RecompOverlayKeys shown, pending;
    static unsigned held;
    static bool logged;
    RecompPadLayout layout;
    RecompOverlayKeys keys;
    RecompHintGame game;
    unsigned port;

    if (!recomp_extra_overlays_enabled()) {
        return;
    }
    ++frame;
    game = current_game(&port);
    memset(&layout, 0, sizeof layout);
    if (game == RECOMP_HINT_VOLLEYBALL) {
        read_layout(port, &layout);
    }
    recomp_button_hint_fill(&keys, game, &layout);
    if (memcmp(&keys, &pending, sizeof keys) != 0) {
        pending = keys;
        held = 0;
    }
    if (++held < SETTLE_FRAMES || (logged && memcmp(&keys, &shown, sizeof keys) == 0)) {
        return;
    }
    if (keys.count != 0u) {
        recomp_island_icons_publish(recomp_disc_root_path);
    }
    recomp_overlay_publish_keys(&keys);
    shown = keys;
    if (recomp_ui_log_enabled() && (logged || keys.count != 0u)) {
        log_keys(game, port, &layout, &keys);
    }
    logged = true;
}
#else
void recomp_button_hint_frame(void) {}
#endif
