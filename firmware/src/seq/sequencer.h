/* SPDX-License-Identifier: GPL-3.0-only */
/* X0X sequencer: runs in the audio ISR, sample accurate.
 *
 * The engine renders a block like this:
 *     while (n) {
 *         k = seq_until_event(&seq, n);       // samples until the next due event (>= 1)
 *         render k samples;
 *         seq_advance(&seq, k, &sink);        // moves time, fires the events now due
 *         n -= k;
 *     }
 * so a step lands on its exact sample, and engine code never sees time.
 *
 * Five parts, each with its own step counter (lengths and rates may differ):
 * 909, 808, 303 A, 303 B, BREAK (always 16ths, 16 to the bar). Each part plays its own
 * pattern (ppat[part]); track t is part t. The 909 part's length defines the bar: cued
 * patterns (or the next one of a chain, or the song's next bar) start when it wraps, and a
 * part whose pattern changed restarts on step 1 while the others keep running.
 *
 * Song: one entry per bar, each part's pattern and the part mutes. song_on plays it (and
 * loops); song_on + song_rec writes the live state (cues, mutes) into it from song_start on.
 *
 * Clock: internal (BPM) or external (24 ppqn MIDI clock, fed with seq_ext_clock()).
 * Swing delays every second 16th (909 shuffle: 0 = straight, 100 = 75 %).
 *
 * 303 semantics (as the TB-303 and Open303's sequencer):
 *   NOTE  plays; the gate falls at half the step, unless the step slides (BS_SLIDE)
 *         or the next step is a TIE, in which case it is held through.
 *   TIE   continues the sounding note (no retrigger); same gate rule.
 *   slide a NOTE following a step with BS_SLIDE glides from the held note
 *         (note_on with slide = 1, envelopes not retriggered).
 *   REST  releases.
 */
#pragma once
#include <stdint.h>
#include "pattern.h"

#define FS 44100u
#define TRK_DRUM 0                    /* + kit: 0 = 909, 1 = 808 */
#define TRK_BASS0 NKIT
#define TRK_BRK (NKIT + NBASS)
#define NTRACKS (NKIT + NBASS + 1)
/* mute bits: kit k voice v = k * NDRUM + v; then 303 A, 303 B, BREAK */
#define MUTE_BASS0 (NKIT * NDRUM)
#define MUTE_BRK (MUTE_BASS0 + NBASS)

/* the mute bits of part p (a drum machine: all its voices) */
static inline uint32_t seq_part_mask(int p)
{
    return p < NKIT ? ((1u << NDRUM) - 1u) << (p * NDRUM) : 1u << (MUTE_BASS0 + p - NKIT);
}

#define NSONG 192
#define SEQ_CH_OFF 0xFFu
/* a step's chance (the T_PROB lanes): 1-100 = that percentage, PROB_COND + k = the cycle k in the order
 * 1:2 2:2 1:3 2:3 3:3 1:4 ... 8:8 (A:B: on the A-th of every B passes of the part's pattern) */
#define PROB_COND 101
#define PROB_NCOND 35                    /* 2 + 3 + ... + 8 */
static inline void prob_cond(int k, int *a, int *b)
{
    int n = 2;
    while (k >= n) {
        k -= n;
        n++;
    }
    *a = k + 1;
    *b = n;
}
#define SONG_REP(b) (((b)->mute >> 5) + 1)    /* a bar plays 1-8 times: mute's bits 5-7 (the parts use 0-4) */
typedef struct {
    uint8_t pat[NTRACKS];             /* each part's pattern */
    uint8_t mute;                     /* bit p = part p muted */
} song_bar_t;
typedef struct {
    uint16_t len;                     /* bars; 0 = empty */
    uint8_t rsv[2];
    song_bar_t bar[NSONG];
} song_t;

typedef struct {
    int (*drum)(void *ctx, int kit, int voice, float vel);      /* 0: the hit did not play (its chance) */
    int (*bass_on)(void *ctx, int part, int note, int accent, int slide, int velocity); /* 0: chance skipped; do not echo MIDI */
    void (*bass_off)(void *ctx, int part);
    void (*brk)(void *ctx, int step16, int bar, float samples_per_16th, int enabled);
    void (*brk_stop)(void *ctx);
    void (*midi)(void *ctx, uint8_t status, uint8_t d1, uint8_t d2);   /* clock / transport / notes out */
    void (*step)(void *ctx, int part, int step);                        /* UI playhead (optional) */
    void *ctx;
    void (*transport)(void *ctx, int running); /* ordered with following input */
} seq_sink_t;

