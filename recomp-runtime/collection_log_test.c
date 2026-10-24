/* Synthetic guest memory; no retail data or generated code. */
#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif
#define RECOMP_FULL_PROGRAM
#include "collection_log.c"

RecompRuntime recomp_runtime;
const char *recomp_disc_root_path = "/nonexistent-disc-root";
static uint8_t memory[RECOMP_XBOX_RAM_SIZE];
uint8_t *recomp_fast_ram;

uint8_t *recomp_memory(uint32_t address, size_t size)
{
    if (address > sizeof memory || size > sizeof memory - address) {
        fprintf(stderr, "Collection memory out of bounds: address=%u size=%zu\n", address, size);
        abort();
    }
    return memory + address;
}
uint32_t *recomp_memory_u32_checked(uint32_t address) { return (uint32_t *)recomp_memory(address, 4); }
uint16_t *recomp_memory_u16(uint32_t address) { return (uint16_t *)recomp_memory(address, 2); }

enum {
    TEST_ITEMS = 0x00400000u,
    TEST_ITEM = 7,
    TEST_SHOP = 0x00500000u,
    TEST_NAME_ENTRY = 0x00500200u,
    TEST_NAME = 0x00500300u,
    TEST_STATE = 0x00500400u,
    TEST_TABLE = 0x00510000u,
    TEST_LIVE = 0x00520000u
};

#define CHECK(c) do { if (!(c)) { fprintf(stderr, "Collection line %d: %s\n", __LINE__, #c); return 1; } } while (0)

