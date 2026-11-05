/* SPDX-License-Identifier: GPL-3.0-only */
/* BREAK part: a 4/8/16/32-slice breakbeat player plus the BB Gen generator.
 *
 * Generator ported from schwung-breakbeat (BB Gen) by mestela, used with the
 * author's permission. See breaks.c for what was kept and what was adapted.
 *
 * A loop is IMA ADPCM in flash (mono, low nibble first, initial state 0/0). It is
 * cut into the loop's selected equal slices; the generator picks a slice on every trigger, and the
 * player resamples it so the loop's stated `bars` follow the current tempo (pitch
 * follows tempo, as in BB Gen), times the Pitch param.
 *
 * Threading: the main loop may call breaks_set_loop while the audio ISR and the
 * second core run breaks_step / breaks_render. A new loop is built in a snapshot
 * no voice is reading, then published with one byte. The voice that is already
 * sounding keeps the snapshot it started on, so a loop change does not mute it
 * and does not rewrite its samples. breaks_set_loop(NULL) drops that loop's
 * voices: the samples may be about to leave. Output rate is BRK_FS. Float only,
 * no libm, no allocation. */
#pragma once
#include <stdint.h>
#include "x0x_param.h"
#include "../seq/pattern.h"           /* BRK_* settings order */

#define BRK_GRAIN_HOP 3969u            /* 90 ms grains; half-rate source progression */
#define BRK_FS 44100                  /* output sample rate */
#define BRK_NCK 480                   /* ADPCM checkpoints per loop (4 bytes each); bounded reverse-state reuse */
#define BRK_RCH 512                   /* reverse playback decodes this many samples at a time */
#ifndef BRK_FADE
#define BRK_FADE 88                   /* crossfade / gate ramp: 2 ms at 44.1 kHz */
#endif

typedef struct {
    const uint8_t *adpcm;   /* IMA ADPCM nibbles, low nibble first, initial predictor 0 / index 0 */
    uint32_t nsamples;      /* decoded samples */
    uint32_t rate;          /* 22050 (or 44100) */
    uint8_t bars;           /* musical length of the loop in bars (the user states it; 1..8) */
    uint8_t slices;         /* 0 = compatibility default 8; otherwise 4,8,16,32 */
} brk_loop_t;

/* indices of breaks_param() past the per-pattern settings */
enum { BRK_LEVEL = BRK_NSET, BRK_PITCH, BRK_REV, BRK_DLY, BRK_NPARAM };

typedef struct {
    int16_t pred;                     /* IMA state BEFORE decoding the nibble at that index */
    uint8_t idx, rsv;
} brk_ima_t;

typedef struct brk_bank {
    const uint8_t *adpcm;
    uint32_t n;                       /* samples */
    float sp16;                       /* source samples per 16th note: n / (bars * 16) */
    uint32_t start[33];                /* exact max-32 boundaries, start[32] = n */
    brk_ima_t at_start[32];            /* exact decoder state at each max-32 boundary */
    brk_ima_t ck[BRK_NCK];            /* decoder state every 1 << ck_shift samples */
    uint8_t ck_shift;
    volatile uint8_t valid;           /* 0 while this snapshot is being built; published only when 1 */
    uint8_t gen;                      /* bumped on every load of this snapshot */
    uint8_t slices;                   /* immutable count for this snapshot */
} brk_bank_t;

typedef struct {
    uint8_t on, rev, bank, gen;
    const struct brk_bank *snap;      /* bank snapshot this voice keeps reading */
    uint32_t pos;                     /* index of s1 (the sample being approached) */
    int32_t pred, idx;                /* forward: decoder state after decoding pos */
    float frac, s0, s1;               /* output = s0 + (s1 - s0) * frac */
    float g, dg;                      /* crossfade gain and its per-sample slope */
    uint32_t lo, hi;                  /* reverse: wraps within [lo, hi) (the slice) */
    uint32_t rbase;                   /* reverse: rbuf holds samples rbase .. rbase + BRK_RCH - 1 */
    uint8_t rvalid, rsv[3];
    int16_t rbuf[BRK_RCH];
    brk_ima_t seek_state;             /* before previous adjacent reverse-cache block */
    uint32_t seek_pos;               /* UINT32_MAX when no saved boundary is valid */
} brk_voice_t;

/* BB Gen's live performance state (perf.h), reduced to this part's keys */
typedef struct {
    int8_t stack[32];                  /* held A slices0..31, most recent last */
    uint8_t count;
    uint8_t reverse, jungle, stutter; /* momentary macros held */
} brk_perf_t;

