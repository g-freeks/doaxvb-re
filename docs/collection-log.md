# Shop collection panel and log

In the game's shops you otherwise learn whether a girl already has an item
only by pressing X and waiting for her reply. Tick **Collection in shops** in
the launcher, or set `RECOMP_SHOP_PANEL=1` before the runner starts, and the
shops show it over the item preview with the game's own icons:

- Bottom left, for the girl you play: the heart is lit when the item is in
  her Collection, the wrapped present when it is in her room.
- Down the right edge, the other seven girls' portraits, each lit when the
  item is in her Collection.

An unlit icon is three quarters transparent. The icons come from the game
disc when the panel first shows. The Direct3D 11 presenter, the default on
Windows, shows the same as two lines of text along the bottom of the screen
for now. Everything is read-only: no save or game state is changed.

`RECOMP_COLLECTION_LOG=1` is the diagnostic behind it: the runner prints a
`[auto:collection]` line to the console whenever the ownership table or a girl's
inventory changes, for each item under the shop cursor, and whenever the panel
changes. The `[auto:...]` prefix marks lines that scripts read; see
[`docs/ui-log.md`](ui-log.md).

## Log output

```text
[auto:collection] frame 1: first read; per girl had/collection: 0(?)=0/0 ... 4(Ayane)=1/0 ...
[auto:collection] frame 812: item 7 (type 2) girl 4 (Ayane): none -> had+collection
[auto:collection] frame 2040: 708 items changed; per girl had/collection: ...
[auto:collection] frame 2040: 408 slots changed; per girl inventory items: ...
[auto:collection] frame 2702: girl 6 (?) slot 3: empty -> item 590 (type 1, -3, order 4, flags 0x00, wrapping 668)
[auto:collection] frame 3884: panel: Ainsel / Had: Ayane  |  In room: Ayane; lit: present
```

A `panel:` line, written while the panel is also on, gives the panel's text
with ` / ` for its line break and the icons it shows lit: `heart`,
`present` and the other girls' names, or `none`.

When up to 16 items change in one frame, each changed girl and item gets its
own line, and so does each changed inventory slot. More than that, such as a
save loading, gets one line counting what each girl has had and what is in
her Collection. Frames count from the first
logged frame. Girls are numbered as in the game's English name table at
`0x3245BC`: 0 Tina, 1 Kasumi, 2 Helena, 3 Leifang, 4 Ayane, 5 Hitomi, 6 Lisa,
7 Christie. Play confirmed Kasumi, Leifang and Ayane.

## Guest contract

Addresses are for the verified USA executable.

| Address | Meaning |
|---|---|
| `0x9D8DCE` | 708 words, one per item id, read from the loaded save |
| `0x9D7810` | Room inventories: 8 girls, `0x198` bytes each, 51 slots of 8 bytes |
| `*(0x317634)` | Live inventories during play: 8 girls, `0x330` bytes each, 51 records of 16 bytes |
| `0x3003E8` | Table of pointers to 28-byte item definitions; the first word is the type |

The save file is a flat image of guest memory from `0x9D7788`, so a guest
address minus `0x9D7788` is its file offset. During play the log reads the
live inventories described under [Shop panel](#shop-panel), which hold the
same slots; the save's copy lags behind them. An inventory slot holds the item
id (`0xFFFF` when empty), a signed word, a word whose high byte numbers the
girl's acquisitions in order, and the id of the wrapping paper. The meaning
of the signed word and the order word's low byte is not known yet.

A girl has two bits in an ownership word, at twice her index: the low bit
says she has had the item, the high bit that it is in her Collection.
Exhibition's swimsuit table at `0x9DAE08` uses the same layout (see
[`swimsuit-unlock.md`](swimsuit-unlock.md)), and its rebuild at `0x000F82F0`
loads each Vacation save in turn and ORs this table into it. In a save file
the table starts at offset `0x1646`. A save read offline matched the encoding:
the girl in play had suits, accessories and other items in her Collection,
the others only their default suit, and no girl had a suit she cannot wear.

## Observed

In one play test (2026-10-24), continuing a Vacation as Ayane loaded the
table with the same counts as the save read offline: 79 items had, 32 in her
Collection. Buying a swimsuit set only her "had" bit for that item
(`none -> had`); the Collection bit stayed clear, and the save written
afterwards matched. A gift given to Leifang in the same session changed
nothing in this table.

Read offline, the same save shows each girl's inventory holding only items
she has had, and every non-default item in the other girls' inventories also
had by Ayane but missing from hers: the gifts she gave. The game's copy
loops at `0x000E6BA0` and `0x000E6C91` read the inventories from `0x9D7810`,
which agrees with the flat-image mapping.

In a second play test, gifts to Leifang and Kasumi each moved one item from
Ayane's inventory into the recipient's (items 557 and 547), set the
recipient's "had" bit, and renumbered Ayane's remaining order bytes. The
received slot had its wrapping cleared and the signed word set to -1.

Item names come from a table of pointers at `0x3095F8`, one per item id,
each to six 8-byte language entries chosen by the byte at `0x4D56D9`; `@|`
in a name is a line break. It names the two gifts White Origami (547, to
Kasumi) and Recipe "Sashimi" (557, to Leifang), as the game did. The
contiguous strings at `0x300EF8` are not in item order.

## Shop

The swimsuit shop's screen is set up by `0x000C9300`, and `0x000C8840`
draws one row of its list: the first argument is the row, the second the
screen context. Both were found by logging the game functions first reached
through dispatch while entering the shop. Entry `i` of shop `*(0x3AF8A1)` is
the first word of 16-byte entry `i` in the table at `*(0x317638)`, 15
entries a shop.

