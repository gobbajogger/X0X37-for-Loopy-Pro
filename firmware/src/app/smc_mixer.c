/* SPDX-License-Identifier: GPL-3.0-only */
/* M-VAVE SMC-Mixer control surface. Included after ui.c in the firmware unity build.
 * DAW messages match Mackie Control; the stock User preset uses CC and MMC SysEx.
 * TIMER5 only copies complete messages. All project, sequencer and display work is in the main loop. */
#include "smc_protocol.h"

#define SMC_Q 32u
/* Stored while Pause is held down: main mix, the 909 MIX page, or the 808 MIX page. */
enum { SMC_MAIN = 0, SMC_909 = 1, SMC_808 = 2 };
extern int plat_ble_smc_send(const uint8_t *msg, uint8_t len);
extern uint8_t smk_clip_mode_on(void);
extern int smk_ck_smc_turn(int n, int delta);
extern int smk_ck_smc_nav(uint8_t note);
static struct {
    uint8_t n, slot, b[6];
} smc_q[SMC_Q];
static volatile uint8_t smc_qw, smc_qr, smc_link_pending[2];
/* ISR owns enqueued/dropped; main loop owns dispatched/reason. Modulo 256. */
static volatile uint8_t smc_enqueued, smc_dropped, smc_dispatched, smc_reason;
uint32_t smc_rx_diag(void)
{
    return (uint32_t)smc_enqueued | (uint32_t)smc_dropped << 8 |
           (uint32_t)smc_dispatched << 16 | (uint32_t)smc_reason << 24;
}
static uint8_t smc_host_q[SMC_Q][3];
static volatile uint8_t smc_host_w, smc_host_r;
static uint8_t smc_mmc_q[2][16];
static volatile uint8_t smc_mmc_w[2], smc_mmc_r[2];
/* Two mixer records. USB and the peripheral User stream stay on record 0.
 * smc is the record currently being handled; outside poll and frame it is record 0. */
static struct smc_rec {
    pref_t target[8];
    uint8_t value[8], last[8], seen, picked, pending;
    uint8_t up, fresh, solo, rec_part;
    uint8_t lit[128], known[128];
    uint8_t fx_restore[3];
    uint8_t lock, mode;
} smc_slot[2];
static struct smc_rec *smc_io = &smc_slot[0];
#define smc (*smc_io)
int smc_tx_slot;

static uint8_t smc_solo_mask(void)
{
    return (uint8_t)(smc_slot[0].solo | smc_slot[1].solo);
}

void smc_ble_midi_slot(unsigned slot, const uint8_t *msg, uint8_t len)
{
    uint8_t w = smc_qw, i;
    if (slot > 1u) slot = 0;
    if (!msg || !len || len > 6u || (uint8_t)(w - smc_qr) >= SMC_Q) {
        smc_dropped++; return;
    }
    smc_q[w % SMC_Q].n = len;
    smc_q[w % SMC_Q].slot = (uint8_t)slot;
    for (i = 0; i < len; i++)
        smc_q[w % SMC_Q].b[i] = msg[i];
    __asm__ volatile("" ::: "memory");
    smc_qw = (uint8_t)(w + 1u);
    smc_enqueued++;
}

void smc_ble_midi(const uint8_t *msg, uint8_t len) { smc_ble_midi_slot(0, msg, len); }

void smc_ble_link_slot(int slot, int up)
{
    if ((unsigned)slot > 1u) return;
    smc_link_pending[slot] = (uint8_t)(up ? 1u : 2u);
}
void smc_ble_link(int up) { smc_ble_link_slot(0, up); }
int smc_ble_role_get(void) { return !!(proj.set.bt & PROJ_BT_SMC_CENTRAL); }
int smc_connected(void)
{
    return (smc_slot[0].up || smc_slot[1].up) && smc_ble_role_get();
}
#ifdef X0X_HOST
uint8_t smc_host_lock(unsigned slot) { return slot < 2u ? smc_slot[slot].lock : 0; }
uint8_t smc_host_mode(unsigned slot) { return slot < 2u ? smc_slot[slot].mode : 0; }
uint8_t smc_host_solo(unsigned slot) { return slot < 2u ? smc_slot[slot].solo : 0; }
#endif

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
    /* pref_t is four bytes; compare its complete packed identity. */
    _Static_assert(sizeof(pref_t) == sizeof(uint32_t), "packed parameter identity");
    union { pref_t ref; uint32_t word; } x = {a}, y = {b};
    return x.word == y.word;
}

