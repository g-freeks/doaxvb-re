#include "collection_log.h"

#ifdef RECOMP_FULL_PROGRAM
#include "gift_reactions.h"
#include "island_icons.h"
#include "kernel_abi.h"
#include "overlay_text.h"
#include "runtime.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Item ownership log, opt-in with RECOMP_COLLECTION_LOG=1, and the shop's
   ownership panel, one of the extra overlays (RECOMP_EXTRA_OVERLAYS=1).

   The loaded save keeps one word per item, 708 words at 0x9D8DCE (file
   offset 0x1646 in the save). A girl has two bits in a word, at twice her
   index: the low one says she has had the item, the high one that it is in
   her Collection. Exhibition's rebuild at 0x000F82F0 loads each save in turn
   and reads this table, which is how the address was found; that it holds
   the active vacation's ownership during play is what this log checks.

   Each girl also has a room inventory: 51 slots of 8 bytes at 0x9D7810,
   0x198 bytes per girl (file offset 0x88; the save is a flat image of guest
   memory from 0x9D7788). A slot holds the item id (0xFFFF when empty), a
   signed word, a word whose high byte orders the girl's acquisitions, and a
   wrapping item id. Only the item id and the order are understood. That
   copy is the save's: during play the game keeps each girl's inventory in
   51 records of 16 bytes at *(0x317634) + girl * 0x330 (the slot's eight
   bytes first), as 0x0009CEA0 adds to it, and a purchase shows there at
   once but reaches 0x9D7810 only later. The log and the panel read the
   live records, and the save's copy while those are not set up.

   Item names come from the table of pointers at 0x3095F8, one per item id,
   each to six 8-byte entries indexed by the language byte at 0x4D56D9; the
   first word of an entry points to the text. "@|" in a name marks a line
   break.

   Once a presented frame the ownership table and the inventories are
   compared with the previous frame's copy and each change is printed;
   frames are counted from the first logged one. When many entries change at
   once (a save being loaded), one summary line with per-girl counts is
   printed instead.

   The shop: 0x000C8840 draws one row of the shop list; its first argument
   is the row and its second the screen context. program_manual.c reports
   the context here before calling the original. Row r shows the item in
   the first word of the 16-byte entry r of shop *(0x3AF8A1) (15 entries a
   shop) in the table at *(0x317638). The selected entry comes from the
   scrolling window in the state at context+0x18 (see shop_selection).
   Whenever the item under the cursor changes, the log prints every girl's
   ownership of it and whether her inventory holds it.

   The panel shows the same over the shop's preview through the overlay's
   status slot: along the preview's bottom the playing girl's heart (the
   item is in her Collection) and present (her room holds it), and down its
   right edge the other girls' portraits, each opaque when the item is in
   her Collection; an inactive icon is three quarters transparent. The
   playing girl's character id is at 0x9D9939; the game's 0x000EF330 maps it
   to a girl through the byte table at 0xEF394 (8 for none). A presenter
   that cannot draw icons shows the same as two lines of text along the
   screen's bottom. Guest memory is never written. */
