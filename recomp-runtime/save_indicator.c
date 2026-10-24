#include "save_indicator.h"

#include "overlay_text.h"
#include "ui_log.h"

#include <stdio.h>

void recomp_save_indicator_begin(RecompSaveIndicator *indicator)
{
    ++indicator->depth;
}

void recomp_save_indicator_end(RecompSaveIndicator *indicator)
{
    if (indicator->depth == 0u) return;
    if (--indicator->depth == 0u) indicator->hold = RECOMP_SAVE_INDICATOR_HOLD;
}

void recomp_save_indicator_tick(RecompSaveIndicator *indicator)
{
    if (indicator->depth == 0u && indicator->hold != 0u) --indicator->hold;
}

RecompSaveNotice recomp_save_indicator_notice(const RecompSaveIndicator *indicator)
{
    if (indicator->depth != 0u) return RECOMP_SAVE_NOTICE_SAVING;
    return indicator->hold != 0u ? RECOMP_SAVE_NOTICE_SAVED : RECOMP_SAVE_NOTICE_NONE;
}

const char *recomp_save_notice_text(RecompSaveNotice notice)
{
    switch (notice) {
    case RECOMP_SAVE_NOTICE_SAVING: return "Saving...";
    case RECOMP_SAVE_NOTICE_SAVED: return "Saved";
    default: return "";
    }
}

static RecompSaveIndicator runner_indicator;
static RecompSaveNotice shown;
static unsigned frame;

static bool enabled(void)
{
    return recomp_extra_overlays_enabled();
}

/* Shows a changed notice at once: a save runs inside one game frame, and
   the log line marks where in the frames it fell. */
static void show(void)
{
    const RecompSaveNotice notice = recomp_save_indicator_notice(&runner_indicator);

    if (notice == shown) return;
    shown = notice;
    recomp_overlay_publish_notice(recomp_save_notice_text(notice), notice == RECOMP_SAVE_NOTICE_SAVING);
    if (recomp_ui_log_enabled()) {
        fprintf(stderr, "[auto:save] frame %u: %s\n", frame,
            notice == RECOMP_SAVE_NOTICE_SAVING ? "saving" : notice == RECOMP_SAVE_NOTICE_SAVED ? "saved" : "idle");
    }
}

void recomp_save_indicator_note_begin(void)
{
    if (!enabled()) return;
    recomp_save_indicator_begin(&runner_indicator);
    show();
}

void recomp_save_indicator_note_end(void)
{
    if (!enabled()) return;
    recomp_save_indicator_end(&runner_indicator);
    show();
}

void recomp_save_indicator_frame(void)
{
    if (!enabled()) return;
    ++frame;
    recomp_save_indicator_tick(&runner_indicator);
    show();
}
