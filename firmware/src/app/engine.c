/* SPDX-License-Identifier: GPL-3.0-only */
/* X0X audio engine; see engine.h. */
#include "engine.h"
#include "plat.h"
#include "../x0x_cold.h"
#include "../dsp/drum909.h"
#include "../dsp/drum808.h"
#include "../dsp/bass303.h"
#if X0X_DEV_WOBBLE
#include "../dsp/synth_engine.h"
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
#include "chicago_aux.h"

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
/* Chicago SEND FX pots (sound_t) + Crush/Ring/EQ/SDly/BPM/Hall insert DSP per 303 voice. */
static uint8_t chi_fx_type[NBASS] POOL, chi_fx_e1[NBASS] POOL, chi_fx_e2[NBASS] POOL;
static chi_crush_t chi_crush[NBASS] POOL;
static chi_ring_t chi_ring[NBASS] POOL;
static chi_eq_t chi_eq[NBASS] POOL;
static chi_sdly_t chi_sdly[NBASS] POOL;
static chi_bpmdly_t chi_bpmdly[NBASS] POOL;
static chi_hall_t chi_hall[NBASS] POOL;
/* Chicago damper: live/MIDI sustain. Pattern sink ignores it. */
static uint8_t chi_damper[NBASS] POOL;
static uint8_t chi_damp_latched[NBASS] POOL; /* note-off deferred while damper on */
static breaks_t brk POOL;
static fxbus_t fx POOL;
void engine_fx_mute(uint8_t mask) { fx.muted = mask & 7u; }
uint8_t engine_fx_muted(void) { return fx.muted; }
static master_t mst;
#define DLY_LEN 88200
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
enum { C_PARAM_T0 = 0, C_DRUM = 8, C_BON, C_BOFF, C_BRK, C_NOP };
#define CQ 1024u
static uint32_t cq[CQ];
static volatile uint32_t cq_w, cq_r;

