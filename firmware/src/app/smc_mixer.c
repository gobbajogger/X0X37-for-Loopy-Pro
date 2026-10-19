/* SPDX-License-Identifier: GPL-3.0-only */
/* M-VAVE SMC-Mixer control surface. Included after ui.c in the firmware unity build.
 * DAW messages match Mackie Control; the stock User preset uses CC and MMC SysEx.
 * TIMER5 only copies complete messages. All project, sequencer and display work is in the main loop. */
#include "smc_protocol.h"

#define SMC_Q 32u
extern int plat_ble_smc_send(const uint8_t *msg, uint8_t len);
extern uint8_t smk_clip_mode_on(void);
extern int smk_ck_smc_turn(int n, int delta);
static struct {
    uint8_t n, b[6];
} smc_q[SMC_Q];
static volatile uint8_t smc_qw, smc_qr, smc_link_pending;
static uint8_t smc_host_q[SMC_Q][3];
static volatile uint8_t smc_host_w, smc_host_r;
static uint8_t smc_mmc_q[2][16];
static volatile uint8_t smc_mmc_w[2], smc_mmc_r[2];
static struct {
    pref_t target[8];
    uint8_t value[8], last[8], seen, picked, pending;
    uint8_t up, fresh, solo, rec_part;
    uint8_t lit[128], known[128];
    uint8_t fx_restore[3];
} smc;

void smc_ble_midi(const uint8_t *msg, uint8_t len)
{
    uint8_t w = smc_qw, i;
    if (!msg || !len || len > 6u || (uint8_t)(w - smc_qr) >= SMC_Q)
        return;
    smc_q[w % SMC_Q].n = len;
    for (i = 0; i < len; i++)
        smc_q[w % SMC_Q].b[i] = msg[i];
    __asm__ volatile("" ::: "memory");
    smc_qw = (uint8_t)(w + 1u);
}

void smc_ble_link(int up) { smc_link_pending = (uint8_t)(up ? 1u : 2u); }
int smc_ble_role_get(void) { return !!(proj.set.bt & PROJ_BT_SMC_CENTRAL); }
int smc_connected(void) { return smc.up && smc_ble_role_get(); }

/* USB or peripheral BLE through the existing MIDI ring: only the explicitly enabled
 * User preset, channel 16. The separate ring has one audio-ISR producer; the BLE
 * central's TIMER5 producer never touches it. A full ring still consumes the message. */
int smc_midi_host(uint32_t pkt)
{
    uint8_t st = (uint8_t)(pkt >> 8), w = smc_host_w;
    uint8_t a = (uint8_t)(pkt >> 16) & 0x7Fu;
    if (!smc_host_accept(st, a, proj.set.bt & PROJ_BT_SMC_USER))
        return 0;
    if ((uint8_t)(w - smc_host_r) < SMC_Q) {
        smc_host_q[w % SMC_Q][0] = st;
        smc_host_q[w % SMC_Q][1] = (uint8_t)(pkt >> 16);
        smc_host_q[w % SMC_Q][2] = (uint8_t)(pkt >> 24);
        __asm__ volatile("" ::: "memory");
        smc_host_w = (uint8_t)(w + 1u);
    }
    return 1;
}

void smc_mmc_host(uint8_t cmd, uint8_t src)
{
    uint8_t w;
    if (src > 1u || cmd < 1u || cmd > 43u)
        return;
    w = smc_mmc_w[src];
    if ((uint8_t)(w - smc_mmc_r[src]) >= 16u)
        return;
    smc_mmc_q[src][w % 16u] = cmd;
    __asm__ volatile("" ::: "memory");
    smc_mmc_w[src] = (uint8_t)(w + 1u);
}

static int smc_same(pref_t a, pref_t b)
{
    return a.kind == b.kind && a.a == b.a && a.b == b.b && a.c == b.c;
}

static pref_t smc_home_target(int n)
{
    if (n < NPARTS)
        return PR(R_ENG, T_MIX, n, MX_LEVEL);
    if (n == 5)
        return PR(R_ENG, T_FX, 0, FX_RV_LEVEL);
    if (n == 6)
        return PR(R_ENG, T_FX, 0, FX_DL_LEVEL);
    return PR(R_ENG, T_FX, 0, FX_VOLUME);
}

/* Every page owns eight faders: its four visible cells, followed by the next page's
 * four cells. A one-page view uses the selected part's mixer strip for faders 5-8. */
