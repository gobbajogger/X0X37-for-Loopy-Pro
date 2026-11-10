/* SPDX-License-Identifier: GPL-3.0-only */
/* Header-DMA event injection. Peer B's SMC change arrives from the poll
 * inside cv_lcd_emit / cv_lcd_full while peer A's paint is in flight.
 * One frame must stay on A's snapshot. The next frame shows B.
 * Held notes, REC and PANIC stay put. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define SMC_UI_MAIN smc_ui_baseline_main
#include "smc_ui_test.c"

static unsigned hooks, fails, chunks;
static int inject_armed, inject_grid, chunk_polls, chunk_buttons;
static uint16_t frame_a[240 * 240], frame_b[240 * 240], frame_now[240 * 240];

struct guard {
    uint8_t held[SMC_PEERS][16];
    uint8_t rec, playing, rec_mask, rec_ext, rec_hold, kbd_note;
    smc_panic_peer_t panic[SMC_PEERS];
    int midi_held;
};

static void paint_fail(int cond, const char *msg)
{
    if (!cond) {
        fprintf(stderr, "FAIL %s\n", msg);
        fails++;
    }
}

static void inject_b(void)
{
    uint8_t saved;
    if (chunk_polls) {
        /* One poll per DMA chunk, so a press and its release both land. */
        now_ms++;
        chunks++;
        held_btn = 0;
        if (chunk_buttons && chunks == 12) {
            held_btn = 1u << B_SEQ;          /* press: HOME -> SEQ, records prev_view */
            ui.part = PART_909;
            ui.sel[PART_909] = 5;
        } else if (chunk_buttons && chunks == 70)
            held_btn = 1u << B_FX;           /* press inside the knob commit */
        return;
    }
    if (!inject_armed)
        return;
    inject_armed = 0;
    hooks++;
    saved = smc_slot;
    smc_slot = 1;
    if (inject_grid) {
        ui.view = V_SEQ;
        ui.part = PART_909;
        ui.sel[PART_909] = 5;           /* live view/selection must not repaint A's grid */
    }
    smc_ui_begin(7, 0);                 /* peer B, fader 8, during A's header DMA */
    smc_slot = saved;
}

static void settle_identity(int peer, int strip, int kind)
{
    uint8_t saved = smc_slot;
    smc_slot = (uint8_t)peer;
    smc_ui_begin(strip, kind);
    smc_slot = saved;
}

static void arm_state(void)
{
    ui.rec = 1;
    /* input() republishes this mask from ui.rec. Preset it so a DMA poll
     * does not look like a recording change. */
    seq.rec_ext_mask = (uint8_t)((1u << NBASS) - 1u);
    seq.playing = 0;
    kbd_held_note[0] = 60;
    memset(smc_states[0].held, 0, sizeof smc_states[0].held);
    memset(smc_states[1].held, 0, sizeof smc_states[1].held);
    smc_states[0].held[0] = 0x11;
    smc_states[1].held[3] = 0x22;
    memset(&smc_panic_peers, 0, sizeof smc_panic_peers);
    smc_panic_peers[0].held = 0x01;
    smc_panic_peers[1].held = 0x02;
    smc_panic_peers[0].mode_seen = (int8_t)smc_panic_mode_now();
    smc_panic_peers[1].mode_seen = smc_panic_peers[0].mode_seen;
}

static struct guard take_guard(void)
{
    struct guard g;
    memset(&g, 0, sizeof g);
    memcpy(g.held[0], smc_states[0].held, sizeof g.held[0]);
    memcpy(g.held[1], smc_states[1].held, sizeof g.held[1]);
    g.rec = ui.rec;
    g.playing = seq.playing;
    g.rec_mask = seq.rec_mask;
    g.rec_ext = seq.rec_ext_mask;
    g.rec_hold = seq.rec_hold_mask;
    g.kbd_note = kbd_held_note[0];
    memcpy(g.panic, smc_panic_peers, sizeof g.panic);
    g.midi_held = engine_midi_held();
    return g;
}

static void force_lcd(void)
{
    int i;
    for (i = 0; i < 4; i++) {
        blit_hash[i] = 0;
        cv_tile_valid[i] = 0;
    }
    header_full_hash = 0;
    popup_knob_full_hash = 0;
}

