# Gift reactions

How each girl reacts to an item given to her as a present, so you can see
who would love it before you buy or give it. It is part of the
[collection panel](collection-log.md), one of the extra overlays
(**Extra overlays** in the launcher, or `RECOMP_EXTRA_OVERLAYS=1`), which
shows over the item preview in the shops, the items screen, a gift's choice
of present at the hotel and the Collection. The reactions start hidden: a small **LS · click: reactions** reminder sits in the
bottom left corner, and a click of the left thumbstick shows a mark beside
each girl's portrait; another click hides them. The girl you play gets hers after the heart and the present.

| Mark | Reaction |
|---|---|
| `++` (green) | Love |
| `+` (green) | Like |
| none | Neutral, or not recorded |
| `-` (red) | Dislike |
| `--` (red) | Hate |

The Direct3D 11 presenter, which shows the panel as text, adds a line such
as `++ Ayane, + Lisa, -- Tina`. With `RECOMP_COLLECTION_LOG=1` the
`panel:` log line ends with `; reactions: Kasumi ++, ...`.

## Source

The table is
[`recomp-runtime/gift_reactions_table.inc`](../recomp-runtime/gift_reactions_table.inc):
one row per item id with at least one reaction that is not neutral, the
eight girls' grades in the game's order, and the guide's name for the item
as a comment. It comes from the "Item Reactions FAQ" by Robert Goode
(shipwreck), version 2.00, on
[GameFAQs](https://gamefaqs.gamespot.com/xbox/561527-dead-or-alive-xtreme-beach-volleyball/faqs),
which lists every girl's reaction to every swimsuit, accessory and item
except the neutral ones. Its names were matched to the game's item ids by
the game's own English names, ignoring case, accents and quotes, with a
swimsuit only matching a swimsuit (several gems share a name with one).

Seven names in the guide are spelled differently from the game's and were
matched by hand: Ukelele, Wild Frau, White Anenome, Recipe "Yakitora",
Chinese Desert, Manhatten and Ribbon Bracelet Red. Two entries name items
the game does not have and are left out: Ayane's Butterfly Hair Clip
(Violet), which the author's own checklist does not list either, and
Leifang's Black Framed Sunglasses.

### Where the guides disagree

The "Swimsuit Reaction Database" by Justin Kim (ju1c3r), also on GameFAQs,
lists likes, dislikes and hates for every swimsuit. It agrees with the table
on 439 of its 453 entries. The table keeps the Item Reactions FAQ's grade for
the other 14:

| Swimsuit | Girl | Item Reactions FAQ (shown) | Swimsuit Reaction Database |
|---|---|---|---|
| Undine | Kasumi | neutral | like or love |
| Acapulco | Ayane | hate | dislike |
| Virgo | Leifang | neutral | like or love |
| Flamingo | Lisa | dislike | hate |
| Goose | Lisa | dislike | hate |
| Heron | Lisa | dislike | hate |
| Hummingbird | Lisa | neutral | hate |
| Swan | Hitomi | neutral | like or love |
| Woodpecker | Tina | neutral | like or love |
| Hela | Lisa | neutral | hate |
| Ratri | Lisa | neutral | hate |
| Sati | Lisa | neutral | like or love |
| Alcyone | Ayane | hate | dislike |
| Thuban | Christie | neutral | like or love |

None of these has been checked in the game yet: giving the item during the
day to a girl who likes you enough to open it shows her reaction.

## Screens

| Screen | Item shown |
|---|---|
| Sports and Accessory Shop | the item under the shop's cursor |
| Items screen (the pool's or the hotel's View items) | the item under the grid's cursor |
| Make a gift (hotel, at night) | the item **inside** the present under the cursor, which the game shows only as its wrapping and "???" |
| Collection (hotel) | the item under the cursor, also for one not collected yet ("???") |

The Collection's preview is smaller than the others, and the portraits are
sized to fit it. Not yet checked: Zack of All Trades, and giving a present
to a girl met during the day, which may use the same grid as the hotel's.

## Guest contract

Addresses are for the verified USA executable. Everything is read-only.

| Address | Meaning |
|---|---|
| `0xB5B209` | The item grid's list on screen (s8): the items screen's tabs, or a gift's presents |
| `0xB5B1A8 + t`, `0xB5B1B7 + t` | List `t`'s entry count (s8) and its first entry in the slot table |
| `0xB5B148` | Slot table: 48 bytes, each a slot of the playing girl's live inventory |
| `0x31BE04` | The grid's cursor |
| `0x9D5B28 + 4t`, `0x9D5B34` | The Collection: tab `t`'s first entry, in a list of item ids |
| `0x31CC7D`, `0x31CE0C` | The Collection's tab and cursor ([`ui-log.md`](ui-log.md)) |
| `0x9D569A` | The item the preview shows: a present's wrapping, not its content |
| `0x5F3138` | Buttons held on any controller; the left thumbstick click is `0x0040` |

The game reads a grid cell as `0x000E12B0` does: slot
`[0xB5B148 + [0xB5B1B7 + t] + cursor]` of the playing girl's inventory at
`*(0x317634) + girl * 0x330`, one cell past the list's count being empty.
For a present `0x000C32C0` then swaps in the wrapping for the preview. A run
checked it on the hotel's gift screen: the wrappings under four cursor
positions matched the preview's, and the contents named on screen with the
overlay matched what the preview showed.

Seam: `grid_preview` in `program_manual.c` wraps `0x000C9FF0`, which queues
the grid's 3D preview once a frame and is reached only through the task
table, to learn that a grid is on screen; it runs the original unchanged.
The shop keeps its own seam on its row draw.
