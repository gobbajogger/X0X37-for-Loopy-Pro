/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <string.h>
#include "app/smk_apc40.h"

static uint8_t g_channel[SMK_APC40_CLIP_COUNT];   /* APC display mode per cell */

static void test_eight_by_four_crops_viewport_margin(void)
{
    uint8_t velocity[SMK_APC40_CLIP_COUNT] = {0};
    uint8_t cols, rows, origin_col, origin_row, has_clips, pad;
    /* An 8x4 project begins on APC row 1 inside the controller's fixed 5-row view. */
    for (pad = 8u; pad < 40u; pad++)
        assert(smk_apc40_feedback_set(velocity, g_channel, pad, 0u, (uint8_t)(pad + 1u)));
    smk_apc40_build_layout(velocity, &cols, &rows, &origin_col, &origin_row, &has_clips);
    assert(cols == 8u && rows == 4u && origin_col == 0u && origin_row == 1u && has_clips);
    for (pad = 0u; pad < 8u; pad++) {
        assert(smk_apc40_note_for_pad(cols, rows, origin_col, origin_row, pad) == (uint8_t)(8u + pad));
        assert(smk_apc40_note_for_pad(cols, rows, origin_col, origin_row, (uint8_t)(pad + 8u)) ==
               (uint8_t)(16u + pad));
    }
}

static void test_four_by_four_folds_into_one_eight_by_two_grid(void)
{
    uint8_t velocity[SMK_APC40_CLIP_COUNT] = {0};
    uint8_t cols, rows, origin_col, origin_row, has_clips, row, col;
    for (row = 1u; row < 5u; row++) {
        for (col = 2u; col < 6u; col++) {
            uint8_t note = (uint8_t)(row * 8u + col);
            assert(smk_apc40_feedback_set(velocity, g_channel, note, (uint8_t)(row & 1u),
                                          (uint8_t)(20u + note)));
        }
    }
    /* An empty cell inside the matrix remains at its coordinate. */
    assert(smk_apc40_feedback_set(velocity, g_channel, 27u, 15u, 0u));
    smk_apc40_build_layout(velocity, &cols, &rows, &origin_col, &origin_row, &has_clips);
    assert(cols == 4u && rows == 4u && origin_col == 2u && origin_row == 1u && has_clips);

    /* Physical pads 1-4 and 9-12 contain source rows 2-3 (here APC rows 3-4). */
    for (col = 0u; col < 4u; col++) {
        uint8_t upper_logical = smk_apc40_logical_pad_for_physical(col);
        uint8_t lower_logical = smk_apc40_logical_pad_for_physical((uint8_t)(col + 8u));
        assert(smk_apc40_note_for_pad(cols, rows, origin_col, origin_row, upper_logical) ==
               (uint8_t)(34u + col));
        assert(smk_apc40_note_for_pad(cols, rows, origin_col, origin_row, lower_logical) ==
               (uint8_t)(26u + col));
    }
    /* Source rows 0-1 occupy physical pads 5-8 and 13-16. */
    for (col = 0u; col < 4u; col++) {
        uint8_t upper_logical = smk_apc40_logical_pad_for_physical((uint8_t)(col + 4u));
        uint8_t lower_logical = smk_apc40_logical_pad_for_physical((uint8_t)(col + 12u));
        assert(smk_apc40_note_for_pad(cols, rows, origin_col, origin_row, upper_logical) ==
               (uint8_t)(18u + col));
        assert(smk_apc40_note_for_pad(cols, rows, origin_col, origin_row, lower_logical) ==
               (uint8_t)(10u + col));
    }
    assert(velocity[27u] == 0u); /* callers leave this mapped pad black and unlaunchable */
}

static void test_sparse_eight_wide_cells_keep_relative_gaps(void)
{
    uint8_t velocity[SMK_APC40_CLIP_COUNT] = {0};
    uint8_t cols, rows, origin_col, origin_row, has_clips;
    assert(smk_apc40_feedback_set(velocity, g_channel, 9u, 0u, 60u));   /* row 1, column 1 */
    assert(smk_apc40_feedback_set(velocity, g_channel, 11u, 1u, 61u));  /* row 1, column 3 */
    assert(smk_apc40_feedback_set(velocity, g_channel, 25u, 15u, 62u)); /* row 3, column 1 */
    smk_apc40_build_layout(velocity, &cols, &rows, &origin_col, &origin_row, &has_clips);
    assert(cols == 4u && rows == 3u && origin_col == 1u && origin_row == 1u && has_clips);
    /* Folded (0.9): logical pads 4-6 and 12 hold rows 0-1; pads 0-2 hold row 2. */
    assert(smk_apc40_note_for_pad(cols, rows, origin_col, origin_row, 4u) == 9u);
    assert(smk_apc40_note_for_pad(cols, rows, origin_col, origin_row, 5u) == 10u);
    assert(velocity[10u] == 0u); /* internal gaps stay black instead of compacting */
    assert(smk_apc40_note_for_pad(cols, rows, origin_col, origin_row, 6u) == 11u);
    assert(smk_apc40_note_for_pad(cols, rows, origin_col, origin_row, 12u) == 17u);
    assert(smk_apc40_note_for_pad(cols, rows, origin_col, origin_row, 0u) == 25u);
}

