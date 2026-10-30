/* SPDX-License-Identifier: GPL-3.0-only */
/* X0X sequencer; see sequencer.h for the model. */
#include "sequencer.h"
#include "tb3po.h"
#include "../x0x_cold.h"              /* X0X_COLD: boot/UI cue/chain — not advance/fire/clock/MIDI */

#define NEVER 0xFFFFFFFFu
#define RANDOM_PASS_STEPS 0x7Fu
#define RANDOM_SWING_ODD  0x80u
#define BASS_SLIDE_OVERLAP_Q8 (FS / 50u * 256u) /* 20 ms after the next note-on */

static const uint8_t TICKS_PER_STEP[NRATES] = {6, 4, 3, 8};     /* 24 ppqn: 1/16, 1/16T, 1/32, 1/8T */
/* One GM note map for seq MIDI out and engine MIDI in (was a second copy in engine.c). */
const uint8_t GM_DRUM[NKIT][NDRUM] = {
    {36, 38, 41, 45, 50, 37, 39, 42, 46, 49, 51},   /* 909: BD SD LT MT HT RS CP CH OH CR RD */
    {36, 38, 41, 45, 50, 37, 39, 56, 49, 46, 42},   /* 808: BD SD LT MT HT RS CP CB CY OH CH */
};
/* Default MIDI channels (0-based): 909 10, 808 11, 303s 2/3, break 4 — also project_apply_io. */
const uint8_t SEQ_CH_DEFAULT[NTRACKS] = {9, 10, 1, 2, 3};

X0X_COLD void seq_init(seq_t *s, pattern_t *patterns)
{
    uint32_t i;
    uint8_t *p = (uint8_t *)s;
    for (i = 0; i < sizeof *s; i++)
        p[i] = 0;
    s->pat = patterns;
    for (i = 0; i < NTRACKS; i++)
        s->pcue[i] = 0xFF;
    s->bpm = 125.0f;
    s->ext_bpm = 125.0f;
    s->accent_q7 = 88;                /* a 909 non-accented hit sits ~3 dB under an accent */
    s->send_clock = 1;
    for (i = 0; i < NTRACKS; i++)
        s->ch[i] = SEQ_CH_DEFAULT[i];
    for (i = 0; i < NTRACKS; i++) {
        s->t[i].to_next = NEVER;
        s->t[i].pp_dir = 1;
        s->t[i].rng = 0x1234567u * (i + 1);
    }
    s->clk_to_next = NEVER;
}

float seq_tempo(const seq_t *s) { return s->ext ? s->ext_bpm : s->bpm; }

uint32_t seq_step_q8(const seq_t *s, int rate)
{
    float bpm = seq_tempo(s), q;
    if (bpm < 20.0f)
        bpm = 20.0f;
    if (bpm > 300.0f)
        bpm = 300.0f;
    q = (float)FS * 60.0f / bpm;                    /* samples per quarter */
    return (uint32_t)(q * 256.0f * (float)TICKS_PER_STEP[rate & 3] / 24.0f);
}

__attribute__((noinline)) static const dpart_t *dpart(const seq_t *s, int k) { return &s->pat[s->ppat[k]].drum[k]; }
static int is_drum(int t) { return t < NKIT; }
static int is_bass(int t) { return t >= TRK_BASS0 && t < TRK_BASS0 + NBASS; }
__attribute__((noinline)) static bpart_t *bpart(const seq_t *s, int b) { return &s->pat[s->ppat[TRK_BASS0 + b]].bass[b]; }

int seq_part_len(const seq_t *s, int t)
{
    int n = is_drum(t) ? dpart(s, t)->len : t == TRK_BRK ? 16 : bpart(s, t - TRK_BASS0)->len;
    return n < 1 ? 1 : n > NSTEPS ? NSTEPS : n;
}

/* noinline: rate lookup shared by swing/advance/fire (else -O2 triples the ternary). */
__attribute__((noinline)) static int part_rate(const seq_t *s, int t)
{
    return (is_drum(t) ? dpart(s, t)->rate : t == TRK_BRK ? RATE_16 : bpart(s, t - TRK_BASS0)->rate) & 3;
}

/* swing offset of a step, Q8: odd 16ths / 32nds come late; triplets are never swung.
 * noinline: one body for step timing paths (else -O2 inlines the groove math). */
__attribute__((noinline)) static uint32_t swing_q8(const seq_t *s, int rate, uint32_t step_q8)
{
    uint32_t sw = s->pat[s->ppat[TRK_DRUM]].swing;   /* the 909 is the bar: its pattern's groove */
    if (rate == RATE_16T || rate == RATE_8T || !sw)
        return 0;
    if (sw > 100u)
        sw = 100u;
    return step_q8 / 200u * sw;                     /* 100 -> half a step late = 75 % (32-bit: no __udivdi3) */
}

__attribute__((noinline)) static uint32_t rng_next(uint32_t *r)
{
    uint32_t x = *r ? *r : 1;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return *r = x;
}

/* the step after p in this part's direction (also used to look ahead for TIEs) */
static int next_pos(seq_t *s, int t, int p, int commit)
{
    seq_track_t *tr = &s->t[t];
    int len = seq_part_len(s, t), dir = is_bass(t) ? bpart(s, t - TRK_BASS0)->dir : DIR_FWD, n;
    switch (dir & 3) {
    case DIR_REV:
        return (p - 1 + len) % len;
    case DIR_PINGPONG:
        if (len == 1)
            return 0;
        /* A live length edit can put the old playhead beyond the new range.
         * Return directly to its last step, then play backwards; never walk
         * through the disabled steps outside the newly selected length. */
        if (p < 0 || p >= len) {
            if (commit)
                tr->pp_dir = (int8_t)(p < 0 ? 1 : -1);
            return p < 0 ? 0 : len - 1;
        }
        n = p + tr->pp_dir;
        if (n >= len || n < 0) {
            n = p - tr->pp_dir;
            if (commit)
                tr->pp_dir = (int8_t)-tr->pp_dir;
        }
        return n;
    case DIR_RANDOM:
        if (!commit)
            return (p + 1) % len;                    /* a TIE look-ahead cannot know: assume forward */
        return (int)(rng_next(&tr->rng) % (uint32_t)len);
    default:
        return (p + 1) % len;
    }
}

__attribute__((noinline)) static void midi(const seq_sink_t *o, uint8_t st, uint8_t d1, uint8_t d2)
{
    if (o->midi)
        o->midi(o->ctx, st, d1, d2);
}

/* Note Off — shared by flush / drum hit / fire_bass / slide expiry (one body on pi32). */
__attribute__((noinline)) static void midi_note_off(const seq_sink_t *o, uint8_t ch, uint8_t note)
{
    midi(o, (uint8_t)(0x80 | ch), note, 0);
}

/* End the overlapping slide Note Off (flush / new onset / slide timer). */
__attribute__((noinline)) static void bass_slide_midi_off(seq_t *s, int b, const seq_sink_t *o)
{
    if (s->bass_slide_valid[b])
        midi_note_off(o, s->bass_slide_ch[b], s->bass_slide_old[b]);
    s->bass_slide_valid[b] = 0;
    s->bass_slide_off[b] = 0;
}

