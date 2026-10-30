/* SPDX-License-Identifier: GPL-3.0-only */
/* X0X audio engine; see engine.h. */
#include "engine.h"
#include "plat.h"
#include "../x0x_cold.h"
#include "../dsp/drum909.h"
#include "../dsp/drum808.h"
#include "original_runtime.h"
#include "../../../experiments/original_synth/chicago_fx2_compact.h"
#include "../dsp/breaks.h"
#include "../dsp/fxbus.h"
#include "../dsp/master.h"
#include "../dsp/fastmath.h"
#include "../dsp/busfx.h"
#include "smc_fx_map.h"
#include "../dsp/ima_adpcm.h"              /* shared IMA tables + ima_decode (outline) */
#if X0X_BASS_PRESET_PREVIEW
#include "bass_presets.h"
#endif

#define POOL X0X_POOL


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
static volatile uint8_t project_hold;
static original_runtime_part original_bass[NBASS] POOL;
static uint16_t original_quiet_samples[NBASS], original_switch_gain[NBASS];
#ifdef X0X_HOST
/* Read-only host diagnostics for the actual adoption boundary. */
#define ENGINE_ORIGINAL_TAIL_OWNED (1u << 16)
unsigned engine_original_tail_state(int v)
{
    return (unsigned)v < NBASS ? original_quiet_samples[v] |
        (original_runtime_owned(&original_bass[v]) ? ENGINE_ORIGINAL_TAIL_OWNED : 0u) | ((unsigned)original_switch_gain[v] << 17) : UINT32_MAX;
}
#endif
static original_runtime_workspace original_workspace POOL;
static original_chicago_tuning original_tuning POOL;
static original_chicago_fx2_compact original_fx[NBASS] POOL;
static original_chicago_fx2_cold_owner original_fx_owner[NBASS];
static uint8_t original_fx_ready;
static uint8_t original_key_pitch[NBASS][38]; /* token -> pitch+1; zero is released */
static uint8_t original_midi_channel[NBASS];
static int16_t original_seq_note[NBASS];
int engine_bass_status(int v) { return (unsigned)v < NBASS ? original_bass[v].status : 0; }
int engine_bass_filter_view(int v, float *eg, float *depth)
{ return (unsigned)v < NBASS && original_runtime_filter_view(&original_bass[v], eg, depth); }
static void original_prepare_all(void)
{
    for (int v = 0; v < NBASS; ++v) (void)original_runtime_prepare(&original_bass[v], seq_tempo(&seq));
}
#ifdef X0X_HOST
unsigned engine_original_keys(int v, int latched)
{
    if ((unsigned)v >= NBASS || original_bass[v].status != ORIGINAL_RUNTIME_OK) return 0;
    const chicago_arp_control *keys = original_bass[v].kind == ORIGINAL_SOUND_MIAMI ?
        &original_bass[v].dsp.miami.keys : &original_bass[v].dsp.chicago.keys;
    return latched ? keys->latched_count : keys->physical_count;
}
uint32_t engine_original_value_bits(int v, unsigned parameter)
{
    if ((unsigned)v >= NBASS || parameter >= original_sound_count(original_bass[v].kind) ||
        original_bass[v].status != ORIGINAL_RUNTIME_OK) return UINT32_MAX;
    float value = original_bass[v].kind == ORIGINAL_SOUND_MIAMI ?
        original_bass[v].dsp.miami.program.macro[parameter] : original_bass[v].dsp.chicago.program.macro[parameter];
    uint32_t bits; __builtin_memcpy(&bits, &value, sizeof bits); return bits;
}
#endif
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
    *arp=0; /* The original allocator owns ARP; UI submits every physical key. */
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

static breaks_t brk POOL;
/* Latest-value physical wheel mailbox: centre cannot be dropped by a full CQ.
 * Only audio writes the DSP bend and the external MIDI owner. */