static void test_viewport_with_one_row_uses_one_pad_row(void)
{
    uint8_t velocity[SMK_APC40_CLIP_COUNT] = {0};
    uint8_t cols, rows, origin_col, origin_row, has_clips;
    assert(smk_apc40_feedback_set(velocity, g_channel, 35u, 0u, 70u));
    smk_apc40_build_layout(velocity, &cols, &rows, &origin_col, &origin_row, &has_clips);
    assert(cols == 8u && rows == 1u && origin_col == 3u && origin_row == 4u && has_clips);
    assert(smk_apc40_note_for_pad(cols, rows, origin_col, origin_row, 0u) == 35u);
    assert(smk_apc40_note_for_pad(cols, rows, origin_col, origin_row, 8u) == SMK_APC40_INVALID_NOTE);
}

static void test_viewport_clamp_can_always_return(void)
{
    int16_t offset = 0;
    assert(!smk_apc40_can_scroll(offset, 1, 0u));
    assert(smk_apc40_can_scroll(offset, 1, 1u));
    offset = smk_apc40_scroll_after_move(offset, 1);
    assert(offset == 1);
    assert(!smk_apc40_can_scroll(offset, 1, 0u));
    assert(smk_apc40_can_scroll(offset, -1, 0u));
    offset = smk_apc40_scroll_after_move(offset, -1);
    assert(offset == 0);
    assert(!smk_apc40_can_scroll(offset, -1, 0u));
}

static void test_physical_pad_rows_are_flipped_only(void)
{
    uint8_t pad;
    for (pad = 0u; pad < SMK_APC40_PAD_COUNT; pad++) {
        uint8_t flipped = (uint8_t)(pad ^ SMK_APC40_PAD_ROW_SIZE);
        assert(smk_apc40_logical_pad_for_physical(pad) == flipped);
        assert(smk_apc40_physical_pad_for_logical(pad) == flipped);
        assert(smk_apc40_physical_pad_for_logical(smk_apc40_logical_pad_for_physical(pad)) == pad);
    }
    assert(smk_apc40_logical_pad_for_physical(16u) == SMK_APC40_INVALID_NOTE);
}

static void test_transport_button_notes_and_feedback_states(void)
{
    uint8_t sent = 0u;
    assert(smk_apc40_transport_event(&sent, 0u, 1u) == SMK_APC40_BUTTON_IGNORED);
    assert(!sent);
    assert(smk_apc40_transport_event(&sent, 1u, 1u) == SMK_APC40_BUTTON_ON);
    assert(sent);
    assert(smk_apc40_transport_event(&sent, 1u, 1u) == SMK_APC40_BUTTON_CONSUMED);
    assert(smk_apc40_transport_event(&sent, 0u, 0u) == SMK_APC40_BUTTON_OFF);
    assert(!sent);
    assert(smk_apc40_transport_event(&sent, 0u, 0u) == SMK_APC40_BUTTON_IGNORED);
    assert(smk_apc40_transport_note(0u) == SMK_APC40_PLAY_NOTE);
    assert(smk_apc40_transport_note(1u) == SMK_APC40_RECORD_NOTE);
    assert(smk_apc40_transport_feedback_note(SMK_APC40_PLAY_NOTE));
    assert(smk_apc40_transport_feedback_note(SMK_APC40_RECORD_NOTE));
    assert(!smk_apc40_transport_feedback_note(0x5Cu));
    assert(smk_apc40_transport_feedback_on(127u));
    assert(!smk_apc40_transport_feedback_on(0u));
}