static pref_t smc_page_target(int n)
{
    int p = cur_page();
    if (n < 4)
        return pg.r[p][n];
    if (p + 1 < pg.n)
        return pg.r[p + 1][n - 4];
    return PR(R_ENG, T_MIX, ui.part, n - 4);
}

static pref_t smc_target(int n)
{
    if (smk_clip_mode_on() || ui.view == V_HOME || ui.view == V_GLO)
        return smc_home_target(n);
    return smc_page_target(n);
}

static uint8_t smc_scaled(pref_t r)
{
    const x0x_param_t *d = pref_desc(r);
    int v;
    if (!d || !d->max || r.kind == R_ACT || r.kind == R_HDR || r.kind == R_NONE)
        return 0;
    v = pref_get(r);
    return (uint8_t)((v * 127 + d->max / 2) / d->max);
}

static void smc_show(int n, int knob, pref_t r)
{
    char t[28], *q = t;
    const char *name = pref_name(r);
    int i = 0;
    q = put_s(q, knob ? "SMC K" : "SMC F");
    q = put_i(q, n + 1);
    *q++ = ' ';
    while (name[i] && i < 12) *q++ = name[i++];
    *q++ = ' ';
    put_i(q, pref_get(r));
    say(t, 0);                        /* the header remains visible above the clip grid */
    ui.touched = -1;                   /* an encoder on the next page is not a local knob cell */
}

static void smc_notice(int n)
{
    pref_t r = smc_target(n);
    uint8_t v = smc_scaled(r);
    uint8_t bit = (uint8_t)(1u << n);
    if (!smc_same(r, smc.target[n]) || v != smc.value[n]) {
        smc.target[n] = r;
        smc.value[n] = v;
        smc.picked &= (uint8_t)~bit;
        smc.pending |= bit;
    }
}

static void smc_fader(int n, uint8_t v)
{
    pref_t r;
    const x0x_param_t *d;
    uint8_t bit, old;
    int crossed, nv;
    if (n < 0 || n >= 8)
        return;
    smc_notice(n);
    bit = (uint8_t)(1u << n);
    r = smc.target[n];
    d = pref_desc(r);
    if (!d || !d->max || r.kind == R_ACT || r.kind == R_HDR || r.kind == R_NONE)
        return;
    smc_show(n, 0, r);
    ui.act_t = plat_ms();
    old = smc.value[n];
    crossed = smc_pickup_reached(smc.last[n], smc.seen & bit, old, v);
    smc.last[n] = v;
    smc.seen |= bit;
    if (!(smc.picked & bit)) {
        if (!crossed)
            return;
        smc.picked |= bit;
    }
    nv = (v * d->max + 63) / 127;
    if (nv != pref_get(r)) {
        pref_set(r, nv);
        motion_touch(r);
        ui.act_t = plat_ms();
        smc.value[n] = smc_scaled(r);
        smc.pending |= bit;
        smc_show(n, 0, r);
    }
}

static void smc_knob(int n, int delta)
{
    if (smk_ck_smc_turn(n, delta))
        return;                          /* a SCALE / CHORD / ARP page owns its visible fields */
    pref_t r = smc_page_target(n);
    if (r.kind == R_NONE || r.kind == R_HDR || r.kind == R_ACT)
        return;
    turn_ref(r, delta, EN_K1 + n % 4);
    ui.act_t = plat_ms();
    smc_show(n, 1, r);
}

static void smc_view(int v, int page)
{
    set_view(v);
    build_pages();
    if (page >= 0 && page < pg.n)
        *page_of(v) = (uint8_t)page;
}

static void smc_toggle_fx(int which)
{
    int param = which == 0 ? FX_RV_LEVEL : which == 1 ? FX_DL_LEVEL : FX_VOLUME;
    pref_t r = PR(R_ENG, T_FX, 0, param);
    int v = pref_get(r);
    if (v) {
        smc.fx_restore[which] = (uint8_t)v;
        pref_set(r, 0);
    } else
        pref_set(r, smc.fx_restore[which] ? smc.fx_restore[which] : 100);
}

static void smc_record_toggle(void)
{
    bass_step_cancel();                /* panel REC does this before key-up changes from step to note */
    ui.rec = !ui.rec;
    if (ui.rec && is_303())
        ui.wpos[bidx()] = 0;
    smc.rec_part = ui.part;
    say(ui.rec ? "RECORD ON" : "RECORD OFF", PART_N[ui.part]);
}

static void smc_select_part(int part)
{
    ui.part = (uint8_t)part;
    ui.mixsel = (uint8_t)part;
    if (ui.view == V_GEN && !is_303())
        set_view(V_PART);
    ui.outline_ok = 0;
    build_pages();
}

