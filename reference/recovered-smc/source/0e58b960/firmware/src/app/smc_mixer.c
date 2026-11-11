/* SPDX-License-Identifier: GPL-3.0-only */
/* M-VAVE SMC-Mixer control surface. Included after ui.c in the firmware unity build.
 * DAW messages match Mackie Control; the legacy User setting is ignored.
 * TIMER5 only copies complete messages. All project, sequencer and display work is in the main loop. */
#include "smc_protocol.h"
#include "smc_fx_map.h"

#define SMC_Q 32u
extern int plat_ble_smc_send(const uint8_t *msg, uint8_t len);
extern int plat_ble_smc_send_slot(unsigned slot, const uint8_t *msg, uint8_t len);
#define SMC_PEERS 2u
/* Main-loop context only. Interrupt producers always address their explicit slot. */
static uint8_t smc_slot;
extern uint8_t smk_clip_mode_on(void);
extern int smk_ck_smc_turn(int n, int delta);
extern int smk_ck_smc_nav(uint8_t note);
extern int ui_pad_diag_active(void); /* pure main-loop getter, owned by the UI */
void smc_pad_diag_boundary(void);    /* UI calls after publishing entry/exit */
typedef struct {
    uint8_t n, b[6];
} smc_event_t;
typedef struct {
    smc_event_t q[SMC_Q];
    volatile uint8_t qw, qr;
    volatile uint32_t link_published;
    uint32_t link_seen;
    volatile uint8_t enqueued, dropped, dispatched, reason;
    volatile uint32_t release_lost[4];
    uint32_t release_seen[4];
} smc_input_t;
/* Each physical peer owns ring capacity, link epoch and lost-release epochs.
 * Hosted USB/peripheral input keeps its historical association with peer zero. */
static smc_input_t smc_inputs[SMC_PEERS];
#define smc_peer_input (smc_inputs[smc_slot])
#define smc_q (smc_peer_input.q)
#define smc_qw (smc_peer_input.qw)
#define smc_qr (smc_peer_input.qr)
#define smc_link_published (smc_peer_input.link_published)
#define smc_link_seen (smc_peer_input.link_seen)
#define smc_enqueued (smc_peer_input.enqueued)
#define smc_dropped (smc_peer_input.dropped)
#define smc_dispatched (smc_peer_input.dispatched)
#define smc_reason (smc_peer_input.reason)
#define smc_release_lost (smc_peer_input.release_lost)
#define smc_release_seen (smc_peer_input.release_seen)
uint32_t smc_rx_diag(void)
{
    return (uint32_t)smc_inputs[0].enqueued | (uint32_t)smc_inputs[0].dropped << 8 |
           (uint32_t)smc_inputs[0].dispatched << 16 | (uint32_t)smc_inputs[0].reason << 24;
}
static volatile uint8_t smc_host_w, smc_host_r;
static volatile uint8_t smc_mmc_w[2], smc_mmc_r[2];
/* Retain reader/generation compatibility for project and diagnostic cutoffs.
 * The retired hosted hooks publish no new User commands or STOP generations. */
static volatile uint32_t smc_mmc_stop_published[2];
static uint32_t smc_mmc_stop_seen[2];
/* Separate single-writer generation counters: TIMER5 (central), audio ISR
 * (host). Main-loop snapshots never clear a producer-owned flag. */
/* Slots 0/2 belong to TIMER5; 1/3 belong to the host audio ISR. Each
 * release class retains a separate full-width generation and acknowledgment. */
#define smc_ble_solo_release_lost smc_release_lost[0]
#define smc_host_solo_release_lost smc_release_lost[1]
#define smc_ble_solo_release_seen smc_release_seen[0]
#define smc_host_solo_release_seen smc_release_seen[1]
#define smc_pause_release_lost (smc_release_lost + 2)
#define smc_pause_release_seen (smc_release_seen + 2)
/* Full-ring key-ups use the existing conservative release-loss cutoff.
 * Include every displayed button so an overflow cannot retain its UI forever. */
static int smc_is_solo_release(uint8_t st, uint8_t note, uint8_t velocity)
{
    uint8_t type = st & 0xF0u;
    return !(note & 0x80u) && !(velocity & 0x80u) &&
           (note < 32u || (note >= 0x2Eu && note <= 0x31u) ||
            (note >= 0x5Bu && note <= 0x63u)) &&
           (type == 0x80u || (type == 0x90u && !velocity));
}
typedef struct {
    pref_t target[8];
    uint8_t value[8], last[8], seen, picked, pending;
    uint8_t up, fresh, solo, rec_part;
    uint8_t input_seen;                /* physical history survives target/pickup refresh */
    uint8_t solo_target[8];            /* physical S key: 1..5 parts, then 909/808 tracks; 0 released */
    uint16_t drum_solo[NKIT];
    uint8_t lit[128], known[128];
    uint8_t fx_restore[3];
    uint32_t knob_t;                   /* acceleration of the knob now turning, not a panel encoder */
    uint8_t knob_run, knob_i, play_down, select_down;
    uint64_t ui_held;                  /* main-loop button inventory for overlay lifetime only */
    uint8_t home_down, fx_down, fx_owned, fx_page, fx_ready, fx_effect[SMCFX_N];
    /* home_down: bit 0 held; bit 1 an unconsumed FX-context Back tap. */
    uint8_t fx_main_wait;              /* main SELECT: waiting for hold vs tap (bits 0..2) */
    uint32_t fx_main_t0[3];
    /* Exact destinations, not a copy of the temporary display or live UI. */
    pref_t lock_fader[8], lock_knob[8];
    uint32_t lock_song_revision, lock_t0;
    uint32_t beat_at, beat_period;     /* free-running audio sample clock, including STOP */
    float beat_internal;
    uint8_t beat_external;
    uint16_t lock_song_len;
    uint8_t locked, lock_down, lock_kit, lock_bank, lock_part, lock_view, lock_page;
} smc_state_t;
static smc_state_t smc_states[SMC_PEERS];
#define smc (smc_states[smc_slot])

int smc_project_active(void)
{
    for (unsigned i = 0; i < SMC_PEERS; i++)
        if (smc_states[i].solo || smc_states[i].drum_solo[0] || smc_states[i].drum_solo[1]) return 1;
    return 0;
}
static void smc_solo_union(void)
{
    uint8_t parts = 0;
    uint16_t drums[NKIT] = {0};
    for (unsigned i = 0; i < SMC_PEERS; i++) {
        parts |= smc_states[i].solo;
        for (int k = 0; k < NKIT; k++) drums[k] |= smc_states[i].drum_solo[k];
    }
    engine_smc_solo_targets(parts, drums[0], drums[1]);
}
static void smc_solo_clear(void)
{
    memset(smc.solo_target, 0, sizeof smc.solo_target);
    smc.solo = 0;
    smc.drum_solo[0] = smc.drum_solo[1] = 0;
    smc_solo_union();
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
    smc_solo_union();
}

