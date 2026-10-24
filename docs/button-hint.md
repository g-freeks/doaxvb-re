# Button hint

During a volleyball match you play and during the Hopping game, the runner
can show which button does what, with the game's own A, B, X and Y buttons,
in a small panel at the right edge of the picture. The panel starts hidden:
a small **Back · Controls** reminder sits in the bottom left corner, and
pressing Back (Select, Share or View on other pads) shows the panel; Back
hides it again, and the choice holds until Back is pressed again. Back is
only taken for this while a match or the Hopping game is on screen; the game
showed no reaction to it there. It is one of the extra
overlays, off by default: tick **Extra overlays** in the launcher, or set
`RECOMP_EXTRA_OVERLAYS=1`, which also turns on the
[shops' collection panel](collection-log.md) and the
[save indicator](save-indicator.md). The casino games are left out: they
already show their choices on screen.

The Direct3D 11 presenter shows the button's letter in place of its image.

## What it shows

**Volleyball** follows the layout chosen under Options > Controller
Settings for the controller the person plays with, with the game's names for
the actions:

| Layout | Rows |
|---|---|
| Analog | A Attack: spike, serve; tap to tip. B Receive: pass; hard to set. |
| Digital (the runtime's default) | Each of A, B, X and Y with its action: Attack (Strong) spike, serve, block; Receive (Strong) set at the net; Attack (Soft) tip over a block; Receive (Soft) pass to partner. A button set to none is left out. |

The button rows go as the buttons sit on the pad: Y, X, B and A from the
top, with the right-hand Y and B indented. Two stick rows follow, their L
and R in a ring like the stick's cap: the left stick moves and aims (at the serve, back for
an underhand serve, forward for a jump serve), the right stick places the
partner (forward to play the net, back to stay deep). The panel shows while
the match scene runs with a person playing, and goes when the results menu
opens. A match with no person in it, such as the title screen's demo or
CPU Exhibition, shows none.

**Hopping game** reuses the layout's strengths
([`recomp-controls.md`](recomp-controls.md)): in Digital, a button set to a
Strong action makes a long jump (two pads) and a Soft one a short jump (one
pad); in Analog, A jumps farther the harder it is pressed. A jump chains when
pressed as the foot lands. The reminder and panel wait while the game shows
its own instructions before START.

The explanations come from the community guides on
[GameFAQs](https://gamefaqs.gamespot.com/xbox/561527-dead-or-alive-xtreme-beach-volleyball/faqs)
(the Volleyball FAQ and the general FAQ). A change of what the panel shows
waits 20 frames, so a screen that brings up its instructions a moment after
it opens does not flash the panel first.

## Guest contract

Addresses are for the verified USA executable. Everything is read-only.

| Address | Meaning |
|---|---|
| `0x5DECE0` | Scene running: 0 an Exhibition match, 1 the island ([`title-idle.md`](title-idle.md)) |
| `0x5DECE4`, `0x5DECE8` | Four bytes each: the player is a person, and that person's controller port |
| `0x5D89EC`, `0x4D6FC8` | Results menu open; match over |
| `0x4D72E3` | Island place; 11 is a vacation's match, 16 the Hopping game ([`ui-log.md`](ui-log.md)) |
| `0x5C5FD1` | Message panel (`0x5C5BA0`) queued-record count; nonzero while the game shows its own instructions |
| `0x41655D + port` | 1 analog, 0 digital; the runtime keeps it as a PC preference ([`recomp-controls.md`](recomp-controls.md)) |
| `0x416570 + 8 * port` | Layout: bytes 1 to 4 the digital actions of A, B, X and Y (3 Attack Strong, 4 Receive Strong, 5 Attack Soft, 6 Receive Soft, 0 none); bytes 5 and 6 the analog actions of A and B (1 Attack, 2 Receive) |

The layout was found on Controller Settings by changing one setting at a
time and comparing guest memory: switching Analog/Digital flipped only the
analog bytes, and changing A's action changed only byte 1 of each port's
record. The runtime starts every port in Digital, or in the mode last
chosen, which it keeps outside the saves. Not yet checked: which code reads
these bytes during a match.

A vacation's matches are not the match scene: they run on the island, place
11. A run checked one at Niki Beach: the reminder showed from the start of
play and went at the results (Score Point, Nice Point).

The buttons are resources 8, 10, 12 and 14 of entry 106 of `datadvd.afs`,
the pack with the Controller Settings buttons, 64-pixel paletted textures;
the runtime decodes them from the player's disc with the island icons
([`island_icons.c`](../recomp-runtime/island_icons.c)). Nothing extracted is
committed.

Back is read from the game's held-button mask for every controller,
`0x5F3138` (Back `0x0020`), once a presented frame.

With `RECOMP_AUTO_LOG=1` the runner prints an `[auto:hint]` line when what
it shows changes; `collapsed` is the reminder alone:

```text
[auto:hint] frame 2067: volleyball port 0 digital: collapsed
[auto:hint] frame 2293: volleyball port 0 digital: A Attack (Strong), B Receive (Strong), X Attack (Soft), Y Receive (Soft), L Move, aim, R Partner
[auto:hint] frame 1525: hopping port 0 digital: A Long jump, B Long jump, X Short jump, Y Short jump,  Chain
[auto:hint] frame 23770: none
```

Implementation: [`button_hint.c`](../recomp-runtime/button_hint.c), the key
panel in [`overlay_imgui_panel.cpp`](../recomp-runtime/overlay_imgui_panel.cpp).
