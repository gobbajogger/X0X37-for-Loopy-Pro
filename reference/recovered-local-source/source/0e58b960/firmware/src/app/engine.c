/* SPDX-License-Identifier: GPL-3.0-only */
/* X0X audio engine; see engine.h. */
#include "engine.h"
#include "plat.h"
#include "../x0x_cold.h"
#include "../dsp/drum909.h"
#include "../dsp/drum808.h"
#include "../dsp/bass303.h"
#if X0X_DEV_WOBBLE || X0X_BASS_TAGGED_SOUND
#include "../dsp/synth_engine.h"
#endif
#if X0X_DEV_WOBBLE
#include "../dsp/wobble_backend.h"
#include "wobble_ui.h"
#endif
#include "../dsp/breaks.h"
#include "../dsp/fxbus.h"
#include "../dsp/master.h"
#include "../dsp/fastmath.h"
#include "../dsp/busfx.h"
#include "smc_fx_map.h"
#include "../dsp/ima_adpcm.h"              /* shared IMA tables + ima_decode (outline) */
#include "chicago_arp.h"
#include "chicago_fx.h"
#include "chicago_crush.h"
#include "chicago_ring.h"
#include "chicago_eq.h"
#include "chicago_sdly.h"
#include "chicago_bpmdly.h"
#include "chicago_hall.h"
#include "chicago_cho.h"
#include "chicago_ens.h"
#include "chicago_phaser.h"
#include "chicago_gaterev.h"
#include "chicago_aux.h"
#if X0X_BASS_PRESET_PREVIEW
#include "bass_presets.h"
#endif

#define POOL X0X_POOL

#if X0X_DEV_WOBBLE
static wobble_t dev_wobble POOL;
static synth_slot_t bass_slot[NBASS] POOL;
#endif

/* time per stage of the render, in plat_cycles() units (the CPU's cycles, or the 24 MHz timer):
 * running totals (wrapping; the PERF screen takes differences) and the frames rendered. */
volatile uint32_t eng_prof[ENG_PROF_N], eng_prof_frames;
#define PROF_T(v) uint32_t v = plat_cycles()
#define PROF_ADD(k, t0) (eng_prof[k] += plat_cycles() - (t0))

/* ---------------------------------------------------------------- state --- */
seq_t seq;
volatile uint8_t eng_step[NTRACKS];
volatile uint16_t eng_peak[NPARTS];
volatile uint8_t eng_hit[NKIT][NDRUM];
int16_t eng_scope[SCOPE_N] X0X_POOL;
volatile uint32_t eng_scope_w;

static drum909_t d909 POOL;
static drum808_t d808 POOL;
static bass303_t b303[NBASS] POOL;
/* Chicago Arp: live-keyboard only. Pattern sink (s_bon) never reads this. */
static uint8_t chi_arp_on[NBASS] POOL, chi_arp_mode[NBASS] POOL, chi_arp_range[NBASS] POOL, chi_arp_speed[NBASS] POOL;
static uint8_t chi_arp_accent[NBASS] POOL, chi_arp_vel[NBASS] POOL;
static chi_arp_run_t chi_arp_run[NBASS] POOL;
static uint8_t bass_held[NBASS], bass_held_count[NBASS], bass_held_src[NBASS];
/* Eligible physical Free Play has its own bounded FIFO. Epoch validation is
 * audio-side: a queued old release cannot affect a newer gate or ARP mode. */
#define LIVE_KEY_Q 32u
typedef struct { uint32_t generation, word; } live_key_command_t;
typedef struct { uint32_t generation, context; uint8_t gate, token, pitch, arp, blocked; } live_key_owner_t;
static live_key_command_t live_key_q[LIVE_KEY_Q] POOL;
static live_key_owner_t live_key_owner[NBASS] POOL;
static volatile uint8_t live_key_w, live_key_r, live_key_cancel;
static void live_key_retire(int voice,int release);
static void live_key_context_retire(int voice,int release);
static void live_key_drain(void);

void engine_live_key_state(int voice,uint32_t *generation,int *arp,uint32_t *context)
{
    if((unsigned)voice>=NBASS) { *generation=0;*arp=0;if(context)*context=0;return; }
    uint32_t flags=plat_cq_lock();
    *generation=live_key_owner[voice].generation;
    if(context)*context=live_key_owner[voice].context;
    *arp=chi_arp_on[voice];
#if X0X_DEV_WOBBLE
    if(bass_slot[voice].active.ops==&wobble_foundation_ops) *arp=0;
#endif
    if(live_key_owner[voice].blocked)*arp=-1; /* fail closed until cold boot */
    plat_cq_unlock(flags);
}
void engine_live_key_cancel(int voice)
{
    if((unsigned)voice>=NBASS)return;
    uint32_t flags=plat_cq_lock();
    live_key_cancel|=(uint8_t)(1u<<voice);
    plat_cq_unlock(flags);
}
int engine_live_key(int voice,uint32_t generation,int token,int pitch,int velocity,int accent,int slide)
{
    if((unsigned)voice>=NBASS || token<1 || token>37 || pitch<1 || pitch>127 || velocity<0 || velocity>127)return 0;
    uint32_t flags=plat_cq_lock();
    if(live_key_owner[voice].blocked){plat_cq_unlock(flags);return 0;}
    unsigned w=live_key_w,next=(w+1u)%LIVE_KEY_Q;
    if(next==live_key_r) {
        live_key_cancel|=(uint8_t)(1u<<voice);plat_cq_unlock(flags);return 0;
    }
    live_key_q[w].generation=generation;
    live_key_q[w].word=(uint32_t)voice|((uint32_t)token<<1)|((uint32_t)pitch<<7)|
        ((uint32_t)velocity<<14)|((uint32_t)!!accent<<21)|((uint32_t)!!slide<<22);
    __asm__ volatile("" ::: "memory");live_key_w=(uint8_t)next;
    plat_cq_unlock(flags);return 1;
}

/* Chicago SEND FX pots (sound_t) + one insert DSP per 303 voice.
 * Fx Type is mutually exclusive — keep one union slot (was ~43 KB of parallel structs). */
static uint8_t chi_fx_type[NBASS] POOL, chi_fx_e1[NBASS] POOL, chi_fx_e2[NBASS] POOL;
typedef union {
    chi_crush_t crush;
    chi_ring_t ring;
    chi_eq_t eq;
    chi_cho_t cho;
    chi_ens_t ens;
    chi_phaser_t phaser;
    chi_sdly_t sdly;
    chi_bpmdly_t bpmdly;
    chi_hall_t hall;
    chi_gaterev_t gaterev;
} chi_fx_insert_t;
static chi_fx_insert_t chi_fx[NBASS] POOL;
static void chi_fx_insert_program(int v)
{
    uint8_t t = chi_fx_type[v], e1 = chi_fx_e1[v], e2 = chi_fx_e2[v];
    if (chi_fx_is_crush_insert(t)) {
        chi_crush_init(&chi_fx[v].crush);
        chi_crush_set(&chi_fx[v].crush, e1, e2);
    } else if (chi_fx_is_ring_insert(t)) {
        chi_ring_init(&chi_fx[v].ring);
        chi_ring_set(&chi_fx[v].ring, e1, e2, BASS303_SR);
    } else if (chi_fx_is_eq_insert(t)) {
        chi_eq_init(&chi_fx[v].eq);
        chi_eq_set(&chi_fx[v].eq, e1, e2, BASS303_SR);
    } else if (chi_fx_is_cho_insert(t)) {
        chi_cho_init(&chi_fx[v].cho);
        chi_cho_set(&chi_fx[v].cho, e1, e2, BASS303_SR);
    } else if (chi_fx_is_ens_insert(t)) {
        chi_ens_init(&chi_fx[v].ens);
        chi_ens_set(&chi_fx[v].ens, e1, e2, BASS303_SR);
    } else if (chi_fx_is_phaser_insert(t)) {
        chi_phaser_init(&chi_fx[v].phaser);
        chi_phaser_set(&chi_fx[v].phaser, e1, e2, BASS303_SR);
    } else if (chi_fx_is_sdly_insert(t)) {
        chi_sdly_init(&chi_fx[v].sdly);
        chi_sdly_set(&chi_fx[v].sdly, e1, e2, BASS303_SR);
    } else if (chi_fx_is_bpmdly_insert(t)) {
        chi_bpmdly_init(&chi_fx[v].bpmdly);
        chi_bpmdly_set(&chi_fx[v].bpmdly, e1, e2, BASS303_SR);
    } else if (chi_fx_is_hall_insert(t)) {
        chi_hall_init(&chi_fx[v].hall);
        chi_hall_set(&chi_fx[v].hall, e1, e2, BASS303_SR);
    } else if (chi_fx_is_gaterev_insert(t)) {
        chi_gaterev_init(&chi_fx[v].gaterev);
        chi_gaterev_set(&chi_fx[v].gaterev, e1, e2, BASS303_SR);
    }
}

/* One insert-process switch for both bass slots (NBASS unrolls under -O2). */
static void __attribute__((noinline)) chi_fx_process(int b, float *buf, int n)
{
    uint8_t t = chi_fx_type[b];
    if (chi_fx_is_crush_insert(t))
        chi_crush_process(&chi_fx[b].crush, buf, n);
    else if (chi_fx_is_ring_insert(t))
        chi_ring_process(&chi_fx[b].ring, buf, n);
    else if (chi_fx_is_eq_insert(t))
        chi_eq_process(&chi_fx[b].eq, buf, n);
    else if (chi_fx_is_cho_insert(t))
        chi_cho_process(&chi_fx[b].cho, buf, n);
    else if (chi_fx_is_ens_insert(t))
        chi_ens_process(&chi_fx[b].ens, buf, n);
    else if (chi_fx_is_phaser_insert(t))
        chi_phaser_process(&chi_fx[b].phaser, buf, n);
    else if (chi_fx_is_sdly_insert(t))
        chi_sdly_process(&chi_fx[b].sdly, buf, n);
    else if (chi_fx_is_bpmdly_insert(t))
        chi_bpmdly_process(&chi_fx[b].bpmdly, buf, n);
    else if (chi_fx_is_hall_insert(t))
        chi_hall_process(&chi_fx[b].hall, buf, n);
    else if (chi_fx_is_gaterev_insert(t))
        chi_gaterev_process(&chi_fx[b].gaterev, buf, n);
}
/* Chicago damper: live/MIDI sustain. Pattern sink ignores it. */
static uint8_t chi_damper[NBASS] POOL;
static uint8_t chi_damp_latched[NBASS] POOL; /* note-off deferred while damper on */
static breaks_t brk POOL;
static fxbus_t fx POOL;
static volatile uint8_t tape_signal_seen;
static volatile uint32_t tape_signal_at;
uint8_t engine_tape_active(void)
{
    uint32_t at = tape_signal_at; /* snapshot before clock: an IRQ may publish a newer tick */
    return tape_signal_seen && (uint32_t)(plat_ms() - at) < 80u;
}
void engine_fx_mute(uint8_t mask) { fx.muted = mask & 7u; }
uint8_t engine_fx_muted(void) { return fx.muted; }
static master_t mst POOL; /* was BSS; pool after Chicago-insert union frees headroom */
/* Mix delay ring: 1.80 s @ 44.1 kHz (was 2.00 s / 88200). Recovers pool for product WBL1 + ≥8192 free. */
#define DLY_LEN 79200
static int16_t dly_buf[DLY_LEN] POOL;
static volatile uint8_t project_hold, project_midi_drop;
void engine_project_hold(int hold)
{
    __asm__ volatile("" ::: "memory");
    project_hold = (uint8_t)(hold != 0);
    __asm__ volatile("" ::: "memory");
}
void engine_project_release(void)
{
    project_midi_drop = 1;             /* no MIDI received during the transaction starts the new project */
    engine_project_hold(0);
}
static motion_t mot;
static const sound_t *mot_base_snd;    /* the knobs' own values (the project's mirror) */
static uint8_t mot_playing;
static volatile uint8_t mot_on = 1;
static uint8_t mot_was_on = 1;
void engine_motion_enable(int on) { mot_on = (uint8_t)(on != 0); }   /* the ISR releases the knobs */
static volatile uint8_t mot_release_req;
void engine_motion_reset(void) { mot_release_req = 1; }   /* lanes changed under it (undo): knobs back */

/* overload guard: past the CPU (a block over 92%, or 85% on average) the two 303s drop their
 * oversampling (~12% of the CPU: a little aliasing on bright notes) and the 808's voices end at
 * -50 dB instead of -90 (~2%); after 3 s back under 65% both return. The platform reports each
 * block's load after rendering it (engine_load, same context as engine_render); the change is
 * made at the start of the next block. Off during the PERF test, which measures the full cost. */
#define GUARD_ON_LAST 92u
#define GUARD_ON_AVG 85u
#define GUARD_OFF 65u
#define GUARD_CALM_FRAMES (3u * 44100u)
extern float drum808_quiet;
static uint8_t guard_on, guard_applied, guard_enabled = 1;
static uint32_t guard_avg_q8, guard_calm;
volatile uint32_t eng_guard_count;
void engine_guard_enable(int on) { guard_enabled = (uint8_t)(on != 0); }
int engine_guard_active(void) { return guard_on; }
void engine_load(uint32_t pct, uint32_t frames)
{
    guard_avg_q8 = (guard_avg_q8 * 15u + pct * 256u) / 16u;
    if (!guard_enabled) {
        guard_on = 0;
        guard_calm = 0;
    } else if (!guard_on) {
        if (pct > GUARD_ON_LAST || guard_avg_q8 > GUARD_ON_AVG * 256u) {
            guard_on = 1;
            guard_calm = 0;
            eng_guard_count++;
        }
    } else if (pct >= GUARD_OFF || guard_avg_q8 >= GUARD_OFF * 256u) {
        guard_calm = 0;
    } else if ((guard_calm += frames) >= GUARD_CALM_FRAMES) {
        guard_on = 0;
    }
}
static void guard_apply(void)
{
    int k;
    if (guard_on == guard_applied)
        return;
    guard_applied = guard_on;
    for (k = 0; k < NBASS; k++)
#if X0X_DEV_WOBBLE
        synth_lite(&bass_slot[k], guard_on);
#else
        bass303_set_lite(&b303[k], guard_on);
#endif
    drum808_quiet = guard_on ? 3.2e-3f : 3.2e-5f;
}

/* mixer: per part Level, Rev send, Dly send (a drum machine's add to its voices' own sends) */
static const x0x_param_t MIX_P[MX_NPARAMS] = {{"LEVEL", 127, 100, 0}, {"REV", 127, 0, 0}, {"DLY", 127, 0, 0},
                                               {"PAN", 127, 64, 0}, {"CUT", 127, 127, 0}};
static const char *const PART_NAMES[NPARTS] = {"909", "808", "303 A", "303 B", "BREAK"};
static float mix_level[NPARTS], mix_rev[NPARTS], mix_dly[NPARTS], mix_cut[NPARTS];
/* Momentary bus FX, snapshotted once per stretch so both cores see the same hold. */
static volatile uint8_t bfx_down_pub;
static volatile uint8_t bfx_fx_pub[SMCFX_N];
static uint8_t bfx_hold[NPARTS], bfx_eff[NPARTS];
static busfx_t bfx_st[NPARTS];
/* Send throws and master filter use the same busfx_t + busfx_apply as the parts
 * (one outlined body). Extra channel slots stay unused for nch=1 / nch=2. */
static busfx_t bfx_send_st[2] X0X_POOL;
static busfx_t bfx_flt_st;
static busfx_stut_hub_t bfx_stut_hub X0X_POOL;

enum {
    BFX_STUT_SEND_RVB = 0,
    BFX_STUT_SEND_DLY = 1,
    BFX_STUT_909 = 2,
    BFX_STUT_808 = 6,
    BFX_STUT_FLT = 10
};

static int bfx_part_stut_base(int part)
{
    if (part == PART_909)
        return BFX_STUT_909;
    if (part == PART_808)
        return BFX_STUT_808;
    return -1;
}
static float bfx_bpm_snap;
static uint32_t bfx_phase_snap;
static uint8_t bfx_send_hold[2], bfx_send_eff[2];
static uint8_t bfx_flt_hold, bfx_flt_eff;
/* Main-loop publication is one aligned word; each render stretch snapshots it
 * once for both audio cores. This never changes the sequencer's saved mutes. */
static volatile uint32_t smc_solo_state;
void engine_smc_solo_targets(uint8_t parts, uint16_t kit909, uint16_t kit808)
{
    smc_solo_state = (uint32_t)(parts & 0x1Fu) | (uint32_t)(kit909 & 0x7FFu) << 5 |
                     (uint32_t)(kit808 & 0x7FFu) << 16;
}
void engine_smc_solo(uint8_t mask) { engine_smc_solo_targets(mask, 0, 0); }
/* SMC main-loop publish — render reads the published words (X0X_COLD) */
X0X_COLD void engine_smc_busfx(uint8_t down, const uint8_t effect[8])
{
    int i;
    bfx_down_pub = down;
    if (!effect)
        return;
    for (i = 0; i < SMCFX_N; i++)
        bfx_fx_pub[i] = effect[i];
}

