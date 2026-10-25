/* SPDX-License-Identifier: GPL-3.0-only */
/* M-VAVE SMC-Mixer control surface. Included after ui.c in the firmware unity build.
 * DAW messages match Mackie Control; the stock User preset uses CC and MMC SysEx.
 * TIMER5 only copies complete messages. All project, sequencer and display work is in the main loop. */
#include "smc_protocol.h"

#define SMC_Q 32u
#include "app/smc_ble_input.h"
#include "app/smc_peers.h"
extern int plat_ble_smc_send_peer(uint8_t peer, const uint8_t *msg, uint8_t len);
extern uint8_t smk_clip_mode_on(void);
extern int smk_ck_smc_turn(int n, int delta);
extern int smk_ck_smc_nav(uint8_t note);
/* Two physical direct peers plus one explicitly shared hosted MIDI source.
 * Only the main loop changes the active application context. TIMER5 addresses
 * ingress by validated slot and never reads this context pointer. */
static smc_ble_input_t smc_inputs[SMC_DIRECT_PEERS];
static volatile uint32_t smc_link_seq[SMC_DIRECT_PEERS];
static uint32_t smc_links_seen[SMC_DIRECT_PEERS];
static uint32_t smc_losses_seen[SMC_DIRECT_PEERS];
/* Pause has an independent release class. Losing Solo/PLAY/SELECT key-up
 * must never turn a still-held Pause repeat into another lock toggle. */
static volatile uint32_t smc_pause_lost[SMC_DIRECT_PEERS + 1u];
static uint32_t smc_pause_seen[SMC_DIRECT_PEERS + 1u];
static volatile uint32_t smc_pause_link_boundary[SMC_DIRECT_PEERS];
static uint8_t smc_peer;
#define smc_ble_input (smc_inputs[smc_peer])
#define smc_q (smc_ble_input.q)
#define smc_qw (smc_ble_input.w)
#define smc_qr (smc_ble_input.r)
#define smc_link_published (smc_ble_input.link)
#define smc_ble_solo_release_lost (smc_ble_input.release_lost)
#define smc_link_ble_loss_boundary (smc_ble_input.link_release_boundary)
#define smc_link_seen (smc_links_seen[smc_peer])
#define smc_ble_solo_release_seen (smc_losses_seen[smc_peer])
_Static_assert(SMC_Q == SMC_BLE_INPUT_CAPACITY, "BLE consumer capacity");
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
/* A full MMC queue can reject several STOP commands at DIFFERENT cutoffs
 * when TIMER/audio refills a freed slot between polls. One overwritten cutoff
 * incorrectly reorders the first STOP after the next PLAY. Store distinct
 * stop boundaries in a single-producer/single-consumer FIFO per source. The
 * 16-command ring admits at most 17 distinct pending cutoffs at once; 32
 * slots leave room for ISR/main-loop interleavings without overwrite. */
#define SMC_STOP_Q 32u
static volatile uint8_t smc_mmc_stop_q[2][SMC_STOP_Q];
static volatile uint8_t smc_mmc_stop_w[2], smc_mmc_stop_r[2];
/* Separate single-writer generation counters: TIMER5 (central), audio ISR
 * (host). Main-loop snapshots never clear a producer-owned flag. */
static volatile uint32_t smc_host_solo_release_lost;
static uint32_t smc_host_solo_release_seen;
static int smc_is_solo_release(uint8_t st, uint8_t note, uint8_t velocity)
{
    uint8_t type = st & 0xF0u;
    return !(note & 0x80u) && !(velocity & 0x80u) &&
           (note < 32u || (note >= 0x5Bu && note <= 0x5Eu)) &&
           (type == 0x80u || (type == 0x90u && !velocity));
}
static uint8_t smc_resuming;
static int smc_clip_on(void)
{
    return !smc_resuming && smk_clip_mode_on();
}
typedef struct {
    pref_t target[8];
    uint8_t value[8], last[8], seen, picked, pending;
    uint8_t up, fresh, solo, rec_part;
    uint8_t preset_user_seen;
    uint8_t input_seen;                /* physical history survives target/pickup refresh */
    uint8_t solo_target[8];            /* physical S key: 1..5 parts, then 909/808 tracks; 0 released */
    uint16_t drum_solo[NKIT];
    uint8_t lit[128], known[128];
    uint8_t muted, mute_edit;          /* exact-target mute epoch; external edits revoke its takeover */
    uint32_t knob_t;                   /* acceleration of the knob now turning, not a panel encoder */
    uint8_t knob_run, knob_i, play_down, select_down, pattern_down;
    /* Exact destinations, not a copy of the temporary display or live UI. */
    pref_t lock_fader[8], lock_knob[8];
    uint32_t lock_song_revision;
    uint16_t lock_song_len;
    uint8_t locked, lock_down, lock_kit, lock_bank, lock_part, lock_view, lock_page;
    uint8_t bank, mode_note, lock_sel[2], lock_mixsel, lock_song_sel, lock_overlay, lock_list_sel, lock_list_top;
    uint32_t nav_down, bank_context;
    pref_t knob_target;
} smc_state_t;
static smc_state_t smc_states[SMC_DIRECT_PEERS + 1u];
/* Main-loop selector only. ISR producers continue to address validated slots. */
static smc_state_t *smc_current = &smc_states[0];
static void smc_select(uint8_t peer)
{
    smc_peer = peer;
    smc_current = &smc_states[peer];
}
#define smc (*smc_current)
static uint8_t smc_solo_all;
static uint16_t smc_drum_solo_all[NKIT];