/* All 32 strip buttons have a role. Strips 1-5 select/mute/solo/record the five
 * parts; 6-8 address reverb, delay and master. Global buttons navigate X0X. */
static void smc_button(uint8_t note, int mmc)
{
    int row, strip;
    if (note < 32u) {
        row = note >> 3;
        strip = note & 7;
        if (strip < NPARTS) {
            if (row == 0) {             /* REC */
                smc_select_part(strip);
                smc_record_toggle();
            } else if (row == 1) {      /* SOLO */
                if (mmc)                /* stock User MMC has no release: latch as fallback */
                    smc.solo ^= (uint8_t)(1u << strip);
                else
                    smc.solo |= (uint8_t)(1u << strip);
                engine_smc_solo(smc.solo);
                say((smc.solo & (1u << strip)) ? "SOLO" : "UNSOLO", PART_N[strip]);
            } else if (row == 2)       /* MUTE */
                mute_part(strip);
            else {                     /* SELECT */
                smc_select_part(strip);
                say("SELECT", PART_N[strip]);
            }
        } else {
            int which = strip - NPARTS;
            if (row == 2)
                smc_toggle_fx(which);
            else if (row == 0)
                smc_view(which == 0 ? V_FX : which == 1 ? V_FX : V_HOME, which == 1 ? 1 : 0);
            else if (row == 1)
                smc_view(which == 0 ? V_MIX : which == 1 ? V_MIX : V_PART, which == 2 ? 0 : -1);
            else
                smc_view(which == 0 ? V_FX : which == 1 ? V_FX : V_MIX, which == 1 ? 1 : which == 2 ? 3 : 0);
        }
        return;
    }
    switch (note) {
    case 0x2E: case 0x30:             /* bank / channel left: part */
        if (ui.part) smc_select_part(ui.part - 1);
        break;
    case 0x2F: case 0x31:
        if (ui.part + 1 < NPARTS) smc_select_part(ui.part + 1);
        break;
    case 0x5B: case 0x5C: {            /* rewind / forward: pattern */
        int next = seq.ppat[ui.part] + (note == 0x5B ? -1 : 1);
        if (next >= 0 && next < NPAT)
            seq_cue_part(&seq, ui.part, next);
        break;
    }
    case 0x5D: seq_stop(&seq); break;
    case 0x5E:
        if (!seq.playing) {
            seq.song_start = ui.song_sel < song()->len ? ui.song_sel : 0;
            seq_start(&seq);
        }
        break;
    case 0x5F:
        smc_record_toggle();
        break;
    case 0x60: case 0x61: {            /* up/down: list row or page */
        int d = note == 0x60 ? -1 : 1;
        if (ui.overlay == O_LIST)
            list_move(d);
        else {
            int p = cur_page() + d;
            if (p >= 0 && p < pg.n) *page_of(ui.view) = (uint8_t)p;
        }
        break;
    }
    case 0x62: case 0x63:             /* left/right: selected value, or page */
        if (ui.overlay == O_LIST)
            ui_menu_arrow(note == 0x62 ? -1 : 1);
        else {
            int p = cur_page() + (note == 0x62 ? -1 : 1);
            if (p >= 0 && p < pg.n) *page_of(ui.view) = (uint8_t)p;
        }
        break;
    default: break;
    }
}

static void smc_release(uint8_t note)
{
    if (note >= 8u && note < 8u + NPARTS) {
        smc.solo &= (uint8_t)~(1u << (note - 8u));
        engine_smc_solo(smc.solo);
    }
}

/* Stock User preset commands 1..32 run M/S/R/SELECT in strip order. */
static void smc_message(const uint8_t *m, uint8_t len)
{
    uint8_t type, ch, a, b, user = !!(proj.set.bt & PROJ_BT_SMC_USER);
    if (len == 6 && user && m[0] == 0xF0 && m[1] == 0x7F && m[2] == 0x7F &&
        m[3] == 0x06 && m[5] == 0xF7) {
        a = smc_mmc_note(m[4]);
        if (a != 0xFFu) smc_button(a, 1);
        return;
    }
    if (len < 3)
        return;
    type = m[0] & 0xF0u;
    ch = m[0] & 0x0Fu;
    a = m[1] & 0x7Fu;
    b = m[2] & 0x7Fu;
    if (type == 0xE0u && !user && ch < 8u)
        smc_fader(ch, (uint8_t)(((uint16_t)b << 7 | a) >> 7));
    else if (type == 0xB0u && a >= 16u && a < 24u && (user || ch == 0u)) {
        int delta = smc_relative(b);
        if (delta) smc_knob(a - 16u, delta);
    } else if (type == 0xB0u && user && a >= 40u && a < 48u)
        smc_fader(a - 40u, b);
    else if ((type == 0x90u || type == 0x80u) && (!ch || user)) {
        if (type == 0x90u && b) smc_button(a, 0);
        else smc_release(a);
    }
}