static uint16_t smc_audible_drums(uint32_t solo, int kit)
{
    /* Whole-part Solo alone retains its existing part-gain glide. A whole kit
     * in the union also includes all of its voices beside any drum targets. */
    return !(solo & ~0x1Fu) || (solo & (1u << kit)) ? 0x7FFu :
           (uint16_t)(solo >> (5 + kit * 11) & 0x7FFu);
}
/* pan: a balance law. Centre is full level on both sides (a centred mix is the mono one exactly);
 * turning toward a side fades the other out along a quarter cosine, the near side stays at 1 */
static float mix_pl[NPARTS] = {1, 1, 1, 1, 1}, mix_pr[NPARTS] = {1, 1, 1, 1, 1};

/* ------------------------------------------------------- command queue --- */
/* uint32 commands, single producer (main loop) / single consumer (audio ISR):
 * type:4 | a:8 | b:8 | c:12 */
/* C_REC reservation/priority matches the separately reviewed atomic REC unit. */
enum { C_PARAM_T0 = 0, C_DRUM = 8, C_BON, C_BOFF, C_BRK, C_NOP, C_REC };
#define CQ 1024u
static uint32_t cq[CQ];
static volatile uint32_t cq_w, cq_r;

/* The scan may be preempted after choosing a victim. Revalidate the slot only
 * while the consumer source is masked, then publish exactly one aligned word.
 * Scanning the 1024-entry queue never runs with the audio source masked. */
static int cq_replace(uint32_t i, uint32_t w, uint32_t word)
{
    uint32_t state = plat_cq_lock();
    uint32_t r = cq_r;
    int live = (i - r < w - r);
    if (live)
        cq[i % CQ] = word;
    plat_cq_unlock(state);
    return live;
}

__attribute__((noinline)) static void cq_put(uint32_t type, uint32_t a, uint32_t b, uint32_t c)
{
    uint32_t w = cq_w;
    uint32_t word = (type << 28) | ((a & 0xFFu) << 20) | ((b & 0xFFu) << 12) | (c & 0xFFFu);
retry:
    if (w - cq_r >= CQ) {
        /* Full: params may drop (mirror still has them). Never drop C_BON,
         * C_BOFF, C_BRK, or C_DRUM — missing a live drum hit or break press
         * under param flood is a silent loss; a retained release without its
         * press is rare but must not displace an already-queued release. */
        uint32_t i, keep = (type == C_BON) || (type == C_BOFF) || (type == C_BRK) || (type == C_DRUM) || (type == C_REC);
        if (!keep)
            return;
        uint32_t r = cq_r;
        for (i = r; i != w; i++) {
            uint32_t t = cq[i % CQ] >> 28;
            if (t < C_DRUM || t == C_DRUM) {
                if (cq_replace(i, w, word)) return;
                goto retry;
            }
        }
        for (i = cq_r; i != w; i++) {
            uint32_t tw = cq[i % CQ], t = tw >> 28, bb = (tw >> 12) & 0xFFu;
            if (t != C_BOFF && !(t == C_BRK && bb == 0u)) {
                if (cq_replace(i, w, word)) return;
                goto retry;
            }
        }
        if (cq_r != r) goto retry; /* consumer freed room during the scan */
        return;
    }
    cq[w % CQ] = word;
    __asm__ volatile("" ::: "memory");
    cq_w = w + 1;
}

/* ------------------------------------------------------------ params --- */
#if X0X_DEV_WOBBLE
static int bass_wobble_voice(int v)
{
    return (unsigned)v < NBASS && bass_slot[v].active.ops == &wobble_foundation_ops;
}
#endif
int engine_nvoices(int t)
{
    switch (t) {
    case T_909: return DR_NUM + 1;
    case T_808: return D8_NUM + 1;
    case T_303: return NBASS;
    case T_MIX: return NPARTS;
    default: return 1;
    }
}

int engine_nparams(int t, int v)
{
    switch (t) {
    case T_909: return drum909_nparams(v);
    case T_808: return drum808_nparams(v);
    case T_303:
#if X0X_BASS_PRESET_PREVIEW
        return engine_bass_bank(v) == FACTORY_BANK_MIAMI ? WOBBLE_EXPERIMENT_NPARAMS : CHI_AUX_END;
#endif
#if X0X_DEV_WOBBLE
        if (bass_wobble_voice(v))
            return WOBBLE_EXPERIMENT_NPARAMS;           /* Miami controls only; no phantom Chicago pots */
#endif
        return CHI_AUX_END;                             /* Chicago Arp / FX / bend and damper */
    case T_BRK: return 2;                                /* Level, Pitch (sends: the mixer's; the rest: the pattern's) */
    case T_FX: return fxbus_nparams();
    case T_MIX: return MX_NPARAMS;                       /* every part: Level, Rev send, Dly send */
    case T_MST: return master_nparams();
    default: return 0;
    }
}

/* Pot metadata for the UI / project — not the audio render (X0X_COLD). */
X0X_COLD const x0x_param_t *engine_param(int t, int v, int i)
{
    switch (t) {
    case T_909: return drum909_param(v, i);
    case T_808: return drum808_param(v, i);
    case T_303:
#if X0X_BASS_PRESET_PREVIEW
        return engine_bass_bank_param(engine_bass_bank(v), i);
#endif
#if X0X_DEV_WOBBLE
        if (bass_wobble_voice(v))
            return wobble_param(i);                     /* Do not fall through into Chicago metadata */
#endif
        if (i >= CHI_ARP_SWITCH && i < CHI_ARP_NPARAMS_END)
            return &CHI_ARP_PARAMS[i - CHI_ARP_SWITCH];
        if (i >= CHI_FX_TYPE && i < CHI_FX_END)
            return &CHI_FX_PARAMS[i - CHI_FX_TYPE];
        if (i >= CHI_PBEND && i < CHI_AUX_END)
            return &CHI_AUX_PARAMS[i - CHI_PBEND];
        return bass303_param(i);
    case T_BRK: return breaks_param(BRK_NSET + i);
    case T_FX: return fxbus_param(i);
    case T_MIX: return (i >= 0 && i < MX_NPARAMS) ? &MIX_P[i] : 0;
    case T_MST: return master_param(i);
    default: return 0;
    }
}

static const char *const N909[DR_NUM + 1] = {"BD", "SD", "LT", "MT", "HT", "RS", "CP", "CH", "OH", "CR", "RD", "KIT"};
static const char *const N808[D8_NUM + 1] = {"BD", "SD", "LT", "MT", "HT", "RS", "CP", "CB", "CY", "OH", "CH", "KIT"};

const char *engine_voice_name(int t, int v)
{
    switch (t) {
    case T_909: return (v >= 0 && v <= DR_NUM) ? N909[v] : "";
    case T_808: return (v >= 0 && v <= D8_NUM) ? N808[v] : "";
    case T_303: return v ? "303 B" : "303 A";
    case T_MIX: return (v >= 0 && v < NPARTS) ? PART_NAMES[v] : "";
    case T_BRK: return "BREAK";
    case T_FX: return "FX";
    case T_MST: return "MASTER";
    default: return "";
    }
}

#if X0X_DEV_WOBBLE
int engine_bass_is_wobble(int voice) { return bass_wobble_voice(voice); }
static void __attribute__((noinline)) bass_voice_note_on(int v, int note, int acc, int slide, int vel)
{
    if (bass_wobble_voice(v))
        synth_note(&bass_slot[v], note, acc, slide, vel);
    else
        bass303_note_on_velocity(&b303[v], note, acc, slide, vel);
}
static void __attribute__((noinline)) bass_voice_note_off(int v)
{
    if (bass_wobble_voice(v))
        synth_off(&bass_slot[v]);
    else
        bass303_note_off(&b303[v]);
}
#endif

/* Live/MIDI note-off: damper sustains the gate; pattern s_boff never calls this. */
__attribute__((noinline)) static void chi_live_note_off(int v)
{
    if ((unsigned)v >= NBASS)
        return;
#if X0X_DEV_WOBBLE
    if (bass_wobble_voice(v)) {
        /* Only live/MIDI release may defer on Miami's own damper. Pattern
         * s_boff and transport panic retain their unconditional gate-off. */
        wobble_t *w = (wobble_t *)bass_slot[v].active.context;
        chi_damp_latched[v] = 0;
        wobble_note_off(w, w->note);
        return;
    }
#endif
    if (chi_damper_on(chi_damper[v])) {
        chi_damp_latched[v] = 1;
        return;
    }
    chi_damp_latched[v] = 0;
#if X0X_DEV_WOBBLE
    bass_voice_note_off(v);
#else
    bass303_note_off(&b303[v]);
#endif
}

/* Called by audio only, before effective-mode changes or another source's
 * gate write. Do not issue an old keyboard off against the new owner's gate. */
static void live_key_retire(int v,int release)
{
    live_key_owner_t *o=&live_key_owner[v];
    if(release && o->gate==1) chi_live_note_off(v);
    if(o->arp) chi_arp_run_clear(&chi_arp_run[v]);
    if(o->token || o->arp) {
        bass_held[v]=bass_held_count[v]=0;
    }
    o->gate=o->token=o->pitch=o->arp=0;
    /* Never recycle a capability or discard another lane's queued release.
     * Saturation blocks this lane until cold boot; warm reset does not rearm. */
    if(o->generation==UINT32_MAX)o->blocked=1;
    else o->generation++;
}

static void live_key_context_retire(int v,int release)
{
    live_key_retire(v,release);
    if(live_key_owner[v].context==UINT32_MAX)live_key_owner[v].blocked=1;
    else live_key_owner[v].context++;
}

static void chi_arp_gate_off(int v)
{
    if ((unsigned)v >= NBASS)
        return;
    if (chi_arp_run[v].cur >= 0) {
        if(!live_key_owner[v].arp || live_key_owner[v].gate==1) chi_live_note_off(v);
        chi_arp_run[v].cur = -1;
    }
}

/* Gate off + clear run — shared by SWITCH off and drain C_BOFF empty/arp-on. */
__attribute__((noinline)) static void chi_arp_halt(int v)
{
    chi_arp_gate_off(v);
    chi_arp_run_clear(&chi_arp_run[v]);
}

static void chi_aux_apply(int v, int i, int val)
{
    if ((unsigned)v >= NBASS)
        return;
    if (val < 0)
        val = 0;
    if (val > 127)
        val = 127;
    if (i == CHI_PBEND) {
        bass303_set_bend(&b303[v], val);
        return;
    }
    if (i == CHI_DAMPER) {
        uint8_t was = chi_damper[v];
        chi_damper[v] = (uint8_t)val;
        /* Pedal up releases only a live/MIDI deferred note — latch is cleared
         * on every pattern/arp note-on so this cannot cut a sequencer gate. */
        if (chi_damper_on(was) && !chi_damper_on((uint8_t)val) && chi_damp_latched[v]) {
#if X0X_DEV_WOBBLE
            synth_off(&bass_slot[v]);
#else
            bass303_note_off(&b303[v]);
#endif
            chi_damp_latched[v] = 0;
        }
    }
}

typedef struct { int part; } chi_arp_step_ctx_t;
static void chi_arp_on_step(int note, void *ctx)
{
    int b = ((chi_arp_step_ctx_t *)ctx)->part;
    if(live_key_owner[b].arp) live_key_owner[b].gate=1;
    else live_key_retire(b,0);
    if ((unsigned)b < NBASS)
        chi_damp_latched[b] = 0; /* arp step owns the gate */
    bass303_note_on_velocity(&b303[b], note, chi_arp_accent[b], 0,
                             chi_arp_vel[b] ? chi_arp_vel[b] : 100);
}

/* Advance live Chicago arp clocks; pattern sink is untouched. */
static void chi_arp_tick(uint32_t samples)
{
    int b;
    float bpm = seq_tempo(&seq);
    for (b = 0; b < NBASS; b++) {
        chi_arp_step_ctx_t ctx;
        if (!chi_arp_on[b] || !chi_arp_run[b].n)
            continue;
        ctx.part = b;
        chi_arp_run_process(&chi_arp_run[b], samples, bpm, 44100.0f, chi_arp_on_step, &ctx);
    }
}

static void chi_arp_apply_cfg(int v, int i, int val)
{
    if ((unsigned)v >= NBASS)
        return;
    if (val < 0)
        val = 0;
    switch (i) {
    case CHI_ARP_SWITCH:
        if (val > 1)
            val = 1;
        if(val!=chi_arp_on[v]) live_key_context_retire(v,1);
        if (!val && chi_arp_on[v]) {
            chi_arp_halt(v);
        }
        chi_arp_on[v] = (uint8_t)val;
        break;
    case CHI_ARP_MODE:
        if (val >= CHI_ARP_NMODE)
            val = CHI_ARP_NMODE - 1;
        chi_arp_mode[v] = (uint8_t)val;
        chi_arp_run[v].mode = (uint8_t)val;
        break;
    case CHI_ARP_RANGE:
        if (val >= CHI_ARP_NRANGE)
            val = CHI_ARP_NRANGE - 1;
        chi_arp_range[v] = (uint8_t)val;
        chi_arp_run[v].range = (uint8_t)val;
        break;
    case CHI_ARP_SPEED:
        if (val >= CHI_ARP_NSPEED)
            val = CHI_ARP_NSPEED - 1;
        chi_arp_speed[v] = (uint8_t)val;
        chi_arp_run[v].speed = (uint8_t)val;
        break;
    default:
        break;
    }
}

static void apply_param(int t, int v, int i, int val)    /* ISR */
{
    switch (t) {
    case T_909: drum909_set(&d909, v, i, val); break;
    case T_808: drum808_set(&d808, v, i, val); break;
    case T_303:
        if (v < 0 || v >= NBASS)
            break;
#if X0X_BASS_TAGGED_SOUND
        if(engine_bass_sound_bank(mot_base_snd,v)<0)break;
#endif
#if X0X_DEV_WOBBLE
        if (bass_wobble_voice(v)) {
            /* WBL1 indices overlap Chicago's private indices; never mutate
             * dormant native303/arp/FX on a Miami pot write. */
            wobble_foundation_set(bass_slot[v].active.context, i, val);
            break;
        }
#endif
        if (i >= CHI_ARP_SWITCH && i < CHI_ARP_NPARAMS_END)
            chi_arp_apply_cfg(v, i, val);
        else if (i >= CHI_FX_TYPE && i < CHI_FX_END) {
            /* sound_t mirror + Crush/Ring/EQ/Cho/Ens/Phaser/SDly/BPM/Hall/GateRev insert coeffs; Mix/fxbus via UI map. */
            if (i == CHI_FX_TYPE) {
                if (val >= CHI_FX_NTYPES)
                    val = CHI_FX_OFF;
                chi_fx_type[v] = (uint8_t)val;
            } else if (i == CHI_FX_EDIT1) {
                chi_fx_e1[v] = (uint8_t)val;
            } else if (i == CHI_FX_EDIT2) {
                chi_fx_e2[v] = (uint8_t)val;
            }
            chi_fx_insert_program(v);
        } else if (i >= CHI_PBEND && i < CHI_AUX_END)
            chi_aux_apply(v, i, val);
        else
            bass303_set(&b303[v], i, val);
        break;
    case T_BRK: breaks_set(&brk, BRK_NSET + i, val); break;
    case T_FX: fxbus_set(&fx, i, val); break;
    case T_MIX:
        if (v < 0 || v >= NPARTS)
            break;
        if (i == MX_LEVEL)
            mix_level[v] = (float)val * (float)val / (100.0f * 100.0f);     /* 100 = unity, square law */
        else if (i == MX_REV)
            mix_rev[v] = (float)val / 127.0f;
        else if (i == MX_DLY)
            mix_dly[v] = (float)val / 127.0f;
        else if (i == MX_PAN)
            x0x_pan_gains(val, &mix_pl[v], &mix_pr[v]);
        else if (i == MX_CUT)
            mix_cut[v] = (float)val;
        break;
    case T_MST: master_set(&mst, i, val); break;
    default: break;
    }
}

void engine_set(int t, int v, int i, int val) { cq_put((uint32_t)t, (uint32_t)v, (uint32_t)i, (uint32_t)val); }

/* the output's scale: Felucca's -6 dBFS ceiling of the 24-bit codec (2^22), or its full scale (LOUD); the
 * clamp keeps an over from wrapping (normal: the codec's own full scale, as before) */
static volatile float out_scale = 4194303.0f, out_lim = 2.0f;
void engine_out_loud(int on)
{
    out_scale = on ? 8388607.0f : 4194303.0f;
    out_lim = on ? 1.0f : 2.0f;
}

