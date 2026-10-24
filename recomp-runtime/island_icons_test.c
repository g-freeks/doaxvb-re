/* A synthetic XPR0 pack; no retail data. */
#include "island_icons.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(c) do { if (!(c)) { fprintf(stderr, "Island icons line %d: %s\n", __LINE__, #c); return 1; } } while (0)

static void put(uint8_t *at, uint32_t value)
{
    at[0] = (uint8_t)value;
    at[1] = (uint8_t)(value >> 8);
    at[2] = (uint8_t)(value >> 16);
    at[3] = (uint8_t)(value >> 24);
}

int main(void)
{
    /* Header: a 4x2 P8 texture (resource 0), its palette (resource 1), the
       end marker; data: 8 texels, then 256 colours. */
    enum { HEADER = 12 + 20 + 12 + 4, TEXELS = 8 };
    static uint8_t pack[HEADER + TEXELS + 1024];
    RecompIconImage image, icons[RECOMP_ICON_COUNT];

    memcpy(pack, "XPR0", 4);
    put(pack + 4, sizeof pack);
    put(pack + 8, HEADER);
    put(pack + 12, 0x00040001u);
    put(pack + 16, 0);
    put(pack + 24, 0x00000B00u | 1u << 16 | 2u << 20 | 1u << 24); /* P8, 4 x 2 */
    put(pack + 32, 0x00830001u);
    put(pack + 36, TEXELS);
    put(pack + 44, 0xFFFFFFFFu);
    /* Swizzled order for 4 x 2: x0 y0 x1 x1... bits go x, y, x. Texel at
       offset o gets palette index o; colour i is (r = i, g = 2i, b = 3i, a). */
    for (unsigned o = 0; o < TEXELS; ++o) {
        pack[HEADER + o] = (uint8_t)o;
    }
    for (unsigned i = 0; i < 256; ++i) {
        uint8_t *bgra = pack + HEADER + TEXELS + i * 4u;

        bgra[0] = (uint8_t)(3u * i);
        bgra[1] = (uint8_t)(2u * i);
        bgra[2] = (uint8_t)i;
        bgra[3] = (uint8_t)(255u - i);
    }

    CHECK(recomp_xpr_decode_p8(pack, sizeof pack, 0, 4, 2, &image));
    CHECK(image.width == 4u && image.height == 2u);
    /* (x, y) -> offset: x bit 0, then y bit 0, then x bit 1. */
    {
        static const unsigned expected[2][4] = {{0, 1, 4, 5}, {2, 3, 6, 7}};

        for (unsigned y = 0; y < 2; ++y) {
            for (unsigned x = 0; x < 4; ++x) {
                const uint8_t *rgba = image.rgba + (y * 4u + x) * 4u;
                const unsigned i = expected[y][x];

                CHECK(rgba[0] == i && rgba[1] == 2u * i && rgba[2] == 3u * i && rgba[3] == 255u - i);
            }
        }
    }
    free(image.rgba);

    /* The wrong size, a palette asked for as a texture, a broken magic. */
    CHECK(!recomp_xpr_decode_p8(pack, sizeof pack, 0, 2, 4, &image) && image.rgba == NULL);
    CHECK(!recomp_xpr_decode_p8(pack, sizeof pack, 1, 4, 2, &image));
    CHECK(!recomp_xpr_decode_p8(pack, sizeof pack, 2, 4, 2, &image));
    pack[0] = 'Y';
    CHECK(!recomp_xpr_decode_p8(pack, sizeof pack, 0, 4, 2, &image));

    /* No disc: every icon absent. */
    CHECK(!recomp_island_icons_load("/nonexistent-disc-root", icons));
    for (unsigned i = 0; i < RECOMP_ICON_COUNT; ++i) {
        CHECK(icons[i].rgba == NULL);
    }
    puts("PASS island icons");
    return 0;
}