#define SEQ_EXT_BEATS 8u              /* the tempo of an external clock: the mean over this many beats */
/* the clock source (GLO > MIDI > CLOCK SOURCE): AUTO follows a clock when one comes in (the default),
 * INTERNAL ignores MIDI Clock, Start, Stop and Continue, EXTERNAL plays only on the incoming clock */
#define SEQ_CLK_AUTO 0u
#define SEQ_CLK_INT 1u
#define SEQ_CLK_EXT 2u
/* AUTO and EXTERNAL follow one clock input (BEHAVIOR.md MID-15). The input of a MIDI in packet is in byte 0,
 * bits 4-5 (plat.h PLAT_MIDI_SRC): 0 USB, 1 Bluetooth, 2 TRS. The first input that sends Clock, Start or
 * Continue gets the clock. It keeps the clock until it sends nothing for SEQ_LOCK_T, or until SEQ_LOCK_T
 * after its Stop (then a Start or Continue of another input gets it). 0.4 s: more than 3 clocks at 20 BPM
 * (125 ms each), and less than the 0.5 s after which X0X goes to its internal clock or waits (MID-12, MID-13).
 * When another input gets the clock, the tempo measure starts again from the tempo that X0X has now. */
#define SEQ_LOCK_T (FS * 2u / 5u)

typedef struct {                      /* one part's runtime */
    uint8_t pos;                      /* current step */
    int8_t pp_dir;                    /* ping-pong direction */
    uint8_t started;                  /* the first step has fired since start */
    uint8_t sounding;                 /* 303: a note is held */
    uint8_t held_note;
    uint8_t ext_due;                  /* external clock: more steps due in one block, not yet played */
    uint32_t to_next;                 /* samples to the next step (Q8 fraction) */
    uint32_t to_off;                  /* 303: samples to the gate falling, Q8; 0 = none */
    uint32_t rng;                     /* DIR_RANDOM and auto-mutate */
    uint16_t bars;                    /* wraps since the pattern started (auto-mutate) */
    uint8_t ext_ticks;                /* external clock: ticks into this step */
    uint8_t rec_skip_step;            /* a live hit recorded on a step not played yet on this pass: */
    uint8_t rec_skip_pat;             /* that step, its pattern, and the voices (bit v; 303: bit 0) */
    uint8_t mut_steps;                /* random 303: low 7 pass count, high bit time-slot swing parity */
    uint16_t rec_skip;                /* that the step does not sound once (the key sounded) */
} seq_track_t;

typedef struct {
    uint32_t at, slot;
    uint8_t active, pat, first, last, prev, note, src, entered;
    uint8_t gen;       /* bumped on context cancel; a reopened DSP gate is not this generation */
    uint8_t own_ok;    /* 1 = own_* is an accepted live press of own_gen */
    uint8_t own_gen;   /* generation that accepted the live press */
    uint8_t own_src;   /* route of that press */
    uint8_t own_note;  /* pitch of that press */
} seq_bass_rec_t;