/* USB AUDIO FULL (#12): the render at full level, for the USB tap, and the MASTER knob after it, for
 * the speaker, headphones and line out (with the master's ~10 ms glide). The limiter then works at
 * full level too: turned down, the FM-1 plays what it sends, quieter. */
static volatile int usb_full;
static float vol_after = -1.0f;
void engine_usb_full(int on) { usb_full = on; }

void engine_volume(int32_t *out_lr, uint32_t n)
{
    float vol = (float)plat_master() / 4096.0f, v = vol_after < 0.0f ? vol : vol_after;
    uint32_t i;
    if (!usb_full) {
        vol_after = -1.0f;
        return;
    }
    for (i = 0; i < n; i++) {
        if (v != vol) {
            v += (vol - v) * 0.0022651f;
            if (fm_fabsf(vol - v) < 1e-5f)
                v = vol;
        }
        out_lr[2u * i] = (int32_t)((float)out_lr[2u * i] * v);
        out_lr[2u * i + 1u] = (int32_t)((float)out_lr[2u * i + 1u] * v);
    }
    vol_after = v;
}

#if X0X_BASS_TAGGED_SOUND
static void bass_tag_set(sound_t *,int,int,int,int);
static void bass_restore_sound(const sound_t *);
#endif
X0X_COLD void engine_sound_defaults(sound_t *s)
{
    int t, v, i;
    uint8_t *z = (uint8_t *)s;
    for (i = 0; i < (int)sizeof *s; i++)
        z[i] = 0;
    for (t = 0; t < NTARGETS; t++)
        for (v = 0; v < engine_nvoices(t) && v < NVOICES_MAX; v++)
            for (i = 0; i < engine_nparams(t, v) && i < NPARAMS_MAX; i++) {
                const x0x_param_t *p = engine_param(t, v, i);
                s->v[t][v][i] = p ? p->def : 0;
            }
#if X0X_BASS_TAGGED_SOUND
    for(v=0;v<NBASS;v++) {
        int bank=X0X_DEV_WOBBLE && v==0?1:0;
        for(i=0;i<(int)(bank?WOBBLE_EXPERIMENT_NPARAMS:CHI_AUX_END);i++)
            s->v[T_303][v][i]=engine_bass_bank_param(bank,i)->def;
        bass_tag_set(s,v,bank,BASS_SOUND_NO_PRESET,BASS_SOUND_EDITED);
    }
#endif
    /* the factory mix: big beat (Chemical Brothers, Fatboy Slim), balanced by tools/mix_report.py
     * against a funk break, a 909 kick under it and a 303 line. The break leads, the kick sits
     * under it, the 303 screams through its RAT; the master squeezes and pumps. Each engine's
     * own voicing (its pot defaults) is left as its original; only these are X0X's. */
    s->v[T_MIX][PART_909][MX_LEVEL] = 75;            /* 9W9's kit is hot (it drives its own glue): -5 dB */
    s->v[T_909][DR_BD][3] = 61;                      /* its kick under the break, its clap and hats over */
    s->v[T_909][DR_BD][2] = 80;                      /* the kick: a little shorter, and driven. Its Attack */
    s->v[T_909][DR_BD][6] = 30;                      /* stays 9W9's 13: at 50 its tick was 12 dB louder, a click on every kick */
    s->v[T_909][DR_OH][1] = 60;
    s->v[T_MIX][PART_808][MX_LEVEL] = 112;
    s->v[T_808][D8_KIT][0] = 127;                    /* kit level */
    s->v[T_BRK][0][0] = 127;                         /* the break: full level, part a little over unity */
    s->v[T_MIX][PART_BRK][MX_LEVEL] = 120;
    s->v[T_MIX][PART_BRK][MX_REV] = 25;
    for (v = 0; v < NBASS; v++) {                    /* acid: low cutoff, high resonance, a long sweep, RAT */
#if X0X_DEV_WOBBLE
        /* Apply Chicago defaults only to the selected Chicago parameter bank. */
        if (
#if X0X_BASS_TAGGED_SOUND
            engine_bass_sound_bank(s,v)==0
#else
            !bass_wobble_voice(v)
#endif
        ) {
#endif
        s->v[T_303][v][BASS303_CUTOFF] = 48;
        s->v[T_303][v][BASS303_RESO] = 100;
        s->v[T_303][v][BASS303_ENVMOD] = 85;
        s->v[T_303][v][BASS303_ACCENT] = 100;
        s->v[T_303][v][BASS303_VOLUME] = 127;
        s->v[T_303][v][BASS303_DRIVE] = 45;
        s->v[T_303][v][BASS303_DRVTYPE] = 2;
#if X0X_DEV_WOBBLE
        }
#endif
        s->v[T_MIX][PART_303A + v][MX_LEVEL] = 74;   /* the RAT makes it dense: -5 dB (square law) */
        s->v[T_MIX][PART_303A + v][MX_DLY] = v ? 60 : 80;
        s->v[T_MIX][PART_303A + v][MX_REV] = 12;
    }
    s->v[T_909][DR_CP][5] = 50;                      /* the clap in the room (90 drowned it) */
    s->v[T_909][DR_LT][1] = 92;                      /* the toms: shorter than 9W9's (its low tom rang */
    s->v[T_909][DR_MT][1] = 88;                      /* on for 1.7 s, which reads as reverb) */
    s->v[T_909][DR_HT][1] = 90;
    s->v[T_FX][0][FX_DL_TYPE] = 1;                   /* tape delay, dotted eighths */
    s->v[T_MST][0][MST_RATIO] = 4;                   /* 4:1 */
    s->v[T_MST][0][MST_THRESH] = 64;                 /* -24 dB */
    s->v[T_MST][0][MST_MAKEUP] = 85;                 /* +16 dB: into the limiter */
    s->v[T_MST][0][MST_ATTACK] = 85;                 /* 10 ms: faster grabbed each kick hard enough to click */
    s->v[T_MST][0][MST_RELEASE] = 58;                /* 100 ms: it breathes with the beat */
    s->v[T_MST][0][MST_PUMP] = 21;                   /* 4 dB of 909-keyed pump */
    s->v[T_MST][0][MST_COMP1] = 95;                  /* the one-knob COMP's nearest to the squash above */
    s->v[T_FX][1][0] = (uint8_t)(16 + s->v[T_FX][0][FX_DL_WEAR] * 111 / 127);   /* TAPE (ui.c R_TAPE): on, its wear */
}

X0X_COLD void engine_sound_centre_drum_pans(sound_t *s)
{
    int t, v;
    for (t = T_909; t <= T_808; t++)
        for (v = 0; v < NDRUM && v < NVOICES_MAX; v++) {
            int i = engine_nparams(t, v) - 1;           /* PAN is every voice's last pot */
            const x0x_param_t *p = engine_param(t, v, i);
            if (p && p->def == 64 && p->max == 127)
                s->v[t][v][i] = 64;
        }
}

X0X_COLD void engine_apply_sound(const sound_t *s)
{
    int t, v, i;
#if X0X_BASS_TAGGED_SOUND
    bass_restore_sound(s); /* called only at full-project initialization/adoption */
#endif
    engine_set(T_MST, 0, MST_COMP1, s->v[T_MST][0][MST_COMP1]);   /* the macros first: the pots they set */
    engine_set(T_MST, 0, MST_DJF, s->v[T_MST][0][MST_DJF]);       /* follow as saved (a project's own sound) */
    for (t = 0; t < NTARGETS; t++)
        for (v = 0; v < engine_nvoices(t) && v < NVOICES_MAX; v++)
            for (i = 0; i < engine_nparams(t, v) && i < NPARAMS_MAX; i++)
                if ((t != T_MST || i < MST_COMP1)
#if X0X_BASS_TAGGED_SOUND
                    && (t!=T_303 || engine_bass_sound_bank(s,v)>=0)
#endif
                    ) engine_set(t, v, i, s->v[t][v][i]);
}

void engine_drum(int kit, int voice, float vel)
{
    cq_put(C_DRUM, (uint32_t)kit, (uint32_t)voice, (uint32_t)(vel * 4095.0f));
}
void engine_bass_on(int part, int note, int accent, int slide)
{
    engine_bass_on_velocity(part, note, accent, slide, 100);
}
void engine_bass_on_velocity(int part, int note, int accent, int slide, int velocity)
{
    if (velocity < 1) velocity = 1;
    if (velocity > 127) velocity = 127;
    cq_put(C_BON, (uint32_t)part, (uint32_t)note,
           (uint32_t)((accent ? 1 : 0) | (slide ? 2 : 0) | (velocity << 2)));
}
void engine_bass_record(int part, int note, int velocity, int flags)
{
    if (velocity < 1) velocity = 1;
    if (velocity > 127) velocity = 127;
    cq_put(C_REC, (uint32_t)(part | ((seq.rec_epoch & 127u) << 1)), (uint32_t)note,
           (uint32_t)((flags & BS_ACCENT ? 1 : 0) | (velocity << 2) | (flags & BS_SLIDE ? 512 : 0)));
}
void engine_bass_off(int part) { cq_put(C_BOFF, (uint32_t)part, 0, 0); }
void engine_bass_off_note(int part, int note)
{
    if (note <= 0)
        engine_bass_off(part);
    else
        cq_put(C_BOFF, (uint32_t)part, (uint32_t)(note & 127), 1u);
}
void engine_brk_live(int key, int down) { cq_put(C_BRK, (uint32_t)key, (uint32_t)(down != 0), 0); }

/* ------------------------------------------------------- sequencer sink --- */
/* probability: each part's (and drum track's) chance on the step playing now, set by its T_PROB lane
 * before the step fires (motion_step runs first); 100 = always */
static uint8_t prob[NPARTS][NDRUM];
static uint32_t prob_rng = 0x2545F491u;
__attribute__((noinline)) static int prob_skip(int part, int v)                 /* ISR: 1 = this hit does not play */
{
    uint32_t p = prob[part][v];
    if (p > 100u && p < PROB_COND + PROB_NCOND) {      /* A:B: the A-th of every B passes (seq_track_t.bars) */
        int a, b;
        prob_cond((int)p - PROB_COND, &a, &b);
        return (int)(seq.t[part].bars % (uint32_t)b) != a - 1;
    }
    if (p >= 100u)
        return 0;
    prob_rng ^= prob_rng << 13;
    prob_rng ^= prob_rng >> 17;
    prob_rng ^= prob_rng << 5;
    return prob_rng % 100u >= p;
}
static void s_drum(void *x, int kit, int v, float vel);
static int s_drum_seq(void *x, int kit, int v, float vel)
{
    if (prob_skip(kit, v))
        return 0;
    s_drum(x, kit, v, vel);
    return 1;
}
__attribute__((noinline)) static void s_drum(void *x, int kit, int v, float vel)
{
    (void)x;
    if (v == 0)                          /* BD (both kits' track 1): the master's PUMP key */
        master_key(&mst, kit, vel);
    if ((unsigned)kit < NKIT && (unsigned)v < NDRUM)   /* MIX's drum pages: the track's meter jumps */
        eng_hit[kit][v] = (uint8_t)(vel >= 1.0f ? 255 : (int)(vel * 255.0f));
    if (kit == 0)
        drum909_trigger(&d909, v, vel);
    else                                 /* the 808's unaccented hit sits at D8_VEL_NORMAL, not at our 909 level */
        drum808_trigger(&d808, v, vel >= 0.999f ? 1.0f : vel * (D8_VEL_NORMAL / (88.0f / 127.0f)));
}
static int s_bon(void *x, int p, int note, int acc, int sl, int velocity)
{
#if X0X_BASS_TAGGED_SOUND
    if(engine_bass_sound_bank(mot_base_snd,p)<0)return 0;
#endif
    (void)x;
    live_key_retire(p,0);
    /* Pattern owns the gate: drop any live/MIDI damper latch so turning Damp
     * Off later cannot cut this step (Bugbot on #342). */
    if ((unsigned)p < NBASS)
        chi_damp_latched[p] = 0;
    if (prob_skip(PART_303A + p, 0)) {                /* skipped: a rest for audio AND MIDI */
#if X0X_DEV_WOBBLE
        synth_off(&bass_slot[p]);
#else
        bass303_note_off(&b303[p]);
#endif
        return 0;
    }
#if X0X_DEV_WOBBLE
    bass_voice_note_on(p, note, acc, sl, velocity);
#else
    bass303_note_on_velocity(&b303[p], note, acc, sl, velocity);
#endif
    return 1;
}
static void s_boff(void *x, int p)
{
    (void)x;
    live_key_retire(p,0);
    if ((unsigned)p < NBASS)
        chi_damp_latched[p] = 0; /* pattern gate wins over damper */
#if X0X_DEV_WOBBLE
    bass_voice_note_off(p);
#else
    bass303_note_off(&b303[p]);
#endif
}
static uint8_t brk_applied[BRK_NSET];  /* the pattern settings the break was last given */
static void s_brk(void *x, int s16, int bar, float spb, int en)
{
    const brkpart_t *bp = &seq.pat[seq.ppat[TRK_BRK]].brk;
    int i;
    if (en && prob_skip(PART_BRK, 0))
        en = 0;
    (void)x;
    breaks_set_quiet(&brk, bp->steps == 0 || (seq.mute & (1u << MUTE_BRK)));   /* unheard: stop decoding */
    for (i = 0; i < BRK_NSET; i++)       /* follow the playing pattern's settings (12 compares) */
        if (bp->set[i] != brk_applied[i]) {
            brk_applied[i] = bp->set[i];
            breaks_set(&brk, i, bp->set[i]);
        }
    breaks_pin(&brk, (int)bp->slice[s16 & 15] - 1);   /* the step's own slice, if it has one */
    breaks_step(&brk, s16, bar, spb, en);
}
static void brk_midi_discard(void);
static void s_brkstop(void *x) { (void)x; breaks_stop(&brk); brk_midi_discard(); }
static void s_midi(void *x, uint8_t st, uint8_t d1, uint8_t d2)
{
    uint32_t cin = st >= 0xF0 ? 0x0Fu : (uint32_t)(st >> 4);
    (void)x;
    plat_midi_out(cin | ((uint32_t)st << 8) | ((uint32_t)d1 << 16) | ((uint32_t)d2 << 24));
}
/* Accepted hits can originate on CPU1 during its BREAK render job. Append
 * descriptors only: transport rings belong exclusively to CPU0. CPU0 sequencing
 * and CPU1 rendering are serialized by plat_cpu2_run/wait, whose native CSYNC
 * job/done handoff already publishes the audio buffers. No spin/IRQ change here. */
enum { BRK_MIDI_EVENTS = 8 };
typedef struct { uint8_t note, channel, bank, generation; } brk_midi_event_t;
static brk_midi_event_t brk_midi_events[BRK_MIDI_EVENTS];
static volatile uint8_t brk_midi_count;
static void brk_midi_discard(void) { brk_midi_count = 0; }
static void s_brk_accepted(void *ctx, int slice, int bank, unsigned gen)
{
    (void)ctx;
    if (seq.send_notes && seq.ch[TRK_BRK] != SEQ_CH_OFF && seq.playing &&
        !(seq.mute & (1u << MUTE_BRK))) {
        unsigned n = brk_midi_count;
        if (n >= BRK_MIDI_EVENTS) return; /* bounded overload: drop the whole pulse */
        brk_midi_events[n] = (brk_midi_event_t){(uint8_t)(12 + slice), seq.ch[TRK_BRK],
                                             (uint8_t)bank, (uint8_t)gen};
        brk_midi_count = (uint8_t)(n + 1u);
    }
}
/* CPU0 only, after the render job's full synchronization or local fallback. */
static void brk_midi_drain(void)
{
    unsigned n = brk_midi_count;
    for (unsigned i = 0; i < n; i++)
        (void)plat_midi_pulse(brk_midi_events[i].channel, brk_midi_events[i].note, 100);
    brk_midi_count = 0;
}
static void s_step(void *x, int t, int p)
{
    (void)x;
    eng_step[t] = (uint8_t)p;
    if (mot_on)
        motion_step(&mot, t, seq.ppat[t], p, seq_next_step(&seq, t, p), seq_part_len(&seq, t));
}

/* --------------------------------------------------------------- motion --- */
static int mot_base(void *x, int t, int v, int i)
{
    (void)x;
    if (t == T_PROB)
        return 100;                                   /* always */
    if (t == T_VEL)
        return 0;                                     /* legacy accent / accent_q7 */
    if (t >= NTARGETS)
        return 100;
    return mot_base_snd ? mot_base_snd->v[t][v][i] : 0;
}
static void mot_apply(void *x, int t, int v, int i, int val)
{
    (void)x;
    if (t == T_PROB) {
        if ((v >> 4) < NPARTS && (v & 15) < NDRUM)
            prob[v >> 4][v & 15] = (uint8_t)val;
    } else if (t == T_VEL) {
        int kit = v >> 4, voice = v & 15;
        if ((unsigned)kit < NKIT && (unsigned)voice < NDRUM)
            seq.drum_vel[kit][voice] = (uint8_t)(val < 0 ? 0 : val > 127 ? 127 : val);
    } else if (t < NTARGETS)
        apply_param(t, v, i, val);
}