static volatile int16_t brk_wheel;
static int16_t brk_wheel_seen;
static int8_t brk_bend_owner;
void engine_brk_bend(int value)
{
    brk_wheel = (int16_t)(value < -8192 ? -8192 : value > 8191 ? 8191 : value);
}
#ifdef X0X_HOST
int engine_brk_bend_value(void) { return brk.bend; }
#endif
static fxbus_t fx POOL;
static volatile uint8_t tape_signal_seen;
static volatile uint32_t tape_signal_at;
/* The original ca8299b6 visual activity observer. No DSP output change. */
uint8_t engine_tape_active(void)
{
    uint32_t at = tape_signal_at;
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
    if (guard_on == guard_applied)
        return;
    guard_applied = guard_on;
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
/* C_REC reservation/priority matches the separately reviewed atomic REC unit; C_BASE is #684's exact base restore. */
enum { C_PARAM_T0 = 0, C_DRUM = 8, C_BON, C_BOFF, C_BRK, C_NOP, C_REC, C_BASE };
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
        uint32_t i, keep = (type == C_BON) || (type == C_BOFF) || (type == C_BRK) || (type == C_DRUM) || (type == C_REC) || (type == C_BASE);
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
        return (int)original_sound_count((unsigned)engine_bass_bank(v));
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
        return original_sound_param((unsigned)engine_bass_bank(v), (unsigned)i);
    case T_BRK: return breaks_param(BRK_NSET + i);
    case T_FX: return fxbus_param(i);
    case T_MIX: return (i >= 0 && i < MX_NPARAMS) ? &MIX_P[i] : 0;
    case T_MST: return master_param(i);
    default: return 0;
    }
}