typedef struct {
    pattern_t *pat;                   /* NPAT patterns (the project's) */
    song_t *song;                     /* the project's song (may be 0) */
    volatile uint8_t playing;         /* 1 = running */
    volatile uint8_t ppat[NTRACKS];   /* the pattern each part plays */
    volatile uint8_t pcue[NTRACKS];   /* each part's cued pattern (0xFF = none) */
    volatile uint8_t chain_a, chain_b;/* chain range (whole patterns); a == b = no chain */
    volatile uint8_t song_on;         /* 1 = the song decides the patterns, bar by bar */
    volatile uint8_t song_rec;        /* with song_on: write the live state into the song */
    volatile uint16_t song_pos;       /* bar playing */
    volatile uint16_t song_start;     /* bar PLAY starts at */
    volatile uint8_t ext;             /* 1 = following an external clock */
    volatile uint8_t send_clock;      /* 1 = emit MIDI clock + transport when internal */
    volatile uint8_t send_notes;      /* 1 = echo the sequence on MIDI, on each part's channel */
    volatile uint8_t ch[NTRACKS];     /* each part's MIDI channel, 0-15 (in and out); SEQ_CH_OFF = none */
    uint8_t song_rep;                 /* the song bar playing: its passes still to play (SONG_REP) */
    volatile uint32_t mute;           /* MUTE_* bits */
    volatile uint32_t vmute;          /* drum tracks muted on their own (bit k * NDRUM + v): kept apart from
                                       * the part mutes, which the song and HOME set as a whole */
    volatile uint8_t accent_q7;       /* drum velocity of a non-accented hit, of 127 */
    volatile uint8_t drum_vel[NKIT][NDRUM]; /* per-voice step velocity from a T_VEL lane (engine); 0 = accent law */
    float bpm;                        /* internal tempo (UI writes; read per step) */
    float ext_bpm;                    /* tempo measured from the external clock */
    seq_track_t t[NTRACKS];           /* TRK_DRUM, TRK_BASS0.., TRK_BRK */
    /* Audio-side visual observer. UI reads only metro_view (one atomic byte):
     * 0 = stopped/waiting, 1..32 = quarter-note phase. Never schedules events. */
    uint32_t metro_phase, metro_period; /* elapsed/period in sample Q8 */
    float metro_bpm;
    uint8_t metro_tick;               /* accepted external clocks, independent of tempo ring */
    volatile uint8_t metro_view;
    uint32_t clk_to_next;             /* internal: samples to the next MIDI clock, Q8 */
    uint32_t ext_since;               /* samples since the last external clock (tempo estimate) */
    uint32_t ext_period;              /* smoothed samples per external clock, Q8 */
    uint32_t ext_now;                 /* samples, free running: the time base of the beat ring */
    uint32_t ext_beat[SEQ_EXT_BEATS]; /* ext_now at the last beats (24 external clocks each), a ring */
    uint8_t ext_nb, ext_bi, ext_q;    /* beats in the ring, the next ring slot, clocks into this beat */
    volatile uint32_t ext_pending;    /* external clocks received, not yet consumed */
    volatile uint32_t req_prior_ticks;/* ticks queued before latest Start/Continue request */
    volatile uint8_t req_start, req_stop, req_cont; /* transport requests from the UI / MIDI */
    volatile uint8_t midi_req;        /* 1 = the pending request came from MIDI in clip mode: no echo */
    volatile uint8_t rec_epoch, rec_dirty, rec_mask, rec_refused; /* accepted audio-side recorder context */
    volatile uint8_t rec_ext_mask;              /* REC on: every 303 records external MIDI (src<3) on its channel */
    volatile uint8_t rec_hold_mask;             /* finish an onset carried from SEQ into EDIT; no new onsets */
    seq_bass_rec_t rec_bass[NBASS];
    uint32_t mut_rng[NBASS];          /* auto-mutate random streams */
    uint8_t bass_notes_out[NBASS];    /* note echoed on MIDI, for the note off */
    uint8_t bass_out_valid[NBASS], bass_out_ch[NBASS]; /* note zero is valid; release on its original channel */
    uint8_t bass_slide_old[NBASS];    /* previous pitch whose MIDI gate overlaps a slide */
    uint8_t bass_slide_valid[NBASS], bass_slide_ch[NBASS];
    uint32_t bass_slide_off[NBASS];   /* remaining samples (Q8), zero = none */
    volatile uint8_t clk_src;         /* SEQ_CLK_*: the clock source that the UI sets */
    uint8_t clk_cur;                  /* SEQ_CLK_*: the clock source that seq_advance applied */
    uint8_t clk_lost;                 /* EXTERNAL: no clock now, the pause (or the wait) is done */
    volatile uint8_t req_rt;          /* 1 = the pending Start or Continue came from MIDI in (seq_midi_rt) */
    volatile uint8_t req_stop_rt;     /* 1 = pending Stop came from MIDI, not local PLAY */
    uint8_t start_local;              /* 1 = the last start came from PLAY (not from MIDI in) */
    uint32_t start_at;                /* ext_now at the last start */
    uint8_t clk_lock;                 /* the input that has the clock (MID-15): 0 none, else input + 1 */
    uint8_t clk_stop;                 /* 1: the last transport message of that input was a Stop */
    uint8_t clk_last;                 /* the input that had the clock last (input + 1): a change restarts the tempo measure */
    uint8_t ext_skip;                 /* 1: the next block of ticks gives no tick period (the first ticks of a new input) */
    uint32_t clk_lock_at;             /* ext_now at its last Clock, Start or Continue */
    uint32_t clk_stop_at;             /* ext_now at its last Stop */
} seq_t;