enum {
    OWNERSHIP = 0x009D8DCEu,
    INVENTORY = 0x009D7810u,
    INVENTORY_STRIDE = 0x198,
    LIVE_INVENTORIES = 0x00317634u,
    LIVE_STRIDE = 0x330,
    LIVE_RECORD = 16,
    SLOT_COUNT = 51,
    EMPTY_SLOT = 0xFFFF,
    ITEM_TABLE = 0x003003E8u,
    ITEM_NAMES = 0x003095F8u,
    LANGUAGE = 0x004D56D9u,
    LANGUAGE_COUNT = 6,
    SHOP_NUMBER = 0x003AF8A1u,
    SHOP_TABLE = 0x00317638u,
    SHOP_ROWS = 15,
    SHOP_ENTRY_SIZE = 16,
    ITEM_COUNT = 0x2C4,
    GIRL_COUNT = 8,
    HAD = 1,
    IN_COLLECTION = 2,
    DETAIL_LIMIT = 16,
    SHOP_GRACE_FRAMES = 30,
    PLAYING_CHARACTER = 0x009D9939u,
    CHARACTER_GIRLS = 0x000EF394u,
    CHARACTER_COUNT = 20,
    /* Buttons held on any controller (docs/title-idle.md); a click of the
       left thumbstick toggles the gift reactions. */
    BUTTONS_HELD = 0x005F3138u,
    BUTTON_LEFT_THUMB = 0x0040u,
    /* The item grids (the items screen's tabs, a gift's presents) list
       inventory slots: list t has s8 GRID_COUNTS[t] entries starting at
       u8 GRID_STARTS[t] in the slot table GRID_SLOTS; GRID_LIST is the list
       on screen and GRID_CURSOR its cursor. 0x000E12B0 reads them so. */
    GRID_LIST = 0x00B5B209u,
    GRID_COUNTS = 0x00B5B1A8u,
    GRID_STARTS = 0x00B5B1B7u,
    GRID_SLOTS = 0x00B5B148u,
    GRID_SLOT_COUNT = 0x30,
    GRID_LISTS = 15,
    GRID_CURSOR = 0x0031BE04u,
    /* The Collection lists item ids: tab t's start is the word at
       CATALOG_STARTS + 4t into the words at CATALOG (0x000C3430). */
    CATALOG_STARTS = 0x009D5B28u,
    CATALOG = 0x009D5B34u,
    CATALOG_TABS = 3,
    COLLECTION_TAB = 0x0031CC7Du,
    COLLECTION_CURSOR = 0x0031CE0Cu,
    SCENE = 0x005DECE0u,
    SCENE_ISLAND = 1,
    ISLAND_PLACE = 0x004D72E3u,
    PLACE_COLLECTION = 6
};

/* Whether the panel shows each girl's reaction to the item as a present,
   from gift_reactions.c. Hidden at first; a left thumbstick click shows
   and hides it. */
static bool reactions_shown;

/* A screen's item preview, in the game's 720 x 480 screen space: the
   shop's, which the items screen and a gift's choice of present share, and
   the Collection's smaller one. The panel's icons go inside it. */
typedef struct PreviewBox {
    float left, top, right, bottom;
} PreviewBox;

static const PreviewBox SHOP_PREVIEW = {391.5f, 35.5f, 663.5f, 299.5f};
static const PreviewBox COLLECTION_PREVIEW = {453.5f, 85.5f, 647.5f, 297.5f};
static const float SCREEN_WIDTH = 720.0f, SCREEN_HEIGHT = 480.0f;

typedef struct InventorySlot {
    uint16_t item;
    int16_t unknown;
    uint16_t order;
    uint16_t wrapping;
} InventorySlot;

static uint16_t previous[ITEM_COUNT];
static InventorySlot previous_inventory[GIRL_COUNT][SLOT_COUNT];
static bool have_previous;
static unsigned frame;
static uint32_t shop_object;
static bool shop_noted;
static unsigned shop_frame;
static int shop_item = -1;
static unsigned grid_frame;
static bool grid_noted;

static bool setting(const char *name, int *cache)
{
    if (*cache < 0) {
        const char *value = getenv(name);

        *cache = value != NULL && strcmp(value, "1") == 0;
    }
    return *cache != 0;
}

static bool log_enabled(void)
{
    static int cache = -1;

    return setting("RECOMP_COLLECTION_LOG", &cache);
}

static bool panel_enabled(void)
{
    return recomp_extra_overlays_enabled();
}

static const char *girl_name(unsigned girl)
{
    /* The game's own name table at 0x3245BC lists the girls in this order;
       play confirmed Kasumi (1), Leifang (3) and Ayane (4). */
    static const char *const names[GIRL_COUNT] = {
        "Tina", "Kasumi", "Helena", "Leifang", "Ayane", "Hitomi", "Lisa", "Christie"
    };

    return girl < GIRL_COUNT ? names[girl] : "?";
}

static unsigned item_type(uint16_t id)
{
    const uint32_t item = *recomp_memory_u32(ITEM_TABLE + id * 4u);

    return item != 0u ? *recomp_memory_u16(item) : 0u;
}