/* noinline: flush glue for seq_midi_flush / release / fire / channel edit. */
__attribute__((noinline)) static void bass_midi_flush(seq_t *s, int b, const seq_sink_t *o)
{
    bass_slide_midi_off(s, b, o);
    if (s->bass_out_valid[b])
        midi_note_off(o, s->bass_out_ch[b], s->bass_notes_out[b]);
    s->bass_out_valid[b] = 0;
}

void seq_midi_flush(seq_t *s, const seq_sink_t *o)
{
    for (int b = 0; b < NBASS; b++)
        bass_midi_flush(s, b, o);
}

__attribute__((noinline)) static void bass_release(seq_t *s, int b, const seq_sink_t *o)
{
    seq_track_t *tr = &s->t[TRK_BASS0 + b];
    bass_midi_flush(s, b, o);
    if (tr->sounding) {
        o->bass_off(o->ctx, b);
        tr->sounding = 0;
    }
    tr->held_note = 0;
    tr->to_off = 0;
}


__attribute__((noinline)) static void drum_midi_hit(seq_t *s, const seq_sink_t *o, int k, int v, uint8_t mv)
{
    if (s->send_notes && s->ch[k] != SEQ_CH_OFF) {
        midi(o, (uint8_t)(0x90 | s->ch[k]), GM_DRUM[k][v], mv);
        midi_note_off(o, s->ch[k], GM_DRUM[k][v]);
    }
}

__attribute__((noinline)) static void fire_drum(seq_t *s, int k, int p, uint32_t skip, const seq_sink_t *o)
{
    const dpart_t *d = dpart(s, k);
    uint32_t bit = 1u << (p & 31);
    int w = p >> 5;
    float accent_vel = (d->accent[w] & bit) ? 1.0f : (float)s->accent_q7 / 127.0f;
    int v;
    for (v = 0; v < NDRUM; v++) {
        uint8_t sv;
        float vel;
        uint8_t mv;
        if (!(d->hit[v][w] & bit) || ((s->mute | s->vmute) & (1u << (k * NDRUM + v))))
            continue;
        /* A T_VEL lane (written by black-key REC) wins; else the shared accent row / accent_q7. */
        sv = s->drum_vel[k][v];
        if (sv) {
            if (sv > 127u)
                sv = 127u;
            vel = (float)sv / 127.0f;
            mv = sv;
        } else {
            vel = accent_vel;
            mv = (uint8_t)(vel * 127.0f);
        }
        if (skip >> v & 1u) {                        /* recorded live on this pass: the key sounded it */
            drum_midi_hit(s, o, k, v, mv);
            continue;
        }
        if (!o->drum(o->ctx, k, v, vel))
            continue;                                /* its chance said no: no MIDI either */
        drum_midi_hit(s, o, k, v, mv);
    }
}

static uint32_t bass_slot_q8(const seq_t *s, int b, int p)
{
    int t = TRK_BASS0 + b;
    uint32_t L = seq_step_q8(s, bpart(s, b)->rate);
    uint32_t sw = swing_q8(s, bpart(s, b)->rate, L);
    if (bpart(s, b)->dir == DIR_RANDOM)
        return (s->t[t].mut_steps & RANDOM_SWING_ODD) ? L + sw : L - sw;
    int np = next_pos((seq_t *)s, t, p, 0);
    return !(np & 1) && !(p & 1) ? L : (np & 1) ? L + sw : L - sw;
}

__attribute__((noinline)) static void fire_bass(seq_t *s, int b, int p, uint32_t step_q8, uint32_t carry, uint32_t skip, const seq_sink_t *o)
{
    seq_track_t *tr = &s->t[TRK_BASS0 + b];
    bpart_t *bp = bpart(s, b);
    const bstep_t *st = &bp->step[p];
    int gate = bstep_gate(st), note, hold, nx;
    seq_bass_rec_t *r = &s->rec_bass[b];
    if (r->active && r->pat != s->ppat[TRK_BASS0 + b])
        seq_rec_cancel(s, b);
    if (r->active) {
        if (r->entered == 2 || (r->entered && p == r->first)) {
            r->entered = 2; /* periodic pattern cannot encode an extra loop count */
            tr->to_off = 0;
            return;
        }
        if (p == r->first && !r->entered) { r->entered = 1; r->at = s->ext_now; r->slot = bass_slot_q8(s, b, p); }
        else if (r->entered) {
            /* Across the loop, keep the onset but suppress its live retrigger. */
            bp->step[r->last].flags &= 15u; /* continuation, not an endpoint */
            if (p != r->first) {
                bp->step[p].note = r->note;
                bp->step[p].velocity = bp->step[r->first].velocity;
                bp->step[p].flags = G_TIE | 0xF0u;
#if X0X_BASS_NOTE_SOUND
                seq_bass_sound_clear(s,b,s->ppat[TRK_BASS0+b],p);
#endif
            }
            r->prev = r->last;
            r->last = (uint8_t)p;
            r->slot = bass_slot_q8(s, b, p);
            r->at = s->ext_now;
        }
        if (!r->entered) { tr->to_off = 0; return; }
        skip = 1; /* live input owns audio; reuse real MIDI echo/gate bookkeeping */
        gate = bstep_gate(st);
    }
    uint8_t prev_slide = tr->held_note & 0x80u;     /* bit 7: the step that sounded slides on */
    if (s->mute & (1u << (MUTE_BASS0 + b)))
        gate = G_REST;
    /* A TIE continues an audible note; an orphan TIE is a rest, never an onset. */
    if (gate == G_REST || (gate == G_TIE && !tr->sounding)) {
        bass_release(s, b, o);
        tr->held_note = 0;
        return;
    }
    note = st->note + (int)bp->transpose - 24;
    note = note < 0 ? 0 : note > 127 ? 127 : note;
    if (gate == G_NOTE || !tr->sounding) {
        int slide = tr->sounding && prev_slide;
        /* A chance-rejected step is a rest for both the internal 303 and MIDI out.
         * REC's live-hit skip still echoes MIDI (REC-06); it is not a chance miss. */
        int played = skip || o->bass_on(o->ctx, b, note, (st->flags & BS_ACCENT) != 0, slide, bstep_velocity(st));
        if (played && s->send_notes && s->ch[TRK_BASS0 + b] != SEQ_CH_OFF) {
            uint8_t old = s->bass_notes_out[b], ch = s->ch[TRK_BASS0 + b];
            uint8_t had_old = s->bass_out_valid[b], old_ch = s->bass_out_ch[b];
            if (s->bass_slide_valid[b])                  /* only one old MIDI pitch can remain open */
                bass_slide_midi_off(s, b, o);
            /* A repeated pitch must end before its new Note On: otherwise its old
             * Note Off could silence the new voice on a monophonic receiver. */
            if (had_old && (old == note || old_ch != ch))
                midi_note_off(o, old_ch, old);
            midi(o, (uint8_t)(0x90 | ch), (uint8_t)note, bstep_velocity(st));
            if (had_old && old != note && old_ch == ch) {
                if (slide) {
                    s->bass_slide_old[b] = old;
                    s->bass_slide_ch[b] = old_ch;
                    s->bass_slide_valid[b] = 1;
                    s->bass_slide_off[b] = BASS_SLIDE_OVERLAP_Q8;
                } else
                    midi_note_off(o, old_ch, old);
            }
            s->bass_notes_out[b] = (uint8_t)note;
            s->bass_out_ch[b] = ch;
            s->bass_out_valid[b] = 1;
        } else if (s->bass_out_valid[b] || s->bass_slide_valid[b]) {
            /* Output was disabled or its channel set OFF while a note sounded. */
            bass_midi_flush(s, b, o);
        }
        if (!played) {                                  /* rejected chance: a real rest, not a held 303 */
            tr->sounding = 0;
            tr->held_note = 0;
            tr->to_off = 0;
            return;
        }
        tr->sounding = 1;
    }
    nx = next_pos(s, TRK_BASS0 + b, p, 0);
    if ((bp->dir & 3) == DIR_RANDOM) {
        /* Preview the next random step without advancing the committed RNG.
         * A random TIE must hold the note picked before it, not the forward
         * neighbour (which may never play on this pass). */
        uint32_t peek_rng = tr->rng;
        nx = (int)(rng_next(&peek_rng) % (uint32_t)seq_part_len(s, TRK_BASS0 + b));
    }
    hold = (st->flags & BS_SLIDE) || bstep_gate(&bp->step[nx]) == G_TIE;
    tr->held_note = (uint8_t)((note & 0x7F) | ((st->flags & BS_SLIDE) ? 0x80 : 0));
    unsigned end = st->flags >> BS_END_SHIFT;
    tr->to_off = r->active ? 0 : end ? carry + (bass_slot_q8(s, b, p) / 15u) * end : hold ? 0 : carry + step_q8 / 2u;
}

