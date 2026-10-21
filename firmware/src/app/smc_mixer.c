/* SPDX-License-Identifier: GPL-3.0-only */
/* M-VAVE SMC-Mixer control surface. Included after ui.c in the firmware unity build.
 * DAW messages match Mackie Control; the stock User preset uses CC and MMC SysEx.
 * TIMER5 only copies complete messages. All project, sequencer and display work is in the main loop. */
#include "smc_protocol.h"

#define SMC_Q 32u
extern int plat_ble_smc_send(const uint8_t *msg, uint8_t len);
extern uint8_t smk_clip_mode_on(void);
extern int smk_ck_smc_turn(int n, int delta);
extern int smk_ck_smc_nav(uint8_t note);
typedef struct {
    uint8_t n, b[6];
} smc_event_t;
static smc_event_t smc_q[SMC_Q];
static volatile uint8_t smc_qw, smc_qr;
/* TIMER5 owns publication; the main loop owns acknowledgment. A generation
 * keeps a reconnect visible even if its final state equals the previous one. */
static volatile uint32_t smc_link_published;
static uint32_t smc_link_seen;
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
static struct {
    pref_t target[8];
    uint8_t value[8], last[8], seen, picked, pending;
    uint8_t up, fresh, solo, rec_part;
    uint8_t lit[128], known[128];
    uint8_t fx_restore[3];
} smc;

int smc_project_active(void) { return smc.solo != 0; }
X0X_COLD static void smc_project_drop(void)
{
    smc_qr = smc_qw;
    smc_host_r = smc_host_w;
    smc_mmc_r[0] = smc_mmc_w[0];
    smc_mmc_r[1] = smc_mmc_w[1];
}
X0X_COLD void smc_project_reset(void)
{
    uint8_t up = smc.up;
    smc_project_drop();
    memset(&smc, 0, sizeof smc);
    smc.up = smc.fresh = up;
    smc.pending = 0xFFu;
    engine_smc_solo(0);
}

void smc_ble_midi(const uint8_t *msg, uint8_t len)
{
    uint8_t w = smc_qw, i;
    if (!msg || !len || len > 6u || (uint8_t)(w - smc_qr) >= SMC_Q) {
        smc_dropped++; return;
    }
    smc_q[w % SMC_Q].n = len;
    for (i = 0; i < len; i++)
        smc_q[w % SMC_Q].b[i] = msg[i];
    __asm__ volatile("" ::: "memory");
    smc_qw = (uint8_t)(w + 1u);
    smc_enqueued++;
}

void smc_ble_link(int up)
{
    uint32_t next = (smc_link_published + 2u) & ~1u;
    smc_link_published = next | (uint32_t)(up != 0);
}
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

/* Faders use the main mixer except on the visible drum MIX pages. The
 * sample bank follows selection, including the final three of eleven tracks. */
static pref_t smc_target(int n)
{
    int kit, track;
    if (ui.help || ui.overlay == O_ASK)
        return NONE;
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
    if (ui.overlay == O_NONE && smk_ck_smc_turn(n, delta))
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
    if ((note == 0x2E || note == 0x2F) && !ui.help && ui.overlay == O_NONE &&
        !smk_clip_mode_on() && drum_mix_page() >= 0) {
        int kit = drum_mix_page();
        ui.sel[kit] = note == 0x2E ? 0 : 8;
        build_pages();
        say(PART_N[kit], note == 0x2E ? "1-8" : "9-11");
        return 1;
    }
    return 0;
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
                engine_smc_solo(smc.solo);
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
        engine_smc_solo(smc.solo);
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
    uint32_t link = smc_link_published;
    uint8_t r, count = 0;
    int allow = !perf_testing();
    if (ui_project_busy()) {
        smc_project_drop();
        return;
    }
    if (link != smc_link_seen) {
        /* Never clear the ISR's publication: it may already contain a newer
         * link edge than this snapshot. The next poll must still see it. */
        smc_link_seen = link;
        smc.up = (uint8_t)(link & 1u);
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
    if (!allow || ui.overlay == O_ASK || ui.help) {
        smc.picked = smc.seen = 0;
        if (smc.solo) {
            smc.solo = 0;
            engine_smc_solo(0);
        }
    }
    while (smc_qr != smc_qw && count++ < SMC_Q) {
        r = smc_qr;
        __asm__ volatile("" ::: "memory");
        smc_event_t msg = smc_q[r % SMC_Q];
        /* The ISR may reuse a full ring's slot as soon as the read index moves.
         * Keep a private event before releasing it. Release before dispatch so
         * a project action can still discard the remaining queued input. */
        __asm__ volatile("" ::: "memory");
        smc_qr = (uint8_t)(r + 1u);   /* an action can replace the project and discard the remaining queue */
        if (!smc.up) smc_reason = 2;
        else if (!(proj.set.bt & PROJ_BT_SMC_CENTRAL)) smc_reason = 3;
        else {
            if (allow && !perf_testing()) smc_dispatched++;
            smc_reason = smc_message(msg.b, msg.n, allow);
        }
    }
    count = 0;
    while (smc_host_r != smc_host_w && count++ < SMC_Q) {
        r = smc_host_r;
        uint8_t msg[3];
        __asm__ volatile("" ::: "memory");
        memcpy(msg, smc_host_q[r % SMC_Q], sizeof msg);
        __asm__ volatile("" ::: "memory");
        smc_host_r = (uint8_t)(r + 1u);
        if (proj.set.bt & PROJ_BT_SMC_USER)
            smc_message(msg, sizeof msg, allow);
    }
    for (int src = 0; src < 2; src++) {
        count = 0;
        while (smc_mmc_r[src] != smc_mmc_w[src] && count++ < 16u) {
            r = smc_mmc_r[src];
            __asm__ volatile("" ::: "memory");
            uint8_t cmd = smc_mmc_q[src][r % 16u];
            __asm__ volatile("" ::: "memory");
            smc_mmc_r[src] = (uint8_t)(r + 1u);
            if (proj.set.bt & PROJ_BT_SMC_USER)
                smc_guarded_button(smc_mmc_note(cmd), 1, allow);
        }
    }
}

static int smc_led(int note)
{
    int strip = note & 7, row = note >> 3;
    if (note == 5) return ui.overlay != O_NONE;
    if (note == 6) return 0;
    if (note == 7) return ui.view == V_HOME;
    if (note == 13) return ui.view == V_MIX;
    if (note == 14) return ui.view == V_GLO;
    if (note == 15) return ui.view == V_PART;
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
        /* SELECT 6/7/8 open reverb, delay and master; drum SELECT was handled above. */
        return (strip == 5 && ui.view == V_FX && cur_page() == 0) ||
               (strip == 6 && ui.view == V_FX && cur_page() == 1) ||
               (strip == 7 && ui.view == V_MIX && cur_page() == 3);
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
    if (ui_project_busy() || !smc.up || !(proj.set.bt & PROJ_BT_SMC_CENTRAL))
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