static void test_eight_by_four_uses_exactly_two_views(void)
{
    uint8_t velocity[SMK_APC40_CLIP_COUNT] = {0};
    uint8_t cols, rows, origin_col, origin_row, has_clips, pad, view;
    for (pad = 0u; pad < 32u; pad++)
        assert(smk_apc40_feedback_set(velocity, g_channel, pad, 0u, 5u));
    smk_apc40_build_layout(velocity, &cols, &rows, &origin_col, &origin_row, &has_clips);
    assert(cols == 8u && rows == 4u && origin_row == 0u);
    assert(smk_apc40_view_count(cols, rows) == 2u);
    /* View 0 = rows 0-1, view 1 = rows 2-3. No view overlaps or shows an extra row. */
    for (pad = 0u; pad < 16u; pad++) {
        assert(smk_apc40_note_for_pad_view(cols, rows, 0u, 0u, 0u, pad) == pad);
        assert(smk_apc40_note_for_pad_view(cols, rows, 0u, 0u, 1u, pad) == (uint8_t)(16u + pad));
    }
    /* Navigation is clamped to the two views. */
    view = smk_apc40_view_after_move(cols, rows, 0u, -1);
    assert(view == 0u);
    view = smk_apc40_view_after_move(cols, rows, view, 1);
    assert(view == 1u);
    view = smk_apc40_view_after_move(cols, rows, view, 1);
    assert(view == 1u);
    /* 8x2 and 8x1 use one view. 8x5 uses three views; the last is clamped to rows 3-4. */
    assert(smk_apc40_view_count(8u, 2u) == 1u && smk_apc40_view_count(8u, 1u) == 1u);
    assert(smk_apc40_view_count(8u, 5u) == 3u && smk_apc40_view_row_start(8u, 5u, 2u) == 3u);
    assert(smk_apc40_view_count(8u, 3u) == 2u && smk_apc40_view_row_start(8u, 3u, 1u) == 1u);
}

static void test_four_by_four_first_half_on_pads_1_4_and_9_12(void)
{
    uint8_t velocity[SMK_APC40_CLIP_COUNT] = {0};
    uint8_t cols, rows, origin_col, origin_row, has_clips, row, col, phys;
    for (row = 0u; row < 4u; row++)
        for (col = 0u; col < 4u; col++)
            assert(smk_apc40_feedback_set(velocity, g_channel, (uint8_t)(row * 8u + col), 0u, 9u));
    smk_apc40_build_layout(velocity, &cols, &rows, &origin_col, &origin_row, &has_clips);
    assert(cols == 4u && rows == 4u && smk_apc40_view_count(cols, rows) == 1u);
    for (phys = 0u; phys < 16u; phys++) {
        uint8_t logical = smk_apc40_logical_pad_for_physical(phys);
        uint8_t note = smk_apc40_note_for_pad_view(cols, rows, origin_col, origin_row, 0u, logical);
        uint8_t source_row = (uint8_t)(note / 8u);
        assert(note != SMK_APC40_INVALID_NOTE && note % 8u == phys % 4u);
        if (phys % 8u < 4u)   /* pads 1-4 and 9-12: second half (rows 2-3) */
            assert(source_row >= 2u);
        else                  /* pads 5-8 and 13-16: first half (rows 0-1) */
            assert(source_row <= 1u);
    }
    /* A 4x4 project with an empty outer column still folds. */
    for (row = 0u; row < 4u; row++)
        velocity[row * 8u + 3u] = 0u;
    smk_apc40_build_layout(velocity, &cols, &rows, &origin_col, &origin_row, &has_clips);
    assert(cols == 4u && rows == 4u);
}

static void test_transport_led_lights_on_press(void)
{
    uint8_t led = 0u;
    led = smk_apc40_transport_led_after_press(led);
    assert(led == 1u);
    led = smk_apc40_transport_led_after_press(led);
    assert(led == 0u);
}

static void test_page_change_snapshot_clears_old_page(void)
{
    uint8_t velocity[SMK_APC40_CLIP_COUNT] = {0}, channel[SMK_APC40_CLIP_COUNT] = {0};
    uint8_t cols, rows, origin_col, origin_row, has_clips, count = 0u, note, phys;
    uint64_t mask = 0u;
    /* Old page: an 8x4 grid of clips. */
    for (note = 0u; note < 32u; note++)
        velocity[note] = 5u;
    /* New page: Loopy sends only the 16 clips of a 4x4 grid, and no "off" notes. */
    for (note = 0u; note < 32u; note++)
        if (note % 8u < 4u) {
            smk_apc40_feedback_set(velocity, channel, note, 0u, 21u);
            smk_apc40_burst_touch(&mask, &count, note);
        }
    assert(count == 16u);
    assert(smk_apc40_burst_finish(velocity, channel, &mask, &count) == 1u);
    assert(mask == 0u && count == 0u);
    for (note = 0u; note < SMK_APC40_CLIP_COUNT; note++)
        assert((velocity[note] != 0u) == (note < 32u && note % 8u < 4u));
    /* The 4x4 now fills all 16 pads in one view. */
    smk_apc40_build_layout(velocity, &cols, &rows, &origin_col, &origin_row, &has_clips);
    assert(cols == 4u && rows == 4u && smk_apc40_view_count(cols, rows) == 1u);
    for (phys = 0u; phys < 16u; phys++) {
        uint8_t n = smk_apc40_note_for_pad_view(cols, rows, origin_col, origin_row, 0u,
                                                smk_apc40_logical_pad_for_physical(phys));
        assert(n != SMK_APC40_INVALID_NOTE && velocity[n] != 0u);
    }
}