int smc_project_active(void)
{
    return smc_solo_all || smc_drum_solo_all[0] || smc_drum_solo_all[1];
}
static void smc_solo_publish(void);
static void smc_solo_clear(void)
{
    memset(smc.solo_target, 0, sizeof smc.solo_target);
    smc.solo = 0;
    smc.drum_solo[0] = smc.drum_solo[1] = 0;
    smc_solo_publish();
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
    smc_solo_all = 0;
    smc_drum_solo_all[0] = smc_drum_solo_all[1] = 0;
    for (uint8_t p = 0; p <= SMC_DIRECT_PEERS; p++) {
        smc_solo_all |= smc_states[p].solo;
        smc_drum_solo_all[0] |= smc_states[p].drum_solo[0];
        smc_drum_solo_all[1] |= smc_states[p].drum_solo[1];
    }
    engine_smc_solo_targets(smc_solo_all, smc_drum_solo_all[0], smc_drum_solo_all[1]);
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
    for (uint8_t p = 0; p < SMC_DIRECT_PEERS; p++) {
        smc_inputs[p].r = smc_inputs[p].w;
        smc_losses_seen[p] = smc_inputs[p].release_lost;
    }
    smc_host_r = smc_host_w;
    smc_mmc_r[0] = smc_mmc_w[0];
    smc_mmc_r[1] = smc_mmc_w[1];
    smc_mmc_stop_r[0] = smc_mmc_stop_w[0];
    smc_mmc_stop_r[1] = smc_mmc_stop_w[1];
    smc_host_solo_release_seen = smc_host_solo_release_lost;
    for (uint8_t p = 0; p <= SMC_DIRECT_PEERS; p++) smc_pause_seen[p] = smc_pause_lost[p];
}
X0X_COLD void smc_project_reset(void)
{
    smc_ui_cancel();
    smc_project_drop();
    for (uint8_t p = 0; p <= SMC_DIRECT_PEERS; p++) {
        uint8_t up = smc_states[p].up;
        memset(&smc_states[p], 0, sizeof smc_states[p]);
        smc_states[p].up = smc_states[p].fresh = up;
        smc_states[p].pending = 0xFFu;
    }
    smc_solo_all = 0;
    smc_drum_solo_all[0] = smc_drum_solo_all[1] = 0;
    engine_smc_solo(0);
    engine_fx_mute(0);
}

void smc_ble_midi_peer(uint8_t peer, const uint8_t *msg, uint8_t len)
{
    if (peer >= SMC_DIRECT_PEERS) return;
    smc_ble_input_t *in = &smc_inputs[peer];
    int release = msg && len == 3u &&
        (proj.set.bt & PROJ_BT_SMC_CENTRAL) && (in->link & 1u) &&
        smc_is_solo_release(msg[0], msg[1], msg[2]) &&
        (!(msg[0] & 15u) || ((proj.set.bt & PROJ_BT_SMC_USER) &&
                              (msg[0] & 15u) == 15u));
    if (smc_ble_input_push(in, msg, len,
                          proj.set.bt & PROJ_BT_SMC_USER, release))
        smc_enqueued++;
    else {
        smc_dropped++;
        if (release && msg[1] == 0x5Du) smc_pause_lost[peer]++;
    }
}
void smc_ble_midi(const uint8_t *msg, uint8_t len)
{
    smc_ble_midi_peer(0, msg, len);
}
void smc_ble_link_peer(uint8_t peer, int up)
{
    if (peer >= SMC_DIRECT_PEERS) return;
    smc_link_seq[peer]++;
    __asm__ volatile("" ::: "memory");
    smc_pause_link_boundary[peer] = smc_pause_lost[peer];
    smc_ble_input_link(&smc_inputs[peer], up);
    __asm__ volatile("" ::: "memory");
    smc_link_seq[peer]++;
}
void smc_ble_link(int up) { smc_ble_link_peer(0, up); }
int smc_ble_role_get(void) { return !!(proj.set.bt & PROJ_BT_SMC_CENTRAL); }
int smc_connected(void)
{
    return (smc_states[0].up || smc_states[1].up) && smc_ble_role_get();
}

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
        if (a == 0x5Du) smc_pause_lost[SMC_DIRECT_PEERS]++;
    }
    return 1;
}