/* Copies the item's name in the game's language, without line breaks. */
static const char *item_name(uint16_t id, char *name, size_t size)
{
    const unsigned language = *recomp_memory(LANGUAGE, 1);
    const uint32_t entry = id < ITEM_COUNT ? *recomp_memory_u32(ITEM_NAMES + id * 4u) : 0u;
    const uint32_t text = entry != 0u && language < LANGUAGE_COUNT
        ? *recomp_memory_u32(entry + language * 8u) : 0u;
    size_t length = 0;

    if (text == 0u) {
        snprintf(name, size, "?");
        return name;
    }
    for (uint32_t at = text; length + 1u < size; ++at) {
        const char c = (char)*recomp_memory(at, 1);

        if (c == '\0') {
            break;
        }
        if (c == '@' && *recomp_memory(at + 1u, 1) == '|') {
            ++at;
            continue;
        }
        name[length++] = c;
    }
    name[length] = '\0';
    return name;
}

static const char *state_name(unsigned bits)
{
    static const char *const names[] = {"none", "had", "collection-only", "had+collection"};

    return names[bits & 3u];
}

static void print_summary(const uint16_t *table, const char *what)
{
    fprintf(stderr, "[auto:collection] frame %u: %s; per girl had/collection:", frame, what);
    for (unsigned girl = 0; girl < GIRL_COUNT; ++girl) {
        unsigned had = 0, collection = 0;

        for (unsigned id = 0; id < ITEM_COUNT; ++id) {
            const unsigned bits = (table[id] >> (girl * 2u)) & 3u;

            had += (bits & HAD) != 0u;
            collection += (bits & IN_COLLECTION) != 0u;
        }
        fprintf(stderr, " %u(%s)=%u/%u", girl, girl_name(girl), had, collection);
    }
    fputc('\n', stderr);
}

static void log_ownership(void)
{
    uint16_t table[ITEM_COUNT];
    unsigned changed = 0;

    memcpy(table, recomp_memory(OWNERSHIP, sizeof table), sizeof table);
    if (!have_previous) {
        print_summary(table, "first read");
        memcpy(previous, table, sizeof table);
        return;
    }
    for (unsigned id = 0; id < ITEM_COUNT; ++id) {
        changed += table[id] != previous[id];
    }
    if (changed == 0u) {
        return;
    }
    if (changed > DETAIL_LIMIT) {
        char what[32];

        snprintf(what, sizeof what, "%u items changed", changed);
        print_summary(table, what);
    } else {
        for (uint16_t id = 0; id < ITEM_COUNT; ++id) {
            if (table[id] == previous[id]) {
                continue;
            }
            for (unsigned girl = 0; girl < GIRL_COUNT; ++girl) {
                const unsigned before = (previous[id] >> (girl * 2u)) & 3u;
                const unsigned after = (table[id] >> (girl * 2u)) & 3u;

                if (before != after) {
                    char name[64];

                    fprintf(stderr, "[auto:collection] frame %u: item %u %s (type %u) girl %u (%s): %s -> %s\n",
                            frame, id, item_name(id, name, sizeof name), item_type(id), girl,
                            girl_name(girl), state_name(before), state_name(after));
                }
            }
        }
    }
    memcpy(previous, table, sizeof table);
}

static void print_inventory_summary(InventorySlot (*inventory)[SLOT_COUNT], const char *what)
{
    fprintf(stderr, "[auto:collection] frame %u: %s; per girl inventory items:", frame, what);
    for (unsigned girl = 0; girl < GIRL_COUNT; ++girl) {
        unsigned items = 0;

        for (unsigned slot = 0; slot < SLOT_COUNT; ++slot) {
            items += inventory[girl][slot].item != EMPTY_SLOT;
        }
        fprintf(stderr, " %u(%s)=%u", girl, girl_name(girl), items);
    }
    fputc('\n', stderr);
}

static void print_slot(const InventorySlot *slot)
{
    if (slot->item == EMPTY_SLOT) {
        fputs("empty", stderr);
    } else {
        char name[64];

        fprintf(stderr, "item %u %s (type %u, %d, order %u, flags 0x%02X, wrapping %u)",
                slot->item, item_name(slot->item, name, sizeof name),
                slot->item < ITEM_COUNT ? item_type(slot->item) : 0u,
                slot->unknown, slot->order >> 8, slot->order & 0xFFu, slot->wrapping);
    }
}

/* Where a girl's inventory slot is now: in the live records, or in the
   save's copy while they are not set up. */