static void test_small_burst_keeps_other_clips(void)
{
    uint8_t velocity[SMK_APC40_CLIP_COUNT] = {0}, channel[SMK_APC40_CLIP_COUNT] = {0};
    uint8_t count = 0u, note;
    uint64_t mask = 0u;
    for (note = 0u; note < 32u; note++)
        velocity[note] = 5u;
    /* A cue or play change of 3 clips is not a page change. */
    for (note = 0u; note < 3u; note++) {
        smk_apc40_feedback_set(velocity, channel, note, 9u, 21u);
        smk_apc40_burst_touch(&mask, &count, note);
    }
    smk_apc40_burst_touch(&mask, &count, 1u);   /* repeated cell counts once */
    assert(count == 3u);
    assert(smk_apc40_burst_finish(velocity, channel, &mask, &count) == 0u);
    for (note = 0u; note < 32u; note++)
        assert(velocity[note] != 0u);
}

static void test_play_led_blinks_with_clock(void)
{
    uint32_t tick, on = 0u;
    assert(smk_apc40_play_led(0u, 1u, 0u) == 0u);
    assert(smk_apc40_play_led(1u, 0u, 13u) == 1u);   /* lit on press, no clock yet */
    for (tick = 0u; tick < 96u; tick++)               /* 4 beats */
        on += smk_apc40_play_led(1u, 1u, tick);
    assert(on == 24u);                                /* 6 ticks lit per beat */
    assert(smk_apc40_play_led(1u, 1u, 0u) && smk_apc40_play_led(1u, 1u, 24u));
    assert(!smk_apc40_play_led(1u, 1u, 12u));
}

static void test_partial_latch_grows_to_full_four_by_four(void)
{
    uint8_t velocity[SMK_APC40_CLIP_COUNT] = {0};
    uint8_t c0, r0, oc0, or0, c1, r1, oc1, or1, has, row, col, phys;
    /* The latch saw only rows 0-1 of a 4x4 grid: an 8-wide 2-row layout. */
    for (row = 0u; row < 2u; row++)
        for (col = 0u; col < 4u; col++)
            velocity[row * 8u + col] = 9u;
    smk_apc40_build_layout(velocity, &c0, &r0, &oc0, &or0, &has);
    assert(c0 == 8u && r0 == 2u);
    /* Rows 2-3 arrive later. The layout grows and folds to one 4x4 view. */
    for (row = 2u; row < 4u; row++)
        for (col = 0u; col < 4u; col++)
            velocity[row * 8u + col] = 9u;
    smk_apc40_build_layout(velocity, &c1, &r1, &oc1, &or1, &has);
    assert(c1 == 4u && r1 == 4u);
    assert(smk_apc40_layout_grows(c0, r0, oc0, or0, c1, r1, oc1, or1));
    for (phys = 0u; phys < 16u; phys++)
        assert(velocity[smk_apc40_note_for_pad_view(c1, r1, oc1, or1, 0u,
                        smk_apc40_logical_pad_for_physical(phys))] != 0u);
    /* A clip LED turning off cannot shrink the layout. */
    assert(!smk_apc40_layout_grows(c1, r1, oc1, or1, 4u, 3u, 0u, 0u));
    assert(!smk_apc40_layout_grows(c1, r1, oc1, or1, c1, r1, oc1, or1));
    /* A 4x4 whose last row is still empty (late feedback) folds as in 0.9. */
    velocity[24] = velocity[25] = velocity[26] = velocity[27] = 0u;
    smk_apc40_build_layout(velocity, &c1, &r1, &oc1, &or1, &has);
    assert(c1 == 4u && r1 == 3u && smk_apc40_view_count(c1, r1) == 1u);
    /* 8-wide layouts never grow after the latch (0.7 behavior). */
    assert(!smk_apc40_layout_grows(8u, 2u, 0u, 0u, 8u, 4u, 0u, 0u));
}

static void test_pending_clips_blink_static_clips_steady(void)
{
    uint8_t ch;
    /* Only blinking channels 11-15 (queued) blink. Static, one-shot and pulsing
     * (playing) states stay steady, so not all pads blink on clip mode entry. */
    for (ch = 11u; ch <= 15u; ch++) {
        assert(smk_apc40_is_queued(ch));
        assert(smk_apc40_brightness(ch, 0u, 0u) == 255u);
        assert(smk_apc40_brightness(ch, 200u, 0u) == 0u);
    }
    for (ch = 0u; ch < 11u; ch++) {
        assert(!smk_apc40_is_queued(ch));
        assert(smk_apc40_brightness(ch, 200u, 1u) == 255u);
    }
    /* One blink per beat: 24 ticks of 24 PPQN clock. */
    assert(SMK_APC40_QUEUED_BLINK_TICKS == 24u);
    /* Protocol rates: 1/24 note is 4 ticks, not 1 tick (the old 1-tick table blinked too fast). */
    assert(smk_apc40_animation_clock_period(11u) == 4u);
    assert(smk_apc40_animation_clock_period(14u) == 24u);
}

