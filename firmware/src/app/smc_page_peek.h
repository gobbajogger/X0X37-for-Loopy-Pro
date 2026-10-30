/* SPDX-License-Identifier: GPL-3.0-only */
/* SMC mapped-page preview state. Pure transition code is host-testable. */
#pragma once
#include <stdint.h>

#define SMC_PAGE_PEEK_MS 1200u

typedef struct {
    uint8_t view, page, mixsel, prev_view;
} smc_page_peek_screen_t;

typedef struct {
    smc_page_peek_screen_t saved, shown;
    uint32_t until;
    uint8_t on;
} smc_page_peek_t;

static inline int smc_page_peek_equal(smc_page_peek_screen_t a,
                                      smc_page_peek_screen_t b)
{
    return a.view == b.view && a.page == b.page &&
           a.mixsel == b.mixsel && a.prev_view == b.prev_view;
}

static inline void smc_page_peek_cancel(smc_page_peek_t *p)
{
    p->on = 0;
}

/* A second mapped control keeps the original restore point; a repeated
 * control extends the deadline. An already visible target needs no peek. */
static inline int smc_page_peek_start(smc_page_peek_t *p,
                                      smc_page_peek_screen_t current,
                                      smc_page_peek_screen_t target,
                                      uint32_t now)
{
    if (p->on && !smc_page_peek_equal(current, p->shown)) p->on = 0;
    if (!p->on) {
        if (smc_page_peek_equal(current, target)) return 0;
        p->saved = current;
    }
    p->shown = target;
    p->until = now + SMC_PAGE_PEEK_MS;
    p->on = 1;
    return 1;
}

/* Returns 1 with the saved screen, 0 while holding, or -1 when navigation
 * changed the live screen. The caller must never restore on -1. */
static inline int smc_page_peek_step(smc_page_peek_t *p,
                                     smc_page_peek_screen_t current,
                                     uint32_t now,
                                     smc_page_peek_screen_t *restore)
{
    if (!p->on) return 0;
    if (!smc_page_peek_equal(current, p->shown)) {
        p->on = 0;
        return -1;
    }
    if ((int32_t)(now - p->until) < 0) return 0;
    *restore = p->saved;
    p->on = 0;
    return 1;
}

/* Before routing a different control, restore its original target context.
 * This prevents a temporary MIX/FX page from changing knob destinations. */
static inline int smc_page_peek_finish(smc_page_peek_t *p,
                                       smc_page_peek_screen_t current,
                                       smc_page_peek_screen_t *restore)
{
    if (!p->on) return 0;
    if (!smc_page_peek_equal(current, p->shown)) {
        p->on = 0;
        return 0;
    }
    *restore = p->saved;
    p->on = 0;
    return 1;
}