static uint32_t slot_address(unsigned girl, unsigned slot)
{
    const uint32_t live = *recomp_memory_u32(LIVE_INVENTORIES);

    return live != 0u ? live + girl * LIVE_STRIDE + slot * LIVE_RECORD
                      : INVENTORY + girl * INVENTORY_STRIDE + slot * (uint32_t)sizeof(InventorySlot);
}

static void log_inventories(void)
{
    InventorySlot inventory[GIRL_COUNT][SLOT_COUNT];
    unsigned changed = 0;

    for (unsigned girl = 0; girl < GIRL_COUNT; ++girl) {
        for (unsigned slot = 0; slot < SLOT_COUNT; ++slot) {
            memcpy(&inventory[girl][slot], recomp_memory(slot_address(girl, slot), sizeof(InventorySlot)),
                   sizeof(InventorySlot));
        }
    }
    if (!have_previous) {
        print_inventory_summary(inventory, "first read");
        memcpy(previous_inventory, inventory, sizeof inventory);
        return;
    }
    for (unsigned girl = 0; girl < GIRL_COUNT; ++girl) {
        for (unsigned slot = 0; slot < SLOT_COUNT; ++slot) {
            changed += memcmp(&inventory[girl][slot], &previous_inventory[girl][slot], sizeof(InventorySlot)) != 0;
        }
    }
    if (changed == 0u) {
        return;
    }
    if (changed > DETAIL_LIMIT) {
        char what[32];

        snprintf(what, sizeof what, "%u slots changed", changed);
        print_inventory_summary(inventory, what);
    } else {
        for (unsigned girl = 0; girl < GIRL_COUNT; ++girl) {
            for (unsigned slot = 0; slot < SLOT_COUNT; ++slot) {
                if (memcmp(&inventory[girl][slot], &previous_inventory[girl][slot], sizeof(InventorySlot)) == 0) {
                    continue;
                }
                fprintf(stderr, "[auto:collection] frame %u: girl %u (%s) slot %u: ", frame, girl, girl_name(girl), slot);
                print_slot(&previous_inventory[girl][slot]);
                fputs(" -> ", stderr);
                print_slot(&inventory[girl][slot]);
                fputc('\n', stderr);
            }
        }
    }
    memcpy(previous_inventory, inventory, sizeof inventory);
}

static unsigned ownership(uint16_t item, unsigned girl)
{
    return (*recomp_memory_u16(OWNERSHIP + item * 2u) >> (girl * 2u)) & 3u;
}

/* Whether the girl's room inventory holds the item now. */
static bool holds(unsigned girl, uint16_t item)
{
    for (unsigned slot = 0; slot < SLOT_COUNT; ++slot) {
        if (*recomp_memory_u16(slot_address(girl, slot)) == item) {
            return true;
        }
    }
    return false;
}

/* The item under the shop cursor, or -1 when it cannot be read. The shop
   state keeps the highlighted row of the visible window at +0x22, the item
   at the top of the window at +0x24 (it wraps), and the item count at +0x26. */
static int shop_selection(unsigned *shop, unsigned *cursor)
{
    const int8_t number = (int8_t)*recomp_memory(SHOP_NUMBER, 1);
    const uint32_t state = *recomp_memory_u32(shop_object + 0x18u);
    const uint32_t table = *recomp_memory_u32(SHOP_TABLE);
    unsigned row, top, count;
    uint16_t item;

    if (number < 0 || state == 0u || table == 0u) {
        return -1;
    }
    row = *recomp_memory_u16(state + 0x22u);
    top = *recomp_memory_u16(state + 0x24u);
    count = *recomp_memory_u16(state + 0x26u);
    if (count == 0u || count > SHOP_ROWS) {
        return -1;
    }
    *shop = (unsigned)number;
    *cursor = (top + row) % count;
    item = *recomp_memory_u16(table + (*shop * SHOP_ROWS + *cursor) * SHOP_ENTRY_SIZE);
    return item < ITEM_COUNT ? item : -1;
}

static int playing_girl(void);

void recomp_collection_note_grid(void)
{
    grid_noted = true;
}

/* The item in the grid cell under the cursor, or -1 when there is none;
   for a present, the item inside it, not its wrapping. The grid stays open
   while its preview was queued within the last few frames. */
