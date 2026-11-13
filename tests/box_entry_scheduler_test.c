/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../firmware/src/app/box_entry_scheduler.h"

static void test_no_asset_and_reduced_motion(void)
{
    dnbox_entry_scheduler_t s = {0};
    uint8_t f = 177;
    dnbox_entry_begin(&s, 100, 0, 0);
    assert(dnbox_entry_poll(&s, 100, 0, 0, 0, &f) == DNBOX_ENTRY_NONE);
    assert(f == 177);
    dnbox_entry_begin(&s, 100, 1, 1);
    assert(dnbox_entry_poll(&s, 100, 0, 0, 1, &f) == DNBOX_ENTRY_NONE);
    assert(f == 177);
}

static void test_exact_frames_and_no_extra_hold(void)
{
    dnbox_entry_scheduler_t s = {0};
    uint8_t f = 255;
    unsigned draws = 0;
    dnbox_entry_begin(&s, 800, 1, 0);
    for (uint32_t delta = 0; delta < DNBOX_ENTRY_END_MS; ++delta) {
        uint8_t state = dnbox_entry_poll(&s, 800u + delta, 0, 0, 1, &f);
        if (state == DNBOX_ENTRY_DRAW) {
            assert(f == draws);
            ++draws;
        } else {
            assert(state == DNBOX_ENTRY_NONE);
        }
    }
    assert(draws == 36u);
    assert(f == 35u);
    assert(dnbox_entry_poll(&s, 1315, 0, 0, 1, &f) == DNBOX_ENTRY_RESTORE_HOME);
    assert(dnbox_entry_poll(&s, 1316, 0, 0, 1, &f) == DNBOX_ENTRY_NONE);
}

static void test_skip_and_reduced_motion_midflight(void)
{
    dnbox_entry_scheduler_t s = {0};
    uint8_t f = 0;
    dnbox_entry_begin(&s, 0, 1, 0);
    assert(dnbox_entry_poll(&s, 0, 0, 0, 1, &f) == DNBOX_ENTRY_DRAW);
    assert(dnbox_entry_poll(&s, 1, 1, 0, 1, &f) == DNBOX_ENTRY_RESTORE_HOME);
    assert(dnbox_entry_poll(&s, 2, 1, 0, 1, &f) == DNBOX_ENTRY_NONE);
    dnbox_entry_begin(&s, 0, 1, 0);
    assert(dnbox_entry_poll(&s, 15, 0, 1, 1, &f) == DNBOX_ENTRY_RESTORE_HOME);
    dnbox_entry_begin(&s, 0, 1, 0);
    assert(dnbox_entry_poll(&s, 15, 0, 0, 0, &f) == DNBOX_ENTRY_RESTORE_HOME);
}

static void test_frame_drop_and_clock_wrap(void)
{
    dnbox_entry_scheduler_t s = {0};
    uint8_t f = 0;
    dnbox_entry_begin(&s, UINT32_MAX - 110u, 1, 0);
    assert(dnbox_entry_poll(&s, UINT32_MAX - 110u, 0, 0, 1, &f) == DNBOX_ENTRY_DRAW);
    assert(f == 0);
    /* 311 ms after start, even though the 32-bit tick counter wrapped. */
    assert(dnbox_entry_poll(&s, 200u, 0, 0, 1, &f) == DNBOX_ENTRY_DRAW);
    assert(f == 21u);
    assert(dnbox_entry_poll(&s, 200u, 0, 0, 1, &f) == DNBOX_ENTRY_NONE);
    assert(dnbox_entry_poll(&s, 410u, 0, 0, 1, &f) == DNBOX_ENTRY_RESTORE_HOME);
    assert(dnbox_entry_poll(&s, 411u, 0, 0, 1, &f) == DNBOX_ENTRY_NONE);
}

int main(void)
{
    test_no_asset_and_reduced_motion();
    test_exact_frames_and_no_extra_hold();
    test_skip_and_reduced_motion_midflight();
    test_frame_drop_and_clock_wrap();
    assert(sizeof(dnbox_entry_scheduler_t) <= 8u);
    assert(dnbox_entry_poll(0, 0, 0, 0, 0, 0) == DNBOX_ENTRY_NONE);
    puts("box-entry scheduler: PASS (no runtime asset or device writes)");
    return 0;
}