int seq_part_muted(const seq_t *s, int p)
{
    uint32_t m = seq_part_mask(p);
    return (s->mute & m) == m;
}

/* the song's bar k into the parts and their mutes (into next[]: the caller switches) */
static void song_read(seq_t *s, int k, uint8_t *next)
{
    const song_bar_t *b = &s->song->bar[k];
    uint32_t mute = 0;                                  /* the bar decides every part's mute (built up, */
    int p;                                              /* not masked: ~mask reads as a ROM address) */
    for (p = 0; p < NTRACKS; p++) {
        next[p] = b->pat[p] < NPAT ? b->pat[p] : 0;
        if (b->mute >> p & 1u)
            mute |= seq_part_mask(p);
    }
    s->mute = mute;
}

/* the live state (next[], the mutes) into the song's bar k, growing it */
static void song_write(seq_t *s, int k, const uint8_t *next)
{
    song_bar_t *b = &s->song->bar[k];
    int p;
    b->mute = 0;
    for (p = 0; p < NTRACKS; p++) {
        b->pat[p] = next[p];
        if (seq_part_muted(s, p))
            b->mute |= (uint8_t)(1u << p);
    }
    if (s->song->len < k + 1)
        s->song->len = (uint16_t)(k + 1);
}

static int song_plays(const seq_t *s) { return s->song_on && s->song && (s->song_rec || s->song->len); }

/* the bar's end (the 909 wrapping), or the start: cues, the chain or the song decide each
 * part's next pattern. Returns the parts whose pattern changed (bit t). */
static uint32_t bar_end(seq_t *s, int start)
{
    uint8_t next[NTRACKS];
    uint32_t changed = 0;
    int t, cued = 0;
    for (t = 0; t < NTRACKS; t++)
        next[t] = s->ppat[t];
    if (song_plays(s) && !s->song_rec) {               /* the song: its next bar (looping), or this one again */
        int k = start ? s->song_start : s->song_pos + 1;
        if (!start && s->song_rep > 1 && s->song_pos < s->song->len) {
            k = s->song_pos;
            s->song_rep--;
        } else {
            if (k >= s->song->len)
                k = 0;
            s->song_rep = (uint8_t)SONG_REP(&s->song->bar[k]);
        }
        s->song_pos = (uint16_t)k;
        song_read(s, k, next);
        for (t = 0; t < NTRACKS; t++)
            s->pcue[t] = 0xFF;
    } else {
        for (t = 0; t < NTRACKS; t++)
            if (s->pcue[t] < NPAT) {
                next[t] = s->pcue[t];
                s->pcue[t] = 0xFF;
                cued = 1;
            }
        if (!cued && !start && s->chain_a != s->chain_b) {
            int a = s->chain_a < s->chain_b ? s->chain_a : s->chain_b;
            int b = s->chain_a < s->chain_b ? s->chain_b : s->chain_a, c = s->ppat[TRK_DRUM];
            c = (c < a || c >= b) ? a : c + 1;
            for (t = 0; t < NTRACKS; t++)
                next[t] = (uint8_t)c;
        }
        if (song_plays(s)) {                           /* recording: what plays is the song */
            int k = start ? s->song_start : s->song_pos + 1;
            if (k >= NSONG) {
                s->song_rec = 0;                       /* full */
            } else {
                if (start && k < s->song->len)         /* overdub from where the song is */
                    song_read(s, k, next);
                s->song_pos = (uint16_t)k;
                song_write(s, k, next);
            }
        }
    }
    for (t = 0; t < NTRACKS; t++)
        if (next[t] != s->ppat[t]) {
            s->ppat[t] = next[t];
            changed |= 1u << t;
        }
    return changed;
}

__attribute__((noinline)) static void auto_mutate(seq_t *s, int b)
{
    bpart_t *bp = bpart(s, b);
    seq_track_t *tr = &s->t[TRK_BASS0 + b];
    tr->bars++;
    if (bp->gen.mutate_bars && tr->bars % bp->gen.mutate_bars == 0)
        tb3po_mutate(bp, &s->mut_rng[b]);
}