static __attribute__((noinline)) pref_t smc_home_target(int n)
{
    static const pref_t targets[8] = {
        PR(R_ENG, T_MIX, 0, MX_LEVEL), PR(R_ENG, T_MIX, 1, MX_LEVEL),
        PR(R_ENG, T_MIX, 2, MX_LEVEL), PR(R_ENG, T_MIX, 3, MX_LEVEL),
        PR(R_ENG, T_MIX, 4, MX_LEVEL), PR(R_ENG, T_FX, 0, FX_RV_LEVEL),
        PR(R_ENG, T_FX, 0, FX_DL_LEVEL), PR(R_ENG, T_MST, 0, MST_DJF)
    };
    return targets[n];
}

/* Only visible cells are targets. Empty positions never fall through to another
 * page or to the selected part's mixer, which can duplicate visible controls. */
static pref_t smc_page_target(int n)
{
    if (ui.overlay == O_LIST)
        return n < 6 && ui.list_top + n < ui.list_n ? list_rows[ui.list_top + n] : NONE;
    return n < 4 ? pg.r[cur_page()][n] : NONE;
}

static int smc_drum_mix(void)
{
    return ui.overlay == O_NONE && !smk_clip_mode_on() ? drum_mix_page() : -1;
}

static int smc_drum_track(int kit, int strip)
{
    int track = (ui.sel[kit] / 8) * 8 + strip;
    return track < NDRUM ? track : -1;
}

/* A locked 909 or 808 record keeps that kit. Main mix is the eight home strips. */
static pref_t smc_mode_target(int n)
{
    int kit, track;
    if (smc.mode == SMC_909 || smc.mode == SMC_808) {
        kit = smc.mode == SMC_808;
        track = smc_drum_track(kit, n);
        return track >= 0 ? find_ref(kit ? T_808 : T_909, track, "Level") : NONE;
    }
    return smc_home_target(n);
}

/* Faders use the main mixer except on the visible drum MIX pages. The
 * sample bank follows selection, including the final three of eleven tracks.
 * A lock keeps the mode stored for this SMC instead of the screen. */
static pref_t smc_target(int n)
{
    int kit, track;
    if (ui.help || ui.overlay == O_ASK)
        return NONE;
    if (smc.lock)
        return smc_mode_target(n);
    kit = smc_drum_mix();
    if (kit >= 0) {
        track = smc_drum_track(kit, n);
        return track >= 0 ? find_ref(kit ? T_808 : T_909, track, "Level") : NONE;
    }
    return smc_home_target(n);
}

/* The three mixer modes share strip destinations between faders and knobs.
 * Sound pages, menus and clip editors keep their visible-field controls. */
static pref_t smc_knob_target(int n)
{
    pref_t r;
    if (ui.help || ui.overlay == O_ASK)
        return NONE;
    if (smc.lock)
        return smc_target(n);            /* locked knobs stay on this SMC's mix strips */
    if (ui.overlay == O_NONE && !smk_clip_mode_on() &&
        (ui.view == V_HOME || (ui.view == V_MIX && (cur_page() == 0 || drum_mix_page() >= 0))))
        return smc_target(n);
    r = smc_page_target(n);
    for (int i = 0; i < n; i++)
        if (smc_same(r, smc_page_target(i))) return NONE;
    return r;                         /* a paired fader does not disable this field */
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
    q = put_s(q, knob ? "SMC K" : (smc.picked & (1u << n)) ? "SMC F" : "PICK F");
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
        smc.seen &= (uint8_t)~bit;
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
    old = smc.value[n];
    crossed = smc_pickup_reached(smc.last[n], smc.seen & bit, old, v);
    smc.last[n] = v;
    smc.seen |= bit;
    if (!(smc.picked & bit)) {
        if (!crossed)
            goto show;
        smc.picked |= bit;
    }
    nv = (v * d->max + 63) / 127;
    if (nv != pref_get(r)) {
        pref_set(r, nv);
        motion_touch(r);
        smc.value[n] = smc_scaled(r);
        smc.pending |= bit;
    }
show:
    smc_show(n, 0, r);
    ui.act_t = plat_ms();
}

