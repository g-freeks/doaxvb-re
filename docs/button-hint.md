# Button hint

During a volleyball match you play and during the Hopping game, the runner
shows which button does what, with the game's own A, B, X and Y buttons, in
a small panel at the right edge of the picture. It is one of the extra
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
| Analog (the game's default) | A Attack: spike, serve; tap to tip. B Receive: pass; hard to set. |
| Digital | Each of A, B, X and Y with its action: Attack (Strong) spike, serve, block; Receive (Strong) set at the net; Attack (Soft) tip over a block; Receive (Soft) pass to partner. A button set to none is left out. |

Two stick rows follow: the left stick moves and aims (at the serve, back for
an underhand serve, forward for a jump serve), the right stick places the
partner (forward to play the net, back to stay deep). The panel shows while
the match scene runs with a person playing, and goes when the results menu
opens. A match with no person in it, such as the title screen's demo or
CPU Exhibition, shows none.

**Hopping game**: A jumps, farther the harder it is pressed, and chains
when pressed as the foot lands. The panel waits while the game shows its own
instructions before START.

The explanations come from the community guides on
[GameFAQs](https://gamefaqs.gamespot.com/xbox/561527-dead-or-alive-xtreme-beach-volleyball/faqs)
(the Volleyball FAQ and the general FAQ). A change of what the panel shows
waits 20 frames, so a screen that brings up its instructions a moment after
it opens does not flash the panel first.

## Guest contract

Addresses are for the verified USA executable. Everything is read-only.

| Address | Meaning |
|---|---|
| `0x5DECE0` | Scene running: 0 a match, 1 the island ([`title-idle.md`](title-idle.md)) |
| `0x5DECE4`, `0x5DECE8` | Four bytes each: the player is a person, and that person's controller port |
| `0x5D89EC`, `0x4D6FC8` | Results menu open; match over |
| `0x4D72E3` | Island place; 16 is the Hopping game ([`ui-log.md`](ui-log.md)) |
| `0x5C5FD1` | Message panel (`0x5C5BA0`) queued-record count; nonzero while the game shows its own instructions |
| `0x41655D + port` | 1 analog, 0 digital |
| `0x416570 + 8 * port` | Layout: bytes 1 to 4 the digital actions of A, B, X and Y (3 Attack Strong, 4 Receive Strong, 5 Attack Soft, 6 Receive Soft, 0 none); bytes 5 and 6 the analog actions of A and B (1 Attack, 2 Receive) |

The layout was found on Controller Settings by changing one setting at a
time and comparing guest memory: switching Analog/Digital flipped only the
analog bytes, and changing A's action changed only byte 1 of each port's
record. The game starts with every port analog. A Vacation's own settings,
chosen through Options on the save select, load with it; an Exhibition
match keeps the start-up layout. Not yet checked: which code reads these
bytes during a match, and a vacation match (the hint shows for any match a
person plays).

The buttons are resources 8, 10, 12 and 14 of entry 106 of `datadvd.afs`,
the pack with the Controller Settings buttons, 64-pixel paletted textures;
the runtime decodes them from the player's disc with the island icons
([`island_icons.c`](../recomp-runtime/island_icons.c)). Nothing extracted is
committed.

With `RECOMP_AUTO_LOG=1` the runner prints an `[auto:hint]` line when the
panel changes:

```text
[auto:hint] frame 2067: volleyball port 0 analog: A Attack, B Receive, L Move, aim, R Partner
[auto:hint] frame 1291: hopping: A Jump, A Chain
[auto:hint] frame 1051: none
```

Implementation: [`button_hint.c`](../recomp-runtime/button_hint.c), the key
panel in [`overlay_imgui_panel.cpp`](../recomp-runtime/overlay_imgui_panel.cpp).