__attribute__((noinline)) static void cq_put(uint32_t type, uint32_t a, uint32_t b, uint32_t c)
{
    uint32_t w = cq_w;
    if (w - cq_r >= CQ)
        return;                        /* full: drop (a param flood; the mirror still has it) */
    cq[w % CQ] = (type << 28) | ((a & 0xFFu) << 20) | ((b & 0xFFu) << 12) | (c & 0xFFFu);
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
    case T_303: return CHI_AUX_END;                      /* … + SEND FX 27..29 + PBend/Damp 30..31 */
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
#if X0X_DEV_WOBBLE
        if (bass_wobble_voice(v)) {
            const x0x_param_t *p = wobble_param(i);
            if (p)
                return p;
        }
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
static void bass_voice_note_on(int v, int note, int acc, int slide, int vel)
{
    if (bass_wobble_voice(v))
        synth_note(&bass_slot[v], note, acc, slide, vel);
    else
        bass303_note_on_velocity(&b303[v], note, acc, slide, vel);
}
static void bass_voice_note_off(int v)
{
    if (bass_wobble_voice(v))
        synth_off(&bass_slot[v]);
    else
        bass303_note_off(&b303[v]);
}
#endif

/* Live/MIDI note-off: damper sustains the gate; pattern s_boff never calls this. */
static void chi_live_note_off(int v)
{
    if ((unsigned)v >= NBASS)
        return;
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

static void chi_arp_gate_off(int v)
{
    if ((unsigned)v >= NBASS)
        return;
    if (chi_arp_run[v].cur >= 0) {
        chi_live_note_off(v);
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
        if (i >= CHI_ARP_SWITCH && i < CHI_ARP_NPARAMS_END)
            chi_arp_apply_cfg(v, i, val);
        else if (i >= CHI_FX_TYPE && i < CHI_FX_END) {
            /* sound_t mirror + Crush/Ring/EQ/SDly/BPM/Hall insert coeffs; Mix/fxbus via UI map. */
            if (i == CHI_FX_TYPE) {
                if (val >= CHI_FX_NTYPES)
                    val = CHI_FX_OFF;
                chi_fx_type[v] = (uint8_t)val;
            } else if (i == CHI_FX_EDIT1) {
                chi_fx_e1[v] = (uint8_t)val;
            } else if (i == CHI_FX_EDIT2) {
                chi_fx_e2[v] = (uint8_t)val;
            }
            chi_crush_init(&chi_crush[v]);
            chi_ring_init(&chi_ring[v]);
            chi_eq_init(&chi_eq[v]);
            chi_sdly_init(&chi_sdly[v]);
            chi_bpmdly_init(&chi_bpmdly[v]);
            chi_hall_init(&chi_hall[v]);
            if (chi_fx_is_crush_insert(chi_fx_type[v]))
                chi_crush_set(&chi_crush[v], chi_fx_e1[v], chi_fx_e2[v]);
            else if (chi_fx_is_ring_insert(chi_fx_type[v]))
                chi_ring_set(&chi_ring[v], chi_fx_e1[v], chi_fx_e2[v], BASS303_SR);
            else if (chi_fx_is_eq_insert(chi_fx_type[v]))
                chi_eq_set(&chi_eq[v], chi_fx_e1[v], chi_fx_e2[v], BASS303_SR);
            else if (chi_fx_is_sdly_insert(chi_fx_type[v]))
                chi_sdly_set(&chi_sdly[v], chi_fx_e1[v], chi_fx_e2[v], BASS303_SR);
            else if (chi_fx_is_bpmdly_insert(chi_fx_type[v]))
                chi_bpmdly_set(&chi_bpmdly[v], chi_fx_e1[v], chi_fx_e2[v], BASS303_SR);
            else if (chi_fx_is_hall_insert(chi_fx_type[v]))
                chi_hall_set(&chi_hall[v], chi_fx_e1[v], chi_fx_e2[v], BASS303_SR);
        } else if (i >= CHI_PBEND && i < CHI_AUX_END)
            chi_aux_apply(v, i, val);
#if X0X_DEV_WOBBLE
        else if (bass_wobble_voice(v))
            wobble_foundation_set(bass_slot[v].active.context, i, val);
#endif
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
        s->v[T_303][v][BASS303_CUTOFF] = 48;
        s->v[T_303][v][BASS303_RESO] = 100;
        s->v[T_303][v][BASS303_ENVMOD] = 85;
        s->v[T_303][v][BASS303_ACCENT] = 100;
        s->v[T_303][v][BASS303_VOLUME] = 127;
        s->v[T_303][v][BASS303_DRIVE] = 45;
        s->v[T_303][v][BASS303_DRVTYPE] = 2;
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
    engine_set(T_MST, 0, MST_COMP1, s->v[T_MST][0][MST_COMP1]);   /* the macros first: the pots they set */
    engine_set(T_MST, 0, MST_DJF, s->v[T_MST][0][MST_DJF]);       /* follow as saved (a project's own sound) */
    for (t = 0; t < NTARGETS; t++)
        for (v = 0; v < engine_nvoices(t) && v < NVOICES_MAX; v++)
            for (i = 0; i < engine_nparams(t, v) && i < NPARAMS_MAX; i++)
                if (t != T_MST || i < MST_COMP1)
                    engine_set(t, v, i, s->v[t][v][i]);
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
static int prob_skip(int part, int v)                 /* ISR: 1 = this hit does not play */
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
static void s_drum(void *x, int kit, int v, float vel)
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
    (void)x;
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
static void s_brkstop(void *x) { (void)x; breaks_stop(&brk); }
static void s_midi(void *x, uint8_t st, uint8_t d1, uint8_t d2)
{
    uint32_t cin = st >= 0xF0 ? 0x0Fu : (uint32_t)(st >> 4);
    (void)x;
    plat_midi_out(cin | ((uint32_t)st << 8) | ((uint32_t)d1 << 16) | ((uint32_t)d2 << 24));
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
static uint8_t bass_held[NBASS];       /* note held from MIDI, for legato = slide */
static uint16_t src_held[3];           /* BLE-16: USB [0], Bluetooth [1], TRS [2]: bit k the 303 k, bit 8 + s the slice s */
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
    uint32_t r = cq_r;
    int k;
    if (brk.keys || src_held[0] || src_held[1] || src_held[2])
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
        if (t == C_DRUM || t == C_BON || (t == C_BRK && ((c >> 12) & 0xFFu)))
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
/* 8 SLICES: slice = note mod 8, so several notes can hold one slice */
static int brk_src_slice(int src, int slice)
{
    int n;
    for (n = slice; n < 128; n += 8)
        if (brk_note_down(src, n))
            return 1;
    return 0;
}
static int brk_any_slice(int slice)
{
    int src;
    for (src = 0; src < MIDI_NSRC; src++)
        if (brk_src_slice(src, slice))
            return 1;
    return 0;
}
/* Default map: notes 36-43, one note per slice. A stale Note Off from the other input does nothing. */
static void brk_midi_legacy(int src, int on, int slice)
{
    int v = 1 << (8 + slice);
    if (src < 0 || src >= MIDI_NSRC)
        return;
    if (!on && !(src_held[src] & (uint16_t)v))
        return;
    breaks_live(&brk, slice, on);
    src_held[0] &= (uint16_t)~v;
    src_held[1] &= (uint16_t)~v;
    src_held[2] &= (uint16_t)~v;
    if (on)
        src_held[src] |= (uint16_t)v;
}
/* 8 SLICES: every note. Note Off releases the slice only when no note of that slice remains.
 * USB, Bluetooth, and TRS share the same per-note refcount — a foreign Note Off must not
 * clear another input's hold (same ownership rule as midiowner / BF-004). */
static void brk_midi_wrap(int src, int on, int note)
{
    int slice = note & 7, v = 1 << (8 + slice);
    if (src < 0 || src >= MIDI_NSRC)
        return;
    if (!on) {
        if (!brk_note_down(src, note))
            return;                         /* stale Note Off: this input does not hold the note */
        brk_note_set(src, note, 0);
        if (!brk_src_slice(src, slice))
            src_held[src] &= (uint16_t)~v;
        if (!brk_any_slice(slice))
            breaks_live(&brk, slice, 0);
        return;
    }
    brk_note_set(src, note, 1);
    src_held[src] |= (uint16_t)v;
    breaks_slice_on(&brk, slice);           /* retrigger; the voice that is playing crossfades */
}
/* 8 SLICES map change: legacy note numbers and wrap (note mod 8) disagree, so MIDI
 * holds tracked in src_held / brk_down must not survive a mode toggle. */
/* UI / pref path when 8 SLICES toggles — not the audio or MIDI ISR (X0X_COLD) */
X0X_COLD void engine_brk_slice_mode_reconcile(void)
{
    int src, s, g;
    for (s = 0; s < 8; s++) {
        uint16_t mask = (uint16_t)(1u << (8 + s));
        if ((src_held[0] | src_held[1] | src_held[2]) & mask)
            breaks_live(&brk, s, 0);
    }
    for (src = 0; src < MIDI_NSRC; src++) {
        src_held[src] &= 0xFFu;
        for (g = 0; g < 16; g++)
            brk_down[src][g] = 0;
    }
}

static void brk_link_end(int src, uint16_t owned)
{
    int s, n;
    if (seq.pat[seq.ppat[TRK_BRK]].brk.slice_mode == 1) {
        for (n = 0; n < 128; n++)
            brk_note_set(src, n, 0);
        for (s = 0; s < 8; s++)
            if ((owned & (uint16_t)(1u << (8 + s))) && !brk_any_slice(s))
                brk.keys = (uint16_t)(brk.keys & ~(uint16_t)(1u << s));
        return;
    }
    brk.keys = (uint16_t)(brk.keys & ~(owned >> 8));
}

/* noinline: keeps GM_DRUM lookup out of engine_render when the map is shared with seq */
__attribute__((noinline)) static void midi_in(uint32_t pkt)
{
    uint8_t st = (uint8_t)(pkt >> 8), d1 = (uint8_t)(pkt >> 16) & 0x7F, d2 = (uint8_t)(pkt >> 24) & 0x7F;
    uint8_t ch = st & 0x0F, ty = st & 0xF0;
    int k, v, src = (int)PLAT_MIDI_SRC(pkt);
    if (!(pkt & ~0x30u)) {                      /* CIN 0, no message: the link of an input ended (usb.c midi_link_check) */
        seq_clk_drop(&seq, (uint32_t)src);      /* MID-15: another input can get the clock at once */
        if (src > 1)
            return;
        v = src_held[src];
        for (k = 0; k < NBASS; k++)             /* BLE-16: release the notes from that input (USB or Bluetooth) */
            if ((v >> k) & 1) {
#if X0X_DEV_WOBBLE
                synth_off(&bass_slot[k]);
#else
                bass303_note_off(&b303[k]);
#endif
                bass_held[k] = 0;
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
        if (seq.pat[seq.ppat[TRK_BRK]].brk.slice_mode == 1)
            brk_midi_wrap(src, ty == 0x90, d1);
        else if (d1 >= 36 && d1 <= 43)
            brk_midi_legacy(src, ty == 0x90, d1 - 36);
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
    for (k = 0; k < NBASS; k++)                         /* the 303s: overlapping notes slide */
        if (seq.ch[TRK_BASS0 + k] == ch) {
            if (ty == 0x90) {
#if X0X_DEV_WOBBLE
                bass_voice_note_on(k, d1, d2 >= 100, bass_held[k] != 0, d2);
#else
                bass303_note_on_velocity(&b303[k], d1, d2 >= 100, bass_held[k] != 0, d2);
#endif
                chi_damp_latched[k] = 0;
                bass_held[k] = d1;
                v = 1 << k;                     /* the input of the last note-on holds the note */
                src_held[0] &= (uint16_t)~v;
                src_held[1] &= (uint16_t)~v;
                src_held[2] &= (uint16_t)~v;
                if (src >= 0 && src < MIDI_NSRC)
                    src_held[src] |= (uint16_t)v;
            } else if (ty == 0x80 && bass_held[k] == d1) {
                v = 1 << k;
                if (src < 0 || src >= MIDI_NSRC || !(src_held[src] & (uint16_t)v))
                    continue;
                chi_live_note_off(k);
                bass_held[k] = 0;
                src_held[0] &= (uint16_t)~(1u << k);
                src_held[1] &= (uint16_t)~(1u << k);
                src_held[2] &= (uint16_t)~(1u << k);
            }
        }
}
#ifdef X0X_HOST
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

/* ---------------------------------------------------------------- render --- */
static void drain(void)
{
    uint32_t r = cq_r, pkt;
    int guard = 64;
    while (r != cq_w) {
        uint32_t c = cq[r % CQ], type = c >> 28, a = (c >> 20) & 0xFFu, b = (c >> 12) & 0xFFu, v = c & 0xFFFu;
        r++;
        if (type < 8u)
            apply_param((int)type, (int)a, (int)b, (int)v);
        else if (type == C_DRUM)
            s_drum(0, (int)a, (int)b, (float)v / 4095.0f);
        else if (type == C_BON && a < NBASS) {
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
                bass_voice_note_on((int)a, (int)b, (int)(v & 1u), (int)((v >> 1) & 1u), (int)(v >> 2));
#else
                bass303_note_on_velocity(&b303[a], (int)b, (int)(v & 1u), (int)((v >> 1) & 1u), (int)(v >> 2));
#endif
            }
            chi_damp_latched[a] = 0;
            bass_held[a] = (uint8_t)b;
        } else if (type == C_BOFF && a < NBASS) {
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
            breaks_live(&brk, (int)a, (int)b);
    }
    cq_r = r;
    while (guard-- && plat_midi_in(&pkt))
        if (!project_midi_drop)
            midi_in(pkt);
    if (guard >= 0)                   /* the ring was empty; subsequent MIDI belongs to live operation */
        project_midi_drop = 0;
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
 * Bits 9..19 carry this stretch's audible 808 tracks. */
static int brk_silent_now = 1;
static uint8_t d808_was_active;
static void core2_job(uint32_t arg)
{
    uint32_t n = arg & 0xFFu ? arg & 0xFFu : 256u, i, timed = !(arg & 0x100u), pt = timed ? plat_cycles() : 0;
    d808_was_active = (uint8_t)drum808_active(&d808);
    if (d808_was_active) {                                /* silent, it does nothing (and is not mixed) */
        for (i = 0; i < n; i++)
            t0[i] = t1[i] = t2[i] = tr[i] = 0.0f;
        drum808_render_st_masked(&d808, t0, tr, t1, t2, (int)n, (uint16_t)(arg >> 9 & 0x7FFu));
        bfx_shape(PART_808, t0, tr, t1, t2, 4, (int)n);
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
__attribute__((noinline)) static void render_sub(float *out_l, float *out_r, uint32_t n)
{
    float *bb[NBASS] = {out_l, out_r};
    int idle[NBASS], b;
    uint32_t solo = smc_solo_state;
    uint32_t arg = (n & 0xFFu) | (uint32_t)smc_audible_drums(solo, PART_808) << 9, i, pt;
    uint8_t solo_parts = (uint8_t)(solo & 0x1Fu);
    if (solo & (0x7FFu << 5)) solo_parts |= 1u << PART_909;
    if (solo & (0x7FFu << 16)) solo_parts |= 1u << PART_808;
    bfx_snapshot();                                   /* before the second core reads the holds */
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
        if (!idle[b] && chi_fx_is_crush_insert(chi_fx_type[b]))
            chi_crush_process(&chi_crush[b], bb[b], (int)n);
        else if (!idle[b] && chi_fx_is_ring_insert(chi_fx_type[b]))
            chi_ring_process(&chi_ring[b], bb[b], (int)n);
        else if (!idle[b] && chi_fx_is_eq_insert(chi_fx_type[b]))
            chi_eq_process(&chi_eq[b], bb[b], (int)n);
        else if (!idle[b] && chi_fx_is_sdly_insert(chi_fx_type[b]))
            chi_sdly_process(&chi_sdly[b], bb[b], (int)n);
        else if (!idle[b] && chi_fx_is_bpmdly_insert(chi_fx_type[b]))
            chi_bpmdly_process(&chi_bpmdly[b], bb[b], (int)n);
        else if (!idle[b] && chi_fx_is_hall_insert(chi_fx_type[b]))
            chi_hall_process(&chi_hall[b], bb[b], (int)n);
        if (!idle[b])
            bfx_shape(PART_303A + b, bb[b], 0, 0, 0, 1, (int)n);
        PROF_ADD(2 + b, pb);
    }
#endif
    pt = plat_cycles();
#if X0X_DEBUG
    eng_dbg_nsub++;
    {
        uint32_t w0 = plat_ticks24();
        int failed = on2 && plat_cpu2_wait();
        eng_dbg_wait += plat_ticks24() - w0;
        if (failed)
            on2 = 0;
    }
    if (!on2)
#else
    if (!on2 || plat_cpu2_wait())
#endif
        core2_job(arg);                                   /* one core (or the second failed): here */
    else
        PROF_ADD(PR_808, pt);                             /* what the first core waited for the second */
    if (d808_was_active) {
        mix_targets(PART_808, mix_level[PART_808], solo_parts);
        add_glide(dry_l, t0, &mix_glide[PART_808][GL_L], n);
        add_glide(dry_r, tr, &mix_glide[PART_808][GL_R], n);
        add_glide(rev, t1, &mix_glide[PART_808][GL_VREV], n);
        add_glide(dly, t2, &mix_glide[PART_808][GL_VDLY], n);
        for (i = 0; i < n; i++)
            t0[i] = (t0[i] + tr[i]) * 0.5f;
        kit_sends(PART_808, t0, n);
    }
    for (b = 0; b < NBASS; b++) {
        int p = PART_303A + b;
#if X0X_DEV_WOBBLE
        PROF_T(pb);
        idle[b] = synth_idle(&bass_slot[b]);
        if (!wobble_backend_stereo(&bass_slot[b], bb[b], bb[1 - b], (int)n))
            synth_render(&bass_slot[b], bb[b], (int)n);
        /* Crush/Ring/EQ/SDly/BPM/Hall inserts are Chicago/native303 only — never on WBL1. */
        if (!idle[b] && bass_slot[b].active.ops != &wobble_foundation_ops) {
            if (chi_fx_is_crush_insert(chi_fx_type[b]))
                chi_crush_process(&chi_crush[b], bb[b], (int)n);
            else if (chi_fx_is_ring_insert(chi_fx_type[b]))
                chi_ring_process(&chi_ring[b], bb[b], (int)n);
            else if (chi_fx_is_eq_insert(chi_fx_type[b]))
                chi_eq_process(&chi_eq[b], bb[b], (int)n);
            else if (chi_fx_is_sdly_insert(chi_fx_type[b]))
                chi_sdly_process(&chi_sdly[b], bb[b], (int)n);
            else if (chi_fx_is_bpmdly_insert(chi_fx_type[b]))
                chi_bpmdly_process(&chi_bpmdly[b], bb[b], (int)n);
            else if (chi_fx_is_hall_insert(chi_fx_type[b]))
                chi_hall_process(&chi_hall[b], bb[b], (int)n);
        }
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
        PROF_ADD(5, pf);
    }
    bfx_flt_shape(out_l, out_r, (int)n);
}

void engine_render(int32_t *out_lr, uint32_t n)
{
    static float out_l[256], out_r[256];
    uint32_t done = 0, i;
    float vol = usb_full ? 1.0f : (float)plat_master() / 4096.0f;   /* (FULL: engine_volume) */
    int p;
    if (n > 256u)
        n = 256u;
    if (project_hold) {
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
#if X0X_DEV_WOBBLE
        for (p = 0; p < NBASS; ++p) {
            if (synth_boundary(&bass_slot[p])) {
                bass_held[p] = 0;
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
        if (mot_on && seq.playing && !seq_clock_waiting(&seq))
            motion_tick(&mot, n);
        PROF_ADD(7, pt);
    }
    while (done < n) {
        uint32_t k = seq_until_event(&seq, n - done);
        chi_arp_tick(k);
        render_sub(out_l + done, out_r + done, k);
        {
            PROF_T(pt);
            seq_advance(&seq, k, &SINK);
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
    *slice = brk.st_slice;
    *bank = brk.st_bank;
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
            breaks_set_loop(&brk, w, &L[w]);
        } else {
            breaks_set_loop(&brk, w, brk_builtin(slot < nb ? slot : 0));   /* built-in, or the first if the slot is empty */
        }
    }
}

void engine_brk_flush(void) { breaks_flush(&brk); }

void engine_brk_drop_range(const uint8_t *base, uint32_t nbytes)
{
    breaks_drop_range(&brk, base, nbytes);
}

X0X_COLD void engine_init(pattern_t *patterns, song_t *song, lane_t *lanes, const sound_t *base)
{
    int p, g;
    cq_w = cq_r = 0;
    src_held[0] = src_held[1] = src_held[2] = 0;
    for (p = 0; p < MIDI_NSRC; p++)
        for (g = 0; g < 16; g++)
            brk_down[p][g] = 0;
    for (p = 0; p < NBASS; p++) {
        bass_held[p] = 0;
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
    chi_crush_init(&chi_crush[0]);
    chi_crush_init(&chi_crush[1]);
    chi_ring_init(&chi_ring[0]);
    chi_ring_init(&chi_ring[1]);
    chi_eq_init(&chi_eq[0]);
    chi_eq_init(&chi_eq[1]);
    chi_sdly_init(&chi_sdly[0]);
    chi_sdly_init(&chi_sdly[1]);
    chi_bpmdly_init(&chi_bpmdly[0]);
    chi_bpmdly_init(&chi_bpmdly[1]);
    chi_hall_init(&chi_hall[0]);
    chi_hall_init(&chi_hall[1]);
#if X0X_DEV_WOBBLE
    synth_init(&bass_slot[0], synth303_binding(&b303[0]));
    synth_init(&bass_slot[1], synth303_binding(&b303[1]));
    wobble_foundation_init(&dev_wobble);
    synth_request(&bass_slot[0], wobble_foundation_binding(&dev_wobble), 0);
    synth_boundary(&bass_slot[0]); /* DEV: 303A is WBL1 before first audio block (UI/SMC pages). */
#endif
    breaks_init(&brk);
    fxbus_init(&fx, dly_buf, DLY_LEN);
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