/* fire track t's next step now; returns the parts whose pattern changed (the 909 wrapping) */
static uint32_t fire_step(seq_t *s, int t, const seq_sink_t *o)
{
    seq_track_t *tr = &s->t[t];
    int rate = part_rate(s, t), p, wrapped;
    uint32_t changed = 0, skip;
    uint32_t L = seq_step_q8(s, rate);
    uint32_t carry = tr->to_next < 256u ? tr->to_next : 0;   /* the step was due this far into the sample: keep it */
    int bass = is_bass(t);
    if (!tr->started) {
        p = (bass && bpart(s, t - TRK_BASS0)->dir == DIR_REV) ? seq_part_len(s, t) - 1 : 0;
        tr->started = 1;
        wrapped = 0;
    } else {
        p = next_pos(s, t, tr->pos, 1);
        /* The reverse pass wraps only at its last step, not also at zero.
         * Random steps have no positional wrap: count a logical pass below. */
        wrapped = !bass ? p == 0 :
            (bpart(s, t - TRK_BASS0)->dir == DIR_REV ? p == seq_part_len(s, t) - 1 :
             bpart(s, t - TRK_BASS0)->dir != DIR_RANDOM && p == 0);
    }
    if (wrapped && t == TRK_DRUM && (changed = bar_end(s, 0)) != 0) {
        rate = part_rate(s, t);                       /* the 909's new pattern */
        L = seq_step_q8(s, rate);
        p = 0;
    }
    if (bass) {
        if (bpart(s, t - TRK_BASS0)->dir == DIR_RANDOM) {
            /* Visiting step zero at random is not a bar boundary. Count
             * LEN actual played steps for both TB-3PO and probability. */
            uint8_t count = (uint8_t)(tr->mut_steps & RANDOM_PASS_STEPS);
            if (count >= seq_part_len(s, t)) {
                count = 0;
                auto_mutate(s, t - TRK_BASS0);
            }
            /* A random pick is a note choice, not a timing slot. Preserve the
             * alternating swing slot across odd-length passes and live edits. */
            tr->mut_steps = (uint8_t)(((tr->mut_steps ^ RANDOM_SWING_ODD) & RANDOM_SWING_ODD) | (count + 1u));
        } else {
            tr->mut_steps = 0;
            if (wrapped)
                auto_mutate(s, t - TRK_BASS0);
        }
    }
    if (wrapped && (t == TRK_BRK || is_drum(t)))   /* (the 303s count theirs in auto_mutate) */
        tr->bars++;
    if (changed & 1u)                                /* the 909's new pattern: its passes from 1 again, as */
        tr->bars = 0;                                /* restart() does for the others (probability cycles) */
    skip = (tr->rec_skip && tr->rec_skip_step == p && tr->rec_skip_pat == s->ppat[t]) ? tr->rec_skip : 0u;
    tr->rec_skip = 0;                                /* one pass only */
    tr->pos = (uint8_t)p;
    if (o->step)                                     /* first: the step's motion and p-locks, which the hit */
        o->step(o->ctx, t, p);                       /* below must hear (a drum takes its tune as it fires) */
    if (is_drum(t))
        fire_drum(s, t, p, skip, o);
    else if (t == TRK_BRK) {
        if (o->brk)
            o->brk(o->ctx, p, tr->bars, (float)L / 256.0f,
                   (s->pat[s->ppat[TRK_BRK]].brk.steps >> p & 1u) && !(s->mute & (1u << MUTE_BRK)));
    } else
        fire_bass(s, t - TRK_BASS0, p, L, carry, skip, o);
    if (s->ext) {
        tr->to_next = tr->ext_due ? 0 : NEVER;       /* the clock arms the next one (a burst: at once) */
        if (tr->ext_due)
            tr->ext_due--;
    } else {
        /* the step that follows p: swung if it is an odd one */
        int np = next_pos(s, t, p, 0);
        uint32_t sw = swing_q8(s, rate, L);
        if (bass && bpart(s, t - TRK_BASS0)->dir == DIR_RANDOM) {
            /* Swing belongs to successive musical time slots, not to the
             * parity of the randomly selected source-step position. */
            tr->to_next = carry + ((tr->mut_steps & RANDOM_SWING_ODD) ? L + sw : L - sw);
        } else {
            tr->to_next = carry + ((np & 1) ? L + sw : L - sw);
            if (!(np & 1) && (p & 1) == 0)           /* even -> even (odd lengths, wraps): no swing */
                tr->to_next = carry + L;
        }
    }
    return changed;
}

/* Shared by Start/Continue and pattern-switch restart: step timers (+ optional from-top fields).
 * clear_to_off: Start/Continue zero note-off timers; restart leaves them (voice already released). */
__attribute__((noinline)) static void track_arm(seq_t *s, int t, int from_top, int clear_to_off)
{
    seq_track_t *tr = &s->t[t];
    if (from_top) {
        tr->started = 0;
        tr->bars = 0;
        tr->pp_dir = 1;
        tr->mut_steps = 0;
    }
    if (clear_to_off)
        tr->to_off = 0;
    tr->ext_due = 0;
    tr->rec_skip = 0;
    tr->to_next = s->ext ? NEVER : 0;
    tr->ext_ticks = (uint8_t)(TICKS_PER_STEP[part_rate(s, t)] - 1);   /* the first clock is the downbeat */
}

__attribute__((noinline)) static void restart(seq_t *s, uint32_t parts)  /* parts whose pattern switched: from step 1 (not the 909's) */
{
    int t;
    for (t = 1; t < NTRACKS; t++) {
        if (!(parts >> t & 1u))
            continue;
        track_arm(s, t, 1, 0);
    }
}

__attribute__((noinline)) static void bass_release_all(seq_t *s, const seq_sink_t *o)
{
    int b;
    for (b = 0; b < NBASS; b++)
        bass_release(s, b, o);
}

__attribute__((noinline)) static void bass_and_brk_stop(seq_t *s, const seq_sink_t *o)
{
    bass_release_all(s, o);
    if (o->brk_stop)
        o->brk_stop(o->ctx);
}

__attribute__((noinline)) static void seq_send_rt(seq_t *s, const seq_sink_t *o, uint8_t remote, uint8_t st)
{
    if (!s->ext && s->send_clock && (!s->midi_req || !remote))
        midi(o, st, 0, 0);
}

__attribute__((noinline)) static void do_start(seq_t *s, int from_top, const seq_sink_t *o, uint8_t remote)
{
    int t;
    bass_release_all(s, o);                /* Start/Continue reinitialises timers: release old notes first */
    if (from_top)
        bar_end(s, 1);                                /* cues, or the song's start bar */
    for (t = 0; t < NTRACKS; t++)
        track_arm(s, t, from_top, 1);
    if (from_top || !s->metro_period) {
        s->metro_phase = s->metro_period = 0;
        s->metro_tick = 24; /* waiting: next accepted clock is phase zero */
    } /* Continue preserves musical position and the stopped visual phase. */
    s->clk_to_next = 0;
    s->run_gen++;
    s->playing = 1;
    if (o->transport) o->transport(o->ctx, 1);
    seq_send_rt(s, o, remote, from_top ? 0xFA : 0xFB);
}

__attribute__((noinline)) static void do_stop(seq_t *s, const seq_sink_t *o, uint8_t remote)
{
    int t;
    s->playing = 0;
    bass_and_brk_stop(s, o);
    if (o->transport) o->transport(o->ctx, 0);
    for (t = 0; t < NTRACKS; t++) {
        s->t[t].to_next = NEVER;
        s->t[t].ext_due = 0;
        s->t[t].rec_skip = 0;
    }
    seq_send_rt(s, o, remote, 0xFC);
}

__attribute__((noinline)) static void ext_set_period(seq_t *s, uint32_t q)   /* samples per external clock, Q8 */
{
    s->ext_period = q;
    s->ext_bpm = (float)FS * 60.0f * 256.0f / (24.0f * (float)q);
}

/* external clock ticks: the tempo, and arm the steps they complete. Bluetooth MIDI gives all the ticks
 * of one connection event (7.5-30 ms) in one block. Thus the time between two blocks is not one tick
 * period. The tempo is the mean tick period over the last beats in the ring (SEQ_EXT_BEATS). */
