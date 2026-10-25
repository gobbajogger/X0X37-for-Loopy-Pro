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
 * keeps a reconnect visible even if its final state equals the previous one.
 * Bits 1..8 carry the queue write cursor at the last link edge: this lets the
 * consumer discard only packets belonging to an old connection. */
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
/* MMC STOP must survive a saturated, per-source queue. The audio ISR only
 * publishes generations; the main loop acknowledges them after queued input. */
static volatile uint32_t smc_mmc_stop_published[2];
static uint32_t smc_mmc_stop_seen[2];
/* ISR publishes the overflowed STOP only after its FIFO cutoff. The consumer
 * must drain all commands before that cutoff before applying STOP. */
static volatile uint8_t smc_mmc_stop_boundary[2];
/* Separate single-writer generation counters: TIMER5 (central), audio ISR
 * (host). Main-loop snapshots never clear a producer-owned flag. */
static volatile uint32_t smc_ble_solo_release_lost, smc_host_solo_release_lost;
static uint32_t smc_ble_solo_release_seen, smc_host_solo_release_seen;
static int smc_is_solo_release(uint8_t st, uint8_t note, uint8_t velocity)
{
    uint8_t type = st & 0xF0u;
    return !(note & 0x80u) && !(velocity & 0x80u) &&
           note >= 8u && note < 16u &&
           (type == 0x80u || (type == 0x90u && !velocity));
}
static struct {
    pref_t target[8];
    uint8_t value[8], last[8], seen, picked, pending;
    uint8_t up, fresh, solo, rec_part;
    uint8_t input_seen;                /* physical history survives target/pickup refresh */
    uint8_t solo_target[8];            /* physical S key: 1..5 parts, then 909/808 tracks; 0 released */
    uint16_t drum_solo[NKIT];
    uint8_t lit[128], known[128];
    uint8_t fx_restore[3];
    uint32_t knob_t;                   /* acceleration of the knob now turning, not a panel encoder */
    uint8_t knob_run, knob_i;
} smc;

int smc_project_active(void) { return smc.solo || smc.drum_solo[0] || smc.drum_solo[1]; }
static void smc_solo_clear(void)
{
    memset(smc.solo_target, 0, sizeof smc.solo_target);
    smc.solo = 0;
    smc.drum_solo[0] = smc.drum_solo[1] = 0;
    engine_smc_solo(0);
}

static void smc_solo_publish(void)
{
    smc.solo = 0;
    smc.drum_solo[0] = smc.drum_solo[1] = 0;
    for (int i = 0; i < 8; i++) {
        int target = smc.solo_target[i];
        if (target > 0 && target <= NPARTS)
            smc.solo |= (uint8_t)(1u << (target - 1));
        else if (target > NPARTS) {
            target -= NPARTS + 1;
            smc.drum_solo[target / NDRUM] |= (uint16_t)(1u << (target % NDRUM));
        }
    }
    engine_smc_solo_targets(smc.solo, smc.drum_solo[0], smc.drum_solo[1]);
}

static void smc_solo_press(int strip, int target, int mmc)
{
    if (mmc)                           /* no key-up: another press toggles, a new bank replaces this key's latch */
        smc.solo_target[strip] = smc.solo_target[strip] == target ? 0 : (uint8_t)target;
    else if (!smc.solo_target[strip])    /* repeats must not move a held key to a different bank or page */
        smc.solo_target[strip] = (uint8_t)target;
    smc_solo_publish();
}
X0X_COLD static void smc_project_drop(void)
{
    smc_qr = smc_qw;
    smc_host_r = smc_host_w;
    smc_mmc_r[0] = smc_mmc_w[0];
    smc_mmc_r[1] = smc_mmc_w[1];
    smc_mmc_stop_seen[0] = smc_mmc_stop_published[0];
    smc_mmc_stop_seen[1] = smc_mmc_stop_published[1];
    /* A reset has already cleared all held solos; do not carry stale
     * overflow releases into the new project's queued presses. */
    smc_ble_solo_release_seen = smc_ble_solo_release_lost;
    smc_host_solo_release_seen = smc_host_solo_release_lost;
}
X0X_COLD void smc_project_reset(void)
{
    uint8_t up = smc.up;
    smc_ui_cancel();
    smc_project_drop();
    memset(&smc, 0, sizeof smc);
    smc.up = smc.fresh = up;
    smc.pending = 0xFFu;
    engine_smc_solo(0);
}