static void paint(void)
{
    now_ms++;
    force_lcd();
    ui_frame();
}

/* A later paint that does not clear the header tiles, so a changed header
 * takes the dirty-tile DMA instead of force_lcd's cold full transfer. */
static void paint_keep(void)
{
    now_ms++;
    ui_frame();
}

static void arm_dirty_header(void)
{
    memcpy(ui.msg, "DIRTY", 6);
    ui.msg_until = now_ms + 50000u;
}

/* Home faders and knobs are per peer. A locked peer keeps its own row.
 * Peer A stays on the live home row (levels, CUT bank). Peer B is locked
 * to a different row (sends / pan) with the opposite end of each value.
 * A paint that reads the live peer's strip targets then cannot match A's snapshot. */
static void split_peer_homes(void)
{
    int n;
    smc_states[0].locked = 0;
    smc_states[0].knob_bank = 0;
    smc_states[1].locked = 1;
    smc_states[1].lock_view = V_HOME;
    smc_states[1].lock_part = (uint8_t)PART_303A;
    smc_states[1].lock_page = 0;
    smc_states[1].lock_pages = 1;
    smc_states[1].knob_bank = 1;
    for (n = 0; n < 8; n++) {
        pref_t home = smc_home_target(n);
        pref_t fader = home;
        pref_t knob;
        if (n < NPARTS) {
            fader.c = MX_REV;
            knob = PR(R_ENG, T_MIX, home.b, MX_PAN);
            pref_set(PR(R_ENG, T_MIX, home.b, MX_CUT), 0);
        } else if (n == 5) {
            fader.c = FX_RV_TONE;
            knob = PR(R_ENG, T_FX, 0, FX_RV_HPF);
        } else if (n == 6) {
            fader.c = FX_DL_TIME;
            knob = PR(R_ENG, T_FX, 0, FX_DL_TONE);
        } else {
            fader.c = MST_CUTOFF;
            knob = PR(R_ENG, T_MST, 0, MST_LIMIT);
        }
        pref_set(home, 0);
        pref_set(fader, 127);
        pref_set(knob, 127);
        smc_states[1].lock_fader[n] = fader;
        smc_states[1].lock_knob[n] = knob;
    }
}

static void require_distinct_homes(void)
{
    int n;
    uint8_t saved = smc_slot;
    for (n = 0; n < 8; n++) {
        pref_t af, ak, bf, bk;
        smc_slot = 0;
        af = smc_target(n);
        ak = smc_knob_target(n);
        smc_slot = 1;
        bf = smc_target(n);
        bk = smc_knob_target(n);
        paint_fail(!smc_same(af, bf) && smc_scaled(af) != smc_scaled(bf),
                   "peer A and B home fader targets differ");
        paint_fail(!smc_same(ak, bk) && smc_scaled(ak) != smc_scaled(bk),
                   "peer A and B home knob targets differ");
    }
    smc_slot = saved;
}

static void prepare(void)
{
    setup(V_HOME, PART_303A);
    smc_ble_link_slot(1, 1);
    smc_poll();
    arm_state();
    inject_grid = 0;
    x0x_lcd_poll_hook(inject_b);
    split_peer_homes();
    require_distinct_homes();
}

static void set_peek(int on)
{
    memset(smc_peek, 0, sizeof *smc_peek);
    if (!on)
        return;
    smc_peek->on = 1;
    smc_peek->view = ui.view;
    smc_peek->page = (uint8_t)cur_page();
    smc_peek->mixsel = ui.mixsel;
    smc_peek->part = ui.part;
    smc_peek->peer = 0;
    smc_peek->until = now_ms + 10000u;
}

static int rows_eq(const uint16_t *a, const uint16_t *b, int y0, int y1)
{
    return memcmp(a + y0 * 240, b + y0 * 240,
                  (size_t)(y1 - y0) * 240u * sizeof(uint16_t)) == 0;
}

static void check_regions(const char *mode, const uint16_t *got, const uint16_t *want)
{
    char msg[96];
    snprintf(msg, sizeof msg, "%s header region stays on one snapshot", mode);
    paint_fail(rows_eq(got, want, 0, HDR_H), msg);
    snprintf(msg, sizeof msg, "%s main region stays on one snapshot", mode);
    paint_fail(rows_eq(got, want, HDR_H, KNOB_Y), msg);
    snprintf(msg, sizeof msg, "%s knob region stays on one snapshot", mode);
    paint_fail(rows_eq(got, want, KNOB_Y, 240), msg);
}