int engine_motion_part(int t, int v)
{
    switch (t) {
    case T_909: return PART_909;
    case T_808: return PART_808;
    case T_303: return PART_303A + (v & 1);
    case T_BRK: return PART_BRK;
    case T_MIX: return v < NPARTS ? v : PART_909;
    case T_PROB:
    case T_VEL: return (v >> 4) < NPARTS ? v >> 4 : PART_909;
    default: return PART_909;                     /* FX, master: the bar */
    }
}
int engine_motion_value(int k) { return motion_value(&mot, k); }
void engine_motion_rec(int k) { motion_req_rec(&mot, k); }
void engine_motion_hold(int k) { motion_req_hold(&mot, k); }
void engine_motion_clear(int k) { motion_clear(&mot, k); }
static const seq_sink_t SINK = {s_drum_seq, s_bon, s_boff, s_brk, s_brkstop, s_midi, s_step, 0};

#if BOARD_SMK37
/* Called with audio interrupts disabled at both clip-mode boundaries. */
void engine_clip_midi_flush(void)
{
    seq_midi_flush(&seq, &SINK);
}
#endif


/* ---------------------------------------------------------------- MIDI in --- */

#ifdef X0X_HOST
static uint8_t bass_attacked[NBASS];
#endif
/* Accepted physical edges, not the DSP gate, decide recorder legato. */
static int live_rec_legato(int part, int src)
{
    const seq_bass_rec_t *r = &seq.rec_bass[part];
    return r->own_ok && r->own_gen == r->gen && r->own_src == (uint8_t)src &&
           r->own_note == bass_held[part] && bass_held_count[part] && bass_held_src[part] == (uint8_t)src;
}
static void live_rec_accept(int part, int src, int note)
{
    seq_bass_rec_t *r = &seq.rec_bass[part];
    r->own_ok = 1; r->own_gen = r->gen;
    r->own_src = (uint8_t)src; r->own_note = (uint8_t)note;
}
static uint16_t src_held[3];           /* USB/Bluetooth/TRS: bass owners only; BREAK has independent raw-note owners. */
static uint32_t brk_local;
static uint8_t brk_input_channel;
static uint8_t brk_down[3][16];        /* 8 SLICES: bit n of source 0/1/2 is MIDI note n held */
enum { MIDI_NSRC = 3 };
#if BOARD_SMK37
#include "app/smk_apc40.h"
void smk_apc_led_set(uint8_t note, uint8_t type, uint8_t velocity);
void smk_apc_transport_led_set(uint8_t note, uint8_t velocity);
uint8_t smk_clip_mode_on(void);
void smk_apc_clock_tick(void);
void smk_apc_clock_start(void);
void smk_apc_clock_position(uint32_t sixteenths);
void smk_ck_midi_stop(void);
#endif

X0X_COLD int engine_project_notes(void)
{
    if(live_key_r!=live_key_w || live_key_cancel)return 1;
    uint32_t r = cq_r;
    int k;
    if (brk.keys || brk.slices || src_held[0] || src_held[1] || src_held[2])
        return 1;
    for (k = 0; k < NBASS; k++)
#if X0X_DEV_WOBBLE
        if (synth_active(&bass_slot[k]) || bass_held[k])
#else
        if (b303[k].gate || bass_held[k])
#endif
            return 1;
    while (r != cq_w) {
        uint32_t c = cq[r++ % CQ], t = c >> 28;
        if (t == C_DRUM || t == C_BON || t == C_REC || (t == C_BRK && ((c >> 12) & 0xFFu)))
            return 1;
    }
    return 0;
}

X0X_COLD void engine_project_flush(void)
{
    seq_midi_flush(&seq, &SINK);
    if (seq.playing && !seq.ext && seq.send_clock
#if BOARD_SMK37
        && !smk_clip_mode_on()
#endif
    )
        s_midi(0, 0xFC, 0, 0);
}

static int brk_note_down(int src, int note)
{
    return (brk_down[src][note >> 3] >> (note & 7)) & 1;
}
static void brk_note_set(int src, int note, int on)
{
    uint8_t m = (uint8_t)(1u << (note & 7));
    if (on)
        brk_down[src][note >> 3] |= m;
    else
        brk_down[src][note >> 3] &= (uint8_t)~m;
}
/* The applied snapshot, not the requested count, owns the held MIDI identity. */
static int brk_note_slice(int note)
{
    int count = breaks_count(&brk, 0);
    return (note + 128 - 12) % count;
}
__attribute__((noinline)) static int brk_src_slice(int src, int slice)
{
    for (int n = 0; n < 128; n++)
        if (brk_note_down(src, n) && brk_note_slice(n) == slice) return 1;
    return 0;
}
static int brk_any_slice(int slice)
{
    if (brk_local & (1u << slice)) return 1;
    for (int src = 0; src < MIDI_NSRC; src++)
        if (brk_src_slice(src, slice)) return 1;
    return 0;
}
static void brk_midi_wrap(int src, int on, int note)
{
    int slice = brk_note_slice(note);
    if ((unsigned)src >= MIDI_NSRC) return;
    if (!on) {
        if (!brk_note_down(src, note)) return;
        brk_note_set(src, note, 0);
        if (!brk_any_slice(slice)) breaks_slice_hold(&brk, slice, 0);
    } else {
        brk_note_set(src, note, 1);
        breaks_slice_on(&brk, slice);
    }
}
static void brk_local_event(int key, int on)
{
    /* Legacy physical macro numbers stay8..10. Paged slices use32..63. */
    if ((unsigned)(key - 8) < 3u) { breaks_live(&brk, key, on); return; }
    int slice = key >= 32 ? key - 32 : key;
    if ((unsigned)slice >= 32u) return;
    if (on) {
        if (slice >= breaks_count(&brk, 0)) return;
        brk_local |= 1u << slice;
        breaks_slice_on(&brk, slice);
    } else {
        brk_local &= ~(1u << slice);
        if (!brk_any_slice(slice)) breaks_slice_hold(&brk, slice, 0);
    }
}
__attribute__((noinline)) static void brk_link_end(int src, uint16_t owned)
{
    (void)owned;
    for (int n = 0; n < 16; n++) brk_down[src][n] = 0;
    for (int sl = 0; sl < 32; sl++)
        if (!brk_any_slice(sl)) breaks_slice_hold(&brk, sl, 0);
}

static void brk_channel_reconcile(void)
{
    uint8_t channel = seq.ch[TRK_BRK];
    if (channel == brk_input_channel) return;
    /* A channel edit retires external owners, not local keys or macros. */
    for (int src = 0; src < MIDI_NSRC; src++) brk_link_end(src, 0);
    brk_input_channel = channel;
}

/* noinline: keeps GM_DRUM lookup out of engine_render when the map is shared with seq */
__attribute__((noinline)) static void midi_in(uint32_t pkt)
{
    uint8_t st = (uint8_t)(pkt >> 8), d1 = (uint8_t)(pkt >> 16) & 0x7F, d2 = (uint8_t)(pkt >> 24) & 0x7F;
    uint8_t ch = st & 0x0F, ty = st & 0xF0;
    int k, v, src = (int)PLAT_MIDI_SRC(pkt);
    if (src < 0 || src >= MIDI_NSRC) return; /* source 3 is internal keyboard ownership only */
    if (!(pkt & ~0x30u)) {                      /* CIN 0, no message: the link of an input ended (usb.c midi_link_check) */
        seq_clk_drop(&seq, (uint32_t)src);      /* MID-15: another input can get the clock at once */
        if (src < 0 || src >= MIDI_NSRC)
            return;
        v = src_held[src];
        for (k = 0; k < NBASS; k++)             /* BLE-16: release the notes from that input (USB or Bluetooth) */
            if ((v >> k) & 1) {
                live_key_retire(k,0); /* accepted link-end gate precedes pending live keys */
#if X0X_DEV_WOBBLE
                synth_off(&bass_slot[k]);
#else
                bass303_note_off(&b303[k]);
#endif
                seq_rec_cancel(&seq, k);
                bass_held[k] = bass_held_count[k] = 0;
            }
        brk_link_end(src, (uint16_t)v);   /* slices this input held; the other input's slices stay */
        src_held[src] = 0;
        return;
    }
    if (!seq_clk_take(&seq, st, PLAT_MIDI_SRC(pkt)))
        return;                     /* another input has the clock: its Clock, Start, Continue, Stop, Song Position (MID-15) */
#if BOARD_SMK37
    /* Count MIDI Clock for the clip-mode PLAY blink. The sequencer path below is unchanged. */
    if (st == 0xF8)
        smk_apc_clock_tick();
    else if (st == 0xFA)
        smk_apc_clock_start();
    else if (st == 0xF2)
        smk_apc_clock_position((uint32_t)d1 | (uint32_t)d2 << 7);   /* Song Position: 6 ticks per unit */
    else if (st == 0xFC)
        smk_ck_midi_stop();         /* clip mode keybed: the arp stops its notes (main_fm1.c) */
    /* Clip mode: Loopy Start, Continue and Stop must not go back to the host as
     * X0X transport (no echo). With clip mode off, nothing changes (agent-rules 2). */
    if (st >= 0xFA && st <= 0xFC && smk_clip_mode_on())
        seq.midi_req = 1;
#endif
    if (seq_midi_rt(&seq, st))      /* Clock, Start, Continue, Stop: GLO > MIDI > CLOCK SOURCE */
        return;
#if BOARD_SMK37
    /* Only USB and peripheral BLE can carry a host-routed SMC User preset.
     * TRS input (or an invalid source) must retain normal instrument routing.
     * Direct SMC central input has its own callback. */
    {
        extern int smc_midi_host(uint32_t pkt);
        if (src < 2 && smc_midi_host(pkt))
            return;
    }
#endif
#if BOARD_SMK37
    /* The USB interrupt stores APC feedback at all times. Keep these notes from the
     * sound engine only while clip mode is on. When clip mode is off, the original
     * MIDI input path stays the same (agent-rules 2). */
    if (ty == 0x90 || ty == 0x80) {
        uint8_t vel = (ty == 0x80) ? 0u : d2;
        if (d1 < SMK_APC40_CLIP_COUNT) {
            /* The USB interrupt (smk_apc_usb_event) already stored this clip feedback.
             * Do not store it again: a second store breaks the flash detection. */
            /* A note on the break's own channel is the break, not a clip pad. */
            if (smk_clip_mode_on() && ch != seq.ch[TRK_BRK])
                return;
        } else if (smk_apc40_transport_feedback_note(d1)) {
            smk_apc_transport_led_set(d1, vel);
            if (smk_clip_mode_on())
                return;
        }
    }
#endif
    if (ty == 0x90 && d2 == 0)
        ty = 0x80;
    if ((ty == 0x90 || ty == 0x80) && seq.ch[TRK_BRK] == ch) {
        /* The break's channel owns the note. Drums and the 303s do not also play it. */
        brk_midi_wrap(src, ty == 0x90, d1);
        return;
    }
    /* Kit MIDI channel owns the note (GLO remap). Same consume rule as BREAK:
     * a Note On/Off on a kit channel must not also hit a 303 that shares that channel. */
    {
        int kit_own = 0;
        for (k = 0; k < NKIT; k++) {
            if (seq.ch[k] != ch)
                continue;
            kit_own = 1;
            if (ty == 0x90) {                            /* machines on GM drum notes (GM_DRUM) */
                for (v = 0; v < NDRUM; v++)
                    if (GM_DRUM[k][v] == d1)
                        s_drum(0, k, v, (float)d2 / 127.0f);
            }
        }
        if (kit_own && (ty == 0x90 || ty == 0x80))
            return;
    }
    if ((ty == 0x90 || ty == 0x80) && (seq.ext_pending || seq.req_start || seq.req_stop || seq.req_cont))
        seq_advance(&seq, 0, &SINK);
    for (k = 0; k < NBASS; k++)                         /* the 303s: overlapping notes slide */
        if (seq.ch[TRK_BASS0 + k] == ch) {
#if X0X_BASS_TAGGED_SOUND
            if(engine_bass_sound_bank(mot_base_snd,k)<0)continue;
#endif
            if (ty == 0x90) {
                live_key_retire(k,0);
                int slide = live_rec_legato(k, src) && bass_held[k] != d1;
#ifdef X0X_HOST
                bass_attacked[k] = (uint8_t)!slide;
#endif
                seq_rec_bass(&seq, k, d1, d2, 1, src, (d2 >= 100 ? BS_ACCENT : 0) | (slide ? SEQ_REC_LEGATO : 0));
#if X0X_DEV_WOBBLE
                bass_voice_note_on(k, d1, d2 >= 100, slide, d2);
#else
                bass303_note_on_velocity(&b303[k], d1, d2 >= 100, slide, d2);
#endif
                chi_damp_latched[k] = 0;
                if (bass_held_count[k] && bass_held[k] == d1 && bass_held_src[k] == (uint8_t)src) {
                    if (bass_held_count[k] < 255u) bass_held_count[k]++;
                } else bass_held_count[k] = 1;
                bass_held_src[k] = (uint8_t)src;
                bass_held[k] = d1;
                live_rec_accept(k, src, d1);
                v = 1 << k;                     /* the input of the last note-on holds the note */
                src_held[0] &= (uint16_t)~v;
                src_held[1] &= (uint16_t)~v;
                src_held[2] &= (uint16_t)~v;
                if (src >= 0 && src < MIDI_NSRC)
                    src_held[src] |= (uint16_t)v;
            } else if (ty == 0x80 && bass_held_count[k] && bass_held[k] == d1 && bass_held_src[k] == (uint8_t)src) {
                v = 1 << k;
                if (src < 0 || src >= MIDI_NSRC || !(src_held[src] & (uint16_t)v))
                    continue;
                if (--bass_held_count[k]) continue;
                live_key_retire(k,0);
                seq_rec_bass(&seq, k, d1, 0, 0, src, 0);
                seq.rec_bass[k].own_ok = 0;
                chi_live_note_off(k);
                bass_held[k] = 0;
                src_held[0] &= (uint16_t)~(1u << k);
                src_held[1] &= (uint16_t)~(1u << k);
                src_held[2] &= (uint16_t)~(1u << k);
            }
        }
}
#ifdef X0X_HOST
int engine_bass_attacked(int part) { return (unsigned)part < NBASS && bass_attacked[part]; }
int engine_midi_held(void)   /* host test: bit k the 303 k gate, bit 8 + s the break slice key s */
{
    int k, r = brk.keys << 8 & 0xFF00;
    for (k = 0; k < NBASS; k++)
#if X0X_DEV_WOBBLE
        r |= synth_active(&bass_slot[k]) ? 1 << k : 0;
#else
        r |= b303[k].gate ? 1 << k : 0;
#endif
    return r;
}
#endif