void smc_ble_midi(const uint8_t *msg, uint8_t len)
{
    uint8_t w = smc_qw, i;
    if (!msg || !len || len > 6u) {
        smc_dropped++; return;
    }
    if ((uint8_t)(w - smc_qr) >= SMC_Q) {
        /* Never leave a held Solo latched after its key-up was rejected.
         * A later main-loop poll clears held solos after draining older keys. */
        if (len == 3u && smc_is_solo_release(msg[0], msg[1], msg[2]) &&
            (!(msg[0] & 15u) || (proj.set.bt & PROJ_BT_SMC_USER)))
            smc_ble_solo_release_lost++;
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
    uint32_t next = (smc_link_published + 0x200u) & ~0x1FFu;
    smc_link_published = next | ((uint32_t)smc_qw << 1) | (uint32_t)(up != 0);
}
int smc_ble_role_get(void) { return !!(proj.set.bt & PROJ_BT_SMC_CENTRAL); }
int smc_connected(void) { return smc.up && smc_ble_role_get(); }

/* USB or peripheral BLE through the existing MIDI ring: only the explicitly enabled
 * User preset, channel 16. The separate ring has one audio-ISR producer; the BLE
 * central's TIMER5 producer never touches it. A full ring still consumes the message. */
int smc_midi_host(uint32_t pkt)
{
    uint8_t st = (uint8_t)(pkt >> 8), w = smc_host_w;
    uint8_t a = (uint8_t)(pkt >> 16), b = (uint8_t)(pkt >> 24);
    /* A malformed data byte must never be masked into an SMC button or CC.
     * In particular 0x88 must not turn into held Solo 1 (note 0x08). */
    if ((a | b) & 0x80u)
        return 0;
    if (!smc_host_accept(st, a, proj.set.bt & PROJ_BT_SMC_USER))
        return 0;
    if ((uint8_t)(w - smc_host_r) < SMC_Q) {
        smc_host_q[w % SMC_Q][0] = st;
        smc_host_q[w % SMC_Q][1] = a;
        smc_host_q[w % SMC_Q][2] = b;
        __asm__ volatile("" ::: "memory");
        smc_host_w = (uint8_t)(w + 1u);
    } else if (smc_is_solo_release(st, a, b)) {
        smc_host_solo_release_lost++;
    }
    return 1;
}

void smc_mmc_host(uint8_t cmd, uint8_t src)
{
    uint8_t w;
    if (src > 1u || cmd < 1u || cmd > 43u)
        return;
    w = smc_mmc_w[src];
    if ((uint8_t)(w - smc_mmc_r[src]) >= 16u) {
        /* User-preset MMC command 34 is STOP. Never drop it on overload. */
        if (cmd == 34u) {
            smc_mmc_stop_boundary[src] = w;
            __asm__ volatile("" ::: "memory");
            smc_mmc_stop_published[src]++;
        }
        return;
    }
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
    if (ui.overlay != O_NONE || smk_clip_mode_on()) return -1;
    /* SEQ follows its selected drum part; MIX follows the kit actually shown. */
    return ui.view == V_SEQ && is_drum() ? ui.part : drum_mix_page();
}

static int smc_drum_track(int kit, int strip)
{
    int track = (ui.sel[kit] / 8) * 8 + strip;
    return track < NDRUM ? track : -1;
}

/* Faders use the main mixer except on drum SEQ and visible drum MIX pages. The
 * sample bank follows selection, including the final three of eleven tracks. */
static pref_t smc_target(int n)
{
    int kit, track;
    /* Only a confirmation parks the surface. Help is a card only: controls keep
     * working under it, including fader pickup toward the live target. */
    if (ui.overlay == O_ASK)
        return NONE;
    kit = smc_drum_mix();
    if (kit >= 0) {
        track = smc_drum_track(kit, n);
        return track >= 0 ? find_ref(kit ? T_808 : T_909, track, "Level") : NONE;
    }
    return smc_home_target(n);
}

/* One-based owner of a visible row, including cross-strip duplicates. */
static int smc_fader_of(pref_t r)
{
    for (int i = 0; i < 8; i++)
        if (r.kind != R_NONE && smc_same(r, smc_target(i))) return i + 1;
    return 0;
}

/* Faders retain levels/returns/filter; knobs add a separate useful dimension.
 * A visible list row on any active fader is parked rather than redirected. */
static pref_t smc_knob_target(int n)
{
    pref_t r;
    int kit = smc_drum_mix();
    if (ui.overlay == O_ASK)
        return NONE;
    if (kit >= 0) {
        int track = smc_drum_track(kit, n);
        return track >= 0 ? find_ref(kit ? T_808 : T_909, track, "Pan") : NONE;
    }
    if (ui.overlay == O_NONE && !smk_clip_mode_on() &&
        (ui.view == V_HOME || (ui.view == V_MIX && cur_page() == 0))) {
        if (n < NPARTS) return PR(R_ENG, T_MIX, n, MX_PAN);
        static const pref_t tail[3] = {
            PR(R_ENG, T_FX, 0, FX_RV_DECAY), PR(R_ENG, T_FX, 0, FX_DL_FDBK),
            PR(R_ENG, T_MST, 0, MST_RESO)
        };
        return tail[n - NPARTS];
    }
    r = smc_page_target(n);
    if (ui.overlay == O_NONE && !smk_clip_mode_on() &&
        ((ui.view == V_GLO && n == 2) || (ui.view == V_MIX && n == 3)) &&
        smc_same(r, PR(R_ENG, T_MST, 0, MST_DJF)))
        r = PR(R_ENG, T_MST, 0, MST_RESO);
    if (smc_fader_of(r)) return NONE;
    for (int i = 0; i < n; i++)
        if (smc_same(r, smc_page_target(i))) return NONE;
    return r;
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

/* Temporary presentation only: the underlying ui/clip state is never replaced or
 * restored. A navigation edge cancels this owner rather than scheduling a write. */
#define SMC_UI_MS 1200u
static struct {
    uint32_t until, context;
    uint8_t on, kind, strip, action;
    char name[24], value[24];
} smc_ui;

static uint32_t smc_ui_context(void)
{
    uint32_t h = 2166136261u;
#define SMC_CTX(v) (h = (h ^ (uint32_t)(v)) * 16777619u)
    SMC_CTX(ui.view); SMC_CTX(ui.part); SMC_CTX(cur_page());
    SMC_CTX(ui.mixsel); SMC_CTX(ui.sel[0]); SMC_CTX(ui.sel[1]);
    SMC_CTX(ui.overlay); SMC_CTX(ui.help); SMC_CTX(ui.list_sel); SMC_CTX(ui.list_top);
    SMC_CTX(ui.spage); SMC_CTX(ui.pbank); SMC_CTX(project_revision);
    SMC_CTX(smk_clip_mode_on());
#undef SMC_CTX
    return h;
}

void smc_ui_cancel(void)
{
    if (!smc_ui.on)
        return;
    smc_ui.on = 0;
    for (int i = 0; i < 4; i++) blit_hash[i] = 0;
#if BOARD_SMK37
    { extern void smk_clip_lcd_force(void); smk_clip_lcd_force(); }
#endif
}

int smc_ui_active(void)
{
    if (smc_ui.on && (ui_project_busy() || ui.help || ui.overlay == O_ASK ||
        smc_ui.context != smc_ui_context() ||
        (int32_t)(plat_ms() - smc_ui.until) >= 0))
        smc_ui_cancel();
    return smc_ui.on;
}

static void smc_ui_begin(int n, int kind)
{
    if (ui.help || ui.overlay == O_ASK || ui_project_busy()) {
        smc_ui_cancel();
        return;                         /* Help keeps its card; confirmation keeps ownership. */
    }
    smc_ui.on = 1;
    smc_ui.strip = (uint8_t)n;
    smc_ui.kind = (uint8_t)kind;
    smc_ui.context = smc_ui_context();
    smc_ui.until = plat_ms() + SMC_UI_MS;
}

/* Bounded copies also cover enum text from the clip editor. */
static void smc_ui_text(char *to, const char *from)
{
    int i = 0;
    while (from && from[i] && i < 23) { to[i] = from[i]; i++; }
    to[i] = 0;
}

void smc_ui_clip_show(int n, const char *name, const char *value)
{
    smc_ui_text(smc_ui.name, name);
    smc_ui_text(smc_ui.value, value);
    smc_ui_begin(n, 2);
}

/* Include the destination when a page labels several different controls Level. */
static void smc_ui_label(pref_t r, char *text)
{
    const char *name = pref_name(r);
    char *q = text;
    if (r.kind == R_ENG && r.a == T_MIX && r.b < NPARTS) {
        q = put_s(q, PART_N[r.b]);
        name = r.c == MX_LEVEL ? "LEVEL" : r.c == MX_PAN ? "PAN" : pref_name(r);
        *q++ = ' ';
    } else if (r.kind == R_ENG && (r.a == T_909 || r.a == T_808) && r.b < NDRUM) {
        q = put_s(q, engine_voice_name(r.a, r.b));
        *q++ = ' ';
    }
    if (r.kind == R_ENG && r.a == T_FX && r.c == FX_RV_DECAY) name = "REVERB DECAY";
    if (r.kind == R_ENG && r.a == T_FX && r.c == FX_DL_FDBK) name = "DELAY FEEDBACK";
    if (r.kind == R_ENG && r.a == T_MST && r.c == MST_RESO) name = "FILTER RESONANCE";
    /* Destination prefixes have at most eight bytes; keep each complete label bounded. */
    while (*name && q < text + 23) *q++ = *name++;
    *q = 0;
}

static int smc_led(int note);

/* Quiet two-column overview; the touched destination owns the main area. */
static UI_DRAW void smc_ui_grid(void)
{
    for (int n = 0; n < 8; n++) {
        pref_t r = smc_ui.kind == 1 ? smc_knob_target(n) : smc_target(n);
        char name[24], tag[5];
        int x = 5 + (n & 1) * 120, y = 6 + (n >> 1) * 18;
        uint16_t c = n == smc_ui.strip && smc_ui.kind < 2 ? C_HI : C_GRAY;
        put_i(put_s(tag, smc_ui.kind == 1 ? "K" : "F"), n + 1);
        cv_text(x, y, &FONT_T, tag, c);
        if (r.kind == R_NONE) {
            put_s(name, "--");
            if (smc_ui.kind == 1) {
                int owner = smc_fader_of(smc_page_target(n));
                if (owner) put_i(put_s(name, "ON FADER "), owner);
            }
        } else smc_ui_label(r, name);
        name[15] = 0;
        cv_text(x + 19, y, &FONT_XS, caps_of(name), c);
    }
}

void smc_ui_draw(void)
{
    char name[24], num[32], unit[12], tag[80];
    const char *status = "ACTIVE";
    pref_t r = smc_ui.kind == 1 ? smc_knob_target(smc_ui.strip) : smc_target(smc_ui.strip);
    int kit = smc_drum_mix();
    cv_begin(240, HDR_H, C_BLACK);
    if (kit >= 0)
        put_i(put_s(put_s(put_s(tag, PART_N[kit]), ui.view == V_SEQ ? " SEQ" : " MIX"), " BANK "), ui.sel[kit] / 8 + 1);
    else if (smk_clip_mode_on()) put_s(tag, "CLIP KEYS");
    else if (ui.overlay == O_LIST) put_s(tag, list_title);
    else if (ui.view == V_HOME) put_s(tag, "HOME / PARTS");
    else knob_tag(tag);
    { char *end = tag;
      while (*end) end++;
      while (tw(&FONT_XS, tag) > 196) *--end = 0;
    }
    cv_text(5, vc(&FONT_XS, 0, HDR_H), &FONT_XS, caps_of(tag), C_GRAY);
    put_i(put_s(tag, smc_ui.kind == 3 ? "KEY " : smc_ui.kind == 1 || smc_ui.kind == 2 ? "K" : "F"), smc_ui.strip + 1);
    text_r(235, vc(&FONT_S, 0, HDR_H), &FONT_S, tag, C_HI);
    cv_commit(0, 0, 0);
    cv_begin(240, 2 * BAND_H, C_BLACK);
    cv_band(0, 2 * BAND_H);
    if (smc_ui.kind == 2) {
        put_s(name, smc_ui.name);
        put_s(num, smc_ui.value); unit[0] = 0;
    } else if (smc_ui.kind == 3) {
        int note = smc_ui.action;
        const char *action = note >= 0x5D ? "TRANSPORT" : note < 8 ? "RECORD" : note < 16 ? "SOLO" : "MUTE";
        if (note < 24) {
            if (kit < 0 && smc_ui.strip == 7) r = PR(R_ENG, T_FX, 0, FX_VOLUME);
            smc_ui_label(r, name);
        } else put_s(name, "TRANSPORT");
        put_s(put_s(put_s(num, action), " "), note >= 0x5D ?
              (note == 0x5F ? ui.rec ? "REC ON" : "REC OFF" : seq.playing ? "PLAYING" : "STOPPED") :
              (note < 8 ? ui.rec : smc_led(note)) ? "ON" : "OFF");
        unit[0] = 0;
    } else if (smc_ui.kind == 1 && r.kind == R_NONE) {
        smc_ui_label(smc_page_target(smc_ui.strip), name);
        put_i(put_s(num, "ON FADER "), smc_fader_of(smc_page_target(smc_ui.strip)));
        unit[0] = 0; status = "USE FADER";
    } else {
        smc_ui_label(r, name);
        pref_value_of(r, pref_get(r), num, unit);
        if (smc_ui.kind == 0 && !(smc.picked & (1u << smc_ui.strip)))
            status = smc.last[smc_ui.strip] < smc_scaled(r) ? "PICKUP: MOVE UP TO TARGET" : "PICKUP: MOVE DOWN TO TARGET";
    }
    text_c(120, 14, tw(&FONT_S, name) > 232 ? &FONT_XS : &FONT_S, caps_of(name), C_WHITE);
    cell_value(120, 48, num, unit, C_HI);
    text_c(120, 94, &FONT_XS, status, C_HI);
    if (smc_ui.kind == 0) {
        cv_rect(14, 119, 212, 2, C_LINE);
        cv_rect(14 + smc_scaled(r) * 210 / 127, 114, 2, 11, C_HI);
        cv_rect(14 + smc.last[smc_ui.strip] * 210 / 127, 117, 2, 5, C_WHITE);
    }
    cv_commit_rect(1, 0, MAIN_Y);
    cv_begin(240, KNOB_H, C_BLACK);
    cv_rect(0, 0, 240, 1, C_LINE);
    smc_ui_grid();
    cv_commit(3, 0, KNOB_Y);
}

static void smc_show(int n, int knob, pref_t r)
{
    (void)r;
    smc_ui_begin(n, knob);
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
    int crossed, nv, moved;
    if (n < 0 || n >= 8)
        return;
    bit = (uint8_t)(1u << n);
    /* A repeated absolute sample is not a new gesture. Keep this history
     * separate from pickup: navigation/external edits deliberately reset seen. */
    moved = !(smc.input_seen & bit) || smc.last[n] != v;
    smc.input_seen |= bit;
    smc_notice(n);
    r = smc.target[n];
    d = pref_desc(r);
    if (!d || !d->max || r.kind == R_ACT || r.kind == R_HDR || r.kind == R_NONE) {
        smc.last[n] = v;
        return;
    }
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
    if (moved) {
        smc_show(n, 0, r);
        ui.act_t = plat_ms();
    }
}

static void smc_knob(int n, int delta)
{
    uint32_t saved_t;
    uint8_t saved_run;
    if (ui.overlay == O_ASK)
        return;
    if (ui.overlay == O_NONE && smk_ck_smc_turn(n, delta))
        return;                          /* a SCALE / CHORD / ARP page owns its visible fields */
    pref_t r = smc_knob_target(n);
    if (r.kind == R_NONE || r.kind == R_HDR || r.kind == R_ACT) {
        if (r.kind == R_NONE && smc_fader_of(smc_page_target(n))) smc_show(n, 1, r);
        return;
    }
    /* Eight strips share one acceleration slot. Keep it off the panel encoders, and
     * start again when the finger moves to another strip. */
    saved_t = ui.enc_t[EN_K1];
    saved_run = ui.enc_run[EN_K1];
    if (smc.knob_i != (uint8_t)(n + 1)) {
        ui.enc_t[EN_K1] = 0;
        ui.enc_run[EN_K1] = 0;
        smc.knob_i = (uint8_t)(n + 1);
    } else {
        ui.enc_t[EN_K1] = smc.knob_t;
        ui.enc_run[EN_K1] = smc.knob_run;
    }
    turn_ref(r, delta, EN_K1);
    smc.knob_t = ui.enc_t[EN_K1];
    smc.knob_run = ui.enc_run[EN_K1];
    ui.enc_t[EN_K1] = saved_t;
    ui.enc_run[EN_K1] = saved_run;
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
    if (part != ui.part) {
        bass_step_cancel();             /* restore a held 303 step's sound before the page changes */
        drum_step_cancel();
    }
    ui.part = (uint8_t)part;
    ui.mixsel = (uint8_t)part;
    if (ui.view == V_GEN && !is_303())
        set_view(V_PART);
    ui.outline_ok = 0;
    build_pages();
}

static void smc_select_drum(int kit, int track)
{
    if (ui.sel[kit] != track) drum_step_cancel();
    ui.sel[kit] = (uint8_t)track;
    smc_select_part(kit);               /* the SMK pads and recording now share the visible drum's selection */
}

static int smc_bank(uint8_t note)
{
    if ((note == 0x2E || note == 0x2F) && ui.overlay == O_NONE &&
        !smk_clip_mode_on() && smc_drum_mix() >= 0) {
        int kit = smc_drum_mix();
        smc_select_drum(kit, note == 0x2E ? 0 : 8);
        return 1;
    }
    return 0;
}

/* All four drum SEQ/MIX rows follow the visible fader/knob bank. Outside those
 * pages the strips retain their whole-part, FX and navigation functions. */
static void smc_button_apply(uint8_t note, int mmc)
{
    int row, strip;
    if (ui.help && note == 6) {          /* R7 closes the card; every other control keeps working under it */
        ui.help = 0;
        return;
    }
    if (ui.overlay == O_ASK) {
        if (note == 5 || note == 6)
            button_tap(note == 5 ? B_SEL : B_HOME);
        return;
    }
    if (ui.overlay == O_NONE && smk_ck_smc_nav(note))
        return;
    if (!mmc && note >= 8u && note < 16u && smc.solo_target[note - 8u])
        return;                        /* a held Solo's repeat cannot turn into a navigation shortcut */
    if (note < 32u) {
        int kit = smc_drum_mix();
        if (kit >= 0) {
            int track = smc_drum_track(kit, note & 7);
            if (track < 0) return;      /* bank 2's empty positions never become navigation shortcuts */
            switch (note >> 3) {
            case 0:
                smc_select_drum(kit, track);
                smc_record_toggle();
                break;
            case 1:
                smc_solo_press(note & 7, NPARTS + 1 + kit * NDRUM + track, mmc);
                say(smc.drum_solo[kit] & (1u << track) ? "SOLO" : "UNSOLO",
                    engine_voice_name(kit ? T_808 : T_909, track));
                break;
            case 2:
                mute_track(kit, track);
                break;
            default:
                smc_select_drum(kit, track);
                break;
            }
            return;
        }
    }
    if (note == 5 || note == 6 || note == 14) {
        button_tap(note == 5 ? (smk_clip_mode_on() && ui.overlay == O_NONE ? B_GLO : B_SEL) :
                   note == 6 ? B_HOME : B_GLO);
        return;
    }
    if ((note == 0x30 || note == 0x31 ||
         ((note == 0x2E || note == 0x2F) && ui.view == V_PART && is_drum())) &&
        ui.overlay == O_NONE && !smk_clip_mode_on()) {
        int kit = smc_drum_mix();
        if (kit < 0 && ui.view == V_PART && is_drum()) kit = ui.part;
        if (kit >= 0) {
            int track = ui.sel[kit] + ((note & 1) ? 1 : -1);
            if (track >= 0 && track < NDRUM) smc_select_drum(kit, track);
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
                smc_solo_press(strip, strip + 1, mmc);
                say((smc.solo & (1u << strip)) ? "SOLO" : "UNSOLO", PART_N[strip]);
            } else if (row == 2)       /* MUTE */
                mute_part(strip);
            else {                     /* SELECT */
                smc_select_part(strip);
                smc_view(V_PART, -1);
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

static void smc_button(uint8_t note, int mmc)
{
    char saved[sizeof ui.msg];
    uint32_t until = ui.msg_until;
    int kit = smc_drum_mix();
    int action = note == 0x5Du || note == 0x5Eu || note == 0x5Fu ||
        (note < 24u && (kit >= 0 ? smc_drum_track(kit, note & 7u) >= 0 :
            (((note & 7u) < NPARTS && note != 14u) || note >= 16u)));
    memcpy(saved, ui.msg, sizeof saved);
    smc_ui_cancel();                     /* navigation owns its new screen immediately */
    smc_button_apply(note, mmc);
    if (action) {
        memcpy(ui.msg, saved, sizeof saved);  /* Help keeps its card and the preexisting notice too. */
        ui.msg_until = until;
        smc_ui.action = note;
        smc_ui_begin(note < 32u ? note & 7u : 0, 3);
    }
}

static void smc_release(uint8_t note)
{
    if (note >= 8u && note < 16u && smc.solo_target[note - 8u]) {
        smc.solo_target[note - 8u] = 0; /* release the original target, even after navigation */
        smc_solo_publish();
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
    /* Never alias malformed BLE MIDI data bytes into SMC notes/CCs. */
    if ((m[1] | m[2]) & 0x80u)
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

/* A full ring can refill while smc_poll drains it. Apply an overflowed STOP
 * at its original FIFO boundary, not merely at this poll's 16-item limit:
 * otherwise a preceding PLAY can execute on the next poll after STOP. */
static void smc_mmc_drain_stop(int src, int allow)
{
    uint32_t lost_stop = smc_mmc_stop_published[src];
    if (lost_stop != smc_mmc_stop_seen[src] &&
        smc_mmc_r[src] == smc_mmc_stop_boundary[src]) {
        smc_mmc_stop_seen[src] = lost_stop;
        if (proj.set.bt & PROJ_BT_SMC_USER)
            smc_guarded_button(0x5Du, 1, allow);
    }
}

void smc_poll(void)
{
    uint32_t link = smc_link_published;
    uint8_t r, count = 0;
    int allow = !perf_testing();
    /* Snapshot producer generations before either input ring drains. An ISR
     * can enqueue a press then lose its release while another ring drains;
     * acknowledging that new loss in this poll would leave Solo latched. */
    uint32_t ble_lost = smc_ble_solo_release_lost;
    uint32_t host_lost = smc_host_solo_release_lost;
    uint32_t ble_seen = smc_ble_solo_release_seen;
    uint32_t host_seen = smc_host_solo_release_seen;
    if (ui_project_busy()) {
        if (smc_project_active()) smc_solo_clear();
        smc_project_drop();
        return;
    }
    if (link != smc_link_seen) {
        /* Never clear the ISR's publication: it may already contain a newer
         * link edge than this snapshot. The next poll must still see it. */
        smc_link_seen = link;
        /* Skip pre-edge central packets without losing packets queued on the
         * newly connected link. Never rewind if another action already drained. */
        uint8_t cutoff = (uint8_t)(link >> 1);
        if ((uint8_t)(cutoff - smc_qr) <= (uint8_t)(smc_qw - smc_qr))
            smc_qr = cutoff;
        smc.up = (uint8_t)(link & 1u);
        smc.fresh = smc.up;
        smc.picked = smc.seen = smc.input_seen = 0;
        smc.knob_i = 0;
        smc.pending = 0xFFu;
        for (int i = 0; i < 128; i++) smc.known[i] = 0;
        smc_solo_clear();              /* also covers a down/up pair coalesced before this poll */
        if (!smc.up) {
            ui_say("SMC LOST", 0);
        } else
            ui_say("SMC CONNECTED", 0);
    }
    /* Confirmation parks pickup and solos. Help does not. */
    if (!allow || ui.overlay == O_ASK) {
        smc.picked = smc.seen = 0;
        smc.knob_i = 0;
        if (smc_project_active()) smc_solo_clear();
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
            smc_mmc_drain_stop(src, allow);
            r = smc_mmc_r[src];
            __asm__ volatile("" ::: "memory");
            uint8_t cmd = smc_mmc_q[src][r % 16u];
            __asm__ volatile("" ::: "memory");
            smc_mmc_r[src] = (uint8_t)(r + 1u);
            if (proj.set.bt & PROJ_BT_SMC_USER)
                smc_guarded_button(smc_mmc_note(cmd), 1, allow);
        }
        /* The generation stays pending if an earlier queued command remains
         * beyond this poll's budget, preserving STOP-before-next-PLAY order. */
        smc_mmc_drain_stop(src, allow);
    }
    /* Clear held Solo after draining presses preceding START-of-poll loss
     * snapshots, without swallowing an ISR refill that belongs to the next
     * poll. Project reset may have advanced seen: never rewind its cursor. */
    uint8_t missed_solo = 0;
    if (ble_lost != ble_seen && smc_ble_solo_release_seen == ble_seen) {
        smc_ble_solo_release_seen = ble_lost;
        missed_solo = 1;
    }
    if (host_lost != host_seen && smc_host_solo_release_seen == host_seen) {
        smc_host_solo_release_seen = host_lost;
        missed_solo = 1;
    }
    if (missed_solo && smc_project_active()) smc_solo_clear();
    if ((perf_testing() || ui.overlay == O_ASK) && smc_project_active())
        smc_solo_clear();              /* an earlier event in this batch may have transferred ownership */
}

static int smc_led(int note)
{
    int strip = note & 7, row = note >> 3;
    if (note < 32) {
        int kit = smc_drum_mix();
        if (kit >= 0) {
            int track = smc_drum_track(kit, strip);
            if (track < 0) return 0;
            if (row == 0) return ui.rec && ui.part == kit && ui.sel[kit] == track;
            if (row == 1) return !!(smc.drum_solo[kit] & (1u << track));
            if (row == 2) return track_muted(kit, track);
            return ui.sel[kit] == track;
        }
    }
    if (note == 5) return ui.overlay != O_NONE;
    if (note == 6) return 0;
    if (note == 7) return ui.view == V_HOME;
    if (note == 13) return ui.view == V_MIX;
    if (note == 14) return ui.view == V_GLO;
    if (note == 15) return ui.view == V_PART;
    if (note < 32) {
        if (strip < NPARTS) {
            /* Panel REC and part changes update ui.part / ui.rec without smc.rec_part. */
            if (row == 0) return ui.rec && ui.part == strip;
            if (row == 1) return !!(smc.solo & (1u << strip));
            if (row == 2) return part_muted(strip);
            return ui.part == strip;
        }
        /* Strips 6–8: M = mute/zero. R8/S6/S7/S8 are handled above; SELECT 6/7/8
         * open reverb, delay and master (drum SELECT was handled above). */
        if (row == 2) return pref_get(strip == 7 ? PR(R_ENG, T_FX, 0, FX_VOLUME) :
                                     smc_home_target(strip)) == 0;
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
