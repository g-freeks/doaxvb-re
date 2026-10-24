#ifndef DOAXBV_RECOMP_GIFT_REACTIONS_H
#define DOAXBV_RECOMP_GIFT_REACTIONS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* How a girl reacts to an item given to her, as the community guides
   record it: 2 love, 1 like, 0 neutral or not recorded, -1 dislike,
   -2 hate. girl is in the game's order (0 Tina ... 7 Christie). */
int recomp_gift_reaction(uint16_t item, unsigned girl);
/* "++", "+", "", "-" or "--" for a reaction. */
const char *recomp_gift_reaction_mark(int reaction);

#ifdef __cplusplus
}
#endif

#endif