void smc_poll(void)
{
    uint8_t link = smc_link_pending, r, count = 0;
    int allow = ui.overlay != O_ASK && !perf_testing();
    if (link) {
        smc_link_pending = 0;
        smc.up = link == 1u;
        smc.fresh = smc.up;
        smc.picked = smc.seen = 0;
        smc.pending = 0xFFu;
        for (int i = 0; i < 128; i++) smc.known[i] = 0;
        if (!smc.up) {
            smc.solo = 0;
            engine_smc_solo(0);
            ui_say("SMC LOST", 0);
        } else
            ui_say("SMC CONNECTED", 0);
    }
    if (!allow) {
        smc.picked = smc.seen = 0;
        if (smc.solo) {
            smc.solo = 0;
            engine_smc_solo(0);
        }
    }
    while (smc_qr != smc_qw && count++ < SMC_Q) {
        r = smc_qr;
        if (allow && smc.up && (proj.set.bt & PROJ_BT_SMC_CENTRAL))
            smc_message(smc_q[r % SMC_Q].b, smc_q[r % SMC_Q].n);
        smc_qr = (uint8_t)(r + 1u);
    }
    count = 0;
    while (smc_host_r != smc_host_w && count++ < SMC_Q) {
        r = smc_host_r;
        if (allow && (proj.set.bt & PROJ_BT_SMC_USER))
            smc_message(smc_host_q[r % SMC_Q], 3);
        smc_host_r = (uint8_t)(r + 1u);
    }
    for (int src = 0; src < 2; src++) {
        count = 0;
        while (smc_mmc_r[src] != smc_mmc_w[src] && count++ < 16u) {
            r = smc_mmc_r[src];
            if (allow && (proj.set.bt & PROJ_BT_SMC_USER))
                smc_button(smc_mmc_note(smc_mmc_q[src][r % 16u]), 1);
            smc_mmc_r[src] = (uint8_t)(r + 1u);
        }
    }
}

static int smc_led(int note)
{
    int strip = note & 7, row = note >> 3;
    if (note < 32) {
        if (strip < NPARTS) {
            if (row == 0) return ui.rec && smc.rec_part == strip;
            if (row == 1) return !!(smc.solo & (1u << strip));
            if (row == 2) return part_muted(strip);
            return ui.part == strip;
        }
        if (row == 2) return pref_get(smc_home_target(strip)) == 0;
        return (strip == 5 && ui.view == V_FX) || (strip == 6 && ui.view == V_FX && cur_page() == 1) ||
               (strip == 7 && (ui.view == V_MIX || ui.view == V_HOME));
    }
    if (note == 0x5D) return !seq.playing;
    if (note == 0x5E) return seq.playing;
    if (note == 0x5F) return ui.rec;
    return 0;
}

void smc_frame(void)
{
    static const uint8_t global[11] = {0x2E, 0x2F, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F, 0x60, 0x61, 0x62, 0x63};
    uint8_t msg[3];
    int i, n, want;
    if (!smc.up || !(proj.set.bt & PROJ_BT_SMC_CENTRAL))
        return;
    for (i = 0; i < 8; i++) smc_notice(i);
    for (i = 0; i < 43; i++) {
        n = i < 32 ? i : global[i - 32];
        want = smc_led(n);
        if (smc.known[n] && smc.lit[n] == want) continue;
        msg[0] = 0x90;
        msg[1] = (uint8_t)n;
        msg[2] = (uint8_t)(want ? 127 : 0);
        if (!plat_ble_smc_send(msg, 3)) return;
        smc.known[n] = 1;
        smc.lit[n] = (uint8_t)want;
    }
    for (i = 0; i < 8; i++)
        if (smc.pending & (1u << i)) {
            msg[0] = (uint8_t)(0xE0u + i);
            msg[1] = smc.value[i];
            msg[2] = smc.value[i];
            if (!plat_ble_smc_send(msg, 3)) return;
            smc.pending &= (uint8_t)~(1u << i);
        }
}