static void ext_ticks(seq_t *s)
{
    int t;
    uint32_t n = s->ext_pending, k = n;
    s->ext_pending = 0;
    if (s->ext_skip || (!s->ext_nb && !s->ext_q)) {   /* first tick after any beat-ring reset is no period */
        s->ext_skip = 0;
        s->ext_since = FS;
    }
    for (; n; n--) {
        if (s->ext_since > 0 && s->ext_since < FS && s->ext_nb < 2u) {   /* the first beats: smoothed */
            uint32_t q = (s->ext_since << 8) / k;     /* the ticks of one block share its time */
            ext_set_period(s, s->ext_period ? s->ext_period - (s->ext_period >> 3) + (q >> 3) : q);
        }
        s->ext_since = 0;
        if (++s->ext_q >= 24u) {                      /* a beat: the mean since the oldest beat in the ring */
            s->ext_q = 0;
            if (s->ext_nb) {
                uint32_t d = s->ext_now - s->ext_beat[s->ext_nb < SEQ_EXT_BEATS ? 0u : s->ext_bi], m = 24u * s->ext_nb;
                uint32_t w = ((d / m) << 8) + ((d % m) << 8) / m, p = s->ext_period;
                if (s->ext_nb >= 2u && (w > p ? w - p : p - w) < (p >> 6))   /* a change under 1/64: smoothed */
                    w = p - (p >> 2) + (w >> 2);
                if (w)                                /* (0: all the ticks of the ring in one block) */
                    ext_set_period(s, w);
            }
            s->ext_beat[s->ext_bi] = s->ext_now;
            s->ext_bi = (uint8_t)((s->ext_bi + 1u) % SEQ_EXT_BEATS);
            if (s->ext_nb < SEQ_EXT_BEATS)
                s->ext_nb++;
        }
        if (!s->playing)
            continue;
        s->metro_tick = s->metro_tick >= 23u ? 0 : (uint8_t)(s->metro_tick + 1u);
        for (t = 0; t < NTRACKS; t++) {
            seq_track_t *tr = &s->t[t];
            int rate = part_rate(s, t), tps = TICKS_PER_STEP[rate];
            if (++tr->ext_ticks < tps)
                continue;
            tr->ext_ticks = 0;
            if (tr->to_next != NEVER) {               /* the last one not played yet (one block): it too */
                if (tr->ext_due < 255u)
                    tr->ext_due++;
                continue;
            }
            {
                int np = tr->started ? next_pos(s, t, tr->pos, 0) : 0;
                int odd = is_bass(t) && bpart(s, t - TRK_BASS0)->dir == DIR_RANDOM
                    ? (tr->mut_steps & RANDOM_SWING_ODD) != 0 : (np & 1) != 0;
                uint32_t L = s->ext_period ? s->ext_period * (uint32_t)tps : seq_step_q8(s, rate);
                tr->to_next = odd ? swing_q8(s, rate, L) : 0;
            }
        }
    }
}

uint32_t seq_until_event(seq_t *s, uint32_t n)
{
    uint32_t m = n, e;
    int t;
    if (!s->playing)
        return n;
    for (t = 0; t < NTRACKS; t++) {
        if (s->t[t].to_next != NEVER && (e = s->t[t].to_next >> 8) < m)
            m = e;
        if (s->t[t].to_off && (e = s->t[t].to_off >> 8) < m)
            m = e;
    }
    for (t = 0; t < NBASS; t++)
        if (s->bass_slide_off[t] && (e = s->bass_slide_off[t] >> 8) < m)
            m = e;
    if (!s->ext && s->send_clock && (e = s->clk_to_next >> 8) < m)
        m = e;
    return m ? m : 1;
}

/* the external clock is gone: a new beat ring, and the internal tempo takes the last measured one */
__attribute__((noinline)) static void clock_lost(seq_t *s)
{
    s->ext_nb = s->ext_bi = s->ext_q = 0;             /* the next clock: a new beat ring */
    if (s->ext_period) {                              /* the internal tempo takes the last measured one */
        float b = (float)(int)(s->ext_bpm + 0.5f);   /* whole BPM, as the TEMPO knob sets it */
        s->bpm = b < 20.0f ? 20.0f : b > 275.0f ? 275.0f : b;
    }
}

static void to_internal(seq_t *s)
{
    int t;
    s->ext = 0;
    clock_lost(s);
    for (t = 0; t < NTRACKS; t++)
        if (s->playing && s->t[t].to_next == NEVER)
            s->t[t].to_next = 0;
}

/* CLOCK SOURCE set to INTERNAL while X0X follows a clock: each part's next step comes when the clock
 * would give it, so no step comes early or late (a clock that is gone: at once, as to_internal) */
__attribute__((noinline)) static void switch_internal(seq_t *s)
{
    int t;
    uint32_t per = s->ext_period ? s->ext_period : seq_step_q8(s, RATE_16) / 6u;   /* one tick, Q8 */
    uint32_t since = s->ext_since < (1u << 23) ? s->ext_since << 8 : NEVER;
    for (t = 0; t < NTRACKS && s->playing; t++) {
        seq_track_t *tr = &s->t[t];
        int rate = part_rate(s, t), tps = TICKS_PER_STEP[rate];
        uint32_t left;
        if (tr->to_next != NEVER)
            continue;
        left = (tr->ext_ticks == 0xFFu ? 1u : (uint32_t)(tps - tr->ext_ticks)) * per;   /* to the next step */
        /* CLOCK SOURCE handoff uses the same next musical time slot as
         * external clock scheduling. RANDOM note index is not timing parity. */
        int odd = is_bass(t) && bpart(s, t - TRK_BASS0)->dir == DIR_RANDOM
            ? (tr->mut_steps & RANDOM_SWING_ODD) != 0
            : ((tr->started ? next_pos(s, t, tr->pos, 0) : 0) & 1) != 0;
        if (odd)
            left += swing_q8(s, rate, per * (uint32_t)tps);
        tr->to_next = left > since ? left - since : 0;
    }
    s->ext = 0;
    clock_lost(s);
}

__attribute__((noinline)) static void rephase(seq_t *s)                         /* the next tick is tick 0 of the step that plays */
{
    int t;
    s->metro_tick = 24;
    for (t = 0; t < NTRACKS; t++) {
        s->t[t].to_next = NEVER;
        s->t[t].ext_due = 0;
        s->t[t].ext_ticks = 0xFFu;                    /* (ext_ticks adds 1 for this tick: 0) */
    }
}

__attribute__((noinline)) static void pause_sound(seq_t *s, const seq_sink_t *o)   /* EXTERNAL waits: the 303 notes and the break stop */
{
    if (!s->playing)
        return;
    bass_and_brk_stop(s, o);
}

/* apply a new clock source (GLO > MIDI > CLOCK SOURCE), in the audio ISR */
static void clock_source(seq_t *s, const seq_sink_t *o)
{
    uint8_t m = s->clk_src;
    if (m > SEQ_CLK_EXT)
        m = SEQ_CLK_AUTO;
    s->clk_cur = m;
    s->clk_lost = 0;
    if (m == SEQ_CLK_INT) {                           /* the clock that came in is not used */
        s->ext_pending = 0;
        s->clk_lock = 0;                              /* MID-13/MID-15: INTERNAL owns no external clock */
        /* CLOCK SOURCE INTERNAL supersedes pending MIDI transport from the
         * previous mode, but must not cancel a newer local PLAY/STOP. */
        if (s->req_rt) {
            s->req_start = s->req_cont = s->req_rt = 0;
            s->req_prior_ticks = 0;
        }
        if (s->req_stop_rt)
            s->req_stop = s->req_stop_rt = 0;
        if (s->ext)
            switch_internal(s);
    } else if (m == SEQ_CLK_EXT && !s->ext) {         /* wait for the clock (AUTO: the clock or the timeout does it) */
        s->ext = 1;
        s->ext_bpm = s->bpm;                          /* no stale tempo */
        s->ext_period = 0;
        s->ext_nb = s->ext_bi = s->ext_q = 0;
        s->clk_lost = 1;
        if (s->playing)
            rephase(s);
        pause_sound(s, o);
    }
}