static void smc_knob(int n, int delta)
{
    if (ui.overlay == O_ASK || ui.help)
        return;
    if (!smc.lock && ui.overlay == O_NONE && smk_ck_smc_turn(n, delta))
        return;                          /* a SCALE / CHORD / ARP page owns its visible fields */
    pref_t r = smc_knob_target(n);
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
    if (part != ui.part)
        bass_step_cancel();             /* restore a held 303 step's sound before the page changes */
    ui.part = (uint8_t)part;
    ui.mixsel = (uint8_t)part;
    if (ui.view == V_GEN && !is_303())
        set_view(V_PART);
    ui.outline_ok = 0;
    build_pages();
}

static int smc_bank(uint8_t note)
{
    int kit = -1;
    if ((note != 0x2E && note != 0x2F) || ui.help || ui.overlay == O_ASK)
        return 0;
    if (smc.lock && (smc.mode == SMC_909 || smc.mode == SMC_808))
        kit = smc.mode == SMC_808;
    else if (ui.overlay == O_NONE && !smk_clip_mode_on() && drum_mix_page() >= 0)
        kit = drum_mix_page();
    else
        return 0;
    ui.sel[kit] = note == 0x2E ? 0 : 8;
    build_pages();
    say(PART_N[kit], note == 0x2E ? "1-8" : "9-11");
    return 1;
}

/* Main strips address parts and FX. Drum MIX pages pair M and SELECT with
 * the visible fader/knob bank. Reserved and global buttons navigate X0X. */
static void smc_button(uint8_t note, int mmc)
{
    int row, strip;
    if (ui.help) {
        if (note == 6) ui.help = 0;       /* R7: back */
        return;
    }
    if (ui.overlay == O_ASK) {
        if (note == 5 || note == 6)
            button_tap(note == 5 ? B_SEL : B_HOME);
        return;
    }
    if (ui.overlay == O_NONE && smk_ck_smc_nav(note))
        return;
    if (smc.lock && smc_bank(note))
        return;                           /* a 909 or 808 lock keeps that kit's two banks */
    if (note == 5 || note == 6 || note == 14) {
        button_tap(note == 5 ? (smk_clip_mode_on() && ui.overlay == O_NONE ? B_GLO : B_SEL) :
                   note == 6 ? B_HOME : B_GLO);
        return;
    }
    if (note >= 16 && note < 32) {
        int kit = smc_drum_mix();
        if (kit >= 0) {
            int track = smc_drum_track(kit, note & 7);
            if (note < 24) {
                if (track >= 0) mute_track(kit, track);
            } else {
                if (track >= 0) ui.sel[kit] = (uint8_t)track;
                build_pages();
            }
            return;
        }
    }
    if ((note == 0x30 || note == 0x31 ||
         ((note == 0x2E || note == 0x2F) && ui.view == V_PART && is_drum())) &&
        ui.overlay == O_NONE && !smk_clip_mode_on()) {
        int kit = drum_mix_page();
        if (kit < 0 && ui.view == V_PART && is_drum()) kit = ui.part;
        if (kit >= 0) {
            int track = ui.sel[kit] + ((note & 1) ? 1 : -1);
            if (track >= 0 && track < NDRUM) ui.sel[kit] = (uint8_t)track;
            build_pages();
            return;
        }
    }
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
                engine_smc_solo(smc_solo_mask());
                say((smc.solo & (1u << strip)) ? "SOLO" : "UNSOLO", PART_N[strip]);
            } else if (row == 2)       /* MUTE */
                mute_part(strip);
            else {                     /* SELECT */
                smc_select_part(strip);
                smc_view(V_PART, -1);
                say("SELECT", PART_N[strip]);
            }
        } else {
            int which = strip - NPARTS;
            if (row == 2)
                smc_toggle_fx(which);
            else {
                static const uint8_t view[4][3] = {
                    {V_FX, V_FX, V_HOME}, {V_MIX, V_MIX, V_PART},
                    {0, 0, 0}, {V_FX, V_FX, V_MIX}
                };
                static const int8_t page[4][3] = {
                    {0, 1, 0}, {-1, -1, 0}, {0, 0, 0}, {0, 1, 3}
                };
                smc_view(view[row][which], page[row][which]);
            }
        }
        return;
    }
    if (smc_bank(note)) return;
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
    case 0x5D:                        /* Pause toggles this SMC's page lock. It does not stop playback. */
        smc.lock = (uint8_t)!smc.lock;
        if (smc.lock) {
            int kit = smc_drum_mix();
            smc.mode = (uint8_t)(kit == 0 ? SMC_909 : kit == 1 ? SMC_808 : SMC_MAIN);
        }
        break;
    case 0x5E:
        if (!seq.playing) {
            seq.song_start = ui.song_sel < song()->len ? ui.song_sel : 0;
            seq_start(&seq);
        }
        break;
    case 0x5F:
        smc_record_toggle();
        break;
    case 0x60: case 0x61: case 0x62: case 0x63: {
        int d = (note & 1) ? 1 : -1;
        if (ui.overlay == O_LIST) {
            if (note < 0x62) list_move(d);
            else ui_menu_arrow(d);
        } else if (note < 0x62) {
            static const uint8_t views[] = {V_HOME, V_PART, V_SEQ, V_GEN, V_FX, V_MIX, V_SONG};
            int i = 0;
            while (i < 6 && views[i] != ui.view) i++;
            do { i = (i + d + 7) % 7; } while (views[i] == V_GEN && !is_303());
            smc_view(views[i], -1);
        } else {
            int p = cur_page() + d;
            if (p >= 0 && p < pg.n) *page_of(ui.view) = (uint8_t)p;
        }
        break;
    }
    default: break;
    }
}