static void run_mode(const char *mode, int peek)
{
    struct guard before, after;
    unsigned hooks_before;
    prepare();
    set_peek(peek);
    arm_state();
    settle_identity(0, 0, 1);
    inject_armed = 0;
    paint();
    memcpy(frame_a, fb, sizeof frame_a);

    prepare();
    set_peek(peek);
    arm_state();
    settle_identity(1, 7, 0);          /* peer B, fader 8 */
    inject_armed = 0;
    paint();
    memcpy(frame_b, fb, sizeof frame_b);
    paint_fail(!rows_eq(frame_a, frame_b, 0, 240), peek ? "peek A and B banners differ" : "overlay A and B frames differ");

    prepare();
    set_peek(peek);
    arm_state();
    settle_identity(0, 0, 1);
    before = take_guard();
    hooks_before = hooks;
    inject_armed = 1;
    paint();                            /* B arrives from the header-DMA poll */
    memcpy(frame_now, fb, sizeof frame_now);
    paint_fail(hooks == hooks_before + 1, peek ? "peek header DMA fired the poll hook" : "overlay header DMA fired the poll hook");
    paint_fail(smc_ui.peer == 1 && smc_ui.strip == 7 && smc_ui.kind == 0,
           peek ? "peek poll committed peer B" : "overlay poll committed peer B");
    paint_fail(smc_slot == 0, peek ? "peek restored smc_slot" : "overlay restored smc_slot");
    check_regions(mode, frame_now, frame_a);
    paint_fail(!rows_eq(frame_now, frame_b, 0, HDR_H),
           peek ? "peek frame is not B's banner" : "overlay frame is not B's header");
    after = take_guard();
    paint_fail(!memcmp(&before, &after, sizeof before),
           peek ? "peek held notes, REC and PANIC unchanged" : "overlay held notes, REC and PANIC unchanged");

    inject_armed = 0;
    paint();                            /* next frame: B is the snapshot */
    paint_fail(rows_eq(fb, frame_b, 0, 240), peek ? "peek next frame shows B" : "overlay next frame shows B");
    after = take_guard();
    paint_fail(!memcmp(&before, &after, sizeof before),
           peek ? "peek recovery keeps held notes, REC and PANIC" : "overlay recovery keeps held notes, REC and PANIC");
}

static void show_b_grid(void)
{
    ui.view = V_SEQ;
    ui.part = PART_909;
    ui.sel[PART_909] = 5;
    settle_identity(1, 7, 0);
}

/* Peer B retargets the overlay and the live view while A's grid is painting. */
static void run_grid(void)
{
    struct guard before, after;
    unsigned hooks_before;
    prepare();
    arm_state();
    settle_identity(0, 0, 1);
    inject_armed = 0;
    paint();
    memcpy(frame_a, fb, sizeof frame_a);

    prepare();
    arm_state();
    show_b_grid();
    inject_armed = 0;
    paint();
    paint();                            /* second frame: same distance as recovery */
    memcpy(frame_b, fb, sizeof frame_b);
    paint_fail(!rows_eq(frame_a, frame_b, HDR_H, KNOB_Y), "grid A and B targets differ");

    prepare();
    arm_state();
    settle_identity(0, 0, 1);
    before = take_guard();
    hooks_before = hooks;
    inject_grid = 1;
    inject_armed = 1;
    paint();
    memcpy(frame_now, fb, sizeof frame_now);
    paint_fail(hooks == hooks_before + 1, "grid header DMA fired the poll hook");
    paint_fail(smc_ui.peer == 1 && smc_ui.strip == 7 && smc_ui.kind == 0, "grid poll committed peer B");
    paint_fail(ui.view == V_SEQ && ui.part == PART_909 && ui.sel[PART_909] == 5, "grid poll kept B's view");
    paint_fail(rows_eq(frame_now, frame_a, HDR_H, KNOB_Y), "grid main region stays on one snapshot");
    paint_fail(!rows_eq(frame_now, frame_b, HDR_H, KNOB_Y), "grid frame is not B's targets");
    after = take_guard();
    paint_fail(!memcmp(&before, &after, sizeof before), "grid held notes, REC and PANIC unchanged");

    inject_armed = 0;
    inject_grid = 0;
    paint();
    paint_fail(rows_eq(fb, frame_b, HDR_H, KNOB_Y), "grid next frame shows B");
    after = take_guard();
    paint_fail(!memcmp(&before, &after, sizeof before), "grid recovery keeps held notes, REC and PANIC");
}