static void test_scene_launch_and_cue_bursts_keep_other_clips(void)
{
    uint8_t velocity[SMK_APC40_CLIP_COUNT] = {0}, channel[SMK_APC40_CLIP_COUNT] = {0};
    uint8_t count = 0u, note;
    uint64_t mask = 0u;
    for (note = 0u; note < 16u; note++)
        velocity[note] = 5u;                  /* an 8x2 page with 16 clips */
    /* Cue burst: 8 clips of row 0 go to a blink state. Not a page change. */
    for (note = 0u; note < 8u; note++) {
        smk_apc40_feedback_set(velocity, channel, note, 14u, 21u);
        smk_apc40_burst_touch(&mask, &count, note);
    }
    assert(smk_apc40_burst_finish(velocity, channel, &mask, &count) == 0u);
    for (note = 0u; note < 16u; note++)
        assert(velocity[note] != 0u);
    /* 32-clip page: a scene launch plays 8 clips. 8 of 32 is less than half. */
    for (note = 0u; note < 32u; note++)
        velocity[note] = 5u, channel[note] = 0u;
    for (note = 0u; note < 8u; note++) {
        smk_apc40_feedback_set(velocity, channel, note, 0u, 21u);
        smk_apc40_burst_touch(&mask, &count, note);
    }
    assert(smk_apc40_burst_finish(velocity, channel, &mask, &count) == 0u);
    for (note = 0u; note < 32u; note++)
        assert(velocity[note] != 0u);
}

static void test_row_launch_on_small_page_keeps_clips(void)
{
    uint8_t velocity[SMK_APC40_CLIP_COUNT] = {0}, channel[SMK_APC40_CLIP_COUNT] = {0};
    uint8_t count = 0u, note;
    uint64_t mask = 0u;
    for (note = 0u; note < 16u; note++)
        velocity[note] = 5u;                  /* 8x2 page, 16 clips */
    for (note = 0u; note < 8u; note++) {      /* row 0 starts: 8 static cells, half of lit */
        smk_apc40_feedback_set(velocity, channel, note, 0u, 21u);
        smk_apc40_burst_touch(&mask, &count, note);
    }
    assert(smk_apc40_burst_finish(velocity, channel, &mask, &count) == 0u);
    for (note = 0u; note < 16u; note++)
        assert(velocity[note] != 0u);
}

/* Replay of captured Loopy output (2026-10-15): a refresh sends note off for all 40
 * clip notes, then all colors (0 for unused cells) 4 ms later. */
static void test_loopy_refresh_keeps_last_colors(void)
{
    uint8_t vel[SMK_APC40_CLIP_COUNT] = {0}, ch[SMK_APC40_CLIP_COUNT] = {0};
    uint8_t flags[SMK_APC40_CLIP_COUNT] = {0}, tick[SMK_APC40_CLIP_COUNT] = {0};
    static const uint8_t cap[8] = {0x0B, 0x0B, 0x0F, 0x0F, 0x13, 0x13, 0x37, 0x37};
    uint8_t n;
    uint32_t ms;
    for (n = 8u; n < 40u; n++)
        vel[n] = 0x27u;                               /* stale colors of the old page */
    vel[3] = 0x1Fu;
    for (n = 0u; n < 40u; n++)
        smk_apc40_cell_event(vel, ch, flags, tick, n, 0u, 0u, (uint8_t)(1000u >> 4));
    for (n = 0u; n < 40u; n++)
        smk_apc40_cell_event(vel, ch, flags, tick, n, 0u, n < 8u ? 0u : cap[n % 8u],
                             (uint8_t)(1004u >> 4));
    for (ms = 1010u; ms < 6000u; ms += 16u) {
        smk_apc40_cells_age(vel, ch, flags, tick, (uint8_t)(ms >> 4));
        for (n = 0u; n < 40u; n++) {
            assert(vel[n] == (n < 8u ? 0u : cap[n % 8u]));  /* never dies, no stale cell */
            assert(!(flags[n] & SMK_APC40_CELL_QUEUED));    /* a refresh is not a flash */
        }
    }
}