X0X_COLD int engine_param_si(int t, int v, int i, int pot, float *val)
{
    switch (t) {
    case T_909: return drum909_param_si(v, i, pot, val);
    case T_808: return drum808_param_si(v, i, pot, val);
    case T_FX: return fxbus_param_si(i, pot, val);
    case T_BRK:                                      /* breaks.c: lvl = set/127 */
        if (i == 0) { *val = (float)pot / 127.0f; return X0X_U_GAIN; }
        return X0X_U_NONE;
    default: return X0X_U_NONE;
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

int engine_bass_is_wobble(int v)
{ return (unsigned)v < NBASS && original_bass[v].kind == ORIGINAL_SOUND_MIAMI; }

/* Live/MIDI note-off: damper sustains the gate; pattern s_boff never calls this. */
__attribute__((noinline)) static void chi_live_note_off(int v)
{
    if ((unsigned)v >= NBASS)
        return;
    (void)original_runtime_release_source(&original_bass[v], ORIGINAL_INPUT_KEYS);
    __builtin_memset(original_key_pitch[v], 0, sizeof original_key_pitch[v]);
}

/* Called by audio only, before effective-mode changes or another source's
 * gate write. Do not issue an old keyboard off against the new owner's gate. */
static void live_key_retire(int v,int release)
{
    live_key_owner_t *o=&live_key_owner[v];
    if(release && o->gate==1) chi_live_note_off(v);
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


static void apply_param(int t, int v, int i, int val)    /* ISR */
{
    switch (t) {
    case T_909: drum909_set(&d909, v, i, val); break;
    case T_808: drum808_set(&d808, v, i, val); break;
    case T_303:
        if (v < 0 || v >= NBASS)
            break;
        {
            float value;
            if (original_sound_control_value(original_bass[v].kind, (unsigned)i, (unsigned)val, &value))
                (void)original_runtime_param(&original_bass[v], (unsigned)i, value);
        }
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
void engine_set_base(int t, int v, int i) { cq_put(C_BASE, (uint32_t)t, (uint32_t)v, (uint32_t)i); }
static void apply_base(int t, int v, int i)
{
    if (!mot_base_snd || (unsigned)t >= NTARGETS || (unsigned)v >= NVOICES_MAX || (unsigned)i >= NPARAMS_MAX) return;
    if (t == T_303 && (unsigned)v < NBASS) {
        float value;
        if (original_sound_value(mot_base_snd->v[t][v], (unsigned)i, &value))
            (void)original_runtime_param(&original_bass[v], (unsigned)i, value);
        return;
    }
    apply_param(t, v, i, mot_base_snd->v[t][v][i]);
}

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
        int bank = v == 0 ? ORIGINAL_SOUND_MIAMI : ORIGINAL_SOUND_CHICAGO;
        (void)original_sound_init(s->v[T_303][v], (unsigned)bank, 0);
        bass_tag_set(s, v, bank, 0, 0);
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
        s->v[T_MIX][PART_303A + v][MX_LEVEL] = 100;
        s->v[T_MIX][PART_303A + v][MX_DLY] = 0;
        s->v[T_MIX][PART_303A + v][MX_REV] = 0;
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
                    && t != T_303 /* already restored from exact factory + explicit edits */
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
    if ((unsigned)p >= NBASS) return 0;
    (void)x;
    (void)acc;
    if (prob_skip(PART_303A + p, 0)) {
        (void)original_runtime_release_source(&original_bass[p], ORIGINAL_INPUT_SEQUENCE);
        original_seq_note[p] = -1;
        return 0;
    }
    int previous = original_seq_note[p];
    if (!sl && previous >= 0)
        (void)original_runtime_key(&original_bass[p], ORIGINAL_INPUT_SEQUENCE, (unsigned)previous, 0);
    int rc = original_runtime_key(&original_bass[p], ORIGINAL_INPUT_SEQUENCE, (unsigned)note, (unsigned)velocity);
    if (sl && previous >= 0)
        (void)original_runtime_key(&original_bass[p], ORIGINAL_INPUT_SEQUENCE, (unsigned)previous, 0);
    original_seq_note[p] = (int16_t)note;
    return rc == ORIGINAL_RUNTIME_OK;
}
static void s_boff(void *x, int p)
{
    (void)x;
    if ((unsigned)p >= NBASS) return;
    (void)original_runtime_release_source(&original_bass[p], ORIGINAL_INPUT_SEQUENCE);
    original_seq_note[p] = -1;
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
static int original_motion_performance(int t, int v, int i)
{
    return t == T_303 && (unsigned)v < NBASS &&
        original_sound_is_performance(original_bass[v].kind, (unsigned)i);
}
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
    if (original_motion_performance(t, v, i)) return;
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
static void mot_restore(void *x, int t, int v, int i)
{
    if (original_motion_performance(t, v, i)) return;
    if (t < NTARGETS) apply_base(t, v, i);
    else mot_apply(x, t, v, i, mot_base(x, t, v, i));
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
void engine_motion_clear(int k)
{
    /* Main-loop removal publishes unused BEFORE the queued restore. An IRQ
     * between them can keep the old value for one block, but cannot run the
     * removed ramp after the restoration or interleave DSP program writes. */
    if (k < 0 || k >= NLANE || !mot.lane) return;
    lane_t *lane = &mot.lane[k];
    int restore = lane->used && lane->t != 0xfdu;
    int t = lane->t, v = lane->v, i = lane->i;
    if (original_motion_performance(t, v, i)) restore = 0;
    lane->used = 0;
    __asm__ volatile("" ::: "memory");
    mot.rt[k].applied = MOT_NONE;
    mot.rt[k].ramp = mot.rt[k].rec_req = mot.rt[k].hold_req = 0;
    if (restore) engine_set_base(t, v, i);
}
static void s_transport(void *, int);
static const seq_sink_t SINK = {s_drum_seq, s_bon, s_boff, s_brk, s_brkstop, s_midi, s_step, 0
    , s_transport
};

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
static void s_transport(void *ctx, int running)
{
    (void)ctx;
    if (running) return;
    /* Explicit firmware Stop policy: release existing notes, including pedal
     * holds, through the original all-off lifecycle. New input after this
     * event is accepted immediately; idle clocks and effect tails still run. */
    for (int v = 0; v < NBASS; ++v) {
        (void)original_runtime_all_off(&original_bass[v]);
        /* Stop retires sounding keys, not the Free Play UI context. Its
         * already accepted FIFO follows this transport boundary; changing
         * generation here would discard the first new press after Stop.
         * Modal/cancel boundaries still revoke generation separately. */
        live_key_owner[v].gate = live_key_owner[v].token = 0;
        live_key_owner[v].pitch = live_key_owner[v].arp = 0;
        __builtin_memset(original_key_pitch[v], 0, sizeof original_key_pitch[v]);
        original_seq_note[v] = -1;
        bass_held[v] = bass_held_count[v] = 0;

    }
    src_held[0] = src_held[1] = src_held[2] = 0;
}
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
    /* A physical strike is already accepted before the audio owner drains its
     * FIFO. Do not start a project transfer in that publication window. */
    if (live_key_r != live_key_w || live_key_cancel)
        return 1;
    if (brk.keys || brk.slices || src_held[0] || src_held[1] || src_held[2])
        return 1;
    for (k = 0; k < NBASS; k++)
        if (original_runtime_active(&original_bass[k]))
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
    if (brk_bend_owner >= 0 && brk_bend_owner < MIDI_NSRC) {
        breaks_bend(&brk, 0); brk_bend_owner = -1;
    }
    brk_input_channel = channel;
}

/* Retire only original-synth MIDI owners when their channel changes. */
static void original_midi_context(void)
{
    for (int v = 0; v < NBASS; ++v) if (original_midi_channel[v] != seq.ch[TRK_BASS0 + v]) {
        for (unsigned source = 0; source < MIDI_NSRC; ++source) {
            (void)original_runtime_release_source(&original_bass[v], source);
            src_held[source] &= (uint16_t)~(1u << v);
        }
        if (bass_held_src[v] < MIDI_NSRC) bass_held[v] = bass_held_count[v] = 0;
        original_midi_channel[v] = seq.ch[TRK_BASS0 + v];
    }
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
        for (k = 0; k < NBASS; ++k) {
            (void)original_runtime_release_source(&original_bass[k], (unsigned)src);
            if ((v >> k) & 1) {

                if (bass_held_src[k] == (uint8_t)src) bass_held[k] = bass_held_count[k] = 0;
            }
        }
        if (brk_bend_owner == src) { breaks_bend(&brk, 0); brk_bend_owner = -1; }
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
    if (ty == 0xe0 && seq.ch[TRK_BRK] == ch) {
        /* Physical gestures own bend until release, otherwise last external
         * source wins. BREAK consumes its channel without live MIDI echo. */
        if (!brk_wheel) {
            breaks_bend(&brk, ((int)d1 | (int)d2 << 7) - 8192);
            brk_bend_owner = brk.bend ? (int8_t)src : -1;
        }
        return;
    }
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
            if (ty == 0x90 || ty == 0x80) {
                unsigned velocity = ty == 0x90 ? d2 : 0;
                if (original_runtime_key(&original_bass[k], (unsigned)src, d1, velocity) != ORIGINAL_RUNTIME_OK) continue;
                if (velocity) {
                    bass_held[k] = d1; bass_held_src[k] = (uint8_t)src;
                    bass_held_count[k] = original_bass[k].keys[src][d1];
                } else if (bass_held_src[k] == (uint8_t)src && bass_held[k] == d1) {
                    bass_held_count[k] = original_bass[k].keys[src][d1];
                    if (!bass_held_count[k]) {
                        bass_held[k] = 0;
                    }
                }
                int held = 0;
                for (unsigned key = 0; key < 128; ++key) held |= original_bass[k].keys[src][key];
                if (held) src_held[src] |= (uint16_t)(1u << k);
                else src_held[src] &= (uint16_t)~(1u << k);
            } else if (ty == 0xb0 && d1 == 64) {
                unsigned parameter = original_sound_count(original_bass[k].kind) - 1;
                (void)original_runtime_param(&original_bass[k], parameter, (float)d2 / 127.0f);
            } else if (ty == 0xe0) {
                unsigned bend = (unsigned)d1 | (unsigned)d2 << 7;
                float value = bend < 8192 ? (float)bend / 16384.0f : 0.5f + (float)(bend - 8192) / 16382.0f;
                unsigned parameter = original_sound_count(original_bass[k].kind) - 2;
                (void)original_runtime_param(&original_bass[k], parameter, value);
            }
        }
}
#ifdef X0X_HOST
int engine_bass_attacked(int part) { return (unsigned)part < NBASS && bass_attacked[part]; }
int engine_midi_held(void)   /* host test: bit k the 303 k gate, bit 8 + s the break slice key s */
{
    int k, r = brk.keys << 8 & 0xFF00;
    for (k = 0; k < NBASS; k++)
        r |= original_runtime_active(&original_bass[k]) ? 1 << k : 0;
    return r;
}
#endif

#include "original_presets.inc"

/* ---------------------------------------------------------------- render --- */
static void drain(void)
{
    original_midi_context();
    /* A UI Stop precedes a newly queued manual press, just as a MIDI Stop
     * precedes the following MIDI note. Retire old owners before CQ notes. */
    if (seq.req_stop) seq_advance(&seq, 0, &SINK);
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
    unsigned budget = 128; /* bound parameter floods to this audio boundary */
    while (r != cq_w && budget--) {
        uint32_t c = cq[r % CQ], type = c >> 28, a = (c >> 20) & 0xFFu, b = (c >> 12) & 0xFFu, v = c & 0xFFFu;
        r++;
        uint32_t rec_epoch = a >> 1;
        if (type == C_REC) a &= 1u;
        if (type < 8u)
            apply_param((int)type, (int)a, (int)b, (int)v);
        else if (type == C_BASE) {
            if (a < NTARGETS) apply_base((int)a, (int)b, (int)v);
            else mot_apply(0, (int)a, (int)b, (int)v, mot_base(0, (int)a, (int)b, (int)v));
        } else if (type == C_DRUM)
            s_drum(0, (int)a, (int)b, (float)v / 4095.0f);
        else if ((type == C_BON || type == C_REC) && a < NBASS) {
            int slide = type == C_REC ? live_rec_legato((int)a, 3) && bass_held[a] != b : (int)((v >> 1) & 1u);
#ifdef X0X_HOST
            bass_attacked[a] = (uint8_t)!slide;
#endif
            if (type == C_REC && rec_epoch == (seq.rec_epoch & 127u))
                seq_rec_bass(&seq, (int)a, (int)b, (int)((v >> 2) & 127u), 1, 3,
                    ((v & 1u) ? BS_ACCENT : 0) | ((v & 512u) ? BS_SLIDE : 0) | (slide ? SEQ_REC_LEGATO : 0));
            v &= 511u; /* the outgoing manual-slide flag is recorder metadata only */
            (void)original_runtime_key(&original_bass[a], ORIGINAL_INPUT_KEYS, b, v >> 2);
            bass_held[a] = (uint8_t)b;
            bass_held_count[a] = 1; bass_held_src[a] = 3;
            src_held[0] &= (uint16_t)~(1u << a); src_held[1] &= (uint16_t)~(1u << a); src_held[2] &= (uint16_t)~(1u << a);
            live_rec_accept((int)a, 3, (int)b);
        } else if (type == C_BOFF && a < NBASS && bass_held_src[a] == 3) {
            if ((!v || b == bass_held[a]) && seq.rec_bass[a].own_src == 3) {
                seq_rec_bass(&seq, (int)a, seq.rec_bass[a].own_note, 0, 0, 3, 0);
                seq.rec_bass[a].own_ok = 0;
                bass_held_count[a] = 0;
            }
            if (v) (void)original_runtime_key(&original_bass[a], ORIGINAL_INPUT_KEYS, b, 0);
            else {
                (void)original_runtime_release_source(&original_bass[a], ORIGINAL_INPUT_KEYS);
                __builtin_memset(original_key_pitch[a], 0, sizeof original_key_pitch[a]);
            }
            if (!v || b == bass_held[a]) bass_held[a] = bass_held_count[a] = 0;
        } else if (type == C_BRK)
            brk_local_event((int)a, (int)b);
    }
    cq_r = r;
    int16_t wheel = brk_wheel;
    if (wheel != brk_wheel_seen) {
        breaks_bend(&brk, wheel);
        brk_bend_owner = wheel ? 3 : -1;
        brk_wheel_seen = wheel;
    }
    while (guard-- > 0 && plat_midi_in(&pkt))
        if (!project_midi_drop) midi_in(pkt);
    if (guard >= 0) project_midi_drop = 0;
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
        (void)accent; (void)slide;
        uint8_t previous = original_key_pitch[v][token];
        if (velocity) {
            if (previous) (void)original_runtime_key(&original_bass[v], ORIGINAL_INPUT_KEYS, previous - 1u, 0);
            if (original_runtime_key(&original_bass[v], ORIGINAL_INPUT_KEYS, pitch, velocity) != ORIGINAL_RUNTIME_OK) continue;
            original_key_pitch[v][token] = (uint8_t)(pitch + 1u);
            o->token = (uint8_t)token; o->pitch = (uint8_t)pitch; o->gate = 1;
            bass_held[v] = (uint8_t)pitch; bass_held_count[v] = 1; bass_held_src[v] = 3;
        } else if (previous == pitch + 1u) {
            (void)original_runtime_key(&original_bass[v], ORIGINAL_INPUT_KEYS, pitch, 0);
            original_key_pitch[v][token] = 0;
            if (o->token == token) o->token = o->pitch = 0;
            unsigned held = 0;
            for (unsigned key = 1; key <= 37; ++key) held |= original_key_pitch[v][key];
            o->gate = held != 0;
            if (!held) bass_held[v] = bass_held_count[v] = 0;
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
    int b;
    float *left808 = t0 + offset, *right808 = tr + offset;
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
        PROF_T(pb);
        (void)original_runtime_prepare(&original_bass[b], seq_tempo(&seq));
        int status = original_runtime_render(&original_bass[b], &original_workspace, bb[b], bb[1-b], n);
        PROF_ADD(2 + b, pb);
        /* Failed/unadmitted DSP is visible through engine_bass_status. No
         * fallback renderer, guessed silence-success or idle clock skipping. */
        if (status != ORIGINAL_RUNTIME_OK) continue;
        /* One coherent request decision per sub-block. Once note ownership is
         * gone, fade residual insert output over 512 samples. Cancellation or
         * a fresh owner ramps back smoothly; DSP histories keep advancing. */
        int fade = engine_bass_preset_pending(b) == 2 && !original_runtime_owned(&original_bass[b]);
        for (i = 0; i < n; ++i) {
            if (fade) { if (original_switch_gain[b]) --original_switch_gain[b]; }
            else if (original_switch_gain[b] < 512u) ++original_switch_gain[b];
            float gain = (float)original_switch_gain[b] * (1.0f/512.0f);
            bb[b][i] *= gain; bb[1-b][i] *= gain;
            if (fm_fabsf(bb[b][i]) > 1.0f/65536.0f || fm_fabsf(bb[1-b][i]) > 1.0f/65536.0f)
                original_quiet_samples[b] = 0;
            else if (original_quiet_samples[b] < 512u) ++original_quiet_samples[b];
        }
        bfx_shape(p, bb[b], bb[1-b], 0, 0, 2, (int)n);
        mix_targets(p, mix_level[p], solo_parts);
        add_glide(dry_l, bb[b], &mix_glide[p][GL_L], n);
        add_glide(dry_r, bb[1-b], &mix_glide[p][GL_R], n);
        meter(p, bb[b], mix_level[p], n);
        meter(p, bb[1-b], mix_level[p], n);
        for (i = 0; i < n; ++i) bb[b][i] = .5f * (bb[b][i] + bb[1-b][i]);
        add_glide(rev, bb[b], &mix_glide[p][GL_REV], n);
        add_glide(dly, bb[b], &mix_glide[p][GL_DLY], n);
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
    static float out_l[256], out_r[256];
    uint32_t done = 0, i;
    float vol = usb_full ? 1.0f : (float)plat_master() / 4096.0f;   /* (FULL: engine_volume) */
    int p;
    if (n > 256u)
        n = 256u;
    if (!n) return;
    if (project_hold) {
        tape_signal_seen = 0;
        for (i = 0; i < n * 2u; i++)
            out_lr[i] = 0;
        return;
    }
    eng_prof_frames += n;
    original_prepare_all(); /* precedes every serial frame-zero edit and key */
    if ((mot_was_on && !mot_on) || mot_release_req) {   /* motion off, or its lanes replaced: knobs back */
        mot_release_req = 0;
        motion_release(&mot);
    }
    mot_was_on = mot_on;
    guard_apply();
    {
        PROF_T(pt);
        drain();
        /* A final MIDI Stop may have no following MIDI note to consume it.
         * Complete transport before the independent physical-key FIFO. */
        if (seq.ext_pending || seq.req_start || seq.req_stop || seq.req_cont)
            seq_advance(&seq, 0, &SINK);
        live_key_drain();
#if X0X_BASS_PRESET_PREVIEW
        bass_preset_boundary();
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
        render_sub(out_l + done, out_r + done, k, 0);
        {
            PROF_T(pt);
            original_prepare_all(); /* next actual sequencer stretch, before its events */
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
        bass_held[p] = bass_held_count[p] = bass_held_src[p] = 0;
        original_seq_note[p] = -1;
        original_midi_channel[p] = 255;
        __builtin_memset(original_key_pitch[p], 0, sizeof original_key_pitch[p]);
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
    brk_wheel = brk_wheel_seen = 0; brk_bend_owner = -1;
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
    mot.restore = mot_restore;
    mot_base_snd = base;
    if (!original_tuning.ready) (void)original_chicago_tuning_bind(&original_tuning, 0, 0, NULL, 0x6d70);
    if (!original_fx_ready) {
        for (p = 0; p < NBASS; ++p) {
            (void)original_chicago_fx2_compact_init(&original_fx[p]);
            original_fx_owner[p] = original_chicago_fx2_compact_owner(&original_fx[p]);
        }
        original_fx_ready = 1;
    }
    bass_restore_sound(base);
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
    case 3:return engine_original_keys(voice,0);
    case 4:return 0;
    case 5:return bass_held[voice];
    case 7:
        return original_runtime_active(&original_bass[voice]);
    case 8:return 0;
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