static uint16_t frame_retake[240 * 240];

static unsigned pix_diff(const uint16_t *a, const uint16_t *b, int y0, int y1)
{
    unsigned n = 0;
    int y, x;
    for (y = y0; y < y1; y++)
        for (x = 0; x < 240; x++)
            n += a[y * 240 + x] != b[y * 240 + x];
    return n;
}

static void paint_seq(int dirty, uint16_t *dst)
{
    prepare();
    ui.view = V_SEQ;
    ui.part = PART_909;
    ui.sel[PART_909] = 5;
    set_peek(1);
    arm_state();
    settle_identity(0, 0, 1);
    inject_armed = 0;
    inject_grid = 0;
    if (dirty) {
        paint();
        paint_keep();
        arm_dirty_header();
        paint_keep();
    } else paint();
    memcpy(dst, fb, sizeof frame_a);
}

static void paint_home(int dirty, uint16_t *dst)
{
    prepare();
    set_peek(1);
    arm_state();
    settle_identity(0, 0, 1);
    inject_armed = 0;
    inject_grid = 0;
    if (dirty) {
        paint();
        paint_keep();
        arm_dirty_header();
        paint_keep();
    } else paint();
    memcpy(dst, fb, sizeof frame_a);
}

/* A frame snapshotted after the poll: peer B, fader 8, on the injected page. */
static void paint_retake(int dirty, uint16_t *dst)
{
    prepare();
    ui.view = V_SEQ;
    ui.part = PART_909;
    ui.sel[PART_909] = 5;
    set_peek(1);
    arm_state();
    settle_identity(1, 7, 0);
    inject_armed = 0;
    inject_grid = 0;
    if (dirty) {
        paint();
        paint_keep();
        arm_dirty_header();
        paint_keep();
    } else paint();
    memcpy(dst, fb, sizeof frame_a);
}

static void paint_peek_inject(int dirty, uint16_t *dst, unsigned *fired)
{
    unsigned before;
    prepare();
    set_peek(1);
    arm_state();
    settle_identity(0, 0, 1);
    inject_grid = 1;
    if (dirty) {
        inject_armed = 0;
        paint();
        paint_keep();
        arm_dirty_header();
        before = hooks;
        inject_armed = 1;
        paint_keep();
    } else {
        before = hooks;
        inject_armed = 1;
        paint();
    }
    *fired = hooks - before;
    memcpy(dst, fb, sizeof frame_a);
    inject_armed = 0;
    inject_grid = 0;
}

/* Header, main and knobs must be the pre-poll page. A live body matches the
 * sequencer page instead, so the three regions are not one state. */
static void report_one_state(const char *tag, const uint16_t *got,
                             const uint16_t *home, const uint16_t *seq,
                             const uint16_t *retake, unsigned fired)
{
    unsigned hh = pix_diff(got, home, 0, HDR_H);
    unsigned hm = pix_diff(got, home, HDR_H, KNOB_Y);
    unsigned hk = pix_diff(got, home, KNOB_Y, 240);
    unsigned sm = pix_diff(got, seq, HDR_H, KNOB_Y);
    unsigned sk = pix_diff(got, seq, KNOB_Y, 240);
    unsigned rh = pix_diff(got, retake, 0, HDR_H);
    char msg[120];
    snprintf(msg, sizeof msg, "%s header DMA fired once", tag);
    paint_fail(fired == 1, msg);
    snprintf(msg, sizeof msg, "%s header, main and knobs are one state", tag);
    paint_fail(hh == 0 && rh != 0 && hm == 0 && hk == 0 && sm != 0 && sk != 0, msg);
}