void seq_init(seq_t *s, pattern_t *patterns);
uint32_t seq_until_event(seq_t *s, uint32_t n);
void seq_advance(seq_t *s, uint32_t k, const seq_sink_t *o);
void seq_midi_flush(seq_t *s, const seq_sink_t *o); /* end tracked output without stopping local voices */

/* main loop / MIDI side (all are flag writes picked up at the next block) */
void seq_start(seq_t *s);             /* from step 1 of the current (or cued) pattern */
void seq_stop(seq_t *s);
void seq_continue(seq_t *s);
void seq_ext_clock(seq_t *s);         /* one 24-ppqn tick (USB MIDI 0xF8) */
int seq_midi_rt(seq_t *s, uint8_t st);  /* MIDI in 0xF8, 0xFA, 0xFB, 0xFC: 1 (INTERNAL drops them); else 0 */
int seq_clk_take(seq_t *s, uint8_t st, uint32_t src);   /* MIDI in from input src: 0 = another input has the clock */
void seq_clk_drop(seq_t *s, uint32_t src);  /* the link of input src ended: it loses the clock at once */
int seq_clock_waiting(const seq_t *s);  /* EXTERNAL: playing, but no clock for 0.5 s (it waits) */
void seq_cue(seq_t *s, int pattern);  /* every part, at the end of the bar (now, if stopped) */
void seq_cue_part(seq_t *s, int part, int pattern);
void seq_chain(seq_t *s, int a, int b);
int seq_cue_of(const seq_t *s, int part);   /* the part's cued pattern, or -1 */
int seq_part_muted(const seq_t *s, int part);

/* GM drum note per kit voice — seq MIDI out and engine MIDI in share one table */
extern const uint8_t GM_DRUM[NKIT][NDRUM];
/* Default 0-based MIDI channels for the five parts (seq_init + project_apply_io) */
extern const uint8_t SEQ_CH_DEFAULT[NTRACKS];

/* steps of one part's rate, in samples Q8 (no swing) at the current tempo */
uint32_t seq_step_q8(const seq_t *s, int rate);
float seq_tempo(const seq_t *s);      /* the tempo in effect (internal or measured) */
int seq_part_len(const seq_t *s, int t);         /* steps in part t's pattern */
int seq_next_step(seq_t *s, int t, int p);       /* the step after p (direction; no commit) */
int seq_rec_step(seq_t *s, int t);               /* the step a live hit now records on: the nearest */
void seq_rec_skip(seq_t *s, int t, int step, int voice);   /* that hit: its step does not sound it once */

#define SEQ_REC_LEGATO 0x40u /* accepted live incoming transition; never stored in step flags */
/* Live MIDI/keyboard edges, at the current audio clock. Last press owns the line;
 * older/different-route releases do not end it. Repeated identical presses count.
 * SEQ_REC_LEGATO is not ownership: overlap also needs the accepted note generation. */
void seq_rec_bass(seq_t *s, int b, int note, int velocity, int down, int src, int flags);
void seq_rec_bass_end(seq_t *s, int b);
/* Context cancel, part or pattern change, REC off, bulk finalize: drop the accepted
 * generation and finish the open capture. Press membership stays with the engine. */
void seq_rec_cancel(seq_t *s, int b);

#if X0X_BASS_NOTE_SOUND
/* Audio recorder storage transaction supplied by the product engine. */
int seq_bass_sound_prepare(seq_t *s,int voice,int pat,int step);
void seq_bass_sound_clear(seq_t *s,int voice,int pat,int step);
#endif