static void smc_solo_press(int strip, int target, int mmc)
{
    if (mmc)                           /* no key-up: another press toggles, a new bank replaces this key's latch */
        smc.solo_target[strip] = smc.solo_target[strip] == target ? 0 : (uint8_t)target;
    else if (!smc.solo_target[strip])    /* repeats must not move a held key to a different bank or page */
        smc.solo_target[strip] = (uint8_t)target;
    smc_solo_publish();
}
X0X_COLD static void smc_peer_drop(void)
{
    smc_qr = smc_qw;
    smc_host_r = smc_host_w;
    smc_mmc_r[0] = smc_mmc_w[0];
    smc_mmc_r[1] = smc_mmc_w[1];
    smc_mmc_stop_seen[0] = smc_mmc_stop_published[0];
    smc_mmc_stop_seen[1] = smc_mmc_stop_published[1];
    /* A reset has already cleared all held solos; do not carry stale
     * overflow releases into the new project's queued presses. */
    for (int i = 0; i < 4; i++) smc_release_seen[i] = smc_release_lost[i];
}
X0X_COLD static void smc_project_drop(void)
{
    uint8_t saved = smc_slot;
    for (smc_slot = 0; smc_slot < SMC_PEERS; smc_slot++) smc_peer_drop();
    smc_slot = saved;
}
static void smc_fx_load_page(int page);
static void smc_fx_clear(void);
X0X_COLD void smc_project_reset(void)
{
    uint8_t saved = smc_slot, up[SMC_PEERS];
    smc_ui_cancel();
    smc_project_drop();
    for (unsigned i = 0; i < SMC_PEERS; i++) up[i] = smc_states[i].up;
    /* Clear every owner before the first FX publication. Otherwise initializing
     * peer zero can transiently re-arm peer one's old-project effect. */
    memset(smc_states, 0, sizeof smc_states);
    for (smc_slot = 0; smc_slot < SMC_PEERS; smc_slot++) {
        smc.up = smc.fresh = up[smc_slot];
        smc.pending = 0xFFu;
        smc_fx_load_page(0);
    }
    smc_slot = saved;
    engine_smc_solo(0);
}

void smc_ble_midi_slot(unsigned slot, const uint8_t *msg, uint8_t len)
{
    if (slot >= SMC_PEERS) return;
    smc_input_t *in = &smc_inputs[slot];
    uint8_t w = in->qw, i;
    if (!msg || !len || len > 6u) { in->dropped++; return; }
    if ((uint8_t)(w - in->qr) >= SMC_Q) {
        if (len == 3u && smc_is_solo_release(msg[0], msg[1], msg[2]) &&
            !(msg[0] & 15u)) {
            in->release_lost[0]++;
            if (msg[1] == 0x5Du) in->release_lost[2]++;
        }
        in->dropped++; return;
    }
    in->q[w % SMC_Q].n = len;
    for (i = 0; i < len; i++) in->q[w % SMC_Q].b[i] = msg[i];
    __asm__ volatile("" ::: "memory");
    in->qw = (uint8_t)(w + 1u);
    in->enqueued++;
}
void smc_ble_midi(const uint8_t *msg, uint8_t len) { smc_ble_midi_slot(0, msg, len); }

void smc_ble_link_slot(unsigned slot, int up)
{
    if (slot >= SMC_PEERS) return;
    smc_input_t *in = &smc_inputs[slot];
    uint32_t next = (in->link_published + 0x200u) & ~0x1FFu;
    in->link_published = next | ((uint32_t)in->qw << 1) | (uint32_t)(up != 0);
}
void smc_ble_link(int up) { smc_ble_link_slot(0, up); }
int smc_ble_role_get(void) { return !!(proj.set.bt & PROJ_BT_SMC_CENTRAL); }
int smc_connected(void) { return (smc_states[0].up || smc_states[1].up) && smc_ble_role_get(); }

/* Hosted USB/peripheral BLE has no SMC identity. Keep the existing hook ABI,
 * but leave every packet to its ordinary MIDI/APC route, including legacy User. */
int smc_midi_host(uint32_t pkt)
{
    (void)pkt;
    return 0;
}

/* Retired User-preset MMC hook; it must not queue remote transport actions. */
void smc_mmc_host(uint8_t cmd, uint8_t src)
{
    (void)cmd;
    (void)src;
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
    pref_t r = targets[n];
    if (n < NPARTS) r.b = (uint8_t)part_at(n);
    return r;
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
    if (smc.locked) return (int)smc.lock_kit - 1;
    if (ui.overlay != O_NONE || smk_clip_mode_on()) return -1;
    /* SEQ follows its selected drum part; MIX follows the kit actually shown. */
    return ui.view == V_SEQ && is_drum() ? ui.part : drum_mix_page();
}

static int smc_drum_track(int kit, int strip)
{
    int track = (smc.locked ? smc.lock_bank : (ui.sel[kit] / 8) * 8) + strip;
    return track < NDRUM ? track : -1;
}

/* Row actions share the fader's captured strip identity while locked. Local
 * navigation or a later presentation-order edit must not redirect REC/Solo/Mute/Select. */
