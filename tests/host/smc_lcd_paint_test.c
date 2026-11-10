/* SPDX-License-Identifier: GPL-3.0-only */
/* Header-DMA event injection. Peer B's SMC change arrives from the poll
 * inside cv_lcd_emit / cv_lcd_full while peer A's paint is in flight.
 * One frame must stay on A's snapshot. The next frame shows B.
 * Held notes, REC and PANIC stay put. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define SMC_UI_MAIN smc_ui_baseline_main
#include "smc_ui_test.c"

static unsigned hooks, fails, chunks;
static int inject_armed, inject_grid, chunk_polls, chunk_buttons;
static int watch_body_view, poll_saw_view, body_rebuild, expire_peek;
static int live_case, live_repeat;
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

static void live_fire(void)
{
    uint8_t msg[3];
    if (!live_case)
        return;
    if (live_repeat && live_case != 17)
        return;
    live_repeat = 1;
    switch (live_case) {
    case 1: smc_page_set(1); break;
    case 2: pref_set(smc_target(0), 90); break;
    case 3: ui.rec = 0; break;
    case 4: seq.playing = 1; break;
    case 5: open_list_of_pages(); break;
    case 6: ui.view = V_FX; break;
    case 7: smc_knob_bank(1); break;
    case 8: ui.sel[PART_909] = 4; break;
    case 9:
        proj.set.bt = (uint8_t)(proj.set.bt | PROJ_BT_SMC_USER);
        msg[0] = 0xB0; msg[1] = 40; msg[2] = 100;
        smc_message(msg, 3, 1);
        smc_peek_cancel();
        smc_ui_begin(0, 1);
        break;
    case 10:
        msg[0] = 0xE0; msg[1] = 0x7F; msg[2] = 0x7F;
        smc_message(msg, 3, 1);
        smc_peek_cancel();
        smc_ui_begin(0, 1);
        break;
    case 11:
        msg[0] = 0xB0; msg[1] = 16; msg[2] = 1;
        smc_message(msg, 3, 1);
        smc_peek_cancel();
        break;
    case 12: turn_ref(smc_target(0), 4, EN_K1); break;
    case 13: seq.bpm += 12.0f; break;
    case 14: mute_part(0); break;
    case 15: smc_solo_press(0, 1, 1); break;
    case 16:
        smc.fx_ready = 1;
        smc.fx_effect[SMCFX_909] ^= 1u;
        break;
    case 18: ui.mixsel ^= 1u; break;
    case 19: smc.locked ^= 1u; break;
    case 17:
        pref_set(smc_target(0), (chunks & 63) + 1);
        if (chunks == 20)
            smc_ui_cancel();
        break;
    default: break;
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
        if (inject_armed && chunks == 1) {
            inject_armed = 0;
            hooks++;
            saved = smc_slot;
            smc_slot = 1;
            if (inject_grid) {
                ui.view = V_SEQ;
                ui.part = PART_909;
                ui.sel[PART_909] = 5;
            }
            smc_ui_begin(7, 0);
            smc_slot = saved;
        }
        if (chunk_buttons && chunks == 12) {
            held_btn = 1u << B_SEQ;          /* press: HOME -> SEQ, records prev_view */
            ui.part = PART_909;
            ui.sel[PART_909] = 5;
        } else if (chunk_buttons && chunks == 70)
            held_btn = 1u << B_FX;           /* press inside the knob commit */
        if (watch_body_view && chunks == 12)
            poll_saw_view = ui.view;
        if (body_rebuild && chunks == 12)
            set_view(V_SEQ);                 /* build_pages runs inside the live side */
        if (live_case && (!live_repeat || live_case == 17) &&
            (live_case == 17 || chunks == 12))
            live_fire();
        return;
    }
    if (live_case)
        live_fire();
    if (expire_peek) {
        expire_peek = 0;
        smc_peek->until = 0;             /* header poll: the peek times out after the header is drawn */
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
    struct guard before_guard, after_guard;
    prepare();
    set_peek(1);
    arm_state();
    settle_identity(0, 0, 1);
    inject_grid = 1;
    before_guard = take_guard();
    if (dirty) {
        inject_armed = 0;
        paint();
        paint_keep();
        arm_dirty_header();
        before = hooks;
        before_guard = take_guard();
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
    paint();                                 /* following frame, same session */
    after_guard = take_guard();
    paint_fail(!memcmp(&before_guard, &after_guard, sizeof before_guard),
               dirty ? "dirty peek inject_grid following frame keeps MIDI REC PANIC"
                     : "cold peek inject_grid following frame keeps MIDI REC PANIC");
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

static void irq_playing(void)
{
    seq.playing = 1;                         /* do_start's store, from outside the poll */
}

static void run_irq_between_polls(void)
{
    prepare();
    set_peek(1);
    arm_state();
    settle_identity(0, 0, 1);
    seq.playing = 0;
    ui_paint_between_polls = irq_playing;
    paint();
    ui_paint_between_polls = 0;
    paint_fail(seq.playing == 1, "audio start during the snapshot is kept");
}

static void run_body_pg_rebuild(void)
{
    prepare();
    set_peek(1);
    arm_state();
    settle_identity(0, 0, 1);
    chunk_polls = 1;
    chunks = 0;
    paint();
    memcpy(frame_a, fb, sizeof frame_a);
    chunk_polls = 0;

    prepare();
    set_peek(1);
    arm_state();
    settle_identity(0, 0, 1);
    chunk_polls = 1;
    chunks = 0;
    body_rebuild = 1;
    paint();
    body_rebuild = 0;
    chunk_polls = 0;
    paint_fail(rows_eq(fb, frame_a, HDR_H, KNOB_Y),
               "body poll rebuilds pg without changing the drawn page");
    paint_fail(ui.view == V_SEQ, "body poll rebuild stays on the live page");
}

static void run_swap_sees_live(void)
{
    prepare();
    set_peek(1);
    arm_state();
    settle_identity(0, 0, 1);
    chunk_polls = 1;
    chunks = 0;
    paint();
    memcpy(frame_a, fb, sizeof frame_a);
    chunk_polls = 0;

    prepare();
    set_peek(1);
    arm_state();
    settle_identity(0, 0, 1);
    inject_grid = 1;
    inject_armed = 1;
    chunk_polls = 1;
    chunks = 0;
    watch_body_view = 1;
    poll_saw_view = -1;
    paint();
    watch_body_view = 0;
    chunk_polls = 0;
    inject_grid = 0;
    paint_fail(poll_saw_view == V_SEQ, "body poll sees the live page, not the snapshot");
    paint_fail(rows_eq(fb, frame_a, HDR_H, KNOB_Y), "body keeps the snapshot after the live poll");
}

static void run_peek_timeout(void)
{
    struct guard before, after;
    prepare();
    set_view(V_FX);
    build_pages();
    *page_of(V_FX) = 0;                  /* the cursor the timeout will restore */
    set_view(V_HOME);
    build_pages();
    smc_peek_page(V_FX, 1, ui.mixsel, ui.part);
    if (pg.n < 2 || cur_page() != 1) {
        paint_fail(0, "peek timeout has a second FX page");
        return;
    }
    arm_state();
    settle_identity(0, 0, 1);
    smc_peek->until = now_ms + 100000u;
    paint();
    memcpy(frame_a, fb, sizeof frame_a);

    prepare();
    set_view(V_HOME);
    build_pages();
    arm_state();
    settle_identity(0, 0, 1);
    paint();
    memcpy(frame_b, fb, sizeof frame_b);
    paint_fail(!rows_eq(frame_a, frame_b, HDR_H, KNOB_Y), "peek page and restored home differ");

    prepare();
    set_view(V_FX);
    build_pages();
    *page_of(V_FX) = 0;
    set_view(V_HOME);
    build_pages();
    smc_peek_page(V_FX, 1, ui.mixsel, ui.part);
    arm_state();
    settle_identity(0, 0, 1);
    before = take_guard();
    smc_peek->until = now_ms + 100000u;
    expire_peek = 1;
    paint();
    memcpy(frame_now, fb, sizeof frame_now);
    paint_fail(rows_eq(frame_now, frame_a, HDR_H, 240),
               "peek timeout body stays on the peeked page");
    paint_fail(!rows_eq(frame_now, frame_b, HDR_H, 240),
               "peek timeout body is not the restored page");
    paint();
    after = take_guard();
    paint_fail(rows_eq(fb, frame_b, HDR_H, KNOB_Y),
               "peek timeout following frame shows the restored page");
    paint_fail(!memcmp(&before, &after, sizeof before),
               "peek timeout following frame keeps MIDI REC PANIC");
}

static void live_clear(void)
{
    live_case = 0;
    live_repeat = 0;
    chunk_polls = 0;
    chunks = 0;
}

static int arm_live(int which, int changed)
{
    prepare();
    set_peek(0);
    arm_state();
    seq.playing = 0;
    switch (which) {
    case 1:
        set_view(V_FX);
        build_pages();
        if (pg.n < 2) {
            paint_fail(0, "fx view has two pages");
            return 0;
        }
        if (changed)
            smc_page_set(1);
        break;
    case 2:
    case 17:
        if (changed)
            pref_set(smc_target(0), 90);
        break;
    case 9:
    case 10:
    case 11:
    case 12:
        if (changed) {
            int saved = live_case;
            live_case = which;
            live_repeat = 0;
            live_fire();
            live_case = saved;
            live_repeat = 0;
        }
        break;
    case 3:
        ui.rec = (uint8_t)(changed ? 0 : 1);
        break;
    case 4:
        seq.playing = (uint8_t)(changed ? 1 : 0);
        break;
    case 5:
        if (changed)
            open_list_of_pages();
        break;
    case 6:
        if (changed)
            ui.view = V_FX;
        break;
    case 7:
        if (changed)
            smc_knob_bank(1);
        break;
    case 8:
        set_view(V_SEQ);
        ui.part = PART_909;
        build_pages();
        ui.sel[PART_909] = (uint8_t)(changed ? 4 : 0);
        break;
    case 13:
        if (changed)
            seq.bpm += 12.0f;
        break;
    case 14:
        if (changed)
            mute_part(0);
        break;
    case 15:
        if (changed)
            smc_solo_press(0, 1, 1);
        break;
    case 16:
        set_view(V_FX);
        build_pages();
        smc.fx_ready = 1;
        if (changed)
            smc.fx_effect[SMCFX_909] ^= 1u;
        break;
    case 18:
        set_view(V_MIX);
        build_pages();
        if (changed)
            ui.mixsel ^= 1u;
        break;
    case 19:
        if (changed)
            smc.locked ^= 1u;
        break;
    default:
        break;
    }
    settle_identity(0, 0, which == 4 || which == 6 || which == 16 ? 3 : 1);
    if (which == 4)
        smc_ui.action = 0x5E;
    if (which == 6)
        smc_ui.action = 0;
    if (which == 16)
        smc_ui.action = SMCFX_NOTE[SMCFX_909];
    return 1;
}

static int which_allows_same(const char *tag);

static void expect_live(const char *tag, const uint16_t *got,
                        const uint16_t *oldf, const uint16_t *newf)
{
    char msg[140];
    int same = rows_eq(got, newf, 0, 240);
    int moved = !rows_eq(oldf, newf, 0, 240);
    snprintf(msg, sizeof msg, "%s is one consistent state", tag);
    paint_fail(same && (moved || which_allows_same(tag)), msg);
}

static int which_allows_same(const char *tag)
{
    /* Tempo is compared, and it is not drawn on the overlay. */
    return strstr(tag, "bpm") != 0;
}

static void run_live_window(int which, int grid, const char *name)
{
    char tag[96];
    if (!arm_live(which, 0))
        return;
    live_clear();
    chunk_polls = grid;
    paint();
    memcpy(frame_a, fb, sizeof frame_a);
    if (!arm_live(which, 1))
        return;
    live_clear();
    chunk_polls = grid;
    paint();
    memcpy(frame_b, fb, sizeof frame_b);
    if (!arm_live(which, 0))
        return;
    live_case = which;
    live_repeat = 0;
    chunks = 0;
    chunk_polls = grid;
    smc_paint_repaints = 0;
    paint();
    snprintf(tag, sizeof tag, "%s %s", name, grid ? "grid" : "header");
    paint_fail(smc_paint_repaints > 0, tag);
    expect_live(tag, fb, frame_a, frame_b);
    live_clear();
}

static void run_kept_dirty(void)
{
    int level;
    if (!arm_live(17, 0))
        return;
    live_case = 17;
    live_repeat = 0;
    chunks = 0;
    chunk_polls = 1;
    paint();
    paint_fail(blit_hash[0] == 0 && cv_tile_valid[0] == 0 && header_full_hash == 0 &&
               part_header_snapshot_valid == 0,
               "kept-dirty cap leaves a full repaint");
    level = (chunks & 63) + 1;
    live_clear();
    paint_keep();
    memcpy(frame_now, fb, sizeof frame_now);
    pref_set(smc_target(0), level);
    smc_ui_cancel();
    paint();
    paint_fail(rows_eq(frame_now, fb, 0, 240),
               "kept-dirty next frame repaints after the overlay closes");
}

static void run_peek_no_retry(void)
{
    prepare();
    set_peek(1);
    arm_state();
    settle_identity(0, 0, 1);
    chunk_polls = 1;
    chunks = 0;
    paint();
    memcpy(frame_a, fb, sizeof frame_a);
    chunk_polls = 0;
    prepare();
    set_peek(1);
    arm_state();
    settle_identity(0, 0, 1);
    smc_paint_repaints = 0;
    chunk_polls = 1;
    chunks = 0;
    body_rebuild = 1;
    paint();
    body_rebuild = 0;
    chunk_polls = 0;
    paint_fail(smc_paint_repaints == 0, "peek does not retry the overlay");
    paint_fail(rows_eq(fb, frame_a, HDR_H, KNOB_Y), "peek body keeps its page");
}

static void run_live_cases(void)
{
    static const struct { int id; const char *name; } cases[] = {
        {1, "page"}, {2, "param"}, {3, "rec"}, {4, "play"}, {5, "overlay"},
        {6, "kind3"}, {7, "bank"}, {8, "voice"}, {9, "cc40"}, {10, "pitchbend"},
        {11, "cc16"}, {12, "encoder"}, {13, "bpm"}, {14, "mute"}, {15, "solo"},
        {16, "fx"}, {18, "select"}, {19, "lock"}
    };
    unsigned i;
    for (i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        run_live_window(cases[i].id, 0, cases[i].name);
        run_live_window(cases[i].id, 1, cases[i].name);
    }
    run_kept_dirty();
    run_peek_no_retry();
    {
        struct smc_paint snap;
        struct smc_paint_live a, b;
        struct timespec t0, t1;
        int i;
        const int n = 200000;
        prepare();
        arm_state();
        settle_identity(0, 0, 1);
        smc_paint_take(&snap);
        smc_paint_live_read(&a, &snap);
        clock_gettime(CLOCK_MONOTONIC, &t0);
        for (i = 0; i < n; i++) {
            smc_paint_live_read(&b, &snap);
            if (memcmp(&a, &b, sizeof a))
                break;
        }
        clock_gettime(CLOCK_MONOTONIC, &t1);
        printf("compare %u bytes, %.3f us each, %.3f us per overlay frame (two checks), stack %u bytes\n",
               (unsigned)sizeof a,
               ((t1.tv_sec - t0.tv_sec) * 1e6 + (t1.tv_nsec - t0.tv_nsec) / 1e3) / n,
               2.0 * ((t1.tv_sec - t0.tv_sec) * 1e6 + (t1.tv_nsec - t0.tv_nsec) / 1e3) / n,
               (unsigned)(2 * sizeof a));
    }
}

int main(void)
{
    run_mode("overlay", 0);
    run_mode("peek", 1);
    run_grid();
    run_peek_live_context();
    run_body_input(0);
    run_body_input(1);
    run_irq_between_polls();
    run_body_pg_rebuild();
    run_swap_sees_live();
    run_peek_timeout();
    run_live_cases();
    if (fails) {
        fprintf(stderr, "FAIL smc lcd paint: %u assertion(s)\n", fails);
        return 1;
    }
    printf("PASS smc lcd paint: overlay, page peek and grid keep one snapshot; next frame shows B; held notes, REC and PANIC unchanged (%u DMA hooks, snapshot %u bytes)\n",
           hooks, (unsigned)sizeof(struct smc_paint));
    return 0;
}
