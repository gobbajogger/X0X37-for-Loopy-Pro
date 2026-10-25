/* SPDX-License-Identifier: GPL-3.0-only */
/* BF-033: Random direction TIE preview must match the next committed random position. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "sequencer.h"
#include "tb3po.h"

static pattern_t patterns[NPAT];
static seq_t seq;
static int note_on, note_off, m_on, m_off, errors, total;

#define EXPECT(c,why) do { total++; if (!(c)) { errors++; fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__,__LINE__,why); } } while (0)

static int drum(void *ctx,int k,int v,float vel) { (void)ctx; (void)k; (void)v; (void)vel; return 1; }
/* BF-028 on nightly: bass_on is int (*)(...); return 1 = accept. */
static int bass_on(void *ctx,int b,int note,int accent,int slide,int vel) {
    (void)ctx; (void)note; (void)accent; (void)slide; (void)vel;
    if (b == 0) note_on++;
    return 1;
}
static void bass_off(void *ctx,int b) { (void)ctx; if (b == 0) note_off++; }
static void brk(void *ctx,int p,int bar,float len,int enable) {
    (void)ctx; (void)p; (void)bar; (void)len; (void)enable;
}
static void brk_stop(void *ctx) { (void)ctx; }
static void midi(void *ctx, uint8_t st,uint8_t n,uint8_t v) {
    (void)ctx; (void)n; (void)v;
    if ((st & 0xF0u)==0x90u && (st & 0x0Fu)==1u) m_on++;
    if ((st & 0xF0u)==0x80u && (st & 0x0Fu)==1u) m_off++;
}
static const seq_sink_t sink = {
    .drum=drum, .bass_on=bass_on, .bass_off=bass_off,
    .brk=brk, .brk_stop=brk_stop, .midi=midi
};

static void setup(uint8_t dir,uint8_t forward_gate,uint8_t actual_random_gate) {
    memset(patterns,0,sizeof(patterns));
    pattern_init(&patterns[0],33,44);
    seq_init(&seq,patterns);
    seq.send_clock=0;
    seq.send_notes=1;
    seq.t[TRK_BASS0].rng=2u; /* next xorshift32 %4 == 2 */
    bpart_t *b=&patterns[0].bass[0];
    b->len=4; b->dir=dir;
    for(int p=0;p<4;p++) {
        b->step[p].flags=G_REST;
        b->step[p].velocity=100;
        b->step[p].note=(uint8_t)(48+p);
    }
    b->step[0].flags=G_NOTE;
    b->step[1].flags=forward_gate;
    b->step[2].flags=actual_random_gate;
    b->step[3].flags=G_REST;
    note_on=note_off=m_on=m_off=0;
}

static void start_then_midpoint(void) {
    uint32_t L=seq_step_q8(&seq,RATE_16);
    seq_start(&seq);
    seq_advance(&seq,1,&sink);
    EXPECT(seq.t[TRK_BASS0].pos==0,"first step must be 0");
    EXPECT(note_on==1,"first NOTE sounds exactly once");
    EXPECT(m_on==1,"first NOTE emits one MIDI NoteOn");
    EXPECT(seq.t[TRK_BASS0].rng==2u,"RANDOM preview does not draw RNG");
    uint32_t mid = (L >> 9) + 2u; /* just past 50%-gate release */
    seq_advance(&seq,mid,&sink);
}

static void finish_next(void) {
    uint32_t L=seq_step_q8(&seq,RATE_16);
    uint32_t mid=(L >> 9)+2u;
    uint32_t remainder=(L>>8)-mid+4u;
    seq_advance(&seq,remainder,&sink);
    EXPECT(seq.t[TRK_BASS0].pos==2,"RANDOM next position agrees with preview (seed2->2)");
    EXPECT(seq.t[TRK_BASS0].rng!=2u,"real RANDOM step commits RNG once");
}

static void case_random_actual_tie(void) {
    setup(DIR_RANDOM,G_REST,G_TIE);
    start_then_midpoint();
    EXPECT(note_off==0,"RANDOM NOTE -> selected TIE holds note through midpoint");
    EXPECT(seq.t[TRK_BASS0].sounding==1,"selected TIE has uninterrupted local 303 gate");
    EXPECT(m_off==0,"selected TIE does not send premature external MIDI NoteOff");
    finish_next();
    EXPECT(note_on==1,"selected TIE does not retrigger local NOTE");
    EXPECT(m_on==1,"selected TIE does not retrigger MIDI NoteOn");
    EXPECT(seq.t[TRK_BASS0].sounding==1,"selected TIE still holds sound after next step");
}

static void case_random_actual_note_not_tie(void) {
    setup(DIR_RANDOM,G_TIE,G_NOTE);
    start_then_midpoint();
    EXPECT(note_off==1,"unselected forward TIE must not wrongly prolong a RANDOM NOTE");
    EXPECT(m_off==1,"unselected forward TIE must not prolong MIDI NOTE");
    finish_next();
    EXPECT(note_on==2,"new selected NOTE triggers normally");
    EXPECT(m_on==2,"new selected NOTE emits MIDI NoteOn normally");
}

static void case_forward_tie(void) {
    setup(DIR_FWD,G_TIE,G_REST);
    start_then_midpoint();
    EXPECT(note_off==0,"FORWARD NOTE -> TIE still holds at midpoint");
    EXPECT(m_off==0,"FORWARD NOTE -> TIE keeps external MIDI gate");
}

static void case_forward_rest(void) {
    setup(DIR_FWD,G_REST,G_TIE);
    start_then_midpoint();
    EXPECT(note_off==1,"FORWARD NOTE -> REST still ends at midpoint");
    EXPECT(m_off==1,"FORWARD NOTE -> REST sends one MIDI NoteOff");
}

int main(void) {
    case_random_actual_tie();
    case_random_actual_note_not_tie();
    case_forward_tie();
    case_forward_rest();
    fprintf(stderr,"BF033 real-linked behavior: %d/%d assertions, errors=%d\n",total-errors,total,errors);
    return errors ? 1 : 0;
}