static void run_peek_live_context(void)
{
    unsigned fired = 0;
    paint_home(0, frame_a);
    paint_seq(0, frame_b);
    paint_retake(0, frame_retake);
    paint_peek_inject(0, frame_now, &fired);
    report_one_state("cold-full peek inject_grid=1", frame_now, frame_a, frame_b, frame_retake, fired);
    paint_home(1, frame_a);
    paint_seq(1, frame_b);
    paint_retake(1, frame_retake);
    paint_peek_inject(1, frame_now, &fired);
    report_one_state("dirty-header peek inject_grid=1", frame_now, frame_a, frame_b, frame_retake, fired);
}

/* Press+release inside the main DMA and again inside the knob DMA.
 * This frame stays on the pre-poll page. The presses survive on the live page. */
static void run_body_input(int dirty)
{
    const char *tag = dirty ? "dirty-header body" : "cold-full body";
    char msg[120];
    prepare();
    ui.prev_view = V_MIX;
    set_peek(1);
    arm_state();
    settle_identity(0, 0, 1);
    inject_armed = 0;
    chunk_buttons = 0;
    chunk_polls = 1;
    chunks = 0;
    held_btn = 0;
    if (dirty) {
        paint();
        paint_keep();
        arm_dirty_header();
        blit_hash[1] = cv_tile_valid[1] = 0;
        blit_hash[2] = cv_tile_valid[2] = 0;
        blit_hash[3] = cv_tile_valid[3] = 0;
        chunks = 0;
        paint_keep();
    } else
        paint();
    chunk_polls = 0;
    memcpy(frame_a, fb, sizeof frame_a);

    prepare();
    ui.view = V_SEQ;
    ui.part = PART_909;
    ui.sel[PART_909] = 5;
    set_peek(1);
    arm_state();
    settle_identity(0, 0, 1);
    paint();
    memcpy(frame_b, fb, sizeof frame_b);
    paint_fail(!rows_eq(frame_a, frame_b, HDR_H, KNOB_Y), "body home and SEQ pages differ");

    prepare();
    ui.prev_view = V_MIX;
    set_peek(1);
    arm_state();
    settle_identity(0, 0, 1);
    held_btn = 0;
    if (dirty) {
        paint();
        paint_keep();
        arm_dirty_header();
        blit_hash[1] = cv_tile_valid[1] = 0;
        blit_hash[2] = cv_tile_valid[2] = 0;
        blit_hash[3] = cv_tile_valid[3] = 0;
    }
    chunk_polls = 1;
    chunk_buttons = 1;
    chunks = 0;
    if (dirty)
        paint_keep();
    else
        paint();
    snprintf(msg, sizeof msg, "%s main and knobs stay on one page", tag);
    /* The bottom footer notes that a button was pressed. The page cells above it stay. */
    paint_fail(pix_diff(fb, frame_a, 0, HDR_H) == 0 &&
               pix_diff(fb, frame_a, HDR_H, KNOB_Y) == 0 &&
               pix_diff(fb, frame_a, KNOB_Y, 228) == 0 &&
               pix_diff(fb, frame_b, HDR_H, KNOB_Y) != 0, msg);
    snprintf(msg, sizeof msg, "%s view part sel and prev_view survive", tag);
    paint_fail(ui.view == V_FX && ui.prev_view == V_SEQ && ui.part == PART_909 &&
               ui.sel[PART_909] == 5 && held_btn == 0, msg);
    chunk_polls = 0;
    chunk_buttons = 0;
    held_btn = 0;
    paint();
    snprintf(msg, sizeof msg, "%s next frame shows the arrived page", tag);
    paint_fail(!rows_eq(fb, frame_a, HDR_H, KNOB_Y), msg);
}

int main(void)
{
    run_mode("overlay", 0);
    run_mode("peek", 1);
    run_grid();
    run_peek_live_context();
    run_body_input(0);
    run_body_input(1);
    if (fails) {
        fprintf(stderr, "FAIL smc lcd paint: %u assertion(s)\n", fails);
        return 1;
    }
    printf("PASS smc lcd paint: overlay, page peek and grid keep one snapshot; next frame shows B; held notes, REC and PANIC unchanged (%u DMA hooks, snapshot %u bytes)\n",
           hooks, (unsigned)sizeof(struct smc_paint));
    return 0;
}
