#ifndef DOAXBV_RECOMP_ISLAND_ICONS_H
#define DOAXBV_RECOMP_ISLAND_ICONS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Icons from the game's island interface pack, read from the player's own
   disc when the runtime needs them: the eight girls' round portraits (the
   map's), the heart and the wrapped present of the item screen's tabs. */
enum {
    RECOMP_ICON_PORTRAIT = 0, /* + girl, in the game's girl order */
    RECOMP_ICON_HEART = 8,
    RECOMP_ICON_PRESENT = 9,
    RECOMP_ICON_COUNT = 10
};

typedef struct RecompIconImage {
    uint32_t width;
    uint32_t height;
    uint8_t *rgba; /* width * height * 4, straight alpha; NULL when absent */
} RecompIconImage;

/* Reads every icon from the pack under disc_root. False, with every image
   left absent, when the pack is missing or not the expected one. */
bool recomp_island_icons_load(const char *disc_root, RecompIconImage images[RECOMP_ICON_COUNT]);
void recomp_island_icons_free(RecompIconImage images[RECOMP_ICON_COUNT]);

/* Decodes texture resource `resource` of an XPR0 pack, an 8-bit paletted
   swizzled texture followed by its palette, into image. False when the
   resource is not such a texture of the given size. */
bool recomp_xpr_decode_p8(const uint8_t *pack, size_t size, unsigned resource,
    uint32_t width, uint32_t height, RecompIconImage *image);

#ifdef __cplusplus
}
#endif

#endif