#if X0X_BASS_PRESET_PREVIEW
static int16_t bass_named_preset[NBASS] = {-1, -1};
static volatile uint8_t bass_preview_dirty, bass_wheel_mask;
static volatile uint32_t bass_request[NBASS], bass_applied[NBASS];
static uint32_t bass_request_serial;
static volatile uint32_t bass_publish_seq[NBASS],bass_midi_mark[NBASS],bass_cq_mark[NBASS];
typedef struct {uint32_t token,midi,cq;} bass_request_t;
static int bass_request_snapshot(int voice,bass_request_t *r)
{
    uint32_t seq=bass_publish_seq[voice];
    if(seq&1u) return 0; /* ISR interrupted producer: defer, never spin. */
    __asm__ volatile("" ::: "memory");
    r->token=bass_request[voice];r->midi=bass_midi_mark[voice];r->cq=bass_cq_mark[voice];
    __asm__ volatile("" ::: "memory");
    return seq==bass_publish_seq[voice];
}
#if defined(X0X_HOST) && defined(X0X_BASS_PUBLISH_TEST)
extern void bass_publish_test_hook(int voice,int stage);
#define BASS_PUBLISH_HOOK(v,s) bass_publish_test_hook(v,s)
#else
#define BASS_PUBLISH_HOOK(v,s) ((void)0)
#endif
static void bass_publish_request(int voice,uint32_t token)
{
    bass_publish_seq[voice]++;
    __asm__ volatile("" ::: "memory");
    BASS_PUBLISH_HOOK(voice,1);
    bass_midi_mark[voice]=plat_midi_mark();bass_cq_mark[voice]=cq_w;
    BASS_PUBLISH_HOOK(voice,2);
    bass_request[voice]=token;
    BASS_PUBLISH_HOOK(voice,3);
    __asm__ volatile("" ::: "memory");
    bass_publish_seq[voice]++;
}
static int bass_has_locks(int voice)
{
    if(mot.lane) for(int k=0;k<NLANE;k++) if(mot.lane[k].used &&
       ((mot.lane[k].t==T_303 && mot.lane[k].v==voice) ||
        (mot.lane[k].t==T_MIX && mot.lane[k].v==PART_303A+voice &&
         (mot.lane[k].i==MX_REV || mot.lane[k].i==MX_DLY)) ||
        (mot.lane[k].t==T_FX && mot.lane[k].v==0 && mot.lane[k].i==FX_COMP) ||
        ((mot.lane[k].t==0xfd || mot.lane[k].t==0xfe) && mot.lane[k].part==PART_303A+voice))) return 1;
    return 0;
}
int engine_bass_preset_pending(int voice)
{
    bass_request_t request;
    if((unsigned)voice>=NBASS) return 0;
    if(!bass_request_snapshot(voice,&request)) return 6;
    if(!(request.token&0x80000000u) || request.token==bass_applied[voice]) return 0;
    if(!plat_midi_passed(request.midi) || (int32_t)(cq_r-request.cq)<0) return 5;
    if(bass_has_locks(voice)) return 4;
    if(bass_wheel_mask & (1u<<voice)) return 3;
    if(bass_held[voice] || ((src_held[0]|src_held[1]|src_held[2])&(1u<<voice))) return 2;
#if X0X_DEV_WOBBLE
    if(!synth_idle(&bass_slot[voice])) return 2;
#else
    if(!b303[voice].idle) return 2;
#endif
    return 1;
}
int engine_bass_requested_bank(int voice)
{ bass_request_t r;return (unsigned)voice<NBASS && bass_request_snapshot(voice,&r) && engine_bass_preset_pending(voice)?(int)((r.token>>7)&1):engine_bass_bank(voice); }
int engine_bass_requested_preset(int voice)
{ bass_request_t r;return (unsigned)voice<NBASS && bass_request_snapshot(voice,&r) && engine_bass_preset_pending(voice)?(int)(r.token&127):engine_bass_preset_index(voice); }
void engine_bass_cancel_preset(int voice)
{
    if((unsigned)voice<NBASS) bass_publish_request(voice,0x40000000u|((++bass_request_serial&0x3fffffu)<<8));
}
const x0x_param_t *engine_bass_bank_param(int bank, int i)
{
    if (i < 0) return 0;
    if (bank == FACTORY_BANK_MIAMI) {
#if X0X_DEV_WOBBLE
        return wobble_param(i);
#else
        return 0;
#endif
    }
    if (bank != FACTORY_BANK_CHICAGO) return 0;
    if (i >= CHI_ARP_SWITCH && i < CHI_ARP_NPARAMS_END) return &CHI_ARP_PARAMS[i-CHI_ARP_SWITCH];
    if (i >= CHI_FX_TYPE && i < CHI_FX_END) return &CHI_FX_PARAMS[i-CHI_FX_TYPE];
    if (i >= CHI_PBEND && i < CHI_AUX_END) return &CHI_AUX_PARAMS[i-CHI_PBEND];
    return bass303_param(i);
}
#if X0X_BASS_TAGGED_SOUND
static const bass_identity_entry_t BASS_SOUND_REGISTRY[2]={
    {SYNTH_ENGINE_303,BASS_IDENTITY_PRODUCT_SCHEMA,CHI_AUX_END,1},
    {0x57424c31u,BASS_IDENTITY_PRODUCT_SCHEMA,WOBBLE_EXPERIMENT_NPARAMS,X0X_DEV_WOBBLE}
};
static int bass_tag_read(const sound_t *sound,int voice,bass_sound_info_t *info)
{
    if(!sound || (unsigned)voice>=NBASS)return BASS_IDENTITY_INVALID;
    return bass_sound_tag_decode(info,&sound->bass[voice],BASS_SOUND_REGISTRY,2);
}
static void bass_tag_set(sound_t *sound,int voice,int bank,int preset,int edited)
{
    bass_sound_info_t info={{BASS_SOUND_REGISTRY[bank].engine,BASS_IDENTITY_PRODUCT_SCHEMA,
        BASS_SOUND_REGISTRY[bank].count,BASS_IDENTITY_KNOWN},(uint16_t)preset,(uint8_t)edited};
    (void)bass_sound_tag_encode(&sound->bass[voice],&info,BASS_SOUND_REGISTRY,2);
}
int engine_bass_sound_bank(const sound_t *sound,int voice)
{
    bass_sound_info_t info;
    if(bass_tag_read(sound,voice,&info)!=BASS_IDENTITY_READY)return -1;
    return info.identity.engine==SYNTH_ENGINE_303?0:1;
}
static int bass_values_valid(const uint8_t *cell,int bank)
{
    const bass_identity_entry_t *entry=&BASS_SOUND_REGISTRY[bank];
    if(!entry->available)return -9;
    for(int i=0;i<entry->count;i++) {
        const x0x_param_t *d=engine_bass_bank_param(bank,i);
        if(!d || cell[i]>d->max)return -1;
    }
    return 0;
}
static int bass_cell_valid(const sound_t *sound,int voice,int bank)
{return bass_values_valid(sound->v[T_303][voice],bank);}
static int bass_tagged_cell_validate(const uint8_t *cell,const bass_sound_tag_t *tag,int *bank)
{
    bass_sound_info_t info;
    int rc=bass_sound_tag_decode(&info,tag,BASS_SOUND_REGISTRY,2);
    if(rc==BASS_IDENTITY_CHOICE_REQUIRED)return -10;
    if(rc==BASS_IDENTITY_UNSUPPORTED || rc==BASS_IDENTITY_UNAVAILABLE)return -9;
    if(rc!=BASS_IDENTITY_READY)return -1;
    *bank=info.identity.engine==SYNTH_ENGINE_303?0:1;
    if(bass_values_valid(cell,*bank))return -1;
    if(info.preset!=BASS_SOUND_NO_PRESET) {
        uint8_t named[NPARAMS_MAX];
        if(bass_preset_decode(*bank,info.preset,engine_bass_bank_param,named))return -1;
        if((__builtin_memcmp(named,cell,NPARAMS_MAX)!=0)!=!!(info.flags&BASS_SOUND_EDITED))return -1;
    }
    return 0;
}
int engine_bass_sound_validate(const sound_t *sound)
{
    int unknown=0,miami=0;
    for(int voice=0;voice<NBASS;voice++) {
        int bank=0,rc=bass_tagged_cell_validate(sound->v[T_303][voice],&sound->bass[voice],&bank);
        if(rc==-10){unknown=1;continue;}
        if(rc)return rc;
        if(bank && ++miami>1)return -9;
    }
    return unknown?-10:0;
}
#if X0X_BASS_NOTE_SOUND
int engine_bass_note_validate(const lane_t *l)
{
    int bank;
    if(l->used!=1 || l->t!=BASS_NOTE_TARGET || l->pat>=NPAT ||
       l->part<PART_303A || l->part>PART_303B || l->v>=NSTEPS ||
       l->i!=BASS_NOTE_VERSION || l->rsv)return -1;
    for(int i=BASS_NOTE_USED;i<NSTEPS;i++)if(l->val[i])return -1;
    int rc=bass_tagged_cell_validate(l->val,(const bass_sound_tag_t *)(l->val+BASS_NOTE_TAG),&bank);
    if(rc)return rc;
    for(int i=0;i<MX_NPARAMS;i++)if(l->val[BASS_NOTE_MIX+i]>engine_param(T_MIX,l->part,i)->max)return -1;
    return 0;
}
static int bass_note_capture(lane_t *lanes,int pat,int voice,int step,const sound_t *sound,int effective)
{
    if(!lanes || sound!=mot_base_snd || (unsigned)pat>=NPAT ||
       (unsigned)voice>=NBASS || (unsigned)step>=NSTEPS || engine_bass_sound_bank(sound,voice)<0)return -1;
    int k=bass_note_find(lanes,pat,voice,step);
    if(k<0)for(int j=0;j<NLANE;j++)if(!lanes[j].used){k=j;break;}
    if(k<0)return -7;
    lane_t next={0};
    next.used=1;next.gen=(uint8_t)(lanes[k].gen+1);next.t=BASS_NOTE_TARGET;
    next.pat=(uint8_t)pat;next.part=(uint8_t)(PART_303A+voice);next.v=(uint8_t)step;next.i=BASS_NOTE_VERSION;
    __builtin_memcpy(next.val,sound->v[T_303][voice],NPARAMS_MAX);
    __builtin_memcpy(next.val+BASS_NOTE_TAG,&sound->bass[voice],BASS_SOUND_TAG_BYTES);
    __builtin_memcpy(next.val+BASS_NOTE_MIX,sound->v[T_MIX][PART_303A+voice],MX_NPARAMS);
    if(effective) {
        /* Audio owns motion runtime. Capture what this accepted edge hears,
         * including an in-flight ramp, without rewriting the selected base. */
        for(int j=0;j<NLANE;j++) {
            const lane_t *l=&mot.lane[j];
            int value=motion_value(&mot,j);
            if(value<0 || l->part!=PART_303A+voice)continue;
            if(l->t==T_303 && l->v==voice && l->i<NPARAMS_MAX)next.val[l->i]=(uint8_t)value;
            if(l->t==T_MIX && l->v==PART_303A+voice && l->i<MX_NPARAMS)
                next.val[BASS_NOTE_MIX+l->i]=(uint8_t)value;
        }
        bass_sound_info_t info;
        if(bass_tag_read(sound,voice,&info)!=BASS_IDENTITY_READY)return -1;
        /* Motion can restore an edited base to the exact named sound. Describe
         * the complete captured cell, rather than inheriting the base flag. */
        uint8_t named[NPARAMS_MAX];
        int edited=info.preset==BASS_SOUND_NO_PRESET;
        if(!edited) {
            int bank=info.identity.engine==SYNTH_ENGINE_303?0:1;
            if(bass_preset_decode(bank,info.preset,engine_bass_bank_param,named))return -1;
            edited=__builtin_memcmp(next.val,named,NPARAMS_MAX)!=0;
        }
        info.flags=(uint8_t)((info.flags&~BASS_SOUND_EDITED)|(edited?BASS_SOUND_EDITED:0));
        if(bass_sound_tag_encode((bass_sound_tag_t *)(next.val+BASS_NOTE_TAG),&info,
                                BASS_SOUND_REGISTRY,2)!=BASS_IDENTITY_READY)return -1;
    }
    if(engine_bass_note_validate(&next))return -1;
    lanes[k].used=0;
    __asm__ volatile("" ::: "memory");
    __builtin_memcpy((uint8_t *)&lanes[k]+1,(const uint8_t *)&next+1,sizeof next-1);
    __asm__ volatile("" ::: "memory");
    lanes[k].used=1;
    return 0;
}
int engine_bass_note_capture(lane_t *lanes,int pat,int voice,int step,const sound_t *sound)
{return bass_note_capture(lanes,pat,voice,step,sound,0);}
/* Sequencer calls these only for actual recorder writes, before changing notes.
 * A refusal affects storage only: the accepted live note still auditions. */
int seq_bass_sound_prepare(seq_t *s,int voice,int pat,int step)
{
    if(s!=&seq || !mot.lane)return -1;
    return bass_note_capture(mot.lane,pat,voice,step,mot_base_snd,1);
}
void seq_bass_sound_clear(seq_t *s,int voice,int pat,int step)
{if(s==&seq && mot.lane)bass_note_clear(mot.lane,pat,voice,step);}
#endif
int engine_bass_sound_adopt(sound_t *sound,int bank0,int bank1)
{
    if(!sound)return -1;
    for(int voice=0;voice<NBASS;voice++) {
        bass_sound_info_t info;
        if(bass_tag_read(sound,voice,&info)!=BASS_IDENTITY_CHOICE_REQUIRED)return -9;
    }
    if((unsigned)bank0>1 || (unsigned)bank1>1 || (bank0 && bank1))return -9;
    int rc=bass_cell_valid(sound,0,bank0);if(rc)return rc;
    rc=bass_cell_valid(sound,1,bank1);if(rc)return rc;
    bass_tag_set(sound,0,bank0,BASS_SOUND_NO_PRESET,BASS_SOUND_EDITED);
    bass_tag_set(sound,1,bank1,BASS_SOUND_NO_PRESET,BASS_SOUND_EDITED);
    return 0;
}
uint32_t engine_bass_choice_serial(void) { return bass_request_serial; }
int engine_bass_base_ready(unsigned mask,const bass_base_state_t *target)
{
    int miami=0;
    for(int v=0;v<NBASS;v++) {
        int bank=0;
        int rc=bass_tagged_cell_validate(target->cell[v],&target->tag[v],&bank);
        if(rc)return -5;
        if(bank && ++miami>1)return -2;
        if(!(mask&(1u<<v)))continue;
        if(bass_has_locks(v))return -4;
        if(bass_wheel_mask&(1u<<v))return 2;
        if(bass_held[v] || ((src_held[0]|src_held[1]|src_held[2])&(1u<<v)))return 2;
#if X0X_DEV_WOBBLE
        if(!synth_idle(&bass_slot[v]))return 2;
#else
        if(!b303[v].idle)return 2;
#endif
        if(bank && !(mask&(1u<<(1-v))) && !engine_bass_bank_available(v,bank))return -2;
    }
    if(cq_r!=cq_w || !plat_midi_passed(plat_midi_mark()))return 2;
    return 0;
}
void engine_bass_base_restore(unsigned mask)
{
    /* UI holds audio around the complete identity/value transaction. Target
     * voices are idle; no unrelated voice, clock, FX processor or mixer resets. */
    for(int v=0;v<NBASS;v++)if(mask&(1u<<v)) {
        engine_bass_cancel_preset(v);
        int bank=engine_bass_sound_bank(mot_base_snd,v);
#if X0X_DEV_WOBBLE
        synth_binding_t binding=bank?wobble_foundation_binding(&dev_wobble):synth303_binding(&b303[v]);
        (void)synth_request(&bass_slot[v],binding,0);synth_boundary(&bass_slot[v]);
#endif
        chi_arp_on[v]=0;chi_arp_run_clear(&chi_arp_run[v]);
        chi_damper[v]=chi_damp_latched[v]=0;
        chi_fx_type[v]=CHI_FX_OFF;chi_fx_insert_program(v);
        int count=bank?WOBBLE_EXPERIMENT_NPARAMS:CHI_AUX_END;
        for(int i=0;i<count;i++)apply_param(T_303,v,i,mot_base_snd->v[T_303][v][i]);
        for(int i=0;i<MX_NPARAMS;i++)apply_param(T_MIX,PART_303A+v,i,mot_base_snd->v[T_MIX][PART_303A+v][i]);
        bass_sound_info_t info;
        bass_named_preset[v]=bass_tag_read(mot_base_snd,v,&info)==BASS_IDENTITY_READY &&
            info.preset!=BASS_SOUND_NO_PRESET?(int16_t)info.preset:-1;
    }
}
void engine_bass_sound_refresh(sound_t *sound,int voice)
{
    bass_sound_info_t info;uint8_t cell[NPARAMS_MAX];
    if(bass_tag_read(sound,voice,&info)!=BASS_IDENTITY_READY)return;
    int bank=info.identity.engine==SYNTH_ENGINE_303?0:1;
    int edited=info.preset==BASS_SOUND_NO_PRESET || bass_preset_decode(bank,info.preset,engine_bass_bank_param,cell) ||
        __builtin_memcmp(cell,sound->v[T_303][voice],NPARAMS_MAX);
    bass_tag_set(sound,voice,bank,info.preset,edited?BASS_SOUND_EDITED:0);
}
static void bass_restore_sound(const sound_t *sound)
{
    for(int voice=0;voice<NBASS;voice++) {
        int bank=engine_bass_sound_bank(sound,voice);
        bass_sound_info_t info;
        bass_named_preset[voice]=-1;
        if(bank<0)continue; /* unknown bytes are quarantined, never applied */
#if X0X_DEV_WOBBLE
        synth_binding_t binding=bank?wobble_foundation_binding(&dev_wobble):synth303_binding(&b303[voice]);
        if(!synth_request(&bass_slot[voice],binding,0))continue;
        synth_boundary(&bass_slot[voice]);
#endif
        if(bass_tag_read(sound,voice,&info)==BASS_IDENTITY_READY && info.preset!=BASS_SOUND_NO_PRESET)
            bass_named_preset[voice]=(int16_t)info.preset;
    }
}
#endif
int engine_bass_bank(int voice)
{
    return engine_bass_is_wobble(voice) ? FACTORY_BANK_MIAMI : FACTORY_BANK_CHICAGO;
}
int engine_bass_bank_available(int voice, int bank)
{
    if ((unsigned)voice >= NBASS) return 0;
    if (bank == FACTORY_BANK_CHICAGO) return 1;
#if X0X_DEV_WOBBLE
    /* There is one actual WBL1 context; never bind it to two voices. */
    if (bank == FACTORY_BANK_MIAMI) return !bass_wobble_voice(1-voice) &&
        !(bass_publish_seq[1-voice]&1u) &&
        !(bass_request[1-voice]!=bass_applied[1-voice] && (bass_request[1-voice]&0x80000000u) && ((bass_request[1-voice]>>7)&1));
#endif
    return 0;
}
int engine_bass_preview_changed(void) { return bass_preview_dirty; }
void engine_bass_wheel_owners(uint8_t mask) { bass_wheel_mask = mask; }
int engine_bass_wheel_active(void) { return bass_wheel_mask!=0; }
int engine_bass_preset_index(int voice)
{
    return (unsigned)voice < NBASS ? bass_named_preset[voice] : -1;
}
int engine_bass_preset_edited(int voice)
{
    uint8_t values[NPARAMS_MAX];
    if ((unsigned)voice >= NBASS || bass_named_preset[voice] < 0 || !mot_base_snd) return 1;
    if (bass_preset_decode(engine_bass_bank(voice),bass_named_preset[voice],engine_bass_bank_param,values)) return 1;
    for (int i=0;i<NPARAMS_MAX;i++) if(values[i]!=mot_base_snd->v[T_303][voice][i]) return 1;
    return 0;
}
int engine_bass_select_preset(int voice, int bank, int preset, sound_t *sound)
{
    uint8_t values[NPARAMS_MAX];
    if((unsigned)voice>=NBASS || !sound || sound!=mot_base_snd) return -1;
#if X0X_BASS_TAGGED_SOUND
    if(engine_bass_sound_bank(sound,voice)<0)return -5; /* explicit legacy adoption first */
#endif
    if(!engine_bass_bank_available(voice,bank)) return -2;
    if(bass_has_locks(voice)) return -4;
    if(bass_preset_decode(bank,preset,engine_bass_bank_param,values)) return -1;
    /* One aligned publication, no project/audio hold or MIDI consumption.
     * Rapid browsing explicitly replaces the not-yet-applied request. */
    uint32_t token=0x80000000u|((++bass_request_serial&0x3fffffu)<<8)|((uint32_t)bank<<7)|(uint32_t)preset;
    bass_preview_dirty=1;
    __asm__ volatile("" ::: "memory");
    bass_publish_request(voice,token);
    return 0;
}
static volatile uint8_t bass_edit_depth;
void engine_bass_edit_hold(int hold)
{
    __asm__ volatile("" ::: "memory");
    if (hold) bass_edit_depth++;
    else if (bass_edit_depth) bass_edit_depth--;
    __asm__ volatile("" ::: "memory");
}
static void bass_preset_boundary(void)
{
    /* Called only by audio, after CQ+MIDI drain, before new sequence triggers.
     * Old accepted notes retain their complete sound through release/legato. */
    if (bass_edit_depth) return;
    for(int voice=0;voice<NBASS;voice++) {
    bass_request_t request;
    if(!bass_request_snapshot(voice,&request)) continue;
    uint32_t token=request.token;
    if(!(token&0x80000000u)) {bass_applied[voice]=token;continue;}
    if(engine_bass_preset_pending(voice)!=1) continue;
    live_key_context_retire(voice,1);
    int bank=(token>>7)&1, preset=token&127;
    uint8_t values[NPARAMS_MAX];
    sound_t *sound=(sound_t *)mot_base_snd;
    uint8_t *cell=sound->v[T_303][voice];
    if(!engine_bass_bank_available(voice,bank) || bass_preset_decode(bank,preset,engine_bass_bank_param,values)) continue;
#if X0X_DEV_WOBBLE
    synth_binding_t binding=bank==FACTORY_BANK_MIAMI?
        wobble_foundation_binding(&dev_wobble):synth303_binding(&b303[voice]);
    if(!synth_request(&bass_slot[voice],binding,0)) continue;
    synth_boundary(&bass_slot[voice]);
#else
    bass303_all_off(&b303[voice]);
#endif
    chi_arp_on[voice]=0; chi_arp_run_clear(&chi_arp_run[voice]);
    chi_damper[voice]=chi_damp_latched[voice]=0;
    chi_fx_type[voice]=CHI_FX_OFF; chi_fx_insert_program(voice);
    int count = bank == FACTORY_BANK_CHICAGO ? CHI_AUX_END : WOBBLE_EXPERIMENT_NPARAMS;
    for (int i=0;i<NPARAMS_MAX;i++) cell[i]=values[i];
#if X0X_BASS_TAGGED_SOUND
    bass_tag_set(sound,voice,bank,preset,0);
#endif
    for (int i=0;i<count;i++) apply_param(T_303,voice,i,values[i]);
    /* Existing Chicago bridge has only Comp as a shared master effect;
     * inserts and Off clear the selected lane sends on explicit selection. */
    sound->v[T_MIX][PART_303A+voice][MX_REV]=0;
    sound->v[T_MIX][PART_303A+voice][MX_DLY]=0;
    apply_param(T_MIX,PART_303A+voice,MX_REV,0);
    apply_param(T_MIX,PART_303A+voice,MX_DLY,0);
    if(bank==FACTORY_BANK_CHICAGO && values[CHI_FX_TYPE]==CHI_FX_COMP) {
        sound->v[T_FX][0][FX_COMP]=values[CHI_FX_EDIT1];
        apply_param(T_FX,0,FX_COMP,values[CHI_FX_EDIT1]);
    }
    bass_named_preset[voice]=(int16_t)preset;
    __asm__ volatile("" ::: "memory");
    bass_applied[voice]=token;
    }
}
#endif