/* Observe the same rendered sample clock that advances the sequencer. No
 * timer, per-track playhead or MIDI-output timer is involved. The CPU0 audio
 * owner publishes one byte; the UI cannot mix phase and transport epochs.
 * External motion advances only on accepted clocks (never predicts lost ticks).
 * Internal tempo edits preserve fractional phase. This observer never changes
 * event partitioning, transport, outgoing MIDI or audio state. */
static void metro_advance(seq_t *s, uint32_t k)
{
    uint32_t period, phase;
    if (!s->playing || seq_clock_waiting(s) || (s->ext && s->metro_tick == 24u)) {
        s->metro_view = 0;
        return;
    }
    period = s->metro_period;
    if (!period || s->metro_bpm != seq_tempo(s)) {
        uint32_t next = seq_step_q8(s, RATE_16) * 4u;
        s->metro_phase = period ? (uint32_t)((float)s->metro_phase * (float)next / (float)period) : 0;
        if (!period) k = 0; /* accepted fresh start begins here, not before */
        s->metro_period = period = next;
        s->metro_bpm = seq_tempo(s);
    }
    if (s->ext) {
        phase = (uint32_t)s->metro_tick * period / 24u;
    } else {
        phase = s->metro_phase + (k << 8);
        while (phase >= period) phase -= period;
    }
    s->metro_phase = phase;
    s->metro_view = (uint8_t)(1u + phase * 32u / period);
}

void seq_advance(seq_t *s, uint32_t k, const seq_sink_t *o)
{
    int t, guard;
    uint32_t dq = k << 8;
    for (t = 0; t < NBASS; t++)
        if (s->rec_bass[t].active && (s->req_stop || s->req_start ||
            (!((s->rec_mask | s->rec_hold_mask | (s->rec_bass[t].src < 3 ? s->rec_ext_mask : 0u)) & (1u << t)) ||
             s->rec_bass[t].pat != s->ppat[TRK_BASS0 + t] || bpart(s, t)->dir >= DIR_PINGPONG)))
            seq_rec_cancel(s, t);
    if (s->ext_since < 2u * FS)   /* all modes: long idle must not wrap into a fake short tick */
        s->ext_since += k;
    s->ext_now += k;
    for (t = 0; t < NBASS; t++)
        if (s->bass_out_valid[t] && (!s->send_notes || s->ch[TRK_BASS0 + t] != s->bass_out_ch[t]))
            bass_midi_flush(s, t, o);    /* direct channel/settings edits cannot strand the old MIDI note */
    if (s->clk_cur != s->clk_src)
        clock_source(s, o);
    if (s->clk_lock && s->ext_now - s->clk_lock_at > SEQ_LOCK_T)
        s->clk_lock = 0;                              /* that input sends nothing: any input can get the clock */
    if (s->clk_cur == SEQ_CLK_INT)
        s->ext_pending = 0;
    if (s->ext_pending) {
        if (!s->ext && s->playing)                    /* playing on the internal clock: the first tick */
            rephase(s);                               /* is tick 0 of the step that plays (no step now) */
        s->ext = 1;
        s->clk_lost = 0;
    }
    else if (s->ext && s->ext_since > FS / 2u) {    /* clock gone for 0.5 s */
        if (s->clk_cur != SEQ_CLK_EXT)
            to_internal(s);                           /* back to internal */
        else if (!s->clk_lost) {                      /* EXTERNAL: pause, keep the place, wait for the clock */
            s->clk_lost = 1;
            clock_lost(s);
            pause_sound(s, o);
        }
    }
    if (s->req_stop) {                                /* (a Start after the Stop stays: seq_stop) */
        uint8_t rt = s->req_stop_rt;
        s->req_stop = 0;
        s->req_stop_rt = 0;
        if (s->playing)
            do_stop(s, o, rt);
    }
    if (s->req_start || s->req_cont) {
        uint8_t rt = s->req_rt;
        s->req_rt = 0;
        /* PLAY started X0X less than one step ago, and a MIDI Start or Continue comes (the host answers the
         * PLAY note, MID-14): X0X already plays step 1, so it does not start again (no second step 1) */
        if (rt && s->playing && s->start_local &&
            s->ext_now - s->start_at < (seq_step_q8(s, part_rate(s, TRK_DRUM)) >> 8))
            s->req_start = s->req_cont = 0;
        else {
            /* MIDI packets in one audio block may bracket Start/Continue.
             * Count only ticks after an accepted request for its new phase.
             * An ignored Loopy PLAY echo must retain every queued tick. */
            s->ext_pending = s->ext_pending >= s->req_prior_ticks
                ? s->ext_pending - s->req_prior_ticks : 0;
            if (s->req_start) {
                s->req_start = 0;
                do_start(s, 1, o, rt);
            }
            if (s->req_cont) {
                s->req_cont = 0;
                do_start(s, 0, o, rt);
            }
            s->start_local = (uint8_t)!rt;
            s->start_at = s->ext_now;
        }
    }
    s->midi_req = 0;                                  /* used only by the requests above */
    if (s->ext_pending)
        ext_ticks(s);
    metro_advance(s, k);
    if (!s->playing) {
        if (!s->ext)
            s->clk_to_next = 0;
        for (t = 0; t < NTRACKS; t++)                /* stopped: a cue takes effect at once */
            if (s->pcue[t] < NPAT) {
                s->ppat[t] = s->pcue[t];
                s->pcue[t] = 0xFF;
            }
        return;
    }
    for (t = 0; t < NTRACKS; t++) {                 /* move time */
        seq_track_t *tr = &s->t[t];
        if (tr->to_next != NEVER)
            tr->to_next = tr->to_next > dq ? tr->to_next - dq : 0;
        if (tr->to_off)
            tr->to_off = tr->to_off > dq ? tr->to_off - dq : 1;   /* 1: due, still "set" */
    }
    for (t = 0; t < NBASS; t++)
        if (s->bass_slide_off[t])
            s->bass_slide_off[t] = s->bass_slide_off[t] > dq ? s->bass_slide_off[t] - dq : 1;
    if (!s->ext && s->send_clock)
        s->clk_to_next = s->clk_to_next > dq ? s->clk_to_next - dq : 0;
    /* fire what is due; the drum part first, since its wrap may switch the pattern */
    for (guard = 0; guard < 8; guard++) {
        int fired = 0;
        for (t = 0; t < NTRACKS; t++) {
            seq_track_t *tr = &s->t[t];
            if (tr->to_off && tr->to_off < 256u) {    /* gate falls (before a new step at the same time) */
                bass_release(s, t - TRK_BASS0, o);
                fired = 1;
            }
            if (tr->to_next < 256u) {
                uint32_t ch = fire_step(s, t, o);
                if (ch)
                    restart(s, ch);                   /* a part's new pattern starts on step 1 */
                fired = 1;
            }
        }
        if (!fired)
            break;
    }
    for (t = 0; t < NBASS; t++)
        if (s->bass_slide_off[t] && s->bass_slide_off[t] < 256u)
            bass_slide_midi_off(s, t, o);
    if (!s->ext && s->send_clock && s->clk_to_next < 256u) {
        midi(o, 0xF8, 0, 0);
        s->clk_to_next += seq_step_q8(s, RATE_16) / 6u;           /* 24 ppqn */
    }
}