typedef struct breaks {
    brk_bank_t bank[2];               /* 0 = A, 1 = B, when pub[] still names them */
#if defined(X0X_HOST)
    brk_bank_t side[2];               /* host tests may use several breaks_t at once */
#endif
    volatile uint8_t pub[2];          /* published snapshot: 0..1 bank[], 2..3 side[] */
    uint8_t pend_on[2];               /* a change waiting for a free snapshot */
    brk_loop_t pend[2];
    brk_voice_t v[2];                 /* the sounding voice and the one fading out */
    uint8_t cur;                      /* index of the sounding voice */
    uint8_t set[BRK_NPARAM];          /* parameter values */
    uint32_t rng;                     /* xorshift32 */

    /* transport / generator (BB Gen's breakbeat_t, tick mode) */
    uint8_t running, engine_bank, render_bank;
    int8_t pending_bank;              /* bank to switch to at the next bar, -1 = none */
    int current_slice;
    int bar;                          /* bar of the latest step */
    int trigger_count;                /* triggers since start or the last bank switch */
    float sp16;                       /* output samples per 16th, from the sequencer */
    float spt;                        /* output samples per trigger */
    int32_t trig_left;                /* samples to a pending mid-16th trigger (1/4 bar length) */
    uint8_t step_on;                  /* this step's enable bit */

    /* retrigger (BB Gen's sub-slice) */
    uint8_t sub_div, sub_count;
    int32_t sub_elapsed;              /* output samples since the trigger */
    float sub_len;                    /* output samples per sub-hit */

    /* gate: the step mask, ramped */
    float gate, gate_to;
    uint8_t quiet;                    /* X0X: the part cannot be heard (muted, no steps on): while the gate
                                       * is shut its voices stop; the next trigger starts them again */

    /* Slice ownership and legacy macro bits are independent; applied by audio. */
    volatile uint8_t local_guard; /* pin/live gesture count-change admission */
    volatile uint32_t slices;
    uint32_t slices_seen;
    volatile uint16_t keys;
    uint16_t keys_seen;
    brk_perf_t perf;
    int16_t bend;                    /* transient -8192..8191, +/-12 semitones; never saved */
    uint16_t grain_left;
    uint32_t grain_anchor;
    float grain_distance;            /* half of source distance travelled during this grain */

    /* status of the latest trigger (the UI's readout, and the tests' probe) */
    uint32_t st_trigs;                /* triggers fired since init */
    int8_t st_slice, st_bank, st_div, st_bp;
    uint8_t st_forced;
    int8_t pin;                       /* X0X: the next step's own slice of loop A (breaks_pin), -1 = the generator's */
    volatile uint8_t restart;         /* slice + 1: retrigger that held slice; 0 = none. Not a loop change. */
    void (*accepted)(void *ctx, int slice, int bank, unsigned gen);
    void *accepted_ctx;
} breaks_t;

void breaks_init(breaks_t *b);
void breaks_set_loop(breaks_t *b, int which /*0 = A, 1 = B*/, const brk_loop_t *loop);   /* NULL = empty (silent) */
void breaks_flush(breaks_t *b);                /* publish a loop change that waited for a free snapshot */
void breaks_drop_range(breaks_t *b, const uint8_t *base, uint32_t nbytes); /* samples at base are going away */
const brk_bank_t *breaks_bank(const breaks_t *b, int which); /* the published loop, for the screen */
void breaks_step(breaks_t *b, int step16, int bar, float samples_per_16th, int enabled);
void breaks_pin(breaks_t *b, int slice);           /* next step: stored A slice0..31 resolved modulo count; -1 generator */
void breaks_stop(breaks_t *b);
void breaks_live(breaks_t *b, int key, int down);  /* 0-7 hold slice of A, 8 reverse, 9 jungle stretch, 10 stutter */
void breaks_slice_on(breaks_t *b, int slice); /* hold slice0..31 and retrigger it; sounding voice crossfades */
void breaks_render(breaks_t *b, float *out, int n);  /* WRITES mono, n <= 256 */
void breaks_set_quiet(breaks_t *b, int quiet);
int breaks_silent(const breaks_t *b);               /* the next render writes zeros */
int breaks_nparams(void);
const x0x_param_t *breaks_param(int i);
void breaks_set(breaks_t *b, int i, int value);
int breaks_get(const breaks_t *b, int i);

void breaks_slice_hold(breaks_t *b, int slice, int down);
int breaks_count(const breaks_t *b, int bank);

void breaks_bend(breaks_t *b, int value); /* runtime offset; saved BRK_PITCH is unchanged */