/* ---------------------------------------------------------------- render --- */
static void drain(void)
{
    uint32_t r = cq_r, pkt;
    int guard = 64;
    brk_channel_reconcile();
    /* Finish old contexts before a queued new press can acquire note ownership. */
    for (int k = 0; k < NBASS; k++) {
        const seq_bass_rec_t *rec = &seq.rec_bass[k];
        if (seq.req_stop || ((seq.req_start || seq.req_cont) && seq.rec_mask) || (rec->active &&
            (!(seq.rec_mask & (1u << k)) || rec->pat != seq.ppat[TRK_BASS0 + k])) ||
            (seq.t[TRK_BASS0 + k].to_off == 1 && bass_held_count[k])) {
            seq_advance(&seq, 0, &SINK);
            break;
        }
    }
    while (r != cq_w) {
        uint32_t c = cq[r % CQ], type = c >> 28, a = (c >> 20) & 0xFFu, b = (c >> 12) & 0xFFu, v = c & 0xFFFu;
        r++;
        uint32_t rec_epoch = a >> 1;
        if (type == C_REC) a &= 1u;
        if (type < 8u)
            apply_param((int)type, (int)a, (int)b, (int)v);
        else if (type == C_DRUM)
            s_drum(0, (int)a, (int)b, (float)v / 4095.0f);
        else if ((type == C_BON || type == C_REC) && a < NBASS) {
#if X0X_BASS_TAGGED_SOUND
            if(engine_bass_sound_bank(mot_base_snd,(int)a)<0)continue;
#endif
            live_key_retire((int)a,0);
            int slide = type == C_REC ? live_rec_legato((int)a, 3) && bass_held[a] != b : (int)((v >> 1) & 1u);
#ifdef X0X_HOST
            bass_attacked[a] = (uint8_t)!slide;
#endif
            if (type == C_REC && rec_epoch == (seq.rec_epoch & 127u))
                seq_rec_bass(&seq, (int)a, (int)b, (int)((v >> 2) & 127u), 1, 3,
                    ((v & 1u) ? BS_ACCENT : 0) | ((v & 512u) ? BS_SLIDE : 0) | (slide ? SEQ_REC_LEGATO : 0));
            v &= 511u; /* the outgoing manual-slide flag is recorder metadata only */
            chi_arp_accent[a] = (uint8_t)(v & 1u);
            chi_arp_vel[a] = (uint8_t)(v >> 2);
            if (chi_arp_on[a]) {
                uint8_t tmp[CHI_ARP_MAXN];
                uint32_t n = chi_arp_run[a].n, i, have = 0;
                for (i = 0; i < n; i++) {
                    tmp[i] = chi_arp_run[a].notes[i];
                    if (tmp[i] == (uint8_t)b)
                        have = 1;
                }
                if (!have && n < CHI_ARP_MAXN) {
                    tmp[n++] = (uint8_t)b;
                    chi_arp_run_set(&chi_arp_run[a], tmp, n, chi_arp_mode[a], chi_arp_range[a],
                                    chi_arp_speed[a]);
                } else if (!n)
                    chi_arp_run_hold(&chi_arp_run[a], (uint8_t)b, chi_arp_mode[a], chi_arp_range[a],
                                     chi_arp_speed[a]);
            } else {
#if X0X_DEV_WOBBLE
                bass_voice_note_on((int)a, (int)b, (int)(v & 1u), slide, (int)(v >> 2));
#else
                bass303_note_on_velocity(&b303[a], (int)b, (int)(v & 1u), slide, (int)(v >> 2));
#endif
            }
            chi_damp_latched[a] = 0;
            bass_held[a] = (uint8_t)b;
            bass_held_count[a] = 1; bass_held_src[a] = 3;
            src_held[0] &= (uint16_t)~(1u << a); src_held[1] &= (uint16_t)~(1u << a); src_held[2] &= (uint16_t)~(1u << a);
            live_rec_accept((int)a, 3, (int)b);
        } else if (type == C_BOFF && a < NBASS && bass_held_src[a] == 3) {
            live_key_retire((int)a,0);
            if ((!v || b == bass_held[a]) && seq.rec_bass[a].own_src == 3) {
                seq_rec_bass(&seq, (int)a, seq.rec_bass[a].own_note, 0, 0, 3, 0);
                seq.rec_bass[a].own_ok = 0;
                bass_held_count[a] = 0;
            }
            if (chi_arp_on[a] && v) {
                uint8_t tmp[CHI_ARP_MAXN];
                uint32_t n = 0, i;
                for (i = 0; i < chi_arp_run[a].n; i++)
                    if (chi_arp_run[a].notes[i] != (uint8_t)b)
                        tmp[n++] = chi_arp_run[a].notes[i];
                if (n) {
                    chi_arp_run_set(&chi_arp_run[a], tmp, n, chi_arp_mode[a], chi_arp_range[a],
                                    chi_arp_speed[a]);
                    bass_held[a] = tmp[0];
                } else {
                    chi_arp_halt((int)a);
                    bass_held[a] = 0;
                }
            } else if (chi_arp_on[a]) {
                chi_arp_halt((int)a);
                bass_held[a] = 0;
            } else {
                chi_live_note_off((int)a);
                bass_held[a] = 0;
            }
        } else if (type == C_BRK)
            brk_local_event((int)a, (int)b);
    }
    cq_r = r;
    while (guard-- && plat_midi_in(&pkt))
        if (!project_midi_drop)
            midi_in(pkt);
    if (guard >= 0)                   /* the ring was empty; subsequent MIDI belongs to live operation */
        project_midi_drop = 0;
}

static void live_key_drain(void)
{
    for(;;) {
        uint32_t flags=plat_cq_lock();
        unsigned cancel=live_key_cancel;live_key_cancel=0;
        plat_cq_unlock(flags);
        for(int v=0;v<NBASS;v++)if(cancel&(1u<<v))live_key_context_retire(v,1);
        if(live_key_r==live_key_w)break;
        __asm__ volatile("" ::: "memory");
        live_key_command_t c=live_key_q[live_key_r];
        __asm__ volatile("" ::: "memory");
        live_key_r=(uint8_t)((live_key_r+1u)%LIVE_KEY_Q);
        unsigned v=c.word&1u,token=(c.word>>1)&63u,pitch=(c.word>>7)&127u;
        unsigned velocity=(c.word>>14)&127u,accent=(c.word>>21)&1u,slide=(c.word>>22)&1u;
        live_key_owner_t *o=&live_key_owner[v];
        if(o->blocked || c.generation!=o->generation)continue;
#if X0X_BASS_TAGGED_SOUND
        if(engine_bass_sound_bank(mot_base_snd,(int)v)<0)continue;
#endif
        int arp=chi_arp_on[v];
#if X0X_DEV_WOBBLE
        if(bass_wobble_voice((int)v))arp=0;
#endif
        if(velocity) {
#if defined(X0X_HOST) && defined(X0X_LIVE_KEY_TEST)
            extern void engine_live_key_accepted(int,int,int,int,int);
            engine_live_key_accepted((int)v,(int)token,(int)pitch,(int)velocity,(int)slide);
#endif
            chi_arp_accent[v]=(uint8_t)accent;chi_arp_vel[v]=(uint8_t)velocity;
            if(arp) {
                uint8_t notes[CHI_ARP_MAXN];unsigned n=0,have=0;
                if(o->arp) {
                    n=chi_arp_run[v].n;
                    for(unsigned i=0;i<n;i++){notes[i]=chi_arp_run[v].notes[i];have|=notes[i]==pitch;}
                } else chi_arp_run_clear(&chi_arp_run[v]);
                if(!have) {
                    if(n>=CHI_ARP_MAXN)continue;
                    notes[n++]=(uint8_t)pitch;
                    chi_arp_run_set(&chi_arp_run[v],notes,n,chi_arp_mode[v],chi_arp_range[v],chi_arp_speed[v]);
                }
                o->arp=1;
            } else {
#if X0X_DEV_WOBBLE
                bass_voice_note_on((int)v,(int)pitch,(int)accent,(int)slide,(int)velocity);
#else
                bass303_note_on_velocity(&b303[v],(int)pitch,(int)accent,(int)slide,(int)velocity);
#endif
                o->gate=1;
            }
            o->token=(uint8_t)token;o->pitch=(uint8_t)pitch;
            bass_held[v]=(uint8_t)pitch;bass_held_count[v]=1;bass_held_src[v]=3;
            chi_damp_latched[v]=0;
            for(int i=0;i<3;i++)src_held[i]&=(uint16_t)~(1u<<v);
        } else if(arp && o->arp) {
            uint8_t notes[CHI_ARP_MAXN];unsigned n=0,found=0;
            for(unsigned i=0;i<chi_arp_run[v].n;i++) {
                uint8_t note=chi_arp_run[v].notes[i];
                if(note==pitch)found=1;else notes[n++]=note;
            }
            if(!found)continue;
            if(n)chi_arp_run_set(&chi_arp_run[v],notes,n,chi_arp_mode[v],chi_arp_range[v],chi_arp_speed[v]);
            else {
                chi_arp_halt((int)v);o->gate=o->token=o->pitch=o->arp=0;
                bass_held[v]=bass_held_count[v]=0;
            }
        } else if(!arp && !o->arp && o->token==token && o->gate==1) {
            chi_live_note_off((int)v);o->gate=o->token=o->pitch=0;
            bass_held[v]=bass_held_count[v]=0;
        }
    }
}

static float part_peak[NPARTS];
__attribute__((noinline)) static void meter(int p, const float *x, float g, uint32_t n)
{
    uint32_t i;
    float pk = part_peak[p];
    for (i = 0; i < n; i++) {
        float a = fm_fabsf(x[i]) * g;
        if (a > pk)
            pk = a;
    }
    part_peak[p] = pk;
}

/* dry into L and R by the part's pan; the sends stay mono (the reverb and delay make the width) */
/* a mix gain that glides to its target (one pole, ~10 ms) instead of stepping each block, which
 * zippered on quick turns of LEVEL, PAN or a send. cur < 0: never used, takes the target at once
 * (no fade-in at boot); settled, cur IS the target, so the product is the unsmoothed one exactly */
typedef struct { float cur, tgt; } glide_t;
#define GLIDE_A 0.0022651f                                /* 1 - exp(-1 / (10 ms * 44.1 kHz)) */
__attribute__((noinline)) static void glide_set(glide_t *g, float t)
{
    g->tgt = t;
    if (g->cur < 0.0f)
        g->cur = t;
}
__attribute__((noinline)) static void add_glide(float *dst, const float *src, glide_t *g, uint32_t n)
{
    uint32_t i;
    float c = g->cur, t = g->tgt;
    if (c == t) {
        if (c != 0.0f)
            for (i = 0; i < n; i++)
                dst[i] += src[i] * c;
        return;
    }
    for (i = 0; i < n; i++) {
        c += (t - c) * GLIDE_A;
        dst[i] += src[i] * c;
    }
    if (fm_fabsf(t - c) < 1e-5f)
        c = t;
    g->cur = c;
}
/* each part's gains: dry left and right, its reverb and delay sends, and (drums) the voices' own sends */
enum { GL_L, GL_R, GL_REV, GL_DLY, GL_VREV, GL_VDLY, GL_N };
static glide_t mix_glide[NPARTS][GL_N] = {
    {{-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}}, {{-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}},
    {{-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}}, {{-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}},
    {{-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}}};