/* Publish the request type last: seq_advance must see its matching origin.
 * Start and Continue share the same setup; cont selects which flag is written last.
 * noinline: one body for seq_start / seq_continue / MIDI RT (else each site inlines). */
__attribute__((noinline)) static void request_play(seq_t *s, int remote, int cont)
{
    s->req_prior_ticks = s->ext_pending; /* clocks before this request belong to the older epoch */
    s->req_rt = (uint8_t)remote;
    if (cont) {
        s->req_start = 0;
        s->req_cont = 1; /* type last */
    } else {
        s->req_cont = 0;
        s->req_start = 1; /* type last */
    }
}
static void request_start(seq_t *s, int remote) { request_play(s, remote, 0); }
static void request_continue(seq_t *s, int remote) { request_play(s, remote, 1); }
void seq_start(seq_t *s) { request_start(s, 0); }
void seq_stop(seq_t *s)                       /* a Stop cancels an earlier Start or Continue, not a later one */
{
    s->req_start = s->req_cont = s->req_rt = 0;
    s->req_stop = 1;
    s->req_stop_rt = 0; /* local Stop supersedes any queued remote Stop */
}
void seq_continue(seq_t *s) { request_continue(s, 0); }
void seq_ext_clock(seq_t *s) { s->ext_pending++; }
int seq_midi_rt(seq_t *s, uint8_t st)
{
    int internal = s->clk_src == SEQ_CLK_INT;
    switch (st) {
    case 0xF8: if (!internal) seq_ext_clock(s); return 1;
    case 0xFA: if (!internal) request_start(s, 1); return 1;
    case 0xFB: if (!internal) request_continue(s, 1); return 1;
    case 0xFC: if (!internal) {
        seq_stop(s);
        s->req_stop_rt = 1; /* preserve remote Stop origin until audio apply */
    } return 1;
    default: return 0;
    }
}
/* MID-15: Clock, Start, Continue, Stop and Song Position of one input only. Called before seq_midi_rt, in the same context
 * as seq_advance. The order of the messages decides when two inputs send at the same time. */
int seq_clk_take(seq_t *s, uint8_t st, uint32_t src)
{
    uint8_t me;
    if (st != 0xF8u && st != 0xF2u && (st < 0xFAu || st > 0xFCu))
        return 1;
    /* Packet origins 0/1/2 are USB/Bluetooth/TRS. Origin 3 is reserved.
     * Reject it before narrowing src+1 to uint8_t: wrapped values can alias a real owner. */
    if (src > 2u)
        return 0;
    me = (uint8_t)(src + 1u);
    if (s->clk_src == SEQ_CLK_INT) {                  /* seq_midi_rt drops them */
        s->clk_lock = 0;
        return 1;
    }
    if (s->clk_lock != me) {
        if (s->clk_lock && !(s->clk_stop && (st == 0xFAu || st == 0xFBu) &&
                             s->ext_now - s->clk_stop_at >= SEQ_LOCK_T))
            return 0;                                 /* another input has the clock */
        if (st == 0xFCu || st == 0xF2u)
            return 1;                                 /* no input has the clock: a Stop or a Song Position gets none */
        if (s->clk_last && s->clk_last != me) {      /* another input: the tempo measure starts again, */
            /* Discard unrendered clocks from the old source before accepting the
             * new source's tick, or one audio block can advance twice. */
            s->ext_pending = 0;                       /* queued ticks belong to the previous input */
            s->req_prior_ticks = 0;
            s->ext_nb = s->ext_bi = s->ext_q = 0;     /* from the tempo now (no gap in the beat ring) */
            s->ext_skip = 1;                          /* (the first ticks give no period) */
        }
        s->clk_lock = s->clk_last = me;
        s->clk_stop = 0;
    }
    if (st == 0xF2u)
        return 1;                                     /* Song Position of the input with the clock */
    if (st == 0xFCu) {
        s->clk_stop = 1;
        s->clk_stop_at = s->ext_now;
    } else {
        if (st != 0xF8u)
            s->clk_stop = 0;
        s->clk_lock_at = s->ext_now;
    }
    return 1;
}
void seq_clk_drop(seq_t *s, uint32_t src)
{
    if (s->clk_lock == src + 1u) {
        s->clk_lock = 0;                              /* its link ended: any input can get the clock now */
        s->ext_pending = 0;                           /* discard queued ticks from the disconnected owner */
        s->req_prior_ticks = 0;                       /* the dropped clock epoch cannot consume new-source ticks */
    }
}
int seq_clock_waiting(const seq_t *s)
{
    return s->clk_cur == SEQ_CLK_EXT && s->playing && s->ext_since > FS / 2u;
}
X0X_COLD void seq_cue(seq_t *s, int p)
{
    int t;
    if (p >= 0 && p < NPAT)
        for (t = 0; t < NTRACKS; t++)
            s->pcue[t] = (uint8_t)p;
}
X0X_COLD void seq_cue_part(seq_t *s, int t, int p)
{
    if (t >= 0 && t < NTRACKS && p >= 0 && p < NPAT)
        s->pcue[t] = (uint8_t)p;
}
X0X_COLD int seq_cue_of(const seq_t *s, int t) { return s->pcue[t] < NPAT ? s->pcue[t] : -1; }
int seq_next_step(seq_t *s, int t, int p) { return next_pos(s, t, p, 0); }
/* the step a live hit now records on: the nearest step. That is the step that plays, or the next one
 * after half of the step (samples on the internal clock, ticks on an external clock; on an external
 * clock also a next step that its tick armed and that did not play yet). Before a part's first step
 * after PLAY or Start: its first step. */
int seq_rec_step(seq_t *s, int t)
{
    const seq_track_t *tr = &s->t[t];
    int rate = part_rate(s, t), late;
    if (!tr->started)
        return (is_bass(t) && bpart(s, t - TRK_BASS0)->dir == DIR_REV) ? seq_part_len(s, t) - 1 : 0;
    if (s->ext)
        late = tr->to_next != NEVER || (tr->ext_ticks != 0xFFu && tr->ext_ticks >= (TICKS_PER_STEP[rate] + 1u) / 2u);
    else {
        /* Record against the actual swung internal step duration, not an unswung half-step. */
        uint32_t L = seq_step_q8(s, rate), sw = swing_q8(s, rate, L);
        int np = next_pos(s, t, tr->pos, 0);
        uint32_t span = (np & 1) ? L + sw : L - sw;
        if (!(np & 1) && !(tr->pos & 1))   /* even -> even on an odd-length wrap: no swing */
            span = L;
        late = tr->to_next != NEVER && tr->to_next < span / 2u;
    }
    return late ? next_pos(s, t, tr->pos, 0) : tr->pos;
}
/* a live hit (voice v; 303: 0) recorded on step p: if p did not play yet on this pass, p does not sound
 * that voice when it plays next (the key sounded it). Once: the next step clears it. */
