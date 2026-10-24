# Save indicator

The game does not tell you when it saves, so it is hard to know when you can
quit without losing progress. The runner shows a small **Saved** notice in the
picture's bottom right corner for about five seconds after each save, over
the game and over the shop panel. Once it has shown, quitting keeps
everything up to that point. It is on by default; set
`RECOMP_SAVE_INDICATOR=0` to turn it off.

A save that is still running shows **Saving...** with a turning mark. In the
saves seen so far the game writes its profile within a single frame, so only
**Saved** reaches the screen.

## When the game saves

Observed on 2026-10-24 with the USA executable, by driving the game with
[`tools/drive_game.py`](automation.md):

- The game saved each time the part of the day changed: taking a rest at the
  pool saved just before morning turned to daytime, daytime to evening and
  evening to night. The notice shows through the fade and the "Now Loading"
  screen and onto the next place.
- Buying a swimsuit and leaving the Sports Shop for the island map did **not**
  save: the profile on disk stayed byte for byte the same. Progress since the
  last change of the day part, such as that purchase, is lost if you quit
  then.
- Continuing a Vacation and walking the island map did not save.

Other places may save too; the notice shows for any save the runtime commits.

## How it works

The runtime already wraps the game's two save routines for its
[save transaction](recomp-save-contract.md). The adapter reports to the
indicator when an outer save begins and once it has committed, and the
indicator publishes the notice through the runtime's overlay, which both the
SDL GPU and Direct3D 11 presenters draw. A save nested inside another joins
it; **Saved** shows only when the outer save commits.

With `RECOMP_AUTO_LOG=1` the runner also prints an `[auto:save]` line when
the notice changes, for scripts (see [`ui-log.md`](ui-log.md)):

```text
[auto:save] frame 1249: saving
[auto:save] frame 1249: saved
[auto:save] frame 1549: idle
```

Implementation: [`save_indicator.c`](../recomp-runtime/save_indicator.c), the
calls in [`save_adapter.c`](../recomp-runtime/save_adapter.c), and the corner
notice in [`overlay_imgui_panel.cpp`](../recomp-runtime/overlay_imgui_panel.cpp).