/* Captured flash of a queued clip: note 0x20 color 0x39, then 0, every ~100 ms. */
static void test_loopy_flash_is_held_and_blinks_on_beat(void)
{
    uint8_t vel[SMK_APC40_CLIP_COUNT] = {0}, ch[SMK_APC40_CLIP_COUNT] = {0};
    uint8_t flags[SMK_APC40_CLIP_COUNT] = {0}, tick[SMK_APC40_CLIP_COUNT] = {0};
    uint32_t ms;
    smk_apc40_cell_event(vel, ch, flags, tick, 0x20u, 0u, 0x37u, 0u);
    for (ms = 1920u; ms < 3200u; ms += 100u) {
        uint8_t on = (uint8_t)(((ms - 1920u) / 100u) % 2u == 0u);
        smk_apc40_cell_event(vel, ch, flags, tick, 0x20u, 0u, on ? 0x39u : 0u, (uint8_t)(ms >> 4));
        smk_apc40_cells_age(vel, ch, flags, tick, (uint8_t)((ms + 50u) >> 4));
        if (ms >= 2320u)                             /* after two toggles */
            assert(vel[0x20] == 0x39u && (flags[0x20] & SMK_APC40_CELL_QUEUED));
    }
    /* Clip starts: steady color, no more flash. Queued state ends, color stays. */
    smk_apc40_cell_event(vel, ch, flags, tick, 0x20u, 0u, 0x39u, (uint8_t)(3226u >> 4));
    smk_apc40_cells_age(vel, ch, flags, tick, (uint8_t)(3700u >> 4));
    assert(vel[0x20] == 0x39u && !(flags[0x20] & SMK_APC40_CELL_QUEUED));
    /* A plain off clears the cell at once. */
    smk_apc40_cell_event(vel, ch, flags, tick, 0x20u, 0u, 0u, (uint8_t)(4000u >> 4));
    assert(vel[0x20] == 0u);
    /* A flash that ends on off clears the cell after the queued state ends. */
    for (ms = 5000u; ms < 5500u; ms += 100u)
        smk_apc40_cell_event(vel, ch, flags, tick, 0x20u, 0u,
                             ((ms - 5000u) / 100u) % 2u == 0u ? 0x39u : 0u, (uint8_t)(ms >> 4));
    smk_apc40_cell_event(vel, ch, flags, tick, 0x20u, 0u, 0u, (uint8_t)(5500u >> 4));
    assert(vel[0x20] == 0x39u);
    smk_apc40_cells_age(vel, ch, flags, tick, (uint8_t)(6000u >> 4));
    assert(vel[0x20] == 0u && !(flags[0x20] & SMK_APC40_CELL_QUEUED));
}

/* Blink period against MIDI Clock: one cycle per 24 ticks, 12 on and 12 off. */
static void test_blink_period_is_one_beat(void)
{
    uint32_t t, on = 0u, edges = 0u;
    uint8_t prev = smk_apc40_beat_phase(0u) < 128u;
    for (t = 0u; t < 24u * 8u; t++) {
        uint8_t lit = smk_apc40_beat_phase(t) < 128u;
        on += lit;
        if (lit && !prev)
            edges++;
        prev = lit;
    }
    assert(on == 12u * 8u);
    assert(edges == 7u);                           /* 8 beats: 7 rising edges after tick 0 */
    assert(smk_apc40_beat_phase(0u) < 128u && smk_apc40_beat_phase(24u) < 128u);
    assert(smk_apc40_beat_phase(12u) >= 128u);
}

static void test_lcd_map_fits_used_area(void)
{
    smk_apc40_lcd_map_t m;
    m = smk_apc40_lcd_map(8u, 2u, 0u, 0u, 0u, 240u, 138u);          /* no clips: full 8x5 */
    assert(m.ncols == 8u && m.nrows == 5u && m.pitch == 26u);
    m = smk_apc40_lcd_map(4u, 4u, 0u, 1u, 1u, 240u, 138u);          /* 4x4: bigger cells */
    assert(m.col0 == 0u && m.row0 == 1u && m.ncols == 4u && m.nrows == 4u && m.pitch == 33u);
    assert(m.x0 >= 0 && m.x0 + 4 * 33 <= 240 && m.y0 >= 0 && m.y0 + 4 * 33 <= 138);
    /* Screen row = nrows - 1 - (source row - row0): the highest APC row (the top
     * Loopy row) is on top, for 4x4 and 8-wide alike. The 4x4 pad fold is not used. */
    assert(smk_apc40_lcd_screen_row(&m, 4u) == 0u && smk_apc40_lcd_screen_row(&m, 1u) == 3u);
    m = smk_apc40_lcd_map(8u, 4u, 0u, 1u, 1u, 240u, 138u);          /* 8x4 */
    assert(smk_apc40_lcd_screen_row(&m, 4u) == 0u && smk_apc40_lcd_screen_row(&m, 1u) == 3u);
    assert(m.ncols == 8u && m.nrows == 4u && m.pitch == 29u);
    m = smk_apc40_lcd_map(3u, 2u, 2u, 3u, 1u, 240u, 138u);          /* small area: big cells */
    assert(m.col0 == 2u && m.row0 == 3u && m.ncols == 3u && m.nrows == 2u && m.pitch == 67u);
}