int main(void)
{
    uint8_t before[ITEM_COUNT * 2u];

#ifdef _WIN32
    _putenv_s("RECOMP_COLLECTION_LOG", "1");
    _putenv_s("RECOMP_EXTRA_OVERLAYS", "1");
#else
    setenv("RECOMP_COLLECTION_LOG", "1", 1);
    setenv("RECOMP_EXTRA_OVERLAYS", "1", 1);
#endif
    *recomp_memory_u16(TEST_ITEMS) = 2u;
    *recomp_memory_u32(ITEM_TABLE + TEST_ITEM * 4u) = TEST_ITEMS;
    *recomp_memory_u16(OWNERSHIP + 1u * 2u) = HAD << 8;
    memset(recomp_memory(INVENTORY, GIRL_COUNT * INVENTORY_STRIDE), 0xFF, GIRL_COUNT * INVENTORY_STRIDE);

    recomp_collection_log_frame();
    CHECK(have_previous && frame == 1u);
    CHECK(previous[1] == HAD << 8);

    /* Girl 4 buys the item: one detail line, and the copy follows. */
    *recomp_memory_u16(OWNERSHIP + TEST_ITEM * 2u) = (HAD | IN_COLLECTION) << 8;
    recomp_collection_log_frame();
    CHECK(previous[TEST_ITEM] == (HAD | IN_COLLECTION) << 8);

    /* Girl 6 receives the item as a gift into her third slot. */
    {
        const InventorySlot gift = {TEST_ITEM, -3, 0x0409u, 668u};

        memcpy(recomp_memory(INVENTORY + 6u * INVENTORY_STRIDE + 2u * 8u, sizeof gift), &gift, sizeof gift);
        recomp_collection_log_frame();
        CHECK(previous_inventory[6][2].item == TEST_ITEM && previous_inventory[6][2].wrapping == 668u);
        CHECK(previous_inventory[6][1].item == EMPTY_SLOT);
    }

    /* The shop cursor on the item: one line, then nothing while it stays. */
    {
        static const char text[] = "white@| origami";
        char name[64];

        *recomp_memory(LANGUAGE, 1) = 1u;
        *recomp_memory_u32(ITEM_NAMES + TEST_ITEM * 4u) = TEST_NAME_ENTRY;
        *recomp_memory_u32(TEST_NAME_ENTRY + 8u) = TEST_NAME;
        memcpy(recomp_memory(TEST_NAME, sizeof text), text, sizeof text);
        CHECK(strcmp(item_name(TEST_ITEM, name, sizeof name), "white origami") == 0);
        CHECK(strcmp(item_name(TEST_ITEM, name, 6), "white") == 0);

        *recomp_memory(SHOP_NUMBER, 1) = 1u;
        *recomp_memory_u32(SHOP_TABLE) = TEST_TABLE;
        *recomp_memory_u32(TEST_SHOP + 0x18u) = TEST_STATE;
        /* Window row 2 below top item 14 of 15 wraps to item 1. */
        *recomp_memory_u16(TEST_STATE + 0x22u) = 2u;
        *recomp_memory_u16(TEST_STATE + 0x24u) = 14u;
        *recomp_memory_u16(TEST_STATE + 0x26u) = 15u;
        *recomp_memory_u16(TEST_TABLE + (1u * SHOP_ROWS + 1u) * SHOP_ENTRY_SIZE) = TEST_ITEM;
        recomp_collection_note_shop(TEST_SHOP);
        recomp_collection_log_frame();
        CHECK(shop_item == TEST_ITEM);
        CHECK(holds(6, TEST_ITEM) && !holds(4, TEST_ITEM));
        {
            RecompOverlayText panel;

            recomp_overlay_take(&panel);
            CHECK(strcmp(panel.hint, "white origami\nCollection: Ayane  |  In room: Lisa") == 0);
            /* No playing girl: eight portraits, Ayane's opaque. */
            CHECK(panel.icon_count == GIRL_COUNT);
            CHECK(panel.icon[4].image == RECOMP_ICON_PORTRAIT + 4u && panel.icon[4].alpha == 1.0f);
            CHECK(panel.icon[6].alpha == 0.25f);
            /* Playing Ayane (character id 13): her heart and present along
               the bottom, the seven others down the side. */
            *recomp_memory(PLAYING_CHARACTER, 1) = 13u;
            *recomp_memory(CHARACTER_GIRLS + 12u, 1) = 4u;
            fill_shop_panel(&panel, TEST_ITEM, &SHOP_PREVIEW);
            CHECK(panel.icon_count == 2u + GIRL_COUNT - 1u);
            CHECK(panel.icon[0].image == RECOMP_ICON_HEART && panel.icon[0].alpha == 1.0f);
            CHECK(panel.icon[1].image == RECOMP_ICON_PRESENT && panel.icon[1].alpha == 0.25f);
            CHECK(panel.icon[6].image == RECOMP_ICON_PORTRAIT + 5u);
            /* A purchase shows in the live records before the save's copy. */
            memset(recomp_memory(TEST_LIVE, GIRL_COUNT * LIVE_STRIDE), 0xFF, GIRL_COUNT * LIVE_STRIDE);
            *recomp_memory_u32(LIVE_INVENTORIES) = TEST_LIVE;
            *recomp_memory_u16(TEST_LIVE + 4u * LIVE_STRIDE + 18u * LIVE_RECORD) = TEST_ITEM;
            fill_shop_panel(&panel, TEST_ITEM, &SHOP_PREVIEW);
            CHECK(panel.icon[1].alpha == 1.0f && holds(4, TEST_ITEM) && !holds(6, TEST_ITEM));
            /* The log follows the live records too: one slot changed. */
            recomp_collection_log_frame();
            CHECK(previous_inventory[4][18].item == TEST_ITEM && previous_inventory[6][2].item == EMPTY_SLOT);
            *recomp_memory_u32(LIVE_INVENTORIES) = 0u;
            *recomp_memory(PLAYING_CHARACTER, 1) = 0u;
        }

        /* Leaving the shop: after the grace frames the selection resets. */
        for (unsigned i = 0; i <= SHOP_GRACE_FRAMES; ++i) {
            recomp_collection_log_frame();
        }
        CHECK(shop_item == -1);
        {
            RecompOverlayText panel;

            recomp_overlay_take(&panel);
            CHECK(recomp_overlay_empty(&panel));
        }
    }

    /* A whole save loading: summary only, copy still follows. */
    for (unsigned id = 0; id < ITEM_COUNT; ++id) {
        *recomp_memory_u16(OWNERSHIP + id * 2u) = HAD;
    }
    memcpy(before, recomp_memory(OWNERSHIP, sizeof before), sizeof before);
    memset(recomp_memory(INVENTORY, GIRL_COUNT * INVENTORY_STRIDE), 0, GIRL_COUNT * INVENTORY_STRIDE);
    recomp_collection_log_frame();
    CHECK(previous_inventory[7][SLOT_COUNT - 1u].item == 0u);
    CHECK(previous[0] == HAD && previous[ITEM_COUNT - 1u] == HAD);

    /* Guest memory is never written; inventory slots are 8 bytes. */
    CHECK(sizeof(InventorySlot) == 8u);
    recomp_collection_log_frame();
    CHECK(memcmp(recomp_memory(OWNERSHIP, sizeof before), before, sizeof before) == 0);
    CHECK(frame == 7u + SHOP_GRACE_FRAMES + 1u);
    puts("PASS collection log");
    return 0;
}
