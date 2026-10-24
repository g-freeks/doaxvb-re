#ifndef DOAXBV_RECOMP_OVERLAY_TEXT_H
#define DOAXBV_RECOMP_OVERLAY_TEXT_H

#include <stdbool.h>

#include "island_icons.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Text the runtime shows over the game: a short panel of lines and a hint
   line along the bottom. The game side fills it once a frame; a presenter
   draws whatever it holds when the frame is presented. Plain data, with no
   knowledge of how or where it is drawn. */
enum {
    RECOMP_OVERLAY_ICONS = 12,
    RECOMP_OVERLAY_LINES = 8,
    RECOMP_OVERLAY_LINE_SIZE = 96,
    RECOMP_OVERLAY_HINT_SIZE = 320,
};

/* An icon from island_icons.h placed over the game's picture. The game
   stretches its screen across the whole picture, 3:2 or 16:9, so x is a
   fraction of the picture's width and y of its height; the icon stays
   square, its size a fraction of the height. The left edge is x plus shift,
   a fraction of the height: a shift of -size puts the right edge on x, and
   icons in a row are a size apart whatever the stretch. */
typedef struct RecompOverlayIcon {
    unsigned image;
    float x, shift, y, size;
    float alpha;
} RecompOverlayIcon;

typedef struct RecompOverlayText {
    char title[RECOMP_OVERLAY_LINE_SIZE];
    char line[RECOMP_OVERLAY_LINES][RECOMP_OVERLAY_LINE_SIZE];
    unsigned count;
    char hint[RECOMP_OVERLAY_HINT_SIZE];
    RecompOverlayIcon icon[RECOMP_OVERLAY_ICONS];
    unsigned icon_count;
} RecompOverlayText;

/* Empties the overlay; an empty overlay draws nothing. */
void recomp_overlay_clear(RecompOverlayText *text);
void recomp_overlay_set_title(RecompOverlayText *text, const char *title);
/* Adds a printf-formatted line; lines past RECOMP_OVERLAY_LINES are dropped
   and long lines are cut. False when the line was dropped. */
bool recomp_overlay_add_line(RecompOverlayText *text, const char *format, ...);
void recomp_overlay_set_hint(RecompOverlayText *text, const char *hint);
/* Adds an icon; false past RECOMP_OVERLAY_ICONS. A presenter that cannot
   draw icons shows the hint instead, so give icons a hint to fall back on. */
bool recomp_overlay_add_icon(RecompOverlayText *text, unsigned image, float x, float shift, float y,
    float size, float alpha);
bool recomp_overlay_empty(const RecompOverlayText *text);

/* The game side hands a finished overlay over once a frame; a presenter,
   possibly on its own thread, takes a copy of the latest one to draw. */
void recomp_overlay_publish(const RecompOverlayText *text);
void recomp_overlay_take(RecompOverlayText *text);
/* A lower-priority overlay, such as the shop's ownership panel: taken only
   while the overlay published above is empty. Publish an empty one to
   remove it. */
void recomp_overlay_publish_status(const RecompOverlayText *text);

/* The images icons refer to, handed over once by the game side, which reads
   them from the disc; a presenter uploads them when it first draws an icon.
   NULL until then; they stay valid for the rest of the run. */
void recomp_overlay_set_icon_images(const RecompIconImage *images);
const RecompIconImage *recomp_overlay_icon_images(void);

#ifdef __cplusplus
}
#endif

#endif