static void test_loopy_history_packets(void)
{
    /* USB-MIDI: CIN 9, 9F 70 7F (undo on); CIN 8, 8F 70 00 (off); 9F 71 7F (redo on). */
    assert(smk_apc40_history_packet(0u, 1u) == (0x09u | 0x9Fu << 8 | 0x70u << 16 | 0x7Fu << 24));
    assert(smk_apc40_history_packet(0u, 0u) == (0x08u | 0x8Fu << 8 | 0x70u << 16));
    assert(smk_apc40_history_packet(1u, 1u) == (0x09u | 0x9Fu << 8 | 0x71u << 16 | 0x7Fu << 24));
    assert(smk_apc40_history_packet(1u, 0u) == (0x08u | 0x8Fu << 8 | 0x71u << 16));
    /* Not an APC40 mkII note: above the clip notes and the APC button range (0x30-0x66). */
    assert(SMK_APC40_UNDO_NOTE > 0x66u && SMK_APC40_REDO_NOTE > 0x66u);
}

/* Bug a67: the first blinks of a queued clip followed the Loopy 100 ms flash. Model of
 * main_fm1.c smk_pads and smk_apc_led_brightness: with clock, half a beat on and half off
 * (phase from the clock count); with no clock, 250 ms on and 250 ms off. Return the time
 * between the first two dark-to-lit edges after the flash starts at 1000 ms. */
static uint32_t first_pending_blink_period(uint32_t bpm)
{
    uint8_t vel[SMK_APC40_CLIP_COUNT] = {0}, ch[SMK_APC40_CLIP_COUNT] = {0};
    uint8_t flags[SMK_APC40_CLIP_COUNT] = {0}, tick[SMK_APC40_CLIP_COUNT] = {0};
    uint8_t prev = 1u, last16 = 0u, lit;
    uint32_t ms, edge[2] = {0u, 0u}, edges = 0u;
    smk_apc40_cell_event(vel, ch, flags, tick, 0x20u, 0u, 0x37u, 0u);   /* idle clip */
    for (ms = 1u; ms < 4000u && edges < 2u; ms++) {
        if (ms >= 1000u && ms < 3500u && (ms - 1000u) % 100u == 0u)    /* Loopy flash */
            smk_apc40_cell_event(vel, ch, flags, tick, 0x20u, 0u,
                                 ((ms - 1000u) / 100u) % 2u ? 0u : 0x39u, (uint8_t)(ms >> 4));
        if ((uint8_t)(ms >> 4) != last16) {                            /* smk_apc_usb_tick */
            last16 = (uint8_t)(ms >> 4);
            smk_apc40_cells_age(vel, ch, flags, tick, last16);
        }
        lit = vel[0x20] != 0u;
        if (lit && ((flags[0x20] & SMK_APC40_CELL_QUEUED) || smk_apc40_is_queued(ch[0x20])))
            lit = bpm ? smk_apc40_beat_phase(ms * bpm * 24u / 60000u) < 128u : !((ms / 250u) & 1u);
        if (ms >= 1000u && lit && !prev)
            edge[edges++] = ms;
        prev = lit;
    }
    assert(edges == 2u);
    return edge[1] - edge[0];
}

static void test_first_pending_blink_has_beat_period(void)
{
    uint32_t p;
    p = first_pending_blink_period(120u);
    assert(p >= 498u && p <= 502u);             /* 120 BPM: 500 ms, not 200 ms */
    p = first_pending_blink_period(90u);
    assert(p >= 664u && p <= 669u);             /* 90 BPM: 667 ms */
    p = first_pending_blink_period(0u);
    assert(p == 500u);                          /* no clock: the 250/250 ms default */
}

/* a74 (MID-14): ui.c input() for PLAY on the SMK-37 (the PLAY note: channel 16, as Undo / Redo). The clip mode handler first (it uses the event in clip
 * mode), then the PLAY note outside clip mode, which does not use the event (PLAY starts or stops X0X). */