static int grid_item(unsigned *list, unsigned *cursor)
{
    int count;
    unsigned index;
    int playing;

    if (grid_noted) {
        grid_frame = frame;
        grid_noted = false;
    }
    if (grid_frame == 0u || frame - grid_frame > SHOP_GRACE_FRAMES) {
        return -1;
    }
    *list = *recomp_memory(GRID_LIST, 1);
    *cursor = *recomp_memory_u16(GRID_CURSOR);
    playing = playing_girl();
    if (*list >= GRID_LISTS || playing < 0) {
        return -1;
    }
    count = (int8_t)*recomp_memory(GRID_COUNTS + *list, 1);
    if ((int)*cursor >= count) {
        return -1;
    }
    index = *recomp_memory(GRID_STARTS + *list, 1) + *cursor;
    if (index >= GRID_SLOT_COUNT) {
        index = 0;
    }
    const uint16_t item = *recomp_memory_u16(slot_address((unsigned)playing,
                                                          *recomp_memory(GRID_SLOTS + index, 1)));
    return item < ITEM_COUNT ? item : -1;
}

/* The item under the Collection's cursor, or -1 off that screen. */
static int collection_item(unsigned *tab, unsigned *cursor)
{
    uint16_t item;

    if (*recomp_memory(SCENE, 1) != SCENE_ISLAND || *recomp_memory(ISLAND_PLACE, 1) != PLACE_COLLECTION) {
        return -1;
    }
    *tab = *recomp_memory(COLLECTION_TAB, 1);
    *cursor = *recomp_memory_u16(COLLECTION_CURSOR);
    if (*tab >= CATALOG_TABS) {
        return -1;
    }
    item = *recomp_memory_u16(CATALOG + (*recomp_memory_u16(CATALOG_STARTS + *tab * 4u) + *cursor) * 2u);
    return item < ITEM_COUNT ? item : -1;
}

/* The item under the shop cursor this frame, or -1 while no shop is open.
   A shop stays open while its row draw ran within the last few frames. */
static int open_shop_item(unsigned *shop, unsigned *cursor)
{
    if (shop_noted) {
        shop_frame = frame;
        shop_noted = false;
    }
    if (shop_object == 0u || frame - shop_frame > SHOP_GRACE_FRAMES) {
        return -1;
    }
    return shop_selection(shop, cursor);
}

static void log_shop(int item, unsigned shop, unsigned cursor)
{
    char name[64];

    fprintf(stderr, "[auto:collection] frame %u: shop %u cursor %u: item %d %s (type %u):", frame, shop, cursor,
            item, item_name((uint16_t)item, name, sizeof name), item_type((uint16_t)item));
    for (unsigned girl = 0; girl < GIRL_COUNT; ++girl) {
        fprintf(stderr, " %s=%s%s", girl_name(girl), state_name(ownership((uint16_t)item, girl)),
                holds(girl, (uint16_t)item) ? ",held" : "");
    }
    fputc('\n', stderr);
}

static void add_name(char *list, size_t size, const char *name)
{
    const size_t used = strlen(list);

    snprintf(list + used, size - used, "%s%s", used != 0u ? ", " : "", name);
}

/* The girl being played, or -1 when the id is not one of the girls. */
static int playing_girl(void)
{
    const unsigned id = *recomp_memory(PLAYING_CHARACTER, 1);
    unsigned girl;

    if (id == 0u || id > CHARACTER_COUNT) {
        return -1;
    }
    girl = *recomp_memory(CHARACTER_GIRLS + id - 1u, 1);
    return girl < GIRL_COUNT ? (int)girl : -1;
}

/* In screen units: the left edge is x plus shift, shift being unstretched
   (see RecompOverlayIcon). An inactive icon is three quarters transparent. */
static void add_icon(RecompOverlayText *text, unsigned image, float x, float shift, float y, float size,
                     bool active)
{
    recomp_overlay_add_icon(text, image, x / SCREEN_WIDTH, shift / SCREEN_HEIGHT, y / SCREEN_HEIGHT,
                            size / SCREEN_HEIGHT, active ? 1.0f : 0.25f);
}