static void smc_release(uint8_t note)
{
    if (note >= 8u && note < 8u + NPARTS) {
        smc.solo &= (uint8_t)~(1u << (note - 8u));
        engine_smc_solo(smc_solo_mask());
    }
}

/* Stock User preset commands 1..32 run M/S/R/SELECT in strip order. */
static void smc_guarded_button(uint8_t note, int mmc, int allow)
{
    if (allow && !perf_testing())
        smc_button(note, mmc);
    else if (perf_testing() && (note == 6u || note == 0x5Du || note == 0x5Eu))
        button_tap(B_HOME);            /* STOP, PLAY and back use the panel abort/restore path */
}

static uint8_t smc_message(const uint8_t *m, uint8_t len, int allow)
{
    uint8_t type, ch, a, b, user = !!(proj.set.bt & PROJ_BT_SMC_USER);
    if (len == 6 && user && m[0] == 0xF0 && m[1] == 0x7F && m[2] == 0x7F &&
        m[3] == 0x06 && m[5] == 0xF7) {
        a = smc_mmc_note(m[4]);
        if (a == 0xFFu) return 4;
        smc_guarded_button(a, 1, allow);
        return !allow || perf_testing() ? 1 : 0;
    }
    if (len < 3)
        return 4;
    type = m[0] & 0xF0u;
    ch = m[0] & 0x0Fu;
    a = m[1] & 0x7Fu;
    b = m[2] & 0x7Fu;
    if (!allow || perf_testing()) {
        if (type == 0x90u && b && (!ch || user))
            smc_guarded_button(a, 0, 0);
        return 1;                     /* restoration still owns every other queued input */
    }
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
    } else return 4; /* message is outside the selected preset's input format */
    return 0;
}