__attribute__((noinline)) static void mix_targets(int p, float g, uint8_t solo)        /* g: the part level */
{
    if (solo && !(solo & (1u << p)))
        g = 0.0f;
    glide_set(&mix_glide[p][GL_L], g * mix_pl[p]);
    glide_set(&mix_glide[p][GL_R], g * mix_pr[p]);
    /* A throw opens the existing send. Release glides back; the fxbus tail stays. */
    glide_set(&mix_glide[p][GL_REV], g * busfx_send(mix_rev[p], bfx_hold[p], bfx_eff[p], BFX_REV));
    glide_set(&mix_glide[p][GL_DLY], g * busfx_send(mix_dly[p], bfx_hold[p], bfx_eff[p], BFX_DLY));
    glide_set(&mix_glide[p][GL_VREV], g);
    glide_set(&mix_glide[p][GL_VDLY], g);
}

#if X0X_DEBUG
/* development: what one block cost, for the worst-block record (main_fm1.c render_block) */
volatile uint32_t eng_dbg_wait, eng_dbg_nsub;  /* 24 MHz ticks waiting for core 1, stretches */
#endif

/* the mix of one stretch between two events: dry left and right, the reverb and delay sends;
 * t0..tr the 808's scratch, kbuf the break's, mono the 909's kit send */
static float dry_l[256], dry_r[256], rev[256], dly[256], t0[256], t1[256], t2[256], tr[256], kbuf[256], mono[256];

/* a glide applied in place to a buffer that started at zero: buf * c is 0 + buf * c exactly, so this
 * is add_glide(zeros, buf) to the bit (the same steps of c) */
__attribute__((noinline)) static void scale_glide(float *buf, glide_t *g, uint32_t n)
{
    uint32_t i;
    float c = g->cur, t = g->tgt;
    if (c == t) {
        for (i = 0; i < n; i++)
            buf[i] *= c;
        return;
    }
    for (i = 0; i < n; i++) {
        c += (t - c) * GLIDE_A;
        buf[i] *= c;
    }
    if (fm_fabsf(t - c) < 1e-5f)
        c = t;
    g->cur = c;
}

static void bfx_snapshot(void)
{
    uint8_t down = bfx_down_pub;
    int i, p;
    bfx_bpm_snap = seq_tempo(&seq);
    bfx_phase_snap = seq.playing ? seq.ext_now - seq.start_at : 0u;
    bfx_send_hold[0] = bfx_send_hold[1] = 0;
    bfx_flt_hold = 0;
    for (p = 0; p < NPARTS; p++) {
        bfx_hold[p] = 0;
        bfx_eff[p] = 0;
    }
    for (i = 0; i < SMCFX_N; i++) {
        if (i < SMCFX_SRC_N)
            bfx_eff[SMCFX_PART[i]] = bfx_fx_pub[i];
        else if (i == SMCFX_RVB)
            bfx_send_eff[0] = bfx_fx_pub[i];
        else if (i == SMCFX_MDLY)
            bfx_send_eff[1] = bfx_fx_pub[i];
        else
            bfx_flt_eff = bfx_fx_pub[i];
        if (!(down & (uint8_t)(1u << i)))
            continue;
        if (i < SMCFX_SRC_N)
            bfx_hold[SMCFX_PART[i]] = 1;
        else if (i == SMCFX_RVB)
            bfx_send_hold[0] = 1;
        else if (i == SMCFX_MDLY)
            bfx_send_hold[1] = 1;
        else
            bfx_flt_hold = 1;
    }
}

__attribute__((noinline)) static void bfx_shape(int part, float *a, float *b, float *c, float *d, int nch, int n)
{
    if (!bfx_hold[part] && !busfx_settling(&bfx_st[part]))
        return;
    busfx_apply(&bfx_st[part], &bfx_stut_hub, bfx_part_stut_base(part), a, b, c, d, nch, (int)n, mix_cut[part],
                bfx_hold[part], bfx_eff[part], (int)n, (int)n, bfx_bpm_snap, bfx_phase_snap);
}

static void bfx_send_shape(int which, float *buf, int n)
{
    busfx_t *st = &bfx_send_st[which];
    uint8_t hold = bfx_send_hold[which], eff = bfx_send_eff[which];
    int throw_id = which ? BFX_DLY : BFX_REV;
    uint32_t i;
    float g;
    if (!hold && !busfx_settling(st))
        return;
    g = busfx_send(BFX_SEND_NOM, hold, eff, throw_id);
    for (i = 0; i < (uint32_t)n; i++)
        buf[i] *= g;
    busfx_apply(st, &bfx_stut_hub, which ? BFX_STUT_SEND_DLY : BFX_STUT_SEND_RVB, buf, 0, 0, 0, 1, n, 127.0f,
                hold, eff, n, n, bfx_bpm_snap, bfx_phase_snap);
}

static void bfx_flt_shape(float *l, float *r, int n)
{
    if (!bfx_flt_hold && !busfx_settling(&bfx_flt_st))
        return;
    busfx_apply(&bfx_flt_st, &bfx_stut_hub, BFX_STUT_FLT, l, r, 0, 0, 2, n, 127.0f, bfx_flt_hold, bfx_flt_eff, n,
                n, bfx_bpm_snap, bfx_phase_snap);
}

/* the second core's job (or the first's, after its own part, with one core): the 808 into t0..tr
 * and the break into kbuf; arg bit 8: not timed (the cycle counter is the first core's). Nothing
 * here is mixed: the first core does that afterwards, in one core's order.
 * Bits 9..19 carry this stretch's audible 808 tracks; bits 20..27 carry
 * its output window offset. Under WBL1, completed windows in t0/tr remain
 * live until the full-block master pass; only this job's window is writable. */
static int brk_silent_now = 1;
static uint8_t d808_was_active;
static void core2_job(uint32_t arg)
{
    uint32_t n = arg & 0xFFu ? arg & 0xFFu : 256u, i, timed = !(arg & 0x100u), pt = timed ? plat_cycles() : 0;
    uint32_t offset = (arg >> 20) & 0xFFu;
    float *left = t0 + offset, *right = tr + offset;
    d808_was_active = (uint8_t)drum808_active(&d808);
    if (d808_was_active) {                                /* silent, it does nothing (and is not mixed) */
        for (i = 0; i < n; i++)
            left[i] = t1[i] = t2[i] = right[i] = 0.0f;
        drum808_render_st_masked(&d808, left, right, t1, t2, (int)n, (uint16_t)(arg >> 9 & 0x7FFu));
        bfx_shape(PART_808, left, right, t1, t2, 4, (int)n);
    } else
        drum808_pan_settle(&d808);
    if (timed) {
        PROF_ADD(PR_808, pt);
        pt = plat_cycles();
    }
    brk_silent_now = breaks_silent(&brk);                 /* the render writes zeros: nothing to mix */
    breaks_render(&brk, kbuf, (int)n);
    if (!brk_silent_now)
        bfx_shape(PART_BRK, kbuf, 0, 0, 0, 1, (int)n);
    if (timed)
        PROF_ADD(PR_BRK, pt);
}

/* a drum machine's mix, from its render (L, R, voice sends; mono, its kit send) */
__attribute__((noinline)) static void kit_sends(int b, const float *m, uint32_t n)
{
    add_glide(rev, m, &mix_glide[b][GL_REV], n);
    add_glide(dly, m, &mix_glide[b][GL_DLY], n);
    meter(b, m, mix_level[b], n);
}

/* one stretch: the 909 (into the mix, in place) and the 303s here while the second core renders the
 * 808 and the break; then those are mixed in, in the order one core always used (909, 808, 303 A,
 * 303 B, break): the same sums either way */
__attribute__((noinline)) static void render_sub(float *out_l, float *out_r, uint32_t n, uint32_t offset)
{
    float *bb[NBASS] = {out_l, out_r};
    float *left808 = t0 + offset, *right808 = tr + offset;
    int idle[NBASS], b;
    uint32_t solo = smc_solo_state;
    uint32_t arg = (n & 0xFFu) | (uint32_t)smc_audible_drums(solo, PART_808) << 9 |
                   (offset << 20), i, pt;
    uint8_t solo_parts = (uint8_t)(solo & 0x1Fu);
    if (solo & (0x7FFu << 5)) solo_parts |= 1u << PART_909;
    if (solo & (0x7FFu << 16)) solo_parts |= 1u << PART_808;
    bfx_snapshot();                                   /* before the second core reads the holds */
    uint8_t brk_event_mark = brk_midi_count;
    int on2 = plat_cpu2_run(core2_job, arg | 0x100u);
    for (i = 0; i < n; i++)
        dry_l[i] = dry_r[i] = rev[i] = dly[i] = 0.0f;
    {   /* the 909, straight into the mix: L, R and its voices' sends, then scaled in place */
        PROF_T(p9);
        if (!drum909_active(&d909)) {
            drum909_render(&d909, mono, mono, mono, (int)n);   /* writes nothing; advances the noise */
            drum909_pan_settle(&d909);                    /* silent: pans move at once, no glide */
        } else {
            drum909_render_st_masked(&d909, dry_l, dry_r, rev, dly, (int)n, smc_audible_drums(solo, PART_909));
            mix_targets(PART_909, mix_level[PART_909], solo_parts);
            bfx_shape(PART_909, dry_l, dry_r, rev, dly, 4, (int)n);
            for (i = 0; i < n; i++)                       /* kit send: filtered sides, still centred */
                mono[i] = (dry_l[i] + dry_r[i]) * 0.5f;
            scale_glide(dry_l, &mix_glide[PART_909][GL_L], n);   /* then the part's PAN over the kit */
            scale_glide(dry_r, &mix_glide[PART_909][GL_R], n);
            scale_glide(rev, &mix_glide[PART_909][GL_VREV], n);
            scale_glide(dly, &mix_glide[PART_909][GL_VDLY], n);
            kit_sends(PART_909, mono, n);
        }
        PROF_ADD(PR_909, p9);
    }
#if !X0X_DEV_WOBBLE
    for (b = 0; b < NBASS; b++) {
        PROF_T(pb);
        idle[b] = b303[b].idle;                           /* idle: the render writes zeros */
        bass303_render(&b303[b], bb[b], (int)n);
        if (!idle[b])
            chi_fx_process(b, bb[b], (int)n);
        if (!idle[b])
            bfx_shape(PART_303A + b, bb[b], 0, 0, 0, 1, (int)n);
        PROF_ADD(2 + b, pb);
    }
#endif
    pt = plat_cycles();
    int failed;
#if X0X_DEBUG
    eng_dbg_nsub++;
    uint32_t w0 = plat_ticks24();
#endif
    failed = on2 && plat_cpu2_wait();
#if X0X_DEBUG
    eng_dbg_wait += plat_ticks24() - w0;
#endif
    if (!on2 || failed) {
        /* Discard descriptors from a failed/partial remote job before fallback.
         * CPU0 step events queued before dispatch remain valid. */
        brk_midi_count = brk_event_mark;
        core2_job(arg);
    } else
        PROF_ADD(PR_808, pt);
    brk_midi_drain();
    if (d808_was_active) {
        mix_targets(PART_808, mix_level[PART_808], solo_parts);
        add_glide(dry_l, left808, &mix_glide[PART_808][GL_L], n);
        add_glide(dry_r, right808, &mix_glide[PART_808][GL_R], n);
        add_glide(rev, t1, &mix_glide[PART_808][GL_VREV], n);
        add_glide(dly, t2, &mix_glide[PART_808][GL_VDLY], n);
        for (i = 0; i < n; i++)
            left808[i] = (left808[i] + right808[i]) * 0.5f;
        kit_sends(PART_808, left808, n);
    }
    for (b = 0; b < NBASS; b++) {
        int p = PART_303A + b;
#if X0X_DEV_WOBBLE
        PROF_T(pb);
        /* Use the accepted internal/measured external clock, including free play.
         * Tempo changes do not retrigger notes or reset the wobble phase. */
        if (bass_wobble_voice(b))
            wobble_set_tempo(bass_slot[b].active.context, seq_tempo(&seq));
        idle[b] = synth_idle(&bass_slot[b]);
        if (!wobble_backend_stereo(&bass_slot[b], bb[b], bb[1 - b], (int)n))
            synth_render(&bass_slot[b], bb[b], (int)n);
        /* Crush/Ring/EQ/Cho/Ens/Phaser/SDly/BPM/Hall/GateRev inserts are Chicago/native303 only — never on WBL1. */
        if (!idle[b] && bass_slot[b].active.ops != &wobble_foundation_ops)
            chi_fx_process(b, bb[b], (int)n);
        PROF_ADD(2 + b, pb);
#endif
        if (idle[b])
            continue;
        mix_targets(p, mix_level[p], solo_parts);
#if X0X_DEV_WOBBLE
        if (bass_slot[b].active.ops == &wobble_foundation_ops) {
            add_glide(dry_l, bb[b], &mix_glide[p][GL_L], n);
            add_glide(dry_r, bb[1 - b], &mix_glide[p][GL_R], n);
            for (i = 0; i < n; ++i)
                bb[b][i] = .5f * (bb[b][i] + bb[1 - b][i]);
        } else
#endif
        {
            add_glide(dry_l, bb[b], &mix_glide[p][GL_L], n);
            add_glide(dry_r, bb[b], &mix_glide[p][GL_R], n);
        }
        add_glide(rev, bb[b], &mix_glide[p][GL_REV], n);
        add_glide(dly, bb[b], &mix_glide[p][GL_DLY], n);
        meter(p, bb[b], mix_level[p], n);
    }
    if (!brk_silent_now) {
        float g = mix_level[PART_BRK];
        mix_targets(PART_BRK, g, solo_parts);
        add_glide(dry_l, kbuf, &mix_glide[PART_BRK][GL_L], n);
        add_glide(dry_r, kbuf, &mix_glide[PART_BRK][GL_R], n);
        add_glide(rev, kbuf, &mix_glide[PART_BRK][GL_REV], n);
        add_glide(dly, kbuf, &mix_glide[PART_BRK][GL_DLY], n);
        meter(PART_BRK, kbuf, g, n);
    }
    bfx_send_shape(0, rev, (int)n);
    bfx_send_shape(1, dly, (int)n);
    {
        PROF_T(pf);
        fxbus_process_st(&fx, dry_l, dry_r, rev, dly, out_l, out_r, (int)n);
        if (fx.dl_type != 1 || fx.dl_level <= 0.0f || (fx.muted & 2u)) tape_signal_seen = 0;
        else if (fx.dl_signal) { tape_signal_at = plat_ms(); tape_signal_seen = 1; }
        PROF_ADD(5, pf);
    }
    bfx_flt_shape(out_l, out_r, (int)n);
}

void engine_render(int32_t *out_lr, uint32_t n)
{
#if X0X_DEV_WOBBLE
    /* The 808 owns only the current t0/tr window until CPU2 join and mixing.
     * Bass rendering and final FX then replace that window. Earlier windows
     * remain intact until master_process_st consumes the complete block.
     * Do not alias these buffers on the legacy early-bass render path. */
    float *out_l = t0, *out_r = tr;
#else
    static float out_l[256], out_r[256];
#endif
    uint32_t done = 0, i;
    float vol = usb_full ? 1.0f : (float)plat_master() / 4096.0f;   /* (FULL: engine_volume) */
    int p;
    if (n > 256u)
        n = 256u;
    if (project_hold) {
        tape_signal_seen = 0;
        for (i = 0; i < n * 2u; i++)
            out_lr[i] = 0;
        return;
    }
    eng_prof_frames += n;
    if ((mot_was_on && !mot_on) || mot_release_req) {   /* motion off, or its lanes replaced: knobs back */
        mot_release_req = 0;
        motion_release(&mot);
    }
    mot_was_on = mot_on;
    guard_apply();
    {
        PROF_T(pt);
        drain();
        live_key_drain();
#if X0X_BASS_PRESET_PREVIEW
        bass_preset_boundary();
#endif
#if X0X_DEV_WOBBLE
        for (p = 0; p < NBASS; ++p) {
            if (synth_boundary(&bass_slot[p])) {
                live_key_context_retire(p,0);
                seq_rec_cancel(&seq, p);
                bass_held[p] = bass_held_count[p] = 0;
                src_held[0] &= (uint16_t)~(1u << p);
                src_held[1] &= (uint16_t)~(1u << p);
                src_held[2] &= (uint16_t)~(1u << p);
            }
            if (bass_slot[p].running != !!seq.playing) {
                bass_slot[p].running = !!seq.playing;
                bass_slot[p].active.ops->transport(bass_slot[p].active.context, !!seq.playing);
            }
        }
#endif
        fxbus_set_bpm(&fx, seq_tempo(&seq));
        seq_advance(&seq, 0, &SINK);
        brk_midi_drain();
        if (mot_on && seq.playing && !seq_clock_waiting(&seq))
            motion_tick(&mot, n);
        PROF_ADD(7, pt);
    }
    while (done < n) {
        uint32_t k = seq_until_event(&seq, n - done);
        chi_arp_tick(k);
#if X0X_DEV_WOBBLE
        render_sub(out_l + done, out_r + done, k, done);
#else
        render_sub(out_l + done, out_r + done, k, 0);
#endif
        {
            PROF_T(pt);
            seq_advance(&seq, k, &SINK);
            brk_midi_drain();
            PROF_ADD(7, pt);
        }
        done += k;
    }
    if (mot_playing && !seq.playing)                     /* stopped: every knob back to its own value */
        motion_release(&mot);
    mot_playing = seq.playing;
    {
        PROF_T(pt);
        master_process_st(&mst, out_l, out_r, (int)n, vol);
        PROF_ADD(6, pt);
    }
    for (i = 0; i < n; i++) {
        float l = out_l[i], r = out_r[i];            /* (out_scale: NORMAL 2^22, Felucca's; LOUD 2^23) */
        l = l > out_lim ? out_lim : l < -out_lim ? -out_lim : l;
        r = r > out_lim ? out_lim : r < -out_lim ? -out_lim : r;
        out_lr[2u * i] = (int32_t)(l * out_scale);
        out_lr[2u * i + 1u] = (int32_t)(r * out_scale);
        if (i & 1u)
            eng_scope[eng_scope_w++ & (SCOPE_N - 1u)] = (int16_t)((out_l[i] + out_r[i]) * 16383.5f);
    }
    for (p = 0; p < NPARTS; p++) {                      /* meters: peak hold, ~300 ms decay at the UI */
        uint32_t q = (uint32_t)(fm_minf(part_peak[p], 1.0f) * 32767.0f);
        if (q > eng_peak[p])
            eng_peak[p] = (uint16_t)q;
        part_peak[p] = 0.0f;
    }
}

