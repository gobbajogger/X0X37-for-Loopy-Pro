/* SPDX-License-Identifier: GPL-3.0-only
 * Host test of the SMK-37 "<" and ">" route and hold repeat (firmware/src/app/smk_arrows.h).
 * tests/scenarios/arrows.x0x tests the same file on the whole app (the menu values). */
#include <assert.h>
#include <stdio.h>
#define ARW_FN __attribute__((unused))
#include "app/smk_arrows.h"

enum { L = 1, R = 2 };

/* run the frames from t0 to t1 (16 ms apart) with the arrows held; return the sum of the steps */
static int32_t hold(arw_t *a, uint32_t keys, uint32_t route, uint32_t t0, uint32_t t1, uint32_t *n)
{
    int32_t sum = 0, d;
    uint32_t t;
    for (t = t0; t < t1; t += 16u) {
        d = arw_frame(a, keys, route, t);
        assert(d >= -1 && d <= 1);
        sum += d;
        if (d && n)
            (*n)++;
    }
    return sum;
}

static void test_route(void)
{
    assert(arw_route(1, 0, 0) == ARW_MENU);
    assert(arw_route(1, 1, 0) == ARW_MENU);       /* GLO in clip mode: the arrows step the value */
    assert(arw_route(0, 1, 0) == ARW_NONE);       /* clip mode, no menu: nothing (CLP-27) */
    assert(arw_route(0, 0, 0) == ARW_SELECT);     /* no menu: SELECT, as before (GEN-29) */
    assert(arw_route(0, 0, 1) == ARW_NONE);       /* HOME held: nothing */
    assert(arw_route(1, 0, 1) == ARW_NONE);
    assert(arw_route(1, 1, 1) == ARW_NONE);
}

static void test_menu_step(void)
{
    arw_t a = {0};
    assert(arw_frame(&a, R, ARW_MENU, 1000) == 1);
    assert(arw_frame(&a, R, ARW_MENU, 1016) == 0);
    assert(arw_frame(&a, 0, ARW_MENU, 1032) == 0);
    assert(arw_frame(&a, L, ARW_MENU, 1048) == -1);
    assert(arw_frame(&a, 0, ARW_MENU, 1064) == 0);
    assert(arw_frame(&a, L | R, ARW_MENU, 1080) == 0);   /* both at once: no step, no repeat */
    assert(hold(&a, L | R, ARW_MENU, 1096, 3000, 0) == 0);
}

static void test_repeat_timing(void)
{
    uint32_t n = 0, t, first = 0, last = 0, k = 0;
    arw_t a = {0};
    int32_t d;
    assert(arw_frame(&a, R, ARW_MENU, 5000) == 1);
    /* no repeat before 400 ms */
    assert(hold(&a, R, ARW_MENU, 5016, 5400, 0) == 0);
    /* then one step each 80 ms (on the 16 ms frames: the first frame at or after each 80 ms point) */
    for (t = 5400; t < 7000; t += 16u) {
        d = arw_frame(&a, R, ARW_MENU, t);
        if (d) {
            assert(d == 1);
            if (!k)
                first = t;
            else
                assert(t - last >= 64u && t - last <= 96u);
            last = t;
            k++;
        }
    }
    assert(first == 5400u);
    assert(k == 20u);                                      /* (7000 - 5400) / 80 */
    /* release: the repeat stops at once */
    assert(hold(&a, 0, ARW_MENU, 7000, 8000, &n) == 0 && n == 0);
    /* "<" repeats down, the same */
    assert(arw_frame(&a, L, ARW_MENU, 9000) == -1);
    assert(hold(&a, L, ARW_MENU, 9016, 9400, 0) == 0);
    assert(hold(&a, L, ARW_MENU, 9400, 10200, 0) == -10);
    /* a late frame (a stall of 500 ms): one step, not a burst */
    assert(arw_frame(&a, L, ARW_MENU, 10700) == -1);
    assert(arw_frame(&a, L, ARW_MENU, 10716) == 0);
    /* the time wraps at 2^32 ms */
    a = (arw_t){0};
    assert(arw_frame(&a, R, ARW_MENU, 0xFFFFFF00u) == 1);
    assert(hold(&a, R, ARW_MENU, 0xFFFFFF10u, 0xFFFFFFF0u, 0) == 0);
    assert(arw_frame(&a, R, ARW_MENU, 0x00000090u) == 1);
}

static void test_other_arrow(void)
{
    arw_t a = {0};
    assert(arw_frame(&a, R, ARW_MENU, 0) == 1);
    assert(arw_frame(&a, R | L, ARW_MENU, 100) == -1);    /* the other arrow: its step, the repeat stops */
    assert(hold(&a, R | L, ARW_MENU, 116, 2000, 0) == 0);
    assert(hold(&a, R, ARW_MENU, 2000, 3000, 0) == 0);    /* "<" up, ">" still down: no new repeat */
}

static void test_select_no_repeat(void)
{
    arw_t a = {0};
    assert(arw_frame(&a, R, ARW_SELECT, 0) == 1);
    assert(hold(&a, R, ARW_SELECT, 16, 3000, 0) == 0);    /* no menu: one detent for each press, as before */
    assert(arw_frame(&a, 0, ARW_SELECT, 3000) == 0);
    assert(arw_frame(&a, L, ARW_SELECT, 3016) == -1);
    assert(arw_frame(&a, L | R, ARW_SELECT, 3032) == 1);   /* ">" pressed while "<" is held */
}

static void test_none(void)
{
    arw_t a = {0};
    assert(arw_frame(&a, R, ARW_NONE, 0) == 0);
    assert(hold(&a, R, ARW_NONE, 16, 2000, 0) == 0);
    /* a press while the route is NONE (clip mode, HOME) does not act later in a menu */
    assert(hold(&a, R, ARW_MENU, 2000, 4000, 0) == 0);
    /* a menu that opens while ">" is held: no step, no repeat until a new press */
    a = (arw_t){0};
    assert(arw_frame(&a, R, ARW_SELECT, 0) == 1);
    assert(hold(&a, R, ARW_MENU, 16, 2000, 0) == 0);
    /* a menu that closes while ">" repeats: the repeat stops */
    a = (arw_t){0};
    assert(arw_frame(&a, R, ARW_MENU, 0) == 1);
    assert(hold(&a, R, ARW_SELECT, 16, 2000, 0) == 0);
    assert(hold(&a, R, ARW_MENU, 2000, 3000, 0) == 0);
}

int main(void)
{
    test_route();
    test_menu_step();
    test_repeat_timing();
    test_other_arrow();
    test_select_no_repeat();
    test_none();
    printf("smk_arrows: all tests passed\n");
    return 0;
}
