/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "app/smc_page_peek.h"

static int checks;
#define CHECK(expr) do { checks++; assert(expr); } while (0)

int main(void)
{
    smc_page_peek_t p;
    smc_page_peek_screen_t screen = {3, 2, 4, 1};
    smc_page_peek_screen_t original = screen;
    smc_page_peek_screen_t mix = {5, 0, 2, 1};
    smc_page_peek_screen_t fx = {6, 1, 4, 1};
    smc_page_peek_screen_t restore;
    memset(&p, 0, sizeof p);

    CHECK(smc_page_peek_start(&p, screen, mix, 100) == 1);
    screen = mix;
    CHECK(p.on && p.until == 1300);
    CHECK(smc_page_peek_step(&p, screen, 1299, &restore) == 0);
    CHECK(smc_page_peek_start(&p, screen, mix, 1000) == 1);
    CHECK(p.until == 2200);
    CHECK(smc_page_peek_step(&p, screen, 2000, &restore) == 0);
    CHECK(smc_page_peek_start(&p, screen, fx, 2100) == 1);
    screen = fx;
    CHECK(smc_page_peek_equal(p.saved, original));
    CHECK(smc_page_peek_step(&p, screen, 3300, &restore) == 1);
    CHECK(smc_page_peek_equal(restore, original));
    CHECK(!p.on);

    screen = original;
    CHECK(smc_page_peek_start(&p, screen, original, 3400) == 0);
    CHECK(!p.on);
    CHECK(smc_page_peek_start(&p, screen, mix, 3500) == 1);
    screen = mix;
    screen.view = 4;  /* panel navigation must cancel, not restore. */
    CHECK(smc_page_peek_step(&p, screen, 5000, &restore) == -1);
    CHECK(!p.on);
    CHECK(screen.view == 4);

    screen = original;
    CHECK(smc_page_peek_start(&p, screen, fx, 0xfffffff0u) == 1);
    screen = fx;
    CHECK(smc_page_peek_step(&p, screen, 0x0000049fu, &restore) == 0);
    CHECK(smc_page_peek_step(&p, screen, 0x000004a0u, &restore) == 1);
    CHECK(smc_page_peek_equal(restore, original));

    CHECK(smc_page_peek_start(&p, original, mix, 6000) == 1);
    CHECK(smc_page_peek_finish(&p, mix, &restore) == 1);
    CHECK(smc_page_peek_equal(restore, original));
    CHECK(!p.on);
    CHECK(smc_page_peek_start(&p, original, mix, 7000) == 1);
    CHECK(smc_page_peek_finish(&p, original, &restore) == 0);
    CHECK(!p.on);

    CHECK(smc_page_peek_start(&p, original, fx, 8000) == 1);
    smc_page_peek_cancel(&p);
    CHECK(!p.on);
    CHECK(smc_page_peek_step(&p, fx, 9000, &restore) == 0);
    printf("PASS: SMC mapped-page preview state: %d checks, 0 failed\n", checks);
    return 0;
}
