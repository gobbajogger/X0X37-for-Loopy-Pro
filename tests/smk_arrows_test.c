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
    assert(arw_route(0, 1, 0) == ARW_NONE);       /* clip mode, no menu: the clip keys page has them (CLP-76) */
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

/* CLP-82: a list change with an arrow held: no step, no repeat, until a new press */
static void test_rearm(void)
{
    arw_t a = {0};
    assert(arw_frame(&a, R, ARW_MENU, 1000) == 1);
    assert(hold(&a, R, ARW_MENU, 1016, 1500, 0) > 0);     /* repeating in the old list */
    arw_rearm(&a, R);
    assert(hold(&a, R, ARW_MENU, 1500, 4000, 0) == 0);
    assert(arw_frame(&a, 0, ARW_MENU, 4016) == 0);
    assert(arw_frame(&a, L, ARW_MENU, 4032) == -1);       /* a new press steps */
    assert(arw_menu_id(5, 1) != arw_menu_id(5, 2) && arw_menu_id(5, 1) != arw_menu_id(6, 1));
}

/* CLP-82 (QA PR #719): the list changes in the same frame as a NEW press: the press steps the new list.
 * Also the first open of a menu (from ARW_NONE and ARW_SELECT) and repeated switches. */
static void test_rearm_fresh(void)
{
    arw_t a = {0};
    uint32_t i;
    /* the old list, ">" released; then the list changes and ">" goes down in the same frame */
    assert(arw_frame(&a, R, ARW_MENU, 1000) == 1);
    assert(arw_frame(&a, 0, ARW_MENU, 1016) == 0);
    arw_rearm(&a, R);
    assert(arw_frame(&a, R, ARW_MENU, 1032) == 1);
    assert(hold(&a, R, ARW_MENU, 1048, 1432, 0) == 0);    /* the repeat timing starts at the fresh press */
    assert(hold(&a, R, ARW_MENU, 1432, 1512, 0) == 1);
    /* "<" held from the old list, ">" fresh in the switch frame: only ">" counts */
    a = (arw_t){0};
    assert(arw_frame(&a, L, ARW_MENU, 2000) == -1);
    arw_rearm(&a, L | R);
    assert(arw_frame(&a, L | R, ARW_MENU, 2016) == 1);
    /* first open from no menu (clip mode) and from SELECT, with a fresh press in that frame */
    a = (arw_t){0};
    assert(arw_frame(&a, 0, ARW_NONE, 3000) == 0);
    arw_rearm(&a, R);
    assert(arw_frame(&a, R, ARW_MENU, 3016) == 1);
    a = (arw_t){0};
    assert(arw_frame(&a, 0, ARW_SELECT, 3100) == 0);
    arw_rearm(&a, L);
    assert(arw_frame(&a, L, ARW_MENU, 3116) == -1);
    /* first open with an arrow held from before: no step, no repeat */
    a = (arw_t){0};
    assert(arw_frame(&a, R, ARW_NONE, 4000) == 0);
    arw_rearm(&a, R);
    assert(hold(&a, R, ARW_MENU, 4016, 6000, 0) == 0);
    /* repeated switches with ">" held: never a step; after the release a new press steps */
    a = (arw_t){0};
    assert(arw_frame(&a, R, ARW_MENU, 7000) == 1);
    for (i = 0; i < 10; i++) {
        arw_rearm(&a, R);
        assert(hold(&a, R, ARW_MENU, 7016 + i * 600, 7600 + i * 600, 0) == 0);
    }
    assert(arw_frame(&a, 0, ARW_MENU, 14000) == 0);
    arw_rearm(&a, R);                                      /* switch and fresh press in one frame again */
    assert(arw_frame(&a, R, ARW_MENU, 14016) == 1);
}

int main(void)
{
    test_rearm();
    test_rearm_fresh();
    test_route();
    test_menu_step();
    test_repeat_timing();
    test_other_arrow();
    test_select_no_repeat();
    test_none();
    printf("smk_arrows: all tests passed\n");
    return 0;
}
