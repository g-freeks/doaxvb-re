#include "gift_reactions.h"

#include <stddef.h>

enum { GIRLS = 8 };

static const struct {
    uint16_t item;
    int8_t reaction[GIRLS];
} rows[] = {
#include "gift_reactions_table.inc"
};

int recomp_gift_reaction(uint16_t item, unsigned girl)
{
    size_t low = 0, high = sizeof rows / sizeof rows[0];

    if (girl >= GIRLS) return 0;
    while (low < high) {
        const size_t middle = low + (high - low) / 2u;

        if (rows[middle].item == item) return rows[middle].reaction[girl];
        if (rows[middle].item < item) {
            low = middle + 1u;
        } else {
            high = middle;
        }
    }
    return 0;
}

const char *recomp_gift_reaction_mark(int reaction)
{
    switch (reaction) {
    case 2: return "++";
    case 1: return "+";
    case -1: return "-";
    case -2: return "--";
    default: return "";
    }
}