void smc_mmc_host(uint8_t cmd, uint8_t src)
{
    uint8_t w;
    /* USB/peripheral BLE can receive MMC regardless of the selected preset.
     * Reject it on arrival when User mode is disabled: otherwise switching
     * User back on before smc_poll replays stale PLAY/STOP controls. */
    if (!(proj.set.bt & PROJ_BT_SMC_USER) ||
        src > 1u || cmd < 1u || cmd > 43u)
        return;
    w = smc_mmc_w[src];
    if ((uint8_t)(w - smc_mmc_r[src]) >= 16u) {
        /* Consecutive STOPs at one cutoff are equivalent; a newer cutoff
         * after an intervening PLAY must remain ordered behind the first. */
        if (cmd == 34u) {
            uint8_t stop_w = smc_mmc_stop_w[src];
            if (stop_w == smc_mmc_stop_r[src] ||
                smc_mmc_stop_q[src][(uint8_t)(stop_w - 1u) & (SMC_STOP_Q - 1u)] != w) {
                smc_mmc_stop_q[src][stop_w & (SMC_STOP_Q - 1u)] = w;
                __asm__ volatile("" ::: "memory");
                smc_mmc_stop_w[src] = (uint8_t)(stop_w + 1u);
            }
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
    if (smc.locked) return (int)smc.lock_kit - 1;
    if (ui.overlay != O_NONE || smc_clip_on()) return -1;
    /* SEQ follows its selected drum part; MIX follows the kit actually shown. */
    return ui.view == V_SEQ && is_drum() ? ui.part : drum_mix_page();
}

static int smc_drum_track(int kit, int strip)
{
    int track = (smc.locked ? smc.lock_bank : (ui.sel[kit] / 8) * 8) + strip;
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

/* Bank zero retains the installed mapping. Further banks use real descriptors;
 * no parameter values or physical fader history live in a navigation bank. */
static void smc_bank_sync(void)
{
    if (smc.locked) return;
    uint32_t context = ui.view | (uint32_t)ui.part << 4 | (uint32_t)cur_page() << 8 |
        (uint32_t)ui.sel[0] << 13 | (uint32_t)ui.sel[1] << 17 |
        (uint32_t)ui.mixsel << 21 | (uint32_t)ui.overlay << 24;
    if (context != smc.bank_context) { smc.bank = smc.mode_note = 0; smc.bank_context = context; }
}
static int smc_mix_context(void)
{
    return smc_drum_mix() >= 0 || ((smc.locked ? smc.lock_overlay : ui.overlay) == O_NONE &&
        ((smc.locked ? smc.lock_view : ui.view) == V_HOME ||
         ((smc.locked ? smc.lock_view : ui.view) == V_MIX && (smc.locked ? smc.lock_page : cur_page()) == 0)));
}
static pref_t smc_extra_target(int index)
{
    pref_t r;
    int total = pg.n * 4 + pg.nmore;
    for (int i = 0; i < total; i++) {
        r = i < pg.n * 4 ? pg.r[i / 4][i % 4] : pg.more[i - pg.n * 4];
        const x0x_param_t *d = pref_desc(r);
        if (!d || !d->max || r.kind == R_ACT || r.kind == R_HDR || smc_fader_of(r)) continue;
        int duplicate = 0;
        for (int j = 0; j < i; j++) {
            pref_t prev = j < pg.n * 4 ? pg.r[j / 4][j % 4] : pg.more[j - pg.n * 4];
            if (smc_same(r, prev)) { duplicate = 1; break; }
        }
        if (!duplicate && index-- == 0) return r;
    }
    return NONE;
}
static pref_t smc_voice_target(int kit, int track, int index)
{
    int t = kit ? T_808 : T_909;
    for (int i = 0; track >= 0 && i < engine_nparams(t, track); i++) {
        const x0x_param_t *d = engine_param(t, track, i);
        if (name_is(d, "Level") || name_is(d, "Pan") || name_is(d, "Rev") || name_is(d, "Dly")) continue;
        if (!index--) return PR(R_ENG, t, track, i);
    }
    return NONE;
}
static int smc_bank_count(void)
{
    int kit = smc_drum_mix(), count = 0;
    smc_bank_sync();
    if (kit >= 0) {
        for (int n = 0; n < 8; n++) {
            int i = 0;
            while (smc_voice_target(kit, smc_drum_track(kit, n), i).kind != R_NONE) i++;
            if (i > count) count = i;
        }
        return 3 + count;
    }
    while (smc_extra_target(count).kind != R_NONE) count++;
    return (smc_mix_context() ? 3 : 1) + (count + 7) / 8;
}
static const char *smc_bank_name(void)
{
    if (smc_mix_context() && smc.bank < 3) {
        static const char *const names[] = {"PAN", "REV SEND", "DLY SEND"};
        return names[smc.bank];
    }
    return smc.bank ? "PARAMETERS" : "PAGE";
}

/* Faders retain levels/returns/filter; knobs add a separate useful dimension.
 * A visible list row on any active fader is parked rather than redirected. */
static pref_t smc_knob_target(int n)
{
    pref_t r;
    int kit = smc_drum_mix();
    smc_bank_sync();
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
        static const char *const fields[] = {"Pan", "Rev", "Dly"};
        return smc.bank >= 3 ? smc_voice_target(kit, track, smc.bank - 3) :
            track >= 0 ? find_ref(kit ? T_808 : T_909, track, fields[smc.bank]) : NONE;
    }
    if (smc.bank) {
        if (smc_mix_context() && smc.bank < 3)
            return n < NPARTS ? PR(R_ENG, T_MIX, n, smc.bank == 1 ? MX_REV : MX_DLY) : NONE;
        return smc_extra_target((smc.bank - (smc_mix_context() ? 3 : 1)) * 8 + n);
    }
    if (ui.overlay == O_NONE && !smc_clip_on() &&
        (ui.view == V_HOME || (ui.view == V_MIX && cur_page() == 0))) {
        if (n < NPARTS) return PR(R_ENG, T_MIX, n, MX_PAN);
        static const pref_t tail[3] = {
            PR(R_ENG, T_FX, 0, FX_RV_DECAY), PR(R_ENG, T_FX, 0, FX_DL_FDBK),
            PR(R_ENG, T_MST, 0, MST_RESO)
        };
        return tail[n - NPARTS];
    }
    r = smc_page_target(n);
    if (ui.overlay == O_NONE && !smc_clip_on() &&
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

/* Capture target identity separately from keyboard navigation. Intentional SMC
 * navigation re-enters this context, dispatches normally, then replaces it. */
static void smc_capture(void)
{
    smc.lock_kit = (uint8_t)(smc_drum_mix() + 1);
    smc.lock_bank = smc.lock_kit ? (uint8_t)(ui.sel[smc.lock_kit - 1u] / 8u * 8u) : 0;
    smc.lock_part = ui.part; smc.lock_view = ui.view;
    smc.lock_page = (uint8_t)cur_page();
    memcpy(smc.lock_sel, ui.sel, sizeof smc.lock_sel);
    smc.lock_mixsel = ui.mixsel; smc.lock_song_sel = ui.song_sel;
    smc.lock_overlay = ui.overlay; smc.lock_list_sel = ui.list_sel; smc.lock_list_top = ui.list_top;
    smc.lock_song_revision = song_structure_revision; smc.lock_song_len = song()->len;
    for (int i = 0; i < 8; i++) {
        smc.lock_fader[i] = smc_target(i);
        smc.lock_knob[i] = smc_bind(smc_knob_target(i));
    }
}
int smc_lock_toggle(void)
{
    if (!smc.locked) {
        if (ui_project_busy() || perf_testing() || ui.overlay == O_ASK || smc_clip_on() ) {
            ui_say("SMC LOCK UNAVAILABLE", 0);
            return 0;
        }
        smc_capture();
    }
    smc.locked ^= 1u;
    /* smc_notice owns pickup invalidation on an actual destination/value change. */
    smc_ui_cancel();
    ui_say(smc.locked ? "SMC TARGETS LOCKED" : "SMC FOLLOWS PAGE", 0);
    return 1;
}

/* The verified physical input route reports both press and release here.
 * Repeated positive messages while held cannot unlock the surface again. */
int smc_lock_button(int pressed)
{
    uint8_t down = pressed != 0, edge = down && !smc.lock_down;
    smc.lock_down = down;
    return edge ? smc_lock_toggle() : 0;
}

/* Main-loop neutral API preserves the accepted target-lock policy. */
int smc_peer_lock(uint8_t peer, int locked)
{
    if (peer >= SMC_DIRECT_PEERS) return 0;
    uint8_t saved = smc_peer;
    smc_select(peer);
    int result = !!locked == smc.locked || smc_lock_toggle();
    smc_select(saved);
    return result;
}
int smc_peer_locked(uint8_t peer)
{
    return peer < SMC_DIRECT_PEERS && smc_states[peer].locked;
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
    uint8_t on, kind, strip, action, peer;
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
    SMC_CTX(smc_clip_on());
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
    smc_ui.peer = smc_peer;
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
    return note == 5 ? (!smc.locked && smc_clip_on() && ui.overlay == O_NONE ? B_GLO : B_SEL) :
           note == 6 ? B_HOME : note == 14 ? B_GLO : -1;
}
static const char *smc_button_caption(int note)
{
    int kit = smc_drum_mix(), strip = note & 7, row = note >> 3;
    int view = smc.locked ? smc.lock_view : ui.view;
    int overlay = smc.locked ? smc.lock_overlay : ui.overlay;
    if (kit >= 0 && smc_drum_track(kit, strip) < 0) return "--";
    if (smc_lock_blocks((uint8_t)note)) return "X";
    if (row == 2) return "M";
    if (kit >= 0 || strip < NPARTS) return row ? "S" : "R";
    int key = smc_nav_key(note);
    if (key == B_SEL) return overlay == O_LIST ? "OK" : "LIST";
    if (key == B_GLO) return view == V_GLO && overlay != O_NONE ? "BACK" : "GLOB";
    if (key == B_HOME) return !smc.locked && smc_clip_on() && overlay == O_NONE ? "GRID" :
        overlay != O_NONE || view == V_GLO ? "BACK" : view == V_HOME ? "SONG" : "HOME";
    view = SMC_VIEW[row][strip - NPARTS];
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
    static const char *const MAIN_N[8] = {"909", "808", "303A", "303B", "BRK", "RVB", "DLY", "FLT"};
    int kit = smc_drum_mix();
    /* Selection follows the keyboard UI, independently of the touched control.
     * A locked bank can differ: no lane is selected if it is outside this bank.
     * LOCK in the header describes captured targets, never the title highlight. */
    int selected = kit >= 0 ? ui.sel[kit] - smc_drum_track(kit, 0) :
        ui.view == V_FX ? 5 + cur_page() :
        ui.view == V_MIX ? (cur_page() == 3 ? 7 : ui.mixsel) : ui.part;
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
        } else put_s(name, MAIN_N[n]);
        if (n == selected) cv_rect(x + 1, 0, 28, 13, C_WHITE);
        text_c(x + 15, 1, &FONT_T, name, n == selected ? C_BLACK : C_WHITE);
        if (k.kind == R_NONE) put_s(name, "--");
        else {
            const char *label = pref_name(k);
            if (k.kind == R_ENG && k.a == T_MIX && k.c == MX_PAN) label = "PAN";
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
        for (int row = 0; row < 3; row++) {
            int y = 64 + row * 14;
            uint16_t c = smc_led(NOTES[row] + n) ? C_HI : f.kind == R_NONE ? C_DIM : C_GRAY;
            const char *caption = smc_button_caption(NOTES[row] + n);
            if (caption[0] == 'X' || caption[0] == '-') c = C_DIM;
            cv_rect(x + 8, y, 22, 13, C_LINE);
            text_c(x + 19, y + 1, &FONT_T, caption, c);
        }
        smc_ui_strip_value(f, x + 15, 122, fc);
    }
}

void smc_ui_draw(void)
{
    uint8_t saved_peer = smc_peer;
    smc_select(smc_ui.peer);
    char name[24], num[32], unit[12], tag[80];
    const char *status = "ACTIVE";
    pref_t r = smc_ui.kind == 1 ? smc_knob_target(smc_ui.strip) : smc_target(smc_ui.strip);
    int kit = smc_drum_mix();
    cv_begin(240, HDR_H, C_BLACK);
    if (kit >= 0)
        put_i(put_s(put_s(put_s(tag, PART_N[kit]), (smc.locked ? smc.lock_view : ui.view) == V_SEQ ? " SEQ" : " MIX"), smc.locked ? " LOCK B" : " BANK "), (smc.locked ? smc.lock_bank / 8 : ui.sel[kit] / 8) + 1);
    else if (smc.locked)
        put_i(put_s(put_s(put_s(tag, "LOCK "), smc.lock_view == V_PART ? PART_N[smc.lock_part] : VIEW_N[smc.lock_view]), " P"), smc.lock_page + 1);
    else if (smc_clip_on()) put_s(tag, "CLIP KEYS");
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
        int owner = !smc.locked && !smc.bank ? smc_fader_of(smc_page_target(smc_ui.strip)) : 0;
        if (owner) {
            smc_ui_label(smc_page_target(smc_ui.strip), name);
            put_i(put_s(num, "ON FADER "), owner);
        } else { put_s(name, "NO PARAMETER"); put_s(num, "--"); }
        unit[0] = 0; status = "INACTIVE";
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
    if (smc_ui.kind == 1) {
        char *end = put_i(put_s(tag, "KNOBS "), smc.bank + 1);
        put_s(put_s(end, " / "), smc_bank_name());
        status = tag;
    }
    text_c(120, 49, &FONT_XS, status, C_HI);
    cv_commit(3, 0, KNOB_Y);
    smc_select(saved_peer);
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
    int same = smc_same(r, smc.target[n]);
    int muted = volume_muted(r);
    /* An epoch starts only on this exact destination. No initial/reconnect
     * packet can claim it: the fader additionally requires physical history. */
    if (muted && !(smc.muted & bit) && (same || !(smc.input_seen & bit)))
        smc.mute_edit |= bit;
    if (!muted || (!same && (smc.input_seen & bit)) || (same && v != smc.value[n]))
        smc.mute_edit &= (uint8_t)~bit;
    if (muted) smc.muted |= bit;
    else smc.muted &= (uint8_t)~bit;
    if (!same || v != smc.value[n]) {
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
    int crossed, nv, moved, history;
    if (n < 0 || n >= 8)
        return;
    bit = (uint8_t)(1u << n);
    /* A repeated absolute sample is not a new gesture. Keep this history
     * separate from acceptance: target edits must retain crossing history. */
    history = smc.input_seen & bit;
    moved = !history || smc.last[n] != v;
    smc_notice(n);
    smc.input_seen |= bit;
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
        if (!moved || (!crossed && !(history && (smc.mute_edit & bit))))
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
    pref_t identity = smc.locked ? r : smc_bind(r);
    if (smc.knob_i != (uint8_t)(n + 1) || !smc_same(identity, smc.knob_target)) {
        smc.knob_target = identity;
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
    smc_notice(which + 5);
    engine_fx_mute(engine_fx_muted() ^ (1u << which));
    if (which < 2) smk_mute_edge(which + 5, engine_fx_muted() & (1u << which));
    smc_notice(which + 5);
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
        !smc_clip_on() && smc_drum_mix() >= 0) {
        int kit = smc_drum_mix();
        smc_select_drum(kit, note == 0x2E ? 0 : 8);
        return 1;
    }
    return 0;
}

static int smc_lock_blocks(uint8_t note)
{
    (void)note;
    return 0; /* lock isolates keyboard routing; SMC buttons remain intentional input */
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
    /* Confirmation owns its modal controls; the page lock never parks SMC keys. */
    if (smc_lock_blocks(note)) {
        ui_say("SMC TARGETS LOCKED", 0);
        return;
    }
    if (!smc.locked && !smc_resuming && ui.overlay == O_NONE && smk_ck_smc_nav(note))
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
                smc_notice(note & 7);
                mute_track(kit, track);
                smc_notice(note & 7);
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
        if (nav_key == B_GLO && ui.view == V_GLO && ui.overlay == O_NONE)
            *page_of(V_GLO) = (uint8_t)((cur_page() + 1) % pg.n);
        else button_tap(nav_key);
        return;
    }
    if ((note == 0x30 || note == 0x31 ||
         ((note == 0x2E || note == 0x2F) && ui.view == V_PART && is_drum())) &&
        ui.overlay == O_NONE && !smc_clip_on()) {
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
            } else if (row == 2) {     /* MUTE: observe both edges even in one queued batch */
                smc_notice(strip);
                mute_part(strip);
                smc_notice(strip);
            } else {                     /* SELECT */
                int repeat_drum = strip < NKIT && ui.part == strip && ui.overlay == O_NONE;
                smc_select_part(strip);
                smc_view(repeat_drum ? V_MIX : V_PART, repeat_drum ? strip + 1 : -1);
            }
        } else {
            int which = strip - NPARTS;
            if (row == 2)
                smc_toggle_fx(which);
            else {
                int view = SMC_VIEW[row][which];
                int repeat = smc.mode_note == note + 1 && ui.view == view;
                smc_view(view, repeat ? -1 : SMC_PAGE[row][which]);
                if (repeat) *page_of(view) = (uint8_t)((cur_page() + 1) % pg.n);
                smc.mode_note = note + 1;
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
        int part = smc.locked ? smc.lock_part : ui.part;
        int pending = seq_cue_of(&seq, part);
        int next = (pending >= 0 ? pending : seq.ppat[part]) + (note == 0x5B ? -1 : 1);
        if (next >= 0 && next < NPAT) {
            if (next == seq.ppat[part]) seq.pcue[part] = 0xFFu;
            else seq_cue_part(&seq, part, next);
        }
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
            int count = smc_bank_count();
            smc.bank = (uint8_t)((smc.bank + d + count) % count);
            smc_ui_begin(0, 1);
        } else {
            *page_of(ui.view) = (uint8_t)((cur_page() + d + pg.n) % pg.n);
        }
        break;
    }
    default: break;
    }
}

static int smc_navigation(uint8_t note)
{
    return note < 8 || (note >= 24 && note < 32) ||
        (note >= 0x2E && note <= 0x31) || (note >= 0x60 && note <= 0x63) ||
        (smc_drum_mix() < 0 && note >= 13 && note <= 15);
}
static void smc_resume(void)
{
    bass_step_cancel(); drum_step_cancel();
    ui.part = smc.lock_part; ui.view = smc.lock_view;
    ui.mixsel = smc.lock_mixsel; ui.song_sel = smc.lock_song_sel;
    memcpy(ui.sel, smc.lock_sel, sizeof smc.lock_sel);
    *page_of(ui.view) = smc.lock_page;
    ui.overlay = O_NONE;
    build_pages();
    if (smc.lock_overlay == O_LIST) {
        if (ui.view == V_GLO) open_global(); else open_list_of_pages();
        ui.overlay = O_LIST; ui.list_sel = smc.lock_list_sel; ui.list_top = smc.lock_list_top;
    }
    uint8_t bank = smc.bank, mode = smc.mode_note;
    smc.locked = 0; smc_bank_sync(); smc.locked = 1;
    smc.bank = bank; smc.mode_note = mode;
}

static void smc_button(uint8_t note, int mmc)
{
    if (!mmc && note == 0x5Du) {
        smc_lock_button(1);
        return;                         /* no transport action card for target lock */
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
    if (!mmc && (note == 0x5Bu || note == 0x5Cu)) {
        uint8_t bit = (uint8_t)(1u << (note - 0x5Bu));
        if (smc.pattern_down & bit) return;
        smc.pattern_down |= bit;
    }
    smc_bank_sync();
    int navigation = smc_navigation(note);
    if (!mmc && navigation && note < 32) {
        uint32_t bit = 1u << note;
        if (smc.nav_down & bit) return;
        smc.nav_down |= bit;
    }
    uint8_t revision = project_revision, modal = ui.overlay == O_ASK;
    int was_locked = smc.locked && navigation && ui.overlay != O_ASK;
    if (was_locked) { smc_resuming = 1; smc_resume(); smc.locked = 0; }
    int mapping_navigation = (note == 0x60 || note == 0x61) && ui.overlay == O_NONE && !smc_clip_on();
    char saved[sizeof ui.msg];
    uint32_t until = ui.msg_until;
    int kit = smc_drum_mix();
    int action = !smc_lock_blocks(note) && (note == 0x5Du || note == 0x5Eu || note == 0x5Fu ||
        (note < 24u && (kit >= 0 ? smc_drum_track(kit, note & 7u) >= 0 :
            (((note & 7u) < NPARTS && note != 14u) || note >= 16u))));
    memcpy(saved, ui.msg, sizeof saved);
    smc_ui_cancel();                     /* navigation owns its new screen immediately */
    smc_button_apply(note, mmc);
    if (navigation && note != 0x60 && note != 0x61 && !modal && ui.overlay != O_ASK) {
        uint8_t mode = smc.mode_note;
        smc_bank_sync(); smc.bank = 0; smc.mode_note = mode;
    }
    if (was_locked) {
        if (revision == project_revision && !ui_project_busy()) {
            if (ui.overlay != O_ASK) smc_capture();
            smc.locked = 1;
        }
        smc_resuming = 0;
    }
    if (mapping_navigation) smc_ui_begin(0, 1);
    if (action) {
        memcpy(ui.msg, saved, sizeof saved);  /* Help keeps its card and the preexisting notice too. */
        ui.msg_until = until;
        smc_ui.action = note;
        smc_ui_begin(note < 32u ? note & 7u : 0, 3);
    }
}

static void smc_release(uint8_t note)
{
    if (note < 32) smc.nav_down &= ~(1u << note);
    if (note == 0x5Du) smc_lock_button(0);
    if (note == 0x5Eu) smc.play_down = 0;
    if (note == 0x5Bu || note == 0x5Cu)
        smc.pattern_down &= (uint8_t)~(1u << (note - 0x5Bu));
    if (note >= 24u && note < 32u)
        smc.select_down &= (uint8_t)~(1u << (note - 24u));
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
    /* Channel controls are exactly three MIDI bytes. Reject truncated or
     * extended frames rather than dispatching an otherwise valid prefix. */
    if (len != 3u)
        return 4;
    /* Never alias malformed BLE MIDI data bytes into SMC notes/CCs. */
    if ((m[1] | m[2]) & 0x80u)
        return 4;
    type = m[0] & 0xF0u;
    ch = m[0] & 0x0Fu;
    a = m[1] & 0x7Fu;
    b = m[2] & 0x7Fu;
    /* Direct BLE User feedback can use DAW channel 1 or the User channel
     * 16; arbitrary musical channels 2..15 must never operate SMC keys,
     * faders or performance-mode abort. Keep both maintained routes. */
    if (user && ch != 0u && ch != 15u)
        return 4;
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
    /* A STOP belongs immediately after all commands before its cutoff,
     * even if a producer refilled the MMC ring while an older STOP waited.
     * Copy the cutoff, retire this FIFO slot, then dispatch so an ISR may
     * safely reuse the slot (or a project action may discard all input). */
    while (smc_mmc_stop_r[src] != smc_mmc_stop_w[src]) {
        uint8_t r = smc_mmc_stop_r[src];
        __asm__ volatile("" ::: "memory");
        uint8_t cutoff = smc_mmc_stop_q[src][r & (SMC_STOP_Q - 1u)];
        if (smc_mmc_r[src] != cutoff) break;
        __asm__ volatile("" ::: "memory");
        smc_mmc_stop_r[src] = (uint8_t)(r + 1u);
        if (proj.set.bt & PROJ_BT_SMC_USER)
            smc_guarded_button(0x5Du, 1, allow);
    }
}

static void smc_context_prepare(int allow)
{
    uint8_t preset_user = !!(proj.set.bt & PROJ_BT_SMC_USER);
    if (smc.preset_user_seen != preset_user) {
        smc.preset_user_seen = preset_user;
        smc.picked = smc.seen = smc.knob_i = 0;
        smc.nav_down = 0;
        smc.lock_down = smc.play_down = smc.select_down = smc.pattern_down = 0;
        smc_solo_clear();
        smc.pending = 0xFFu;
        memset(smc.known, 0, sizeof smc.known);
    }
    if (!allow || ui.overlay == O_ASK) {
        smc.picked = smc.seen = smc.knob_i = 0;
        smc.nav_down = 0;
        smc.lock_down = smc.play_down = smc.select_down = smc.pattern_down = 0;
        smc_solo_clear();
    }
}
static void smc_poll_peer(int allow)
{
    uint32_t seq, link, ble_link_loss, pause_link_loss;
    do {
        seq = smc_link_seq[smc_peer];
        if (seq & 1u) continue;
        __asm__ volatile("" ::: "memory");
        link = smc_link_published;
        ble_link_loss = smc_link_ble_loss_boundary;
        pause_link_loss = smc_pause_link_boundary[smc_peer];
        __asm__ volatile("" ::: "memory");
    } while (seq != smc_link_seq[smc_peer] || (seq & 1u));
    uint32_t ble_lost = smc_ble_solo_release_lost;
    uint32_t ble_seen = smc_ble_solo_release_seen;
    uint32_t pause_lost = smc_pause_lost[smc_peer];
    uint32_t pause_seen = smc_pause_seen[smc_peer];
    uint8_t r, count = 0;
    if (link != smc_link_seen) {
        smc_link_seen = link;
        if ((uint32_t)(ble_link_loss - ble_seen) < 0x80000000u) {
            ble_seen = ble_link_loss;
            smc_ble_solo_release_seen = ble_link_loss;
        }
        if ((uint32_t)(pause_link_loss - pause_seen) < 0x80000000u) {
            pause_seen = pause_link_loss;
            smc_pause_seen[smc_peer] = pause_link_loss;
        }
        uint8_t cutoff = (uint8_t)(link >> 1);
        if ((uint8_t)(cutoff - smc_qr) <= (uint8_t)(smc_qw - smc_qr))
            smc_qr = cutoff;
        /* Retain captured targets, as in accepted single-peer behavior. Retire
         * only this peer's held edges, history, pickup, Solo and feedback. */
        if (smc_ui.on && smc_ui.peer == smc_peer) smc_ui_cancel();
        smc_solo_clear();
        uint8_t up = (uint8_t)(link & 1u);
        smc.up = smc.fresh = up;
        smc.picked = smc.seen = smc.input_seen = smc.muted = smc.mute_edit = 0;
        smc.nav_down = 0;
        smc.lock_down = smc.play_down = smc.select_down = smc.pattern_down = 0;
        smc.knob_i = smc.knob_run = 0;
        smc.knob_t = 0;
        smc.pending = 0xFFu;
        memset(smc.known, 0, sizeof smc.known);
        ui_say(up ? "SMC CONNECTED" : "SMC LOST", 0);
    }
    smc_context_prepare(allow);
    while (smc_qr != smc_qw && count++ < SMC_Q) {
        r = smc_qr;
        __asm__ volatile("" ::: "memory");
        smc_event_t msg = smc_q[r % SMC_Q];
        /* A TIMER5 link edge may arrive after this poll's epoch snapshot.
         * Leave any post-edge packet for the next poll to apply that edge's
         * cutoff and clear the old Solo without erasing a new-session press. */
        __asm__ volatile("" ::: "memory");
        if (smc_link_published != link) break;
        /* The ISR may reuse a full ring's slot as soon as the read index moves.
         * Keep a private event before releasing it. Release before dispatch so
         * a project action can still discard the remaining queued input. */
        __asm__ volatile("" ::: "memory");
        smc_qr = (uint8_t)(r + 1u);   /* an action can replace the project and discard the remaining queue */
        if (!smc.up) smc_reason = 2;
        else if (!(proj.set.bt & PROJ_BT_SMC_CENTRAL)) smc_reason = 3;
        else if (!!(msg.n & SMC_EVENT_USER) !=
                 !!(proj.set.bt & PROJ_BT_SMC_USER)) {
            /* Stale frame from the previous preset, even if its bytes would
             * also be valid in this one. Preserve the new preset's queue. */
            smc_reason = 4;
        } else {
            if (allow && !perf_testing()) smc_dispatched++;
            smc_reason = smc_message(msg.b,
                                     (uint8_t)(msg.n & SMC_EVENT_LEN_MASK), allow);
        }
    }
    if (ble_lost != ble_seen && smc_ble_solo_release_seen == ble_seen) {
        smc_ble_solo_release_seen = ble_lost;
        smc_solo_clear();
        smc.nav_down = 0;
        smc.play_down = smc.select_down = smc.pattern_down = 0;
    }
    if (pause_lost != pause_seen && smc_pause_seen[smc_peer] == pause_seen) {
        smc_pause_seen[smc_peer] = pause_lost;
        smc.lock_down = 0;
    }
    if (perf_testing() || ui.overlay == O_ASK) smc_solo_clear();
}
void smc_poll(void)
{
    uint8_t saved = smc_peer;
    int allow = !perf_testing();
    if (ui_project_busy()) {
        smc_project_reset();
        return;
    }
    for (uint8_t peer = 0; peer < SMC_DIRECT_PEERS; peer++) {
        smc_select(peer);
        smc_poll_peer(allow);
    }
    /* Hosted USB/peripheral BLE cannot identify physical controllers. Give
     * this explicitly shared source its own state; never alias it to peer 0. */
    smc_select(SMC_DIRECT_PEERS);
    uint32_t host_lost = smc_host_solo_release_lost;
    uint32_t host_seen = smc_host_solo_release_seen;
    uint32_t pause_lost = smc_pause_lost[SMC_DIRECT_PEERS];
    uint32_t pause_seen = smc_pause_seen[SMC_DIRECT_PEERS];
    uint8_t r, count;
    smc_context_prepare(allow);
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
            /* A STOP callback can change projects and discard this ring.
             * Do not read a stale slot or move the reset reader past its
             * writer after smc_mmc_drain_stop() returns. */
            r = smc_mmc_r[src];
            smc_mmc_drain_stop(src, allow);
            if (smc_mmc_r[src] != r) break;
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
    if (host_lost != host_seen && smc_host_solo_release_seen == host_seen) {
        smc_host_solo_release_seen = host_lost;
        smc_solo_clear();
        smc.nav_down = 0;
        smc.play_down = smc.select_down = smc.pattern_down = 0;
    }
    if (pause_lost != pause_seen && smc_pause_seen[SMC_DIRECT_PEERS] == pause_seen) {
        smc_pause_seen[SMC_DIRECT_PEERS] = pause_lost;
        smc.lock_down = 0;
    }
    if (perf_testing() || ui.overlay == O_ASK) smc_solo_clear();
    smc_select(saved);
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
            if (row == 1) return !!(smc_drum_solo_all[kit] & (1u << track));
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
            if (row == 1) return !!(smc_solo_all & (1u << strip));
            if (row == 2) return part_muted(strip);
            return ui.part == strip;
        }
        /* Strips 6–8: M = mute/zero. R8/S6/S7/S8 are handled above; SELECT 6/7/8
         * open reverb, delay and master (drum SELECT was handled above). */
        if (row == 2) return !!(engine_fx_muted() & (1u << (strip - 5)));
        return (strip == 5 && ui.view == V_FX && cur_page() == 0) ||
               (strip == 6 && ui.view == V_FX && cur_page() == 1) ||
               (strip == 7 && ui.view == V_MIX && cur_page() == 3);
    }
    /* Existing Mackie NoteOn feedback route; physical LED identity still
     * needs native owner verification for the captured preset. */
    if (note == 0x5D) return smc.locked || ((plat_ms() >> 9) & 1u);
    if (note == 0x5E) return seq.playing;
    if (note == 0x5F) return ui.rec;
    return 0;
}

