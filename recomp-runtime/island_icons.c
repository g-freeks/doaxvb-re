#include "island_icons.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The pack is entry 170 of datadvd.afs on the verified USA disc: an XPR0
   resource pack the game loads whole for the island screens (the map, the
   shops, the item screen). An AFS archive starts "AFS\0", an entry count and
   one (offset, size) pair per entry. An XPR0 pack starts "XPR0", its total
   size and its header size, then resource headers from offset 12: a texture
   is five words (common, data offset, lock, format, size), a palette three
   (common, data offset, lock); data offsets count from the header's end.
   Each icon texture is 8-bit paletted and swizzled, and the palette resource
   after it holds its 256 BGRA colours. */
enum {
    PACK_ENTRY = 170,
    XPR_HEADER = 12,
    TEXTURE_WORDS = 5,
    PALETTE_WORDS = 3,
    KIND_PALETTE = 3,
    KIND_TEXTURE = 4,
    FORMAT_P8 = 0x0B,
    PALETTE_BYTES = 256 * 4,
    MAX_PACK = 16 * 1024 * 1024
};

static const struct {
    unsigned resource;
    uint32_t size;
} icons[RECOMP_ICON_COUNT] = {
    {0, 128}, {2, 128}, {4, 128}, {6, 128}, {8, 128}, {10, 128}, {12, 128}, {14, 128},
    {84, 32}, /* heart */
    {82, 32}, /* present */
};

static uint32_t word(const uint8_t *bytes)
{
    return (uint32_t)bytes[0] | (uint32_t)bytes[1] << 8 | (uint32_t)bytes[2] << 16 | (uint32_t)bytes[3] << 24;
}

/* Byte offset of texel (x, y) in a swizzled texture: the bits of x and y
   interleave from the lowest, x first, while each axis still has bits. */
static uint32_t swizzled(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
    uint32_t offset = 0, bit = 0;

    while (width > 1u || height > 1u) {
        if (width > 1u) {
            offset |= (x & 1u) << bit++;
            x >>= 1;
            width >>= 1;
        }
        if (height > 1u) {
            offset |= (y & 1u) << bit++;
            y >>= 1;
            height >>= 1;
        }
    }
    return offset;
}

/* The header offset of resource `resource`, or 0 when the pack has fewer. */
static size_t find_resource(const uint8_t *pack, size_t header, unsigned resource)
{
    size_t at = XPR_HEADER;

    for (unsigned n = 0;; ++n) {
        uint32_t common, kind;

        if (at + PALETTE_WORDS * 4u > header) {
            return 0;
        }
        common = word(pack + at);
        if (common == 0u || common == 0xFFFFFFFFu) {
            return 0;
        }
        if (n == resource) {
            return at;
        }
        kind = (common >> 16) & 7u;
        at += (kind == KIND_TEXTURE ? TEXTURE_WORDS : PALETTE_WORDS) * 4u;
    }
}

bool recomp_xpr_decode_p8(const uint8_t *pack, size_t size, unsigned resource,
    uint32_t width, uint32_t height, RecompIconImage *image)
{
    size_t header, texture, palette;
    uint32_t format, pixels, colors;
    const uint8_t *data;

    memset(image, 0, sizeof *image);
    if (size < XPR_HEADER || memcmp(pack, "XPR0", 4) != 0) {
        return false;
    }
    header = word(pack + 8);
    if (header > size) {
        return false;
    }
    texture = find_resource(pack, header, resource);
    palette = find_resource(pack, header, resource + 1u);
    if (texture == 0 || palette == 0 || texture + TEXTURE_WORDS * 4u > header ||
        ((word(pack + texture) >> 16) & 7u) != KIND_TEXTURE ||
        ((word(pack + palette) >> 16) & 7u) != KIND_PALETTE) {
        return false;
    }
    format = word(pack + texture + 12);
    if (((format >> 8) & 0xFFu) != FORMAT_P8 || 1u << ((format >> 20) & 15u) != width ||
        1u << ((format >> 24) & 15u) != height) {
        return false;
    }
    data = pack + header;
    pixels = word(pack + texture + 4);
    colors = word(pack + palette + 4);
    if ((uint64_t)pixels + width * height > size - header || (uint64_t)colors + PALETTE_BYTES > size - header) {
        return false;
    }
    image->rgba = malloc((size_t)width * height * 4u);
    if (image->rgba == NULL) {
        return false;
    }
    image->width = width;
    image->height = height;
    for (uint32_t y = 0; y < height; ++y) {
        for (uint32_t x = 0; x < width; ++x) {
            const uint8_t *bgra = data + colors + data[pixels + swizzled(x, y, width, height)] * 4u;
            uint8_t *out = image->rgba + ((size_t)y * width + x) * 4u;

            out[0] = bgra[2];
            out[1] = bgra[1];
            out[2] = bgra[0];
            out[3] = bgra[3];
        }
    }
    return true;
}

static uint8_t *read_pack(const char *disc_root, size_t *size)
{
    char path[1200];
    uint8_t toc[8], *pack;
    uint32_t offset, length;
    FILE *file;

    snprintf(path, sizeof path, "%s/datadvd.afs", disc_root);
    file = fopen(path, "rb");
    if (file == NULL) {
        return NULL;
    }
    pack = NULL;
    if (fread(toc, 1, 8, file) == 8 && memcmp(toc, "AFS", 4) == 0 && word(toc + 4) > PACK_ENTRY &&
        fseek(file, 8 + PACK_ENTRY * 8, SEEK_SET) == 0 && fread(toc, 1, 8, file) == 8) {
        offset = word(toc);
        length = word(toc + 4);
        if (length >= XPR_HEADER && length <= MAX_PACK && fseek(file, (long)offset, SEEK_SET) == 0) {
            pack = malloc(length);
            if (pack != NULL && fread(pack, 1, length, file) != length) {
                free(pack);
                pack = NULL;
            }
            *size = length;
        }
    }
    fclose(file);
    return pack;
}

bool recomp_island_icons_load(const char *disc_root, RecompIconImage images[RECOMP_ICON_COUNT])
{
    size_t size = 0;
    uint8_t *pack;
    bool loaded = true;

    memset(images, 0, sizeof *images * RECOMP_ICON_COUNT);
    if (disc_root == NULL) {
        return false;
    }
    pack = read_pack(disc_root, &size);
    if (pack == NULL) {
        return false;
    }
    for (unsigned i = 0; i < RECOMP_ICON_COUNT && loaded; ++i) {
        loaded = recomp_xpr_decode_p8(pack, size, icons[i].resource, icons[i].size, icons[i].size, &images[i]);
    }
    free(pack);
    if (!loaded) {
        recomp_island_icons_free(images);
    }
    return loaded;
}

void recomp_island_icons_free(RecompIconImage images[RECOMP_ICON_COUNT])
{
    for (unsigned i = 0; i < RECOMP_ICON_COUNT; ++i) {
        free(images[i].rgba);
        memset(&images[i], 0, sizeof images[i]);
    }
}