static uint32_t play_out[8];
static int play_nout, play_taps;
static void play_button(uint8_t *clip_sent, uint8_t *note_sent, uint8_t clip_mode, uint8_t home, uint8_t pressed)
{
    uint8_t ev = smk_apc40_transport_event(clip_sent, clip_mode, pressed);   /* smk_clip_transport_button */
    if (ev != SMK_APC40_BUTTON_IGNORED) {
        if (ev == SMK_APC40_BUTTON_ON || ev == SMK_APC40_BUTTON_OFF)
            play_out[play_nout++] = (ev == SMK_APC40_BUTTON_ON ? 0x09u | 0x90u << 8 | 0x5Bu << 16 | 0x7Fu << 24
                                                                : 0x08u | 0x80u << 8 | 0x5Bu << 16 | 0x7Fu << 24);
        return;                                                              /* used: X0X does not start */
    }
    ev = smk_apc40_play_note_event(note_sent, clip_mode, home, pressed);     /* smk_play_note_button */
    if (ev == SMK_APC40_BUTTON_ON || ev == SMK_APC40_BUTTON_OFF)
        play_out[play_nout++] = smk_apc40_play_note_packet(ev == SMK_APC40_BUTTON_ON);
    if (pressed && !home)
        play_taps++;                                                         /* button_tap(B_PLAY): start or stop */
}
static void test_play_note_outside_clip_mode(void)
{
    uint8_t cs = 0u, ns = 0u;
    const uint32_t on = 0x09u | 0x9Fu << 8 | 0x5Bu << 16 | 0x7Fu << 24, off = 0x08u | 0x8Fu << 8 | 0x5Bu << 16;   /* channel 16 */
    assert(smk_apc40_play_note_packet(1u) == on);
    assert(smk_apc40_play_note_packet(0u) == off);
    play_nout = play_taps = 0;                    /* outside clip mode: note on, note off, and X0X starts */
    play_button(&cs, &ns, 0u, 0u, 1u);
    assert(play_nout == 1 && play_out[0] == on && play_taps == 1);
    play_button(&cs, &ns, 0u, 0u, 0u);
    assert(play_nout == 2 && play_out[1] == off && play_taps == 1);
    play_button(&cs, &ns, 0u, 0u, 1u);            /* the second press: X0X stops, the note again */
    play_button(&cs, &ns, 0u, 0u, 0u);
    assert(play_nout == 4 && play_out[2] == on && play_out[3] == off && play_taps == 2);
    play_nout = play_taps = 0;                    /* clip mode: one APC PLAY note on and off, X0X does not start */
    play_button(&cs, &ns, 1u, 0u, 1u);
    play_button(&cs, &ns, 1u, 0u, 0u);
    assert(play_nout == 2 && play_out[0] == (0x09u | 0x90u << 8 | 0x5Bu << 16 | 0x7Fu << 24) &&
           (play_out[1] & 0xFFFFFFu) == (0x08u | 0x80u << 8 | 0x5Bu << 16));   /* the APC PLAY: channel 1 */
    assert(play_taps == 0 && !ns && !cs);
    play_nout = play_taps = 0;                    /* HOME + PLAY (X0X redo): no note */
    play_button(&cs, &ns, 0u, 1u, 1u);
    play_button(&cs, &ns, 0u, 1u, 0u);
    assert(play_nout == 0 && play_taps == 0);
    play_nout = 0;                                /* pressed outside, clip mode starts, released: one note off */
    play_button(&cs, &ns, 0u, 0u, 1u);
    play_button(&cs, &ns, 1u, 0u, 0u);
    assert(play_nout == 2 && play_out[0] == on && play_out[1] == off && !ns && !cs);
    play_nout = 0;                                /* pressed in clip mode, clip mode ends, released: the APC off only */
    play_button(&cs, &ns, 1u, 0u, 1u);
    play_button(&cs, &ns, 0u, 0u, 0u);
    assert(play_nout == 2 && (play_out[1] & 0xFFFFFFu) == (0x08u | 0x80u << 8 | 0x5Bu << 16) && !ns && !cs);
}

int main(void)
{
    test_play_note_outside_clip_mode();
    test_first_pending_blink_has_beat_period();
    test_loopy_history_packets();
    test_lcd_map_fits_used_area();
    test_loopy_refresh_keeps_last_colors();
    test_loopy_flash_is_held_and_blinks_on_beat();
    test_blink_period_is_one_beat();
    test_row_launch_on_small_page_keeps_clips();
    test_scene_launch_and_cue_bursts_keep_other_clips();
    test_partial_latch_grows_to_full_four_by_four();
    test_pending_clips_blink_static_clips_steady();
    test_play_led_blinks_with_clock();
    test_page_change_snapshot_clears_old_page();
    test_small_burst_keeps_other_clips();
    test_eight_by_four_uses_exactly_two_views();
    test_four_by_four_first_half_on_pads_1_4_and_9_12();
    test_transport_led_lights_on_press();
    test_eight_by_four_crops_viewport_margin();
    test_four_by_four_folds_into_one_eight_by_two_grid();
    test_sparse_eight_wide_cells_keep_relative_gaps();
    test_viewport_with_one_row_uses_one_pad_row();
    test_viewport_clamp_can_always_return();
    test_physical_pad_rows_are_flipped_only();
    test_transport_button_notes_and_feedback_states();
    return 0;
}