The list scrolls under a fixed highlight. The state at context `+0x18`
holds the highlighted row of the five-row window at `+0x22` (2), the entry
at the top of the window at `+0x24`, and the entry count at `+0x26` (15);
the top wraps, so the selected entry is `(top + row) % count`. In a scripted
run (2026-10-24) memory dumps at three cursor positions, and four more
scrolling up past the first entry, found these fields; the log then named
the highlighted suit correctly for eight entries in a row, checked against
screenshots. The word at `+0xC` of the list at state `+8`, which the init
reads, stayed 0 while scrolling.

An earlier guess, the task at `0x000DD7F0` with stock lists at `0x9D5B28`
and `0x9D5B34`, never ran in two shop visits. Buying calls `0x000CA9D0`,
which adds the item to the girl's inventory and sets her "had" bit.

Each time the item under the cursor changes, the log prints it with every
girl's ownership and whether her inventory holds it:

```text
[auto:collection] frame 4380: shop 0 cursor 3: item 163 Oreas (type 2): Tina=none ... Ayane=had+collection ...
```

In one shop visit Ayane said she had never seen Ainsel (no bits) and had
seen Oreas (both bits); she holds neither. Which bit decides the reply is
still open, because no suit with only the low bit was on sale. The game's own
checks near the shop test the high bit.

## Shop panel

The panel does not need the log. Its icons sit on the item preview, the
container at x 391.5-663.5, y 35.5-299.5 of the game's 720 x 480 screen. The
game stretches
its 720 x 480 screen across the whole picture, 3:2 at render scale 1 and
16:9 above it, so icons are placed by fractions of the picture's width and
height but kept square, and the portraits keep to the container's right
border. The playing girl is the character id at `0x9D9939` mapped to a girl
through the byte table at `0xEF394`, as the game's `0x000EF330` does (8 for
none); when it is not a girl, all eight portraits are shown and the bottom
row is left out. The
panel follows the cursor and ownership every presented frame and clears
when the shop closes.

The icons are the game's own. The island screens load entry 170 of
`datadvd.afs`, an XPR0 resource pack holding, among the frames and labels,
the eight girls' round map portraits (resources 0 to 14, in girl order,
128 x 128) and the item screen's tab icons (the present at 82 and the heart
at 84, 32 x 32), each an 8-bit paletted swizzled texture followed by its
palette. `island_icons.c` reads that entry from the player's disc the first
time the panel shows and decodes the ten icons; nothing extracted is kept
in the repository. The pack was found by its bytes: the heart's and the
present's texels, read from a memory dump of the item screen, are stored
uncompressed in that entry.

The panel goes through the overlay's status slot
(`recomp_overlay_publish_status`), which a presenter draws only while the
free camera or the viewer has nothing on screen; the images reach the
presenter through `recomp_overlay_set_icon_images`. The SDL GPU presenter
draws the icons. The D3D11 presenter, and any presenter when the pack cannot
be read, shows the same as two centred lines of text instead: the item's
name, then which girls have it in their Collection, which have only had it,
and whose room holds it now.

In a scripted run (2026-10-24) the Sports Shop showed Shylph with every
icon faded, and Oreas with Ayane's heart opaque and the rest faded, as the
log read them; checked in captures of the window, since runtime screenshots
leave the overlay out. A second run at `RECOMP_D3D_SCALE=2` (a 16:9
picture) showed the icons inside the container, after placing them by
picture height alone had shifted them in a 16:9 picture.

A purchase shows at once: buying Shylph unwrapped turned the present opaque
and left the heart faint, since buying sets only the "had" bit. The present
reads the live inventories, which the game keeps during play in 51 records
of 16 bytes a girl at `*(0x317634) + girl * 0x330`, the slot's eight bytes
first, as `0x0009CEA0` adds to them. The copy at `0x9D7810` is the save's.
RAM dumps before and after the purchase showed the two holding the same
items for every girl before it, and the bought suit in Ayane's live records
only, after it. The log follows the live records too: in a second purchase
it printed Ayane's slot 18 going from empty to Shylph in the same frame as
her "had" bit. At boot, before a save loads, the log prints one summary
line when the live records are first set up.

Not checked yet: the Accessory Shop, the D3D11 presenter, and
`RECOMP_D3D_WIDESCREEN=0` above scale 1.

## Seams

`program_manual.c` registers `shop_row` for `0x000C8840`. Only a code store
in the task table reaches that address, so the manual lookup wins without a
recipe change. The wrapper reports the shop object and runs the generated
original unchanged. `recomp_d3d_swap_adapter` calls
`recomp_collection_log_frame` once per presented frame. The log and the
panel only read guest memory.

## Checks

`collection_log_test.c` runs the log on synthetic guest memory: the first
read, one changed item, a gift into an inventory, name lookup, the shop
selection (including a window that wraps) and its reset after leaving, the
panel's text, its icons with and without a playing girl, and its removal, a
whole-table change, and that guest memory is left unchanged.
`island_icons_test.c` decodes a synthetic XPR0 pack and checks the swizzle
order, the size and format checks, and a missing disc.
`overlay_text_test.c` checks that the status slot waits while another
overlay is up.

`tools/check_collection.py` checks the same in the running game, by script
through [`tools/drive_game.py`](automation.md), with the log and the panel on.
It walks to the Sports Shop and checks:

- that the save select read the vacation's ownership table;
- three cursor moves: the collection log names the same item as the
  `[auto:ui]` shop line, and the panel shows it;
- a purchase, preferring an item no girl has had: the item goes into the
  playing girl's room, her had bit comes on, and the panel then lights the
  present.

It prints one `[auto:check] PASS` or `FAIL` line per check and exits with the
number that failed. The run's saves are put back afterwards; a purchase stays
in memory and is not saved. `test_check_collection.py` tests how it reads
the logs.