void smc_poll(void)
{
    uint8_t r, count = 0;
    int allow = !perf_testing(), slot;
    for (slot = 0; slot < 2; slot++) {
        uint8_t link = smc_link_pending[slot];
        if (!link) continue;
        smc_link_pending[slot] = 0;
        smc_io = &smc_slot[slot];
        smc.up = link == 1u;
        smc.fresh = smc.up;
        smc.picked = smc.seen = 0;
        smc.pending = 0xFFu;
        for (int i = 0; i < 128; i++) smc.known[i] = 0;
        if (!smc.up) {
            smc.lock = 0;
            smc.solo = 0;
            engine_smc_solo(smc_solo_mask());
            ui_say("SMC LOST", 0);
        } else {
            smc.lock = 0;
            ui_say("SMC CONNECTED", 0);
        }
    }
    if (!allow || ui.overlay == O_ASK || ui.help) {
        for (slot = 0; slot < 2; slot++) {
            smc_slot[slot].picked = smc_slot[slot].seen = 0;
            if (smc_slot[slot].solo) smc_slot[slot].solo = 0;
        }
        engine_smc_solo(0);
    }
    while (smc_qr != smc_qw && count++ < SMC_Q) {
        r = smc_qr;
        smc_io = &smc_slot[smc_q[r % SMC_Q].slot & 1u];
        if (!smc.up) smc_reason = 2;
        else if (!(proj.set.bt & PROJ_BT_SMC_CENTRAL)) smc_reason = 3;
        else {
            if (allow && !perf_testing()) smc_dispatched++;
            smc_reason = smc_message(smc_q[r % SMC_Q].b, smc_q[r % SMC_Q].n, allow);
        }
        smc_qr = (uint8_t)(r + 1u);
    }
    smc_io = &smc_slot[0];            /* USB and peripheral User bytes stay on the first record */
    count = 0;
    while (smc_host_r != smc_host_w && count++ < SMC_Q) {
        r = smc_host_r;
        if (proj.set.bt & PROJ_BT_SMC_USER)
            smc_message(smc_host_q[r % SMC_Q], 3, allow);
        smc_host_r = (uint8_t)(r + 1u);
    }
    for (int src = 0; src < 2; src++) {
        count = 0;
        while (smc_mmc_r[src] != smc_mmc_w[src] && count++ < 16u) {
            r = smc_mmc_r[src];
            if (proj.set.bt & PROJ_BT_SMC_USER)
                smc_guarded_button(smc_mmc_note(smc_mmc_q[src][r % 16u]), 1, allow);
            smc_mmc_r[src] = (uint8_t)(r + 1u);
        }
    }
    smc_io = &smc_slot[0];
}

static int smc_led(int note)
{
    int strip = note & 7, row = note >> 3;
    if (note == 5) return ui.overlay != O_NONE;
    if (note == 6) return 0;
    if (note == 14) return ui.view == V_GLO;
    if (note >= 16 && note < 32) {
        int kit = smc_drum_mix();
        if (kit >= 0) {
            if (note >= 24) return strip == ui.sel[kit] % 8;
            if (!ui.help) {
                int track = smc_drum_track(kit, strip);
                return track >= 0 && track_muted(kit, track);
            }
        }
    }
    if (note < 32) {
        if (strip < NPARTS) {
            if (row == 0) return ui.rec && smc.rec_part == strip;
            if (row == 1) return !!(smc.solo & (1u << strip));
            if (row == 2) return part_muted(strip);
            return ui.part == strip;
        }
        if (row == 2) return pref_get(strip == 7 ? PR(R_ENG, T_FX, 0, FX_VOLUME) :
                                     smc_home_target(strip)) == 0;
        return (strip == 5 && ui.view == V_FX) || (strip == 6 && ui.view == V_FX && cur_page() == 1) ||
               (strip == 7 && (ui.view == V_MIX || ui.view == V_HOME));
    }
    if (note == 0x5D) return smc.lock ? 1 : 0;
    if (note == 0x5E) return seq.playing;
    if (note == 0x5F) return ui.rec;
    return 0;
}

static void smc_frame_one(void)
{
    static const uint8_t global[11] = {0x2E, 0x2F, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F, 0x60, 0x61, 0x62, 0x63};
    uint8_t msg[3];
    int i, n, want;
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

void smc_frame(void)
{
    int slot;
    if (!(proj.set.bt & PROJ_BT_SMC_CENTRAL))
        return;
    for (slot = 0; slot < 2; slot++) {
        smc_io = &smc_slot[slot];
        if (!smc.up) continue;
        smc_tx_slot = slot;           /* lamps and fader positions return on this connection */
        smc_frame_one();
    }
    smc_io = &smc_slot[0];
    smc_tx_slot = 0;
}
