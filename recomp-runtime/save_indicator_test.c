#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif
#include "save_indicator.h"

#include "overlay_text.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures;

static void check(bool condition, const char *what)
{
    if (!condition) {
        fprintf(stderr, "save indicator test failed: %s\n", what);
        ++failures;
    }
}

static RecompOverlayText taken(void)
{
    RecompOverlayText text;
    recomp_overlay_take(&text);
    return text;
}

/* Without RECOMP_EXTRA_OVERLAYS=1 the runner's indicator shows nothing. */
static int check_off(void)
{
    recomp_save_indicator_note_begin();
    check(taken().notice[0] == '\0', "no saving notice without the extra overlays");
    recomp_save_indicator_note_end();
    check(taken().notice[0] == '\0', "no saved notice without the extra overlays");
    if (failures == 0) printf("save indicator off test passed\n");
    return failures == 0 ? 0 : 1;
}

int main(int argc, char **argv)
{
    RecompSaveIndicator indicator = {0};

    if (argc > 1 && strcmp(argv[1], "--off") == 0) return check_off();
#ifdef _WIN32
    _putenv_s("RECOMP_EXTRA_OVERLAYS", "1");
#else
    setenv("RECOMP_EXTRA_OVERLAYS", "1", 1);
#endif

    check(recomp_save_indicator_notice(&indicator) == RECOMP_SAVE_NOTICE_NONE, "no notice before a save");
    recomp_save_indicator_begin(&indicator);
    check(recomp_save_indicator_notice(&indicator) == RECOMP_SAVE_NOTICE_SAVING, "a save shows saving");
    recomp_save_indicator_begin(&indicator);
    recomp_save_indicator_end(&indicator);
    recomp_save_indicator_tick(&indicator);
    check(recomp_save_indicator_notice(&indicator) == RECOMP_SAVE_NOTICE_SAVING,
        "a nested save leaves the outer one saving");
    recomp_save_indicator_end(&indicator);
    check(recomp_save_indicator_notice(&indicator) == RECOMP_SAVE_NOTICE_SAVED, "the outer end shows saved");
    for (unsigned i = 1u; i < RECOMP_SAVE_INDICATOR_HOLD; ++i) recomp_save_indicator_tick(&indicator);
    check(recomp_save_indicator_notice(&indicator) == RECOMP_SAVE_NOTICE_SAVED, "saved holds its frames");
    recomp_save_indicator_tick(&indicator);
    check(recomp_save_indicator_notice(&indicator) == RECOMP_SAVE_NOTICE_NONE, "saved then goes");
    recomp_save_indicator_end(&indicator);
    check(recomp_save_indicator_notice(&indicator) == RECOMP_SAVE_NOTICE_NONE, "a stray end is ignored");
    check(strcmp(recomp_save_notice_text(RECOMP_SAVE_NOTICE_NONE), "") == 0, "no notice has no words");

    /* The runner's indicator publishes the notice beside the shop panel. */
    RecompOverlayText shop;
    recomp_overlay_clear(&shop);
    recomp_overlay_set_hint(&shop, "Shylph");
    recomp_overlay_publish_status(&shop);
    recomp_save_indicator_note_begin();
    RecompOverlayText text = taken();
    check(strcmp(text.notice, "Saving...") == 0 && text.notice_busy, "the runner shows saving at once");
    check(strcmp(text.hint, "Shylph") == 0, "the notice joins the panel");
    recomp_save_indicator_note_end();
    text = taken();
    check(strcmp(text.notice, "Saved") == 0 && !text.notice_busy, "the runner shows saved");
    for (unsigned i = 0u; i < RECOMP_SAVE_INDICATOR_HOLD; ++i) recomp_save_indicator_frame();
    recomp_overlay_clear(&shop);
    recomp_overlay_publish_status(&shop);
    text = taken();
    check(recomp_overlay_empty(&text), "the overlay is empty once saved has gone");

    if (failures == 0) printf("save indicator test passed\n");
    return failures == 0 ? 0 : 1;
}