static void add_shop_icons(RecompOverlayText *text, uint16_t item, const PreviewBox *box)
{
    const int playing = playing_girl();
    const float margin = 8.0f, gap = 4.0f;
    /* Seven portraits down the edge, smaller when the preview is short. */
    const float fit = (box->bottom - box->top - 2.0f * margin - 6.0f * gap) / 7.0f;
    const float size = playing >= 0 ? (fit < 32.0f ? fit : 32.0f) : 28.0f;
    float y = box->top + margin;

    if (playing >= 0) {
        const float row = size < 28.0f ? size : 28.0f, top = box->bottom - margin - row;

        add_icon(text, RECOMP_ICON_HEART, box->left, margin, top, row,
                 (ownership(item, (unsigned)playing) & IN_COLLECTION) != 0u);
        add_icon(text, RECOMP_ICON_PRESENT, box->left, margin + row + gap, top, row,
                 holds((unsigned)playing, item));
        if (reactions_shown) {
            recomp_overlay_label_icon(text, recomp_gift_reaction_mark(recomp_gift_reaction(item, (unsigned)playing)),
                false);
        }
    }
    for (unsigned girl = 0; girl < GIRL_COUNT; ++girl) {
        if ((int)girl == playing) {
            continue;
        }
        add_icon(text, RECOMP_ICON_PORTRAIT + girl, box->right, -margin - size, y, size,
                 (ownership(item, girl) & IN_COLLECTION) != 0u);
        if (reactions_shown) {
            recomp_overlay_label_icon(text, recomp_gift_reaction_mark(recomp_gift_reaction(item, girl)), true);
        }
        y += size + gap;
    }
}

/* Two centred lines: the item's name, then which girls have it in their
   Collection, which have only had it, and whose room holds it now. */
static void fill_shop_panel(RecompOverlayText *text, uint16_t item, const PreviewBox *box)
{
    char name[64], collection[96] = "", had[96] = "", room[96] = "";
    char hint[RECOMP_OVERLAY_HINT_SIZE];
    size_t used;

    for (unsigned girl = 0; girl < GIRL_COUNT; ++girl) {
        const unsigned bits = ownership(item, girl);

        if (bits & IN_COLLECTION) {
            add_name(collection, sizeof collection, girl_name(girl));
        } else if (bits & HAD) {
            add_name(had, sizeof had, girl_name(girl));
        }
        if (holds(girl, item)) {
            add_name(room, sizeof room, girl_name(girl));
        }
    }
    used = (size_t)snprintf(hint, sizeof hint, "%s\n", item_name(item, name, sizeof name));
    if (collection[0] == '\0' && had[0] == '\0' && room[0] == '\0') {
        snprintf(hint + used, sizeof hint - used, "No girl has had it");
    } else {
        const char *parts[3][2] = {{"Collection: ", collection}, {"Had: ", had}, {"In room: ", room}};
        const char *separator = "";

        for (unsigned i = 0; i < 3u && used < sizeof hint; ++i) {
            if (parts[i][1][0] != '\0') {
                used += (size_t)snprintf(hint + used, sizeof hint - used, "%s%s%s", separator,
                                         parts[i][0], parts[i][1]);
                separator = "  |  ";
            }
        }
    }
    if (reactions_shown) {
        /* A third line for a presenter without icons: each girl with a
           reaction, loves first. */
        char reactions[160] = "";

        for (int reaction = 2; reaction >= -2; --reaction) {
            for (unsigned girl = 0; girl < GIRL_COUNT && reaction != 0; ++girl) {
                if (recomp_gift_reaction(item, girl) == reaction) {
                    char entry[24];

                    snprintf(entry, sizeof entry, "%s %s", recomp_gift_reaction_mark(reaction), girl_name(girl));
                    add_name(reactions, sizeof reactions, entry);
                }
            }
        }
        used = strlen(hint);
        snprintf(hint + used, sizeof hint - used, "\n%s", reactions[0] != '\0' ? reactions : "No reactions known");
    }
    recomp_overlay_clear(text);
    recomp_overlay_set_hint(text, hint);
    recomp_overlay_set_key_tip(&text->keys, "LS", reactions_shown ? "click: hide reactions" : "click: reactions");
    add_shop_icons(text, item, box);
}

/* With the log on, one line for what the panel now shows: its text, with
   " / " for the line break, and which icons are lit, so a script can check
   the panel without looking at the screen. */