void seq_rec_skip(seq_t *s, int t, int p, int v)
{
    seq_track_t *tr = &s->t[t];
    if (tr->started && p == tr->pos)                  /* it played on this pass */
        return;
    if (tr->rec_skip_step != p || tr->rec_skip_pat != s->ppat[t])
        tr->rec_skip = 0;
    tr->rec_skip_step = (uint8_t)p;
    tr->rec_skip_pat = s->ppat[t];
    tr->rec_skip |= (uint16_t)(1u << v);
}
X0X_COLD void seq_chain(seq_t *s, int a, int b)
{
    /* Reject invalid endpoints before narrowing to uint8_t: an out-of-range
     * chain otherwise selects a non-existent pattern at the next bar. */
    if (a < 0 || a >= NPAT || b < 0 || b >= NPAT)
        return;
    s->chain_a = (uint8_t)a;
    s->chain_b = (uint8_t)b;
}


void seq_rec_cancel(seq_t *s, int b)
{
    seq_bass_rec_t *r = &s->rec_bass[b];
    r->gen++;
    r->own_ok = 0; /* the old press no longer owns a gate the sequencer may reopen */
    if (r->active)
        seq_rec_bass_end(s, b);
}

void seq_rec_bass_end(seq_t *s, int b)
{
    seq_bass_rec_t *r = &s->rec_bass[b];
    if (!r->active) return;
    bpart_t *bp = &s->pat[r->pat].bass[b];
    seq_track_t *tr = &s->t[TRK_BASS0 + b];
    uint32_t L = r->slot;
    uint32_t elapsed = s->ext_now - r->at;
    /* Clamp before Q8 conversion/multiply: stopped external clocks may be stale.
     * Slowest slot (20 BPM,8 ticks,+50% swing) is <=16934400 Q8;
     * slot*15+slot/2 fits uint32_t, so no 64-bit division helper is needed. */
    unsigned end = !r->entered ? 1 : r->entered == 2 || elapsed >= (L >> 8) ? 15 :
                   (unsigned)(((elapsed << 8) * 15u + L / 2u) / L);
    if (!end && r->last != r->first) {
        bp->step[r->last].flags = G_REST;
#if X0X_BASS_NOTE_SOUND
        seq_bass_sound_clear(s,b,r->pat,r->last);
#endif
        r->last = r->prev;
        end = 15; /* boundary belongs to previous slot, including reverse/wrap */
    }
    if (end < 1) end = 1;
    if (end > 15) end = 15;
    bstep_t *st = &bp->step[r->last];
    st->flags = (uint8_t)((st->flags & 15u) | (end << BS_END_SHIFT));
    r->active = 0;
    s->rec_dirty = 1;
    tr->to_off = 1; /* flush sequencer MIDI output at the actual live release */
}

void seq_rec_bass(seq_t *s, int b, int note, int velocity, int down, int src, int flags)
{
    seq_bass_rec_t *r = &s->rec_bass[b];
    if (!down) {
        if (r->active && r->note == note && r->src == src)
            seq_rec_bass_end(s, b);
        return;
    }
    if (!s->playing || !((s->rec_mask | (src < 3 ? s->rec_ext_mask : 0u)) & (1u << b))) return;
    /* A cell may be visited repeatedly before the onset repeats. One endpoint
     * nibble cannot identify that occurrence. Refuse before any pattern write;
     * the accepted live edge still auditions through the engine. */
    if (bpart(s, b)->dir >= DIR_PINGPONG) {
        s->rec_refused = 1;
        return;
    }
    /* The flag alone is not proof. A context cancel bumps gen and clears own_ok,
     * so a later open DSP gate cannot slide from the cancelled note. */
    int owned = r->own_ok && r->own_gen == r->gen && r->own_src == (uint8_t)src;
    int overlap = (flags & SEQ_REC_LEGATO) && owned && r->active && r->src == src;
    int t = TRK_BASS0 + b, p = seq_rec_step(s, t);
    int old = r->last == p ? r->prev : r->last, oldpat = r->pat;
#if X0X_BASS_NOTE_SOUND
    if(seq_bass_sound_prepare(s,b,s->ppat[t],p)) {
        s->rec_refused=2; /* sound pool full or sound identity unavailable */
        return;
    }
    /* Preparation is the atomic admission check and already publishes sound.
     * A just-entered provisional tail can round back to the previous slot on
     * release. That tail is now the new onset: finalize the previous slot at
     * its full length without clearing the sound we just admitted. */
    if (r->active && r->pat == s->ppat[t] && r->last == p &&
        r->last != r->first && r->entered == 1) {
        uint32_t elapsed = s->ext_now - r->at, L = r->slot;
        if (elapsed < (L >> 8) && ((elapsed << 8) * 15u + L / 2u) / L == 0) {
            r->last = r->prev;
            r->entered = 2;
        }
    }
#endif
    seq_rec_bass_end(s, b);
    bpart_t *bp = bpart(s, b);
    if (overlap && oldpat == s->ppat[t] && old != p)
        bp->step[old].flags = (uint8_t)((bp->step[old].flags & 15u) | BS_SLIDE);
    bp->step[p].note = (uint8_t)note;
    bp->step[p].velocity = (uint8_t)velocity;
    bp->step[p].flags = (uint8_t)(G_NOTE | (flags & (BS_ACCENT | BS_SLIDE)) | 0xF0u);
    r->active = 1; r->pat = s->ppat[t]; r->first = r->last = r->prev = (uint8_t)p;
    r->note = (uint8_t)note; r->src = (uint8_t)src;
    s->rec_dirty = 1;
    r->entered = s->t[t].started && p == s->t[t].pos;
    r->at = s->ext_now; r->slot = bass_slot_q8(s, b, p);
    if (r->entered) {
        uint32_t L = bass_slot_q8(s, b, p), elapsed;
        if (s->ext) {
            elapsed = (uint32_t)s->t[t].ext_ticks * s->ext_period + (s->ext_since << 8);
            uint32_t sw = (p & 1) ? swing_q8(s, bp->rate, seq_step_q8(s, bp->rate)) : 0;
            elapsed = elapsed > sw ? elapsed - sw : 0;
        } else elapsed = s->t[t].to_next <= L ? L - s->t[t].to_next : 0;
        r->at -= elapsed >> 8;
    }
    seq_rec_skip(s, t, p, 0);
    s->t[t].to_off = 0;
    s->t[t].sounding = 1; /* tie/stop bookkeeping follows the live authoritative gate */
    s->t[t].held_note = (uint8_t)(note | (((flags & BS_SLIDE) || overlap) ? 0x80u : 0));
}
