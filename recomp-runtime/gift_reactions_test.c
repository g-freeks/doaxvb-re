#include "gift_reactions.h"

#include <stdio.h>
#include <string.h>

static int failures;

static void check(int condition, const char *what)
{
    if (!condition) {
        fprintf(stderr, "gift reactions test failed: %s\n", what);
        ++failures;
    }
}

int main(void)
{
    enum { TINA = 0, AYANE = 4, CHRISTIE = 7 };

    /* Rows checked against the guide by hand. */
    check(recomp_gift_reaction(523, AYANE) == 2, "Ayane loves Grape Juice");
    check(recomp_gift_reaction(603, TINA) == 2 && recomp_gift_reaction(603, CHRISTIE) == 2, "Tina and Christie love the Pistol");
    for (unsigned girl = 0; girl < 8u; ++girl) check(recomp_gift_reaction(629, girl) == -2, "every girl hates Zack's Autograph");
    check(recomp_gift_reaction(1, CHRISTIE) == -1, "Christie dislikes Sirius");
    check(recomp_gift_reaction(0, TINA) == 0 && recomp_gift_reaction(0xFFFF, TINA) == 0, "items without a row are neutral");
    check(recomp_gift_reaction(523, 8) == 0, "no ninth girl");
    check(strcmp(recomp_gift_reaction_mark(2), "++") == 0 && strcmp(recomp_gift_reaction_mark(-2), "--") == 0 &&
        strcmp(recomp_gift_reaction_mark(0), "") == 0, "marks");
    if (failures == 0) printf("gift reactions test passed\n");
    return failures == 0 ? 0 : 1;
}