static void log_panel(const RecompOverlayText *text)
{
    char hint[RECOMP_OVERLAY_HINT_SIZE];
    const char *separator = "";
    size_t length = 0;

    for (const char *at = text->hint; *at != '\0' && length + 4u < sizeof hint; ++at) {
        if (*at == '\n') {
            memcpy(hint + length, " / ", 3);
            length += 3;
        } else {
            hint[length++] = *at;
        }
    }
    hint[length] = '\0';
    fprintf(stderr, "[auto:collection] frame %u: panel: %s; lit:", frame, length != 0u ? hint : "(empty)");
    for (unsigned i = 0; i < text->icon_count; ++i) {
        const RecompOverlayIcon *icon = &text->icon[i];

        if (icon->alpha < 1.0f) {
            continue;
        }
        if (icon->image == RECOMP_ICON_HEART) {
            fprintf(stderr, "%s heart", separator);
        } else if (icon->image == RECOMP_ICON_PRESENT) {
            fprintf(stderr, "%s present", separator);
        } else {
            fprintf(stderr, "%s %s", separator, girl_name(icon->image - RECOMP_ICON_PORTRAIT));
        }
        separator = ",";
    }
    fprintf(stderr, "%s", separator[0] == '\0' ? " none" : "");
    /* The reactions shown beside the icons, by girl; the playing girl's
       sits beside the present. */
    separator = "; reactions:";
    for (unsigned i = 0; i < text->icon_count; ++i) {
        const RecompOverlayIcon *icon = &text->icon[i];

        if (icon->label[0] != '\0') {
            fprintf(stderr, "%s %s %s", separator,
                    icon->image == RECOMP_ICON_PRESENT ? "playing" : girl_name(icon->image - RECOMP_ICON_PORTRAIT),
                    icon->label);
            separator = ",";
        }
    }
    fputc('\n', stderr);
}

/* Publishes the panel when it changes, and an empty one once the shop
   closes. Ownership can change under a fixed cursor, as when buying. */
static void show_shop_panel(int item, const PreviewBox *box)
{
    static RecompOverlayText shown;
    RecompOverlayText text;

    if (item >= 0) {
        recomp_island_icons_publish(recomp_disc_root_path);
        fill_shop_panel(&text, (uint16_t)item, box);
    } else {
        recomp_overlay_clear(&text);
    }
    if (memcmp(&text, &shown, sizeof text) != 0) {
        recomp_overlay_publish_status(&text);
        shown = text;
        if (log_enabled()) {
            log_panel(&text);
        }
    }
}

void recomp_collection_note_shop(uint32_t shop)
{
    shop_object = shop;
    shop_noted = shop != 0u;
}

void recomp_collection_log_frame(void)
{
    unsigned shop = 0, cursor = 0;
    const PreviewBox *box = &SHOP_PREVIEW;
    int item;

    if (!log_enabled() && !panel_enabled()) {
        return;
    }
    ++frame;
    if (log_enabled()) {
        log_ownership();
        log_inventories();
        have_previous = true;
    }
    item = open_shop_item(&shop, &cursor);
    if (item >= 0) {
        if (log_enabled() && item != shop_item) {
            log_shop(item, shop, cursor);
        }
    } else {
        /* The same panel over the items screen, a gift's presents and the
           Collection, whose previews sit where the shop's does. */
        unsigned list = 0;
        const char *source = "grid list";

        item = grid_item(&list, &cursor);
        if (item < 0) {
            item = collection_item(&list, &cursor);
            source = "collection tab";
            box = &COLLECTION_PREVIEW;
        }
        if (log_enabled() && item >= 0 && item != shop_item) {
            char name[64];

            fprintf(stderr, "[auto:collection] frame %u: %s %u cursor %u: item %d %s\n", frame, source, list, cursor,
                    item, item_name((uint16_t)item, name, sizeof name));
        }
    }
    shop_item = item;
    if (panel_enabled()) {
        static bool click_was_down;
        const bool click_down = (*recomp_memory_u32(BUTTONS_HELD) & BUTTON_LEFT_THUMB) != 0u;

        if (click_down && !click_was_down && item >= 0) {
            reactions_shown = !reactions_shown;
        }
        click_was_down = click_down;
        show_shop_panel(item, box);
    }
}
#else
void recomp_collection_note_shop(uint32_t shop) { (void)shop; }
void recomp_collection_note_grid(void) {}
void recomp_collection_log_frame(void) {}
#endif
