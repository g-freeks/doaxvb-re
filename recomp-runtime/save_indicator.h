#ifndef DOAXBV_RECOMP_SAVE_INDICATOR_H
#define DOAXBV_RECOMP_SAVE_INDICATOR_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* A corner notice while the game saves its profile and for a few seconds
   after the save commits, so a player knows when quitting keeps their
   progress. In play the game saved as the part of the day changed, inside
   one frame. Plain state counted in presented frames: the save adapter
   reports each save, the frame adapter ticks once a frame. Nested saves
   join the outer one, which alone commits. */
enum {
    /* About five seconds at the game's 60 frames a second: a save comes as
       the screen fades to load the next place, so the notice outlasts the
       fade and the loading screen. */
    RECOMP_SAVE_INDICATOR_HOLD = 300
};

typedef enum RecompSaveNotice {
    RECOMP_SAVE_NOTICE_NONE,
    RECOMP_SAVE_NOTICE_SAVING,
    RECOMP_SAVE_NOTICE_SAVED
} RecompSaveNotice;

typedef struct RecompSaveIndicator {
    unsigned depth;
    unsigned hold;
} RecompSaveIndicator;

void recomp_save_indicator_begin(RecompSaveIndicator *indicator);
/* Call only once the save has committed or joined its outer save. */
void recomp_save_indicator_end(RecompSaveIndicator *indicator);
void recomp_save_indicator_tick(RecompSaveIndicator *indicator);
RecompSaveNotice recomp_save_indicator_notice(const RecompSaveIndicator *indicator);
/* The words shown for a notice, or "" for none. */
const char *recomp_save_notice_text(RecompSaveNotice notice);

/* The runner's indicator, one of the extra overlays (RECOMP_EXTRA_OVERLAYS=1),
   off by default. The notes come
   from the save adapter; the frame publishes the overlay notice and, with
   RECOMP_AUTO_LOG=1, an [auto:save] line when it changes. */
void recomp_save_indicator_note_begin(void);
void recomp_save_indicator_note_end(void);
void recomp_save_indicator_frame(void);

#ifdef __cplusplus
}
#endif

#endif