static int smc_strip_part(int strip)
{
    if (!smc.locked) return part_at(strip);
    pref_t r = smc.lock_fader[strip];
    return r.kind == R_ENG && r.a == T_MIX && r.b < NPARTS && r.c == MX_LEVEL ? r.b : -1;
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
    if (smc.locked) return smc.lock_fader[n];
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
    if (smc.locked) {
        r = smc.lock_knob[n];
        /* A song edit may remove the captured bar. Never alias the last bar or
         * silently recreate it on a later turn. Unlock to select a new target. */
        return (r.kind == R_SPAT || r.kind == R_SREP) &&
               (smc.lock_song_revision != song_structure_revision || smc.lock_song_len != song()->len ||
                (r.b && r.b - 1u >= song()->len)) ? NONE : r;
    }
    if (kit >= 0) {
        int track = smc_drum_track(kit, n);
        return track >= 0 ? find_ref(kit ? T_808 : T_909, track, "Pan") : NONE;
    }
    if (ui.overlay == O_NONE && !smk_clip_mode_on() &&
        (ui.view == V_HOME || (ui.view == V_MIX && cur_page() == 0))) {
        if (n < NPARTS) return PR(R_ENG, T_MIX, part_at(n), MX_CUT);
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

/* b is unused by these normal refs. Binding includes the pattern number:
 * changing a playing pattern while locked must not redirect an editor knob. */
static pref_t smc_bind(pref_t r)
{
    int part;
    if (in_pattern(r)) {
        part = r.kind == R_SWING ? PART_909 : r.kind == R_DLEN || r.kind == R_DRATE ? r.a :
               r.kind >= R_BLEN && r.kind <= R_BTRANS ? NKIT + r.a : ui.part;
        r.b = (uint8_t)(seq.ppat[part] + 1u);
    } else if (r.kind == R_SPAT || r.kind == R_SREP) {
        /* An empty song's apparent pattern is a fallback, not an existing bar. */
        if (ui.song_sel >= song()->len) return NONE;
        r.b = (uint8_t)(ui.song_sel + 1u);
    } else if (r.kind == R_SBAR) return NONE; /* changing selection is navigation */
    return r;
}

/* Captures destinations, without opening a page or retaining a display overlay.
 * Physical Pause input is owner-captured Note 0x5D; MMC STOP stays separate. */
int smc_lock_toggle(void)
{
    if (!smc.locked) {
        if (ui_project_busy() || perf_testing() || ui.overlay == O_ASK || smk_clip_mode_on()) {
            ui_say("SMC LOCK UNAVAILABLE", 0);
            return 0;                  /* clip stages have a separate field owner */
        }
        smc.lock_kit = (uint8_t)(smc_drum_mix() + 1);
        smc.lock_bank = smc.lock_kit ? (uint8_t)(ui.sel[smc.lock_kit - 1u] / 8u * 8u) : 0;
        smc.lock_part = ui.part; smc.lock_view = ui.view;
        smc.lock_page = (uint8_t)cur_page();
        smc.lock_song_revision = song_structure_revision; smc.lock_song_len = song()->len;
        for (int i = 0; i < 8; i++) {
            smc.lock_fader[i] = smc_target(i); /* levels/returns/filter are engine refs, never pattern refs */
            smc.lock_knob[i] = smc_bind(smc_knob_target(i));
        }
    }
    smc_fx_clear(); /* a routing-epoch change ends owned FX before key-up can use a new mapping */
    smc.locked ^= 1u;
    if (smc.lock_down) smc.lock_down = 1; /* consume a hold, including a programmatic toggle */
    /* Rearm acceptance for this routing epoch, keeping the last physical
     * sample so the next move can cross the real target in either direction. */
    smc.picked = 0;
    smc.knob_i = 0; smc.pending = 0xFFu;
    smc_ui_cancel();
    ui_say(smc.locked ? "SMC TARGETS LOCKED" : "SMC FOLLOWS PAGE", 0);
    return 1;
}

static int smc_lock_ready(void)
{
    return smc.lock_down == 2 && !ui_project_busy() && !perf_testing() && ui.overlay != O_ASK &&
        smc_pause_release_lost[0] == smc_pause_release_seen[0] &&
        smc_pause_release_lost[1] == smc_pause_release_seen[1] &&
        (uint32_t)(plat_ms() - smc.lock_t0) >= 900u;
}

/* A press begun while following captures immediately. Only a NEW press begun
 * locked can arm the 900 ms unlock. Repeats never rearm or extend that hold. */
int smc_lock_button(int pressed)
{
    if (!pressed) {
        /* The first poll after the deadline can contain the actual key-up. */
        int changed = smc_lock_ready() ? smc_lock_toggle() : 0;
        smc.lock_down = 0;
        return changed;
    } else if (!smc.lock_down) {
        smc.lock_down = 1;
        if (!smc.locked) return smc_lock_toggle();
        smc.lock_down = 2;
        smc.lock_t0 = plat_ms();
    }
    return 0;
}

static void smc_lock_tick(void)
{
    /* Drain input/loss first. A producer may have published a newer release
     * during this poll; defer rather than unlock before its cancellation. */
    if (smc_qr == smc_qw && (smc_slot || smc_host_r == smc_host_w) && smc_lock_ready())
        smc_lock_toggle();
}

/* Follow the sequencer's accepted 24-ppqn clock, not wall time or transport.
 * Its sample counter runs while stopped. Preserve the last period/anchor when
 * clock loss resets the external beat ring; a later pickup rephases it. */
static int smc_beat_lit(void)
{
    const volatile seq_t *clock = &seq;
    uint32_t now, since, tick_period, period = smc.beat_period;
    uint8_t q;
    int external;
    float internal;
    /* The audio ISR owns these fields. Retry if it advanced during the read,
     * rather than combine a new tick count with an older elapsed timestamp. */
    do {
        now = clock->ext_now;
        since = clock->ext_since; tick_period = clock->ext_period; q = clock->ext_q;
        external = clock->ext && !clock->clk_lost; internal = clock->bpm;
    } while (now != clock->ext_now);
    if (!period || (!external && !smc.beat_external && smc.beat_internal != internal)) {
        float bpm = internal;
        if (!(bpm >= 20.0f && bpm <= 300.0f)) bpm = 125.0f;
        period = (uint32_t)((float)FS * 60.0f / bpm);
        smc.beat_at = now;
    }
    if (external) {
        if (tick_period) period = tick_period / 32u * 3u; /* Q8 tick -> beat */
        if (period < FS / 5u) period = FS / 5u;
        if (period > FS * 3u) period = FS * 3u;
        /* ext_q counts the tick just consumed: tick one is the lit downbeat. */
        uint32_t phase = (uint32_t)((q + 23u) % 24u) * period / 24u + since;
        smc.beat_at = now - phase % period;
    }
    smc.beat_period = period;
    smc.beat_internal = internal; /* loss may round internal BPM; keep the exact visual period */
    smc.beat_external = (uint8_t)external;
    uint32_t phase = (now - smc.beat_at) % period;
    smc.beat_at = now - phase; /* keep subtraction bounded across sample-counter wrap */
    return phase < period / 2u;
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
struct smc_ui_state {
    uint32_t until, context;
    uint8_t on, kind, strip, action, peer;
    char name[24], value[24];
};
/* Byte buffer (not a naked struct): IPA-SRA must not split fields into
 * separate BSS symbols — each split field costs a 6-byte abs load on pi32.
 * Same XIP fit technique as the Stage-2 peek repair; applied to the current
 * temporary full-control screen that replaced mapped-page peek on nightly. */
static uint8_t smc_ui_raw[sizeof(struct smc_ui_state)];
#define smc_ui (*((struct smc_ui_state *)(void *)smc_ui_raw))

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
    if (smc_ui.on && smc_ui.peer < SMC_PEERS && smc_states[smc_ui.peer].ui_held)
        smc_ui.until = plat_ms() + SMC_UI_MS;
    if (smc_ui.on && (ui_pad_diag_active() || ui_project_busy() || ui.help || ui.overlay == O_ASK ||
        smc_ui.context != smc_ui_context() ||
        (int32_t)(plat_ms() - smc_ui.until) >= 0))
        smc_ui_cancel();
    return smc_ui.on;
}

static void smc_ui_begin(int n, int kind)
{
    if (ui_pad_diag_active() || ui.help || ui.overlay == O_ASK || ui_project_busy()) {
        smc_ui_cancel();
        return;                         /* Help keeps its card; confirmation keeps ownership. */
    }
    smc_ui.on = 1;
    smc_ui.peer = smc_slot;
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
        name = r.c == MX_LEVEL ? "LEVEL" : r.c == MX_PAN ? "PAN" : r.c == MX_CUT ? "CUTOFF" : pref_name(r);
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
static void smc_fx_publish(void);
static void smc_fx_main_tick(void);
static void smc_fx_clear(void);
static void smc_fx_main_nav(int which);
static int smc_fx_bus_now(uint8_t note);
#define SMC_FX_HOLD_MS 180u
static int smc_lock_blocks(uint8_t note);
/* Shared navigation descriptors: input dispatch and screen captions read these. */
static const uint8_t SMC_VIEW[4][3] = {
    {V_FX, V_FX, V_HOME}, {V_MIX, V_MIX, V_PART},
    {0, 0, 0}, {V_FX, V_FX, V_MIX}
};
static const int8_t SMC_PAGE[4][3] = {
    {0, 1, 0}, {-1, -1, 0}, {0, 0, 0}, {0, 1, 3}
};
static int smc_nav_key(int note)
{
    return note == 5 ? (smk_clip_mode_on() && ui.overlay == O_NONE ? B_GLO : B_SEL) :
           note == 6 ? B_HOME : note == 14 ? B_GLO : -1;
}
static const char *smc_button_caption(int note)
{
    int kit = smc_drum_mix(), strip = note & 7, row = note >> 3;
    if (kit >= 0 && smc_drum_track(kit, strip) < 0) return "--";
    if (smc_lock_blocks((uint8_t)note)) return "X";
    if (row == 2) return "M";
    if (row == 3 && kit >= 0) return "SEL";
    if (row == 3 && kit < 0) {
        int bus = smc_fx_bus_now((uint8_t)note);
        if (bus >= 0) {
            if (!smc.fx_ready)
                smc_fx_publish();
            if (smc.fx_effect[bus] < SMCFX_NFX)
                return SMCFX_SHORT[smc.fx_effect[bus]];
        }
        return strip < NPARTS ? "SEL" : "SEL";
    }
    if (kit >= 0 || strip < NPARTS) return row ? "S" : "R";
    int key = smc_nav_key(note);
    if (key == B_SEL) return ui.overlay == O_LIST ? "OK" : "LIST";
    if (key == B_GLO) return ui.view == V_GLO ? "BACK" : "GLOB";
    if (key == B_HOME) return smk_clip_mode_on() && ui.overlay == O_NONE ? "GRID" :
        ui.overlay != O_NONE || ui.view == V_GLO ? "BACK" : ui.view == V_HOME ? "SONG" : "HOME";
    int view = SMC_VIEW[row][strip - NPARTS];
    return view == V_HOME ? "HOME" : view == V_MIX ? "MIX" : view == V_PART ? "PART" : "FX";
}


/* The encoder and fader use the same bounded native value rendering. Keeping
 * one outline avoids duplicating formatting and width trimming in every grid. */
static UI_DRAW void smc_ui_strip_value(pref_t r, int x, int y, uint16_t color)
{
    char value[32], unit[12];
    if (r.kind == R_NONE) put_s(value, "--");
    else pref_value_of(r, pref_get(r), value, unit);
    char *end = value; while (*end) end++;
    while (tw(&FONT_T, value) > 27 && end > value) *--end = 0;
    text_c(x, y, &FONT_T, value, color);
}

/* Eight physical strips, left to right: encoder above, fader left of M/S/R.
 * Keep this geometry even when a target is inactive; position is the user's map. */
static UI_DRAW void smc_ui_grid(void)
{
    static const uint8_t NOTES[3] = {16, 8, 0};
    /* Strips 0..4 = part_s (PART_N / BRK); 5..7 = send/master FX. */
    static const char *const MAIN_FX[3] = {"RVB", "DLY", "FLT"};
    int kit = smc_drum_mix();
    /* Selection follows the keyboard UI, independently of the touched control.
     * A locked bank can differ: no lane is selected if it is outside this bank.
     * LOCK in the header describes captured targets, never the title highlight. */
    int selected = kit >= 0 ? ui.sel[kit] - smc_drum_track(kit, 0) :
        ui.view == V_FX ? 5 + cur_page() :
        ui.view == V_MIX ? (cur_page() == 3 ? 7 : part_position(ui.mixsel)) : part_position(ui.part);
    for (int n = 0; n < 8; n++) {
        pref_t f = smc_target(n), k = smc_knob_target(n);
        char name[24];
        int x = n * 30, active = smc_ui.kind < 2 && n == smc_ui.strip;
        uint16_t fc = active && smc_ui.kind == 0 ? C_HI : C_GRAY;
        uint16_t kc = active && smc_ui.kind == 1 ? C_HI : C_GRAY;
        if (n) cv_rect(x, 0, 1, 138, C_LINE);
        if (kit >= 0) {
            int track = smc_drum_track(kit, n);
            if (track < 0) put_s(name, "--");
            else if ((smc.locked ? smc.lock_view : ui.view) == V_SEQ)
                put_s(name, engine_voice_name(kit ? T_808 : T_909, track));
            else put_i(name, track + 1);
        } else put_s(name, n < NPARTS ? part_s(part_at(n)) : MAIN_FX[n - NPARTS]);
        if (n == selected) cv_rect(x + 1, 0, 28, 13, C_WHITE);
        text_c(x + 15, 1, &FONT_T, name, n == selected ? C_BLACK : C_WHITE);
        if (k.kind == R_NONE) put_s(name, "--");
        else {
            const char *label = pref_name(k);
            if (k.kind == R_ENG && k.a == T_MIX && k.c == MX_PAN) label = "PAN";
            if (k.kind == R_ENG && k.a == T_MIX && k.c == MX_CUT) label = "CUT";
            if (k.kind == R_ENG && k.a == T_FX && k.c == FX_RV_DECAY) label = "DECAY";
            if (k.kind == R_ENG && k.a == T_FX && k.c == FX_DL_FDBK) label = "FDBK";
            if (k.kind == R_ENG && k.a == T_MST && k.c == MST_RESO) label = "RESO";
            int i = 0; while (label[i] && i < 5) { name[i] = label[i]; i++; } name[i] = 0;
            while (i && tw(&FONT_T, name) > 27) name[--i] = 0;
        }
        text_c(x + 15, 14, &FONT_T, caps_of(name), k.kind == R_NONE ? C_DIM : kc);
        /* Same authoritative value/arc renderer as the keyboard knob strip. */
        if (k.kind != R_NONE) {
            int bipolar = k.kind == R_BTRANS || is_pan(k) ||
                (k.kind == R_ENG && k.a == T_MST && k.c == MST_DJF);
            if (bipolar) cv_rect(x + 14, 25, 2, 3, C_WHITE);
            arc(x + 15, 36, 8, (float)smc_scaled(k) / 127.0f, C_LINE, kc, bipolar);
        } else text_c(x + 15, 30, &FONT_T, "--", C_DIM);
        smc_ui_strip_value(k, x + 15, 46, k.kind == R_NONE ? C_DIM : kc);
        cv_rect(x + 3, 64, 2, 51, f.kind == R_NONE ? C_LINE : C_DIM);
        if (f.kind != R_NONE) {
            int target = 111 - smc_scaled(f) * 45 / 127;
            cv_rect(x + 1, target, 6, 4, fc);
            /* A fine white marker is physical position until pickup is acquired. */
            if (!(smc.picked & (1u << n)))
                cv_rect(x + 1, 111 - smc.last[n] * 45 / 127, 6, 1, C_WHITE);
        }
        for (unsigned row = 0; row < sizeof NOTES / sizeof NOTES[0]; row++) {
            int y = 62 + row * 12;
            uint16_t c = smc_led(NOTES[row] + n) ? C_HI : f.kind == R_NONE ? C_DIM : C_GRAY;
            const char *caption = smc_button_caption(NOTES[row] + n);
            if (caption[0] == 'X' || caption[0] == '-') c = C_DIM;
            cv_rect(x + 8, y, 22, 11, C_LINE);
            text_c(x + 19, y + 1, &FONT_T, caption, c);
        }
        smc_ui_strip_value(f, x + 15, 122, fc);
    }
}

static void smc_ui_draw_peer(void)
{
    char name[24], num[32], unit[12], tag[80];
    const char *status = "ACTIVE";
    pref_t r = smc_ui.kind == 1 ? smc_knob_target(smc_ui.strip) : smc_target(smc_ui.strip);
    int kit = smc_drum_mix();
    cv_begin(240, HDR_H, C_BLACK);
    if (kit >= 0)
        put_i(put_s(put_s(put_s(tag, PART_N[kit]), (smc.locked ? smc.lock_view : ui.view) == V_SEQ ? " SEQ" : " MIX"), smc.locked ? " LOCK B" : " BANK "), (smc.locked ? smc.lock_bank / 8 : ui.sel[kit] / 8) + 1);
    else if (smc.locked)
        put_i(put_s(put_s(put_s(tag, "LOCK "), smc.lock_view == V_PART ? PART_N[smc.lock_part] : VIEW_N[smc.lock_view]), " P"), smc.lock_page + 1);
    else if (smk_clip_mode_on()) put_s(tag, "CLIP KEYS");
    else if (ui.overlay == O_LIST) put_s(tag, list_title);
    else if (ui.view == V_HOME) put_s(tag, "HOME / PARTS");
    else knob_tag(tag);
    { char *end = tag;
      while (*end) end++;
      while (tw(&FONT_XS, tag) > 165) *--end = 0;
    }
    cv_text(5, vc(&FONT_XS, 0, HDR_H), &FONT_XS, caps_of(tag), C_GRAY);
    put_i(put_s(tag, smc_ui.kind == 3 ? "KEY " : smc_ui.kind == 1 || smc_ui.kind == 2 ? "KNOB " : "FADER "), smc_ui.strip + 1);
    text_r(235, vc(&FONT_S, 0, HDR_H), &FONT_S, tag, C_HI);
    cv_commit(0, 0, 0);
    cv_begin(240, 2 * BAND_H, C_BLACK);
    cv_band(0, 2 * BAND_H);
    smc_ui_grid();
    cv_commit_rect(1, 0, MAIN_Y);
    cv_begin(240, KNOB_H, C_BLACK);
    cv_rect(0, 0, 240, 1, C_LINE);
    if (smc_ui.kind == 2) {
        put_s(name, smc_ui.name);
        put_s(num, smc_ui.value); unit[0] = 0;
    } else if (smc_ui.kind == 3) {
        int note = smc_ui.action;
        int bus = smc_fx_bus_now((uint8_t)note);
        if (bus >= 0) {
            smc_fx_publish();
            put_s(name, SMCFX_BUS_NAME[bus]);
            put_s(num, SMCFX_LONG[smc.fx_effect[bus]]);
            status = (smc.fx_down & (uint8_t)(1u << bus)) ? "HOLD" : SMCFX_PAGE_NAME[smc.fx_page];
        } else {
            const char *action = note >= 0x5D ? "TRANSPORT" : note < 8 ? "RECORD" : note < 16 ? "SOLO" : "MUTE";
            if (note < 24) {
                if (kit < 0 && smc_ui.strip == 7) r = PR(R_ENG, T_FX, 0, FX_VOLUME);
                smc_ui_label(r, name);
            } else put_s(name, "TRANSPORT");
            put_s(put_s(put_s(num, action), " "), note >= 0x5D ?
                  (note == 0x5F ? ui.rec ? "REC ON" : "REC OFF" : seq.playing ? "PLAYING" : "STOPPED") :
                  (note < 8 ? ui.rec : smc_led(note)) ? "ON" : "OFF");
        }
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
    text_c(120, 4, tw(&FONT_S, name) > 232 ? &FONT_XS : &FONT_S, caps_of(name), C_WHITE);
    { const felucca_font_t *font = &FONT_M;
      int uw = unit[0] ? tw(&FONT_XS, unit) + 2 : 0;
      if (tw(font, num) + uw > 228) font = &FONT_B;
      int x = 120 - (tw(font, num) + uw) / 2;
      x = cv_text(x, 22, font, num, C_HI);
      if (unit[0]) cv_text(x + 2, base_y(font, 22, &FONT_XS), &FONT_XS, unit, C_GRAY);
    }
    text_c(120, 49, &FONT_XS, status, C_HI);
    cv_commit(3, 0, KNOB_Y);
}
void smc_ui_draw(void)
{
    uint8_t saved = smc_slot;
    smc_slot = smc_ui.peer < SMC_PEERS ? smc_ui.peer : 0;
    smc_ui_draw_peer();
    smc_slot = saved;
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
     * separate from acceptance: target edits must retain crossing history. */
    moved = !(smc.input_seen & bit) || smc.last[n] != v;
    smc.input_seen |= bit;
    smc_notice(n);
    r = smc.target[n];
    d = pref_desc(r);
    if (!d || !d->max || r.kind == R_ACT || r.kind == R_HDR || r.kind == R_NONE) {
        smc.last[n] = v;
        smc.seen |= bit;
        return;
    }
    old = smc.value[n];
    crossed = smc_pickup_reached(smc.last[n], smc.seen & bit, old, v);
    smc.last[n] = v;
    smc.seen |= bit;
    if (!(smc.picked & bit)) {
        if (!moved || !crossed)
            goto show;
        smc.picked |= bit;
    } else crossed = 0;                /* acquisition must publish even an equal saved value */
    nv = (v * d->max + 63) / 127;
    if (crossed || nv != pref_get(r)) {
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
    if (!smc.locked && ui.overlay == O_NONE && smk_ck_smc_turn(n, delta))
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
    /* The panel curve (up to x8, x16 on a wide range) is too far once the
     * relative byte has already counted the detents. A slow click stays one. */
    accel_smc = 1;
    turn_ref(r, delta, EN_K1);
    accel_smc = 0;
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

static int smc_lock_blocks(uint8_t note)
{
    if (!smc.locked || note >= 32u) return 0;
    /* Lock retains destinations, not button navigation. Keep unavailable
     * captured strips parked; the existing dispatcher owns all valid actions. */
    if (smc.lock_kit)
        return smc_drum_track((int)smc.lock_kit - 1, note & 7u) < 0;
    return (note & 7u) < NPARTS && smc_strip_part(note & 7u) < 0;
}

static void smc_fx_publish(void)
{
    if (!smc.fx_ready) {
        int i;
        for (i = 0; i < SMCFX_N; i++)
            smc.fx_effect[i] = SMCFX_PAGE[0][i];
        smc.fx_page = 0;
        smc.fx_ready = 1;
    }
    /* The engine has one effect per bus. Peer zero wins a simultaneous
     * conflict; releasing it reveals peer one's still-held effect. */
    uint8_t down = 0, effects[SMCFX_N];
    for (int bus = 0; bus < SMCFX_N; bus++) {
        effects[bus] = SMCFX_PAGE[0][bus];
        for (unsigned peer = 0; peer < SMC_PEERS; peer++)
            if (smc_states[peer].fx_down & (1u << bus)) {
                down |= (uint8_t)(1u << bus);
                effects[bus] = smc_states[peer].fx_effect[bus];
                break;
            }
    }
    engine_smc_busfx(down, effects);
}

static void smc_fx_load_page(int page)
{
    int i, announce = smc.fx_ready;
    if (page < 0)
        page = SMCFX_NPAGE - 1;
    if (page >= SMCFX_NPAGE)
        page = 0;
    smc.fx_page = (uint8_t)page;
    for (i = 0; i < SMCFX_N; i++)
        smc.fx_effect[i] = SMCFX_PAGE[page][i];
    smc.fx_ready = 1;
    smc_fx_publish();
    if (announce)
        say(SMCFX_PAGE_NAME[page], "FX PAGE");
}

/* Translate physical strips only; effect IDs and held bus ownership stay stable. */
static int smc_fx_bus(uint8_t note)
{
    int bus = smcfx_bus(note);
    return bus >= 0 && bus < NPARTS ? smc_strip_part(bus) : bus;
}

static int smc_fx_bus_now(uint8_t note)
{
    if (ui.view != V_FX || ui.overlay != O_NONE || smk_clip_mode_on() || smc_drum_mix() >= 0)
        return -1;
    return smc_fx_bus(note);
}

static int smc_home_held(void)
{
    return smc.home_down || ((plat_buttons() >> B_HOME) & 1);
}

static void smc_fx_reset_cuts(void)
{
    int p;
    for (p = 0; p < NPARTS; p++) {
        uint8_t stored = proj.sound.v[T_MIX][NPARTS][p];
        pref_set(PR(R_ENG, T_MIX, p, MX_CUT), stored ? (int)stored - 1 : 127);
    }
    say("CUTOFF RESET", 0);
}

static void smc_fx_save_preset(void)
{
    int p;
    for (p = 0; p < NPARTS; p++)
        proj.sound.v[T_MIX][NPARTS][p] = (uint8_t)(pref_get(PR(R_ENG, T_MIX, p, MX_CUT)) + 1);
    mark_dirty();
    say("CUTOFF SAVED", 0);
}

static void smc_fx_show(uint8_t note)
{
    smc_ui.action = note;
    smc_ui_begin(note & 7, 3);
}

static void smc_fx_main_nav(int which)
{
    if (which < 0 || which > 2)
        return;
    smc_view(SMC_VIEW[3][which], SMC_PAGE[3][which]);
}

static void smc_fx_main_tick(void)
{
    int i;
    if (smc_fx_bus_now(24u) < 0) {
        smc_fx_clear();                 /* keyboard navigation cancels the original FX gesture */
        return;
    }
    for (i = 0; i < 3; i++) {
        uint8_t note = (uint8_t)(SMCFX_NOTE[SMCFX_RVB + i]);
        uint8_t sel = (uint8_t)(1u << (note - 24u));
        if (!(smc.fx_main_wait & (uint8_t)(1u << i)))
            continue;
        if (!(smc.select_down & sel)) {
            smc.fx_main_wait &= (uint8_t)~(1u << i);
            continue;
        }
        if ((uint32_t)(plat_ms() - smc.fx_main_t0[i]) >= SMC_FX_HOLD_MS &&
            !(smc.fx_down & (uint8_t)(1u << (SMCFX_RVB + i)))) {
            smc.fx_down |= (uint8_t)(1u << (SMCFX_RVB + i));
            smc_fx_show(note); /* main-bus holds need the same assignment/value feedback */
        }
    }
    if (smc.fx_down)
        smc_fx_publish();
}

/* 1 = this note was an FX gesture (do not also run its old action). */
static int smc_fx_try(uint8_t note, int mmc)
{
    int bus;
    smc_fx_publish();
    if (smc_home_held() && ui.overlay == O_NONE && !smk_clip_mode_on() &&
        note >= 0x60 && note <= 0x63) {
        smc.home_down &= 1u; /* consume the deferred Back tap */
        if (note >= 0x62)
            smc_fx_load_page((int)smc.fx_page + ((note & 1) ? 1 : -1));
        else if (note == 0x60)
            smc_fx_save_preset();
        else
            smc_fx_reset_cuts();
        return 1;
    }
    bus = smc_fx_bus_now(note);
    if (bus < 0)
        return 0;
    if (smc_home_held()) {
        smc.home_down &= 1u;
        smc.fx_effect[bus] = smcfx_cycle(bus, smc.fx_effect[bus]);
        smc_fx_publish();
        say(SMCFX_LONG[smc.fx_effect[bus]], SMCFX_BUS_NAME[bus]);
        smc_fx_show(note);
        return 1;                         /* assign only; the effect stays as it was */
    }
    if (!mmc) smc.fx_owned |= (uint8_t)(1u << bus);
    if (smcfx_is_main(bus)) {
        int which = bus - SMCFX_RVB;
        smc.fx_main_t0[which] = plat_ms();
        smc.fx_main_wait |= (uint8_t)(1u << which);
        return 1;                         /* tap navigates on release; hold arms momentary FX */
    }
    if (mmc)
        smc.fx_down ^= (uint8_t)(1u << bus);
    else
        smc.fx_down |= (uint8_t)(1u << bus);
    smc_fx_publish();
    say(SMCFX_LONG[smc.fx_effect[bus]], SMCFX_BUS_NAME[bus]);
    smc_fx_show(note);
    return 1;
}

static void smc_fx_release_note(uint8_t note)
{
    int bus = smc_fx_bus(note);
    if (bus < 0 || !(smc.fx_owned & (uint8_t)(1u << bus)))
        return;                         /* a non-FX press cannot acquire a release action later */
    smc.fx_owned &= (uint8_t)~(1u << bus);
    if (smcfx_is_main(bus)) {
        int which = bus - SMCFX_RVB;
        uint32_t dt = plat_ms() - smc.fx_main_t0[which];
        smc.fx_main_wait &= (uint8_t)~(1u << which);
        smc.fx_down &= (uint8_t)~(1u << bus);
        smc_fx_publish();
        if (smc_fx_bus_now(note) >= 0 && !smc_home_held() && dt < SMC_FX_HOLD_MS)
            smc_fx_main_nav(which);
        return;
    }
    smc.fx_down &= (uint8_t)~(1u << bus);
    smc_fx_publish();
}

static void smc_fx_clear(void)
{
    smc.home_down &= 1u; /* context/ownership cancellation revokes a deferred Back tap */
    if (!smc.fx_down && !smc.fx_main_wait && !smc.fx_owned)
        return;
    smc.fx_owned = 0;
    smc.fx_down = 0;
    smc.fx_main_wait = 0;
    smc_fx_publish();
}

/* All four drum SEQ/MIX rows follow the visible fader/knob bank. Outside those
 * pages the strips retain their whole-part, FX and navigation functions. */
__attribute__((noinline))
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
    /* Lock fixes destinations; valid button navigation and transport remain live. */
    if (smc_lock_blocks(note)) {
        ui_say("SMC TARGETS LOCKED", 0);
        return;
    }
    if (smc_fx_try(note, mmc))
        return;
    if (!smc.locked && ui.overlay == O_NONE && smk_ck_smc_nav(note))
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
                /* A different sound still selects it. Press its SELECT again
                 * on a drum mixer to return to the main parts mixer. */
                if (ui.view == V_MIX && ui.part == kit && ui.sel[kit] == track)
                    smc_view(V_MIX, 0);
                else smc_select_drum(kit, track);
                break;
            }
            return;
        }
    }
    int nav_key = smc_nav_key(note);
    if (nav_key >= 0) {
        button_tap(nav_key);
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
            int part = smc_strip_part(strip);
            if (part < 0) return;
            if (row == 0) {             /* REC */
                smc_select_part(part);
                smc_record_toggle();
            } else if (row == 1) {      /* SOLO */
                smc_solo_press(strip, part + 1, mmc);
                say((smc.solo & (1u << part)) ? "SOLO" : "UNSOLO", PART_N[part]);
            } else if (row == 2)       /* MUTE */
                mute_part(part);
            else {                     /* SELECT */
                int repeat_drum = part < NKIT && ui.part == part && ui.overlay == O_NONE;
                smc_select_part(part);
                smc_view(repeat_drum ? V_MIX : V_PART, repeat_drum ? part + 1 : -1);
            }
        } else {
            int which = strip - NPARTS;
            if (row == 2)
                smc_toggle_fx(which);
            else
                smc_view(SMC_VIEW[row][which], SMC_PAGE[row][which]);
        }
        return;
    }
    if (smc_bank(note)) return;
    switch (note) {
    case 0x2E: case 0x30:             /* bank / channel left: part */
        if (part_position(ui.part) > 0) smc_select_part(part_neighbor(ui.part, -1));
        break;
    case 0x2F: case 0x31:
        if (part_position(ui.part) + 1 < NPARTS) smc_select_part(part_neighbor(ui.part, 1));
        break;
    case 0x5B: case 0x5C: {            /* rewind / forward: pattern */
        int part = smc.locked ? smc.lock_part : ui.part;
        int next = seq.ppat[part] + (note == 0x5B ? -1 : 1);
        if (next >= 0 && next < NPAT)
            seq_cue_part(&seq, part, next);
        break;
    }
    case 0x5D: seq_stop(&seq); break;
    case 0x5E:
        /* Match the keyboard's PLAY toggle, including a queued start before
         * the audio loop sees it. STOP retains its separate existing role. */
        if ((seq.playing || seq.req_start || seq.req_cont) && !seq.req_stop)
            seq_stop(&seq);
        else {
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

/* Dense inventory for the existing 32 strip and 13 navigation/transport keys.
 * It owns display lifetime only: releases still follow their gesture owners. */
static uint64_t smc_button_bit(uint8_t note)
{
    int bit = note < 32u ? note : note >= 0x2Eu && note <= 0x31u ? 32 + note - 0x2E :
              note >= 0x5Bu && note <= 0x63u ? 36 + note - 0x5B : -1;
    return bit < 0 ? 0 : (uint64_t)1u << bit;
}

static void smc_button(uint8_t note, int mmc)
{
    if (!mmc) smc.ui_held |= smc_button_bit(note);
    if (!mmc && note == 0x5Du) {
        smc_lock_button(1);
        return;                         /* no transport action card for target lock */
    }
    if (!mmc && note == 6) {
        if (smc.home_down) return;
        smc.home_down = 1;
        if (smc_fx_bus_now(24u) >= 0) {
            smc.home_down = 3; /* FX modifier first; a lone Back tap runs on release */
            return;
        }
    }
    if (!mmc && note >= 24u && note < 32u) {
        uint8_t bit = (uint8_t)(1u << (note - 24u));
        if (smc.select_down & bit) return;
        smc.select_down |= bit;
    }
    if (!mmc && note == 0x5Eu) {
        if (smc.play_down) return;
        smc.play_down = 1;
    }
    char saved[sizeof ui.msg];
    uint32_t until = ui.msg_until;
    int kit = smc_drum_mix();
    int action = !smc_lock_blocks(note) && (note == 0x5Du || note == 0x5Eu || note == 0x5Fu ||
        (note < 24u && (kit >= 0 ? smc_drum_track(kit, note & 7u) >= 0 :
            (((note & 7u) < NPARTS && note != 14u) || note >= 16u))));
    memcpy(saved, ui.msg, sizeof saved);
    smc_ui_cancel();                     /* navigation owns its new screen immediately */
    smc_button_apply(note, mmc);
    if (smc_fx_bus_now(24u) < 0) smc_fx_clear(); /* queued SMC navigation also cancels holds */
    if (action) {
        memcpy(ui.msg, saved, sizeof saved);  /* Help keeps its card and the preexisting notice too. */
        ui.msg_until = until;
        smc_ui.action = note;
        smc_ui_begin(note < 32u ? note & 7u : 0, 3);
    }
}

static void smc_release(uint8_t note)
{
    uint64_t bit = smc_button_bit(note);
    if (smc.ui_held & bit) {
        smc.ui_held &= ~bit;
        if (smc_ui.on && smc_ui.peer == smc_slot)
            smc_ui.until = plat_ms() + SMC_UI_MS;
    }
    if (note == 0x5Du) smc_lock_button(0);
    if (note == 6) {
        int back = (smc.home_down & 2u) && smc_fx_bus_now(24u) >= 0;
        smc.home_down = 0;
        if (back) smc_button_apply(6u, 0);
    }
    if (note == 0x5Eu) smc.play_down = 0;
    smc_fx_release_note(note);
    if (smc_fx_bus_now(24u) < 0) smc_fx_clear(); /* a main tap may have navigated out of FX */
    if (note >= 24u && note < 32u)
        smc.select_down &= (uint8_t)~(1u << (note - 24u));
    if (note >= 8u && note < 16u && smc.solo_target[note - 8u]) {
        smc.solo_target[note - 8u] = 0; /* release the original target, even after navigation */
        smc_solo_publish();
    }
}

/* Keep the existing performance-panel guard for DAW button presses. */
static void smc_guarded_button(uint8_t note, int mmc, int allow)
{
    if (allow && !perf_testing())
        smc_button(note, mmc);
    else if (perf_testing() && (note == 6u || note == 0x5Du || note == 0x5Eu))
        button_tap(B_HOME);            /* STOP, PLAY and back use the panel abort/restore path */
}

static uint8_t smc_message(const uint8_t *m, uint8_t len, int allow)
{
    uint8_t type, ch, a, b;
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
        if (type == 0x90u && b && !ch)
            smc_guarded_button(a, 0, 0);
        return 1;                     /* restoration still owns every other queued input */
    }
    if (type == 0xE0u && ch < 8u)
        smc_fader(ch, (uint8_t)(((uint16_t)b << 7 | a) >> 7));
    else if (type == 0xB0u && a >= 16u && a < 24u && ch == 0u) {
        int delta = smc_relative(b);
        if (delta) smc_knob(a - 16u, delta);
    } else if ((type == 0x90u || type == 0x80u) && !ch) {
        if (type == 0x90u && b) smc_button(a, 0);
        else smc_release(a);
    } else return 4; /* message is outside the DAW input format */
    return 0;
}

/* Acknowledge obsolete User MMC overflow without dispatching any command. */
static void smc_mmc_drain_stop(int src, int allow)
{
    (void)allow;
    smc_mmc_stop_seen[src] = smc_mmc_stop_published[src];
}

static void smc_poll_peer(void)
{
    uint32_t link = smc_link_published;
    uint8_t r, count = 0;
    int allow = !perf_testing();
    smc_fx_main_tick();
    /* Snapshot producer generations before either input ring drains. An ISR
     * can enqueue a press then lose its release while another ring drains;
     * acknowledging that new loss in this poll would leave Solo latched. */
    uint32_t release_lost[4], release_seen[4];
    for (int i = 0; i < 4; i++) {
        release_lost[i] = smc_release_lost[i];
        release_seen[i] = smc_release_seen[i];
    }
    if (ui_project_busy()) {
        smc.ui_held = 0;
        smc.lock_down = smc.play_down = smc.select_down = smc.home_down = 0;
        smc_fx_clear();
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
        smc.ui_held = 0;
        smc.lock_down = smc.play_down = smc.select_down = smc.home_down = 0; /* reconnect clears physical press edges */
        smc_fx_clear();
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
        smc.ui_held = 0;
        smc.lock_down = smc.play_down = smc.select_down = smc.home_down = 0;
        smc_fx_clear();
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
    if (smc_slot == 0) {
        /* Discard any obsolete hosted/User backlog. Only main-loop readers
         * advance; producer cursors and the central DAW rings are untouched. */
        smc_host_r = smc_host_w;
        for (int src = 0; src < 2; src++) {
            smc_mmc_r[src] = smc_mmc_w[src];
            smc_mmc_drain_stop(src, allow);
        }
    }
    /* A dropped Solo release is effective only after all presses queued
     * before the START-of-poll snapshot have been drained. Never consume a
     * newer ISR generation: its preceding press may still be queued. A
     * project drop may already have advanced a cursor; do not rewind it. */
    uint8_t missed_solo = 0;
    for (int i = 0; i < 4; i++)
        if (release_lost[i] != release_seen[i] && smc_release_seen[i] == release_seen[i]) {
            smc_release_seen[i] = release_lost[i];
            if (i < 2) missed_solo = 1;
            else smc.lock_down = 0;
        }
    if (missed_solo) {
        if (smc_project_active()) smc_solo_clear();
        /* The shared release-loss epoch also rearms edge-sensitive buttons.
         * Losing a key-up must never park PLAY, SELECT, or HOME until reconnect. */
        smc.ui_held = 0;
        smc.play_down = smc.select_down = smc.home_down = 0;
        smc_fx_clear();
    }
    smc_lock_tick();
    if ((perf_testing() || ui.overlay == O_ASK) && smc_project_active())
        smc_solo_clear();              /* an earlier event in this batch may have transferred ownership */
}

void smc_poll(void)
{
    if (ui_pad_diag_active()) {
        smc_pad_diag_boundary();
        return; /* no SMC commands, hold timers or transport while diagnosing */
    }
    uint8_t saved = smc_slot;
    for (smc_slot = 0; smc_slot < SMC_PEERS; smc_slot++) smc_poll_peer();
    smc_slot = saved;
}

/* Called immediately after the UI publishes entry=1 or exit=0, before normal
 * dispatch. End held gestures without normal SELECT tap/Pause release actions,
 * then consume backlog; never change ISR-owned writers or link publications.
 * Repeated active polls also discard input, so nothing replays on panel exit. */
void smc_pad_diag_boundary(void)
{
    uint8_t saved = smc_slot;
    smc_ui_cancel();
    for (smc_slot = 0; smc_slot < SMC_PEERS; smc_slot++) {
        smc_fx_clear();
        smc_solo_clear();
        smc.ui_held = 0;
        smc.lock_down = smc.play_down = smc.select_down = smc.home_down = 0;
        smc.picked = smc.seen = smc.input_seen = 0;
        smc.knob_run = smc.knob_i = 0;
        smc_peer_drop(); /* read-side cutoff, including hosted input/lost STOP */
    }
    smc_slot = saved;
}

static int smc_led(int note)
{
    int strip = note & 7, row = note >> 3;
    if (smc_lock_blocks((uint8_t)note)) return 0;
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
    if (smc.locked && note >= 24 && note < 32) {
        int bus = smc_fx_bus_now((uint8_t)note);
        if (bus >= 0) return (smc.fx_down >> bus) & 1u;
    }
    if (note == 5) return ui.overlay != O_NONE;
    if (note == 6) return 0;
    if (note == 7) return ui.view == V_HOME;
    if (note == 13) return ui.view == V_MIX;
    if (note == 14) return ui.view == V_GLO;
    if (note == 15) return ui.view == V_PART;
    if (note < 32) {
        if (strip < NPARTS) {
            int part = smc_strip_part(strip);
            if (part < 0) return 0;
            /* Panel REC and part changes update ui.part / ui.rec without smc.rec_part. */
            if (row == 0) return ui.rec && ui.part == part;
            if (row == 1) return !!(smc.solo & (1u << part));
            if (row == 2) return part_muted(part);
            /* SELECT feedback follows the same mode as its press action. Source
             * bus identity follows the presentation mapping, not the strip index. */
            if (row == 3 && ui.view == V_FX && ui.overlay == O_NONE && !smk_clip_mode_on())
                return (smc.fx_down >> part) & 1;
            return ui.part == part;
        }
        /* Strips 6–8: M = mute/zero. R8/S6/S7/S8 are handled above; SELECT 6/7/8
         * open reverb, delay and master (drum SELECT was handled above). */
        if (row == 2) return pref_get(strip == 7 ? PR(R_ENG, T_FX, 0, FX_VOLUME) :
                                     smc_home_target(strip)) == 0;
        return (strip == 5 && ui.view == V_FX && cur_page() == 0) ||
               (strip == 6 && ui.view == V_FX && cur_page() == 1) ||
               (strip == 7 && ui.view == V_MIX && cur_page() == 3);
    }
    /* Existing Mackie NoteOn feedback route; physical LED identity still
     * needs native owner verification for the captured preset. */
    if (note == 0x5D) {
        int beat = smc_beat_lit(); /* keep phase current even while locked */
        return smc.locked || beat;
    }
    if (note == 0x5E) return seq.playing;
    if (note == 0x5F) return ui.rec;
    return 0;
}

static void smc_frame_peer(void)
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
        if (!plat_ble_smc_send_slot(smc_slot, msg, 3)) return;
        smc.known[n] = 1;
        smc.lit[n] = (uint8_t)want;
    }
    for (i = 0; i < 8; i++)
        if (smc.pending & (1u << i)) {
            msg[0] = (uint8_t)(0xE0u + i);
            msg[1] = smc.value[i];
            msg[2] = smc.value[i];
            if (!plat_ble_smc_send_slot(smc_slot, msg, 3)) return;
            smc.pending &= (uint8_t)~(1u << i);
        }
}

void smc_frame(void)
{
    if (ui_pad_diag_active()) return; /* no SMC feedback inside the one-shot panel */
    uint8_t saved = smc_slot;
    for (smc_slot = 0; smc_slot < SMC_PEERS; smc_slot++) smc_frame_peer();
    smc_slot = saved;
}