/* -------------------------------------------------------------- loops --- */
/* a user loop's length in bars: 1, 2, 4 or 8, whichever puts its tempo nearest 120 BPM
 * inside 80..180 (breaks are recorded at their own tempo; the player stretches to ours) */
X0X_COLD static uint8_t loop_bars(uint32_t ns, uint32_t rate)
{
    float sec = (float)ns / (float)(rate ? rate : 22050u), best = 1e9f;
    uint8_t bars = 1, b;
    for (b = 1; b <= 8; b = (uint8_t)(b * 2)) {
        float bpm = (float)b * 240.0f / sec, d = bpm > 120.0f ? bpm - 120.0f : 120.0f - bpm;
        if (bpm >= 80.0f && bpm <= 180.0f && d < best) {
            best = d;
            bars = b;
        }
    }
    return bars;
}

#include "x0x_break_bank.h"              /* empty by default; factory or X0X_BREAK_BANK fills it */
#include "app/brk_pack.h"                 /* optional pack in the free tail of the app image */

#if X0X_NBREAKS == 0 && !defined(X0X_HOST)
extern const uint8_t _start[];            /* crt0: app.bin offset 0, the XIP image */
static brk_pack_loop_t pack_loops[BRK_PACK_MAX];
static int pack_n = -1;

static int load_pack(void)
{
#if defined(BOARD_SMK37) && BOARD_SMK37
    const uint32_t limit = 0x86EE0u;      /* FL_APP_CODE_MAX: code ends where Bluetooth begins */
#else
    const uint32_t limit = 0x8DFBCu;
#endif
    pack_n = brk_pack_parse(_start, limit, pack_loops, BRK_PACK_MAX);
    return pack_n;
}
#endif

static int builtin_count(void)
{
#if X0X_NBREAKS > 0
    return X0X_NBREAKS;                   /* compiled into the image */
#else
#ifndef X0X_HOST
    if (pack_n < 0)
        load_pack();
    return pack_n > 0 ? pack_n : 0;
#else
    return 0;
#endif
#endif
}

X0X_COLD static const brk_loop_t *brk_builtin(int k)
{
    static brk_loop_t L[2];
    static int which;
    brk_loop_t *l = &L[which ^= 1];      /* two: A and B are set one after the other */
    int n = builtin_count();
    if (k < 0 || k >= n)
        return 0;
#if X0X_NBREAKS > 0
    l->adpcm = X0X_BREAKS[k].adpcm;
    l->nsamples = X0X_BREAKS[k].n;
    l->rate = X0X_BREAK_RATE;
    l->bars = X0X_BREAKS[k].bars;
#else
#ifndef X0X_HOST
    l->adpcm = pack_loops[k].adpcm;
    l->nsamples = pack_loops[k].n;
    l->rate = pack_loops[k].rate;
    l->bars = pack_loops[k].bars;
#else
    (void)l;
    return 0;
#endif
#endif
    return l;
}

/* the LOOP choices: the built-in bank, then every loop (zone) of the user slots, in order.
 * The list follows uploads: engine_brk_loops() rescans it. Room for a pack of 8 when the
 * image itself has fewer built-in loops. */
#define NLOOPS_MAX ((X0X_NBREAKS > BRK_PACK_MAX ? X0X_NBREAKS : BRK_PACK_MAX) + PLAT_NSLOTS * PLAT_SLOT_ZONES)
static const char *loop_names[NLOOPS_MAX];
static char user_names[PLAT_NSLOTS * PLAT_SLOT_ZONES][8];
static uint8_t loop_slot[NLOOPS_MAX], loop_zone[NLOOPS_MAX];
static int nloops, loops_ready;

X0X_COLD __attribute__((noinline)) static void scan_loops(void)
{
    int i, k, z, u = 0, nb = builtin_count();
    nloops = 0;
    for (i = 0; i < nb; i++) {
#if X0X_NBREAKS > 0
        loop_names[nloops++] = X0X_BREAKS[i].name;
#else
#ifndef X0X_HOST
        loop_names[nloops++] = pack_loops[i].name;
#else
        break;
#endif
#endif
    }
    for (k = 0; k < PLAT_NSLOTS; k++) {                 /* yours: BR1, BR2 ... in order, whatever slot */
        int nz = plat_slot_zones(k);                    /* holds them (the break page packs the list) */
        for (z = 0; z < nz && z < PLAT_SLOT_ZONES; z++) {
            char *n = user_names[k * PLAT_SLOT_ZONES + z];
            int j = 2;
            u++;
            n[0] = 'B';
            n[1] = 'R';
            if (u >= 10)
                n[j++] = (char)('0' + u / 10);
            n[j++] = (char)('0' + u % 10);
            n[j] = 0;
            loop_slot[nloops] = (uint8_t)k;
            loop_zone[nloops] = (uint8_t)z;
            loop_names[nloops++] = n;
        }
    }
    loops_ready = 1;
}

X0X_COLD int engine_brk_nslots(void)
{
    if (!loops_ready)
        scan_loops();
    return nloops;
}
X0X_COLD const char *const *engine_brk_slot_names(void)
{
    if (!loops_ready)
        scan_loops();
    return loop_names;
}

int engine_brk_slice(void) { return brk.running ? brk.st_slice : -1; }

void engine_brk_state(int *slice, int *bank, int *div, int *running)
{
    *slice = brk.perf.count ? brk.current_slice : brk.st_slice;
    *bank = brk.perf.count ? 0 : brk.st_bank;
    *div = brk.st_div;
    *running = brk.running;
}

/* Peak magnitude → screen column 0..255. IMA can hit -32768; abs is 32768, and
 * (uint8_t)(32768 >> 7) wraps to 0 so a full-scale negative loop drew blank. */
static uint8_t brk_outline_peak_u8(int32_t pk)
{
    return (uint8_t)(pk >= 32768 ? 255 : pk >> 7);
}

/* the loop's outline for the screen: n columns of peak level 0..255 (main loop: decodes the
 * whole loop once, ~20k IMA steps; called when the loop changes, never per frame) */
X0X_COLD int engine_brk_outline(int which, uint8_t *peaks, int n)
{
    const brk_bank_t *bk = &brk.bank[which & 1];
    uint32_t i, ns = bk->n;
    int32_t pred = 0, idx = 0, col = 0, pk = 0;
    uint32_t per;
    if (!bk->adpcm || !ns || n <= 0)
        return 0;
    per = (ns + (uint32_t)n - 1u) / (uint32_t)n;       /* samples per column (32-bit: no __udivdi3) */
    for (i = 0; i < ns; i++) {
        int32_t s = ima_decode(bk->adpcm, i, &pred, &idx), c;
        c = (int32_t)(i / per);
        if (c != col) {
            peaks[col] = brk_outline_peak_u8(pk);
            col = c;
            pk = 0;
        }
        if ((s < 0 ? -s : s) > pk)
            pk = s < 0 ? -s : s;
    }
    peaks[col] = brk_outline_peak_u8(pk);
    for (col++; col < n; col++)
        peaks[col] = 0;
    return 1;
}
float engine_gr_db(void) { return mst.gr_view; }
int engine_master_get(int i) { return master_get(&mst, i); }
void engine_master_format(int i, char *buf) { master_format(&mst, i, buf); }

X0X_COLD void engine_brk_loops(void)
{
    const brkpart_t *bp = &seq.pat[seq.ppat[TRK_BRK]].brk;
    brk_loop_t L[2];
    int w;
    scan_loops();                                       /* an upload may have changed the user loops */
    /* No mute. The voice that is already sounding keeps its snapshot, and the
     * new loop is published for the next trigger. Skipping the break here used
     * to drop a whole buffer. */
    breaks_flush(&brk);                                /* a change that waited for a free snapshot */
    for (w = 0; w < 2; w++) {
        int slot = w ? bp->slot_b : bp->slot_a;
        uint32_t ns = 0, rate = 0;
        char name[9];
        int nb = builtin_count();
        const uint8_t *d = (slot >= nb && slot < nloops)
                               ? plat_slot(loop_slot[slot], loop_zone[slot], &ns, &rate, name) : 0;
        if (d) {
            L[w].adpcm = d;
            L[w].nsamples = ns;
            L[w].rate = rate;
            L[w].bars = loop_bars(ns, rate);
        } else {
            const brk_loop_t *builtin = brk_builtin(slot < nb ? slot : 0);
            if (builtin) L[w] = *builtin;
            else L[w] = (brk_loop_t){0};
        }
        L[w].slices = (uint8_t)brk_slice_count(bp, (unsigned)w);
        breaks_set_loop(&brk, w, &L[w]);
    }
}

uint32_t engine_brk_held(void) { return brk.slices; }
void engine_brk_flush(void) { breaks_flush(&brk); }
void engine_brk_edit_hold(int held) { brk.local_guard = (uint8_t)(held != 0); }
int engine_brk_count(int bank) { return breaks_count(&brk, bank); }

void engine_brk_drop_range(const uint8_t *base, uint32_t nbytes)
{
    breaks_drop_range(&brk, base, nbytes);
}

X0X_COLD void engine_init(pattern_t *patterns, song_t *song, lane_t *lanes, const sound_t *base)
{
#if X0X_BASS_PRESET_PREVIEW
    bass_named_preset[0]=bass_named_preset[1]=-1;
    bass_preview_dirty=bass_wheel_mask=0;
    bass_request_serial=0;
    for(int v=0;v<NBASS;v++) bass_request[v]=bass_applied[v]=bass_publish_seq[v]=bass_midi_mark[v]=bass_cq_mark[v]=0;
#endif
    int p, g;
    cq_w = cq_r = 0;
    brk_local = 0;
    brk_midi_count = 0;
    brk_input_channel = 255;
    src_held[0] = src_held[1] = src_held[2] = 0;
    for (p = 0; p < MIDI_NSRC; p++)
        for (g = 0; g < 16; g++)
            brk_down[p][g] = 0;
    for (p = 0; p < NBASS; p++) {
        live_key_context_retire(p,0);
        live_key_r=live_key_w;live_key_cancel=0;
        bass_held[p] = bass_held_count[p] = bass_held_src[p] = 0;
        chi_arp_on[p] = chi_arp_mode[p] = chi_arp_range[p] = chi_arp_speed[p] = 0;
        chi_arp_accent[p] = chi_arp_vel[p] = 0;
        chi_arp_run_clear(&chi_arp_run[p]);
    }
    for (p = 0; p < BRK_NSET; p++)
        brk_applied[p] = 0xFFu;
    mot_playing = mot_release_req = 0;
    mot_on = mot_was_on = 1;
    guard_on = guard_applied = 0;
    guard_enabled = 1;
    guard_avg_q8 = guard_calm = 0;
    drum808_quiet = 3.2e-5f;
    smc_solo_state = 0;
    bfx_down_pub = 0;
    for (p = 0; p < SMCFX_N; p++)
        bfx_fx_pub[p] = 0;
    for (p = 0; p < NPARTS; p++) {
        mix_cut[p] = 127.0f;
        busfx_init(&bfx_st[p]);
        bfx_hold[p] = 0;
        bfx_eff[p] = 0;
    }
    busfx_init(&bfx_send_st[0]);
    busfx_init(&bfx_send_st[1]);
    busfx_init(&bfx_flt_st);
    busfx_stut_init(&bfx_stut_hub);
    prob_rng = 0x2545F491u;
    brk_silent_now = 1;
    d808_was_active = 0;
    vol_after = -1.0f;
    for (p = 0; p < NPARTS * NDRUM; p++)
        prob[p / NDRUM][p % NDRUM] = 100;             /* every step plays, until a lane says */
    drum909_init(&d909);
    drum808_init(&d808);
    bass303_init(&b303[0]);
    bass303_init(&b303[1]);
    chi_fx_type[0] = chi_fx_type[1] = CHI_FX_OFF;
    chi_fx_e1[0] = chi_fx_e1[1] = CHI_FX_PARAMS[1].def;
    chi_fx_e2[0] = chi_fx_e2[1] = CHI_FX_PARAMS[2].def;
    chi_fx_insert_program(0);
    chi_fx_insert_program(1);
#if X0X_DEV_WOBBLE
    synth_init(&bass_slot[0], synth303_binding(&b303[0]));
    synth_init(&bass_slot[1], synth303_binding(&b303[1]));
    wobble_foundation_init(&dev_wobble);
    synth_request(&bass_slot[0], wobble_foundation_binding(&dev_wobble), 0);
    synth_boundary(&bass_slot[0]); /* Product Miami: 303A is WBL1 before first audio block. */
#endif
    breaks_init(&brk);
    brk.accepted = s_brk_accepted;
    fxbus_init(&fx, dly_buf, DLY_LEN);
    tape_signal_seen = 0;
    master_init(&mst);
    seq_init(&seq, patterns);
    seq.song = song;
    motion_init(&mot, lanes);
    mot.base = mot_base;
    mot.apply = mot_apply;
    mot_base_snd = base;
    for (p = 0; p < NPARTS; p++) {
        mix_level[p] = 1.0f;
        mix_rev[p] = mix_dly[p] = 0.0f;
        mix_pl[p] = mix_pr[p] = 1.0f;
        part_peak[p] = 0.0f;
        eng_step[p] = 0;
        eng_peak[p] = 0;
        for (g = 0; g < GL_N; g++) {
            mix_glide[p][g].cur = -1.0f;
            mix_glide[p][g].tgt = 0.0f;
        }
    }
    for (p = 0; p < NKIT * NDRUM; p++)
        eng_hit[p / NDRUM][p % NDRUM] = 0;
    for (p = 0; p < (int)SCOPE_N; p++)
        eng_scope[p] = 0;
    eng_scope_w = 0;
}

#if defined(X0X_HOST) && defined(X0X_LIVE_KEY_TEST)
void engine_live_key_force_serial(int voice,uint32_t generation,uint32_t context)
{live_key_owner[voice].generation=generation;live_key_owner[voice].context=context;}
unsigned engine_live_key_probe(int voice,int field)
{
    if((unsigned)voice>=NBASS)return 0;
    switch(field) {
    case 0:return live_key_owner[voice].token;
    case 1:return live_key_owner[voice].pitch;
    case 2:return live_key_owner[voice].gate;
    case 3:return chi_arp_run[voice].n;
    case 4:return chi_arp_vel[voice];
    case 5:return bass_held[voice];
    case 7:
#if X0X_DEV_WOBBLE
        if(bass_wobble_voice(voice))return (unsigned)dev_wobble.gate;
#endif
        return (unsigned)b303[voice].gate;
    case 8:return chi_arp_on[voice];
    case 6:return (unsigned)(sizeof live_key_q+sizeof live_key_owner+sizeof live_key_w+sizeof live_key_r+sizeof live_key_cancel);
    default:return 0;
    }
}
#endif

#if defined(X0X_HOST) && defined(X0X_LIVE_KEY_TEST)
void engine_live_key_motion_step(int voice,int step)
{ motion_step(&mot,PART_303A+voice,seq.ppat[PART_303A+voice],step,(step+1)%16,16); }
void engine_live_key_pattern_note(int voice,int note)
{ if(note>=0)s_bon(0,voice,note,0,0,100);else s_boff(0,voice); }
#endif