static void smc_frame_peer(void)
{
    static const uint8_t global[11] = {0x2E, 0x2F, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F, 0x60, 0x61, 0x62, 0x63};
    uint8_t msg[3];
    int i, n, want;
    /* TIMER5 can disconnect/reconnect while the BLE transmit queue accepts a
     * frame. Only acknowledge feedback delivered during this same link epoch;
     * otherwise a former session can consume a fresh session's LED/pitch state. */
    uint32_t link = smc_link_published;
    if (ui_project_busy() || !smc.up || !(link & 1u) ||
        link != smc_link_seen || !(proj.set.bt & PROJ_BT_SMC_CENTRAL))
        return;
    for (i = 0; i < 8; i++) smc_notice(i);
    for (i = 0; i < 43; i++) {
        n = i < 32 ? i : global[i - 32];
        want = smc_led(n);
        if (smc.known[n] && smc.lit[n] == want) continue;
        msg[0] = 0x90;
        msg[1] = (uint8_t)n;
        msg[2] = (uint8_t)(want ? 127 : 0);
        if (smc_link_published != link || !plat_ble_smc_send_peer(smc_peer, msg, 3)) return;
        if (smc_link_published != link) return;
        smc.known[n] = 1;
        smc.lit[n] = (uint8_t)want;
    }
    for (i = 0; i < 8; i++)
        if (smc.pending & (1u << i)) {
            msg[0] = (uint8_t)(0xE0u + i);
            msg[1] = smc.value[i];
            msg[2] = smc.value[i];
            if (smc_link_published != link || !plat_ble_smc_send_peer(smc_peer, msg, 3)) return;
            if (smc_link_published != link) return;
            smc.pending &= (uint8_t)~(1u << i);
        }
}

void smc_frame(void)
{
    uint8_t saved = smc_peer;
    for (uint8_t peer = 0; peer < SMC_DIRECT_PEERS; peer++) {
        smc_select(peer);
        smc_frame_peer();
    }
    smc_select(saved);
}
