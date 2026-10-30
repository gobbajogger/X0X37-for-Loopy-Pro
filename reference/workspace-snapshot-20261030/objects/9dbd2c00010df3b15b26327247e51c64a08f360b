/* SPDX-License-Identifier: GPL-3.0-only */
/* Linked production sequencer regression: a live 303 ping-pong length shrink
 * cannot step through positions outside the new range or mutate lookahead.
 * Compile against firmware/src/seq/{sequencer,pattern,tb3po}.c. */
#include <stdio.h>
#include "../../firmware/src/seq/sequencer.h"

static unsigned checks, failures;
#define CHECK(expr, what) do { checks++; if (!(expr)) { \
    failures++; if (failures <= 12u) printf("FAIL %s:%d %s\n", __FILE__, __LINE__, what); \
} } while (0)

static pattern_t patterns[NPAT];
static seq_t seq;
static unsigned observed_steps;
static int observed_last_step;
static unsigned note_on_count, note_off_count;

static void on_step(void *context, int track, int step)
{
    (void)context;
    if (track == TRK_BASS0) {
        observed_steps++;
        observed_last_step = step;
    }
}
static int on_bass(void *context, int part, int pitch, int accent, int slide, int velocity)
{
    (void)context; (void)part; (void)pitch; (void)accent; (void)slide; (void)velocity;
    note_on_count++;
 return 1; }
static void off_bass(void *context, int part)
{
    (void)context; (void)part;
    note_off_count++;
}
static const seq_sink_t sink = {
    .bass_on = on_bass, .bass_off = off_bass, .step = on_step
};

static void reset_seq(int length, int direction)
{
    for (int i=0; i<NPAT; i++) pattern_init(&patterns[i], (unsigned)(i+1), (unsigned)(i+101));
    seq_init(&seq, patterns);
    seq.send_clock = 0;
    patterns[0].bass[0].dir = DIR_PINGPONG;
    patterns[0].bass[0].len = (uint8_t)length;
    seq.t[TRK_BASS0].pp_dir = (int8_t)direction;
    observed_steps = 0;
    observed_last_step = -1;
    note_on_count = note_off_count = 0;
}

static void test_lookahead(void)
{
    static const int pvals[] = {0,1,2,3,4,5,7,15,31,63};
    for (int dir=-1; dir<=1; dir+=2) {
        reset_seq(4,dir);
        for (unsigned i=0; i<sizeof pvals/sizeof pvals[0]; i++) {
            int p=pvals[i];
            int got=seq_next_step(&seq, TRK_BASS0,p);
            CHECK(got>=0 && got<4, "lookahead within shortened 303 pattern");
            CHECK(seq.t[TRK_BASS0].pp_dir == dir, "lookahead preserves committed ping-pong direction");
            if (p>=4) CHECK(got==3, "out-of-range high position snaps to last valid step");
        }
    }
    reset_seq(4,1);
    CHECK(seq_next_step(&seq,TRK_BASS0,-1)==0, "negative lookahead index snaps to first step");
    CHECK(seq.t[TRK_BASS0].pp_dir==1, "negative lookahead cannot flip direction");
    reset_seq(1,1);
    CHECK(seq_next_step(&seq,TRK_BASS0,15)==0, "one-step pattern clamps old playhead");
    reset_seq(8,1);
    CHECK(seq_next_step(&seq,TRK_BASS0,3)==4, "expanding pattern preserves interior forward progression");
    reset_seq(4,-1);
    patterns[0].bass[0].dir=DIR_REV;
    CHECK(seq_next_step(&seq,TRK_BASS0,3)==2, "reverse mode unchanged");
    reset_seq(4,1);
    patterns[0].bass[0].dir=DIR_FWD;
    CHECK(seq_next_step(&seq,TRK_BASS0,3)==0, "forward mode unchanged");
}

static void test_committed_audio_step(void)
{
    for (int direction=-1; direction<=1; direction+=2) {
        reset_seq(16,direction);
        seq_start(&seq);
        seq_advance(&seq,0,&sink);
        CHECK(seq.playing, "transport started");
        patterns[0].bass[0].len=4;   /* user shortens while original step 16 is still current */
        seq.t[TRK_BASS0].started=1;
        seq.t[TRK_BASS0].pos=15;
        seq.t[TRK_BASS0].pp_dir=(int8_t)direction;
        seq.t[TRK_BASS0].to_next=0;
        unsigned prior=observed_steps;
        seq_advance(&seq,0,&sink);
        CHECK(seq.t[TRK_BASS0].pos==3, "committed next playback step is inside newly shortened pattern");
        CHECK(seq.t[TRK_BASS0].pp_dir==-1, "committed out-of-range correction points back inward");
        CHECK(observed_steps==prior+1, "next 303 step emitted exactly once");
        CHECK(observed_last_step==3, "UI step callback saw last enabled step, not removed step");
        seq.t[TRK_BASS0].to_next=0;
        seq_advance(&seq,0,&sink);
        CHECK(seq.t[TRK_BASS0].pos==2, "subsequent playback walks inward one step");
        CHECK(observed_last_step==2, "next UI callback sees inward step");
    }
}

static void test_shrink_matrix(void)
{
    for (int old_len=2; old_len<=64; old_len++) {
        for (int new_len=1; new_len<=old_len; new_len++) {
            reset_seq(new_len,1);
            for (int p=0; p<old_len; p++) {
                for (int dir=-1; dir<=1; dir+=2) {
                    seq.t[TRK_BASS0].pp_dir=(int8_t)dir;
                    int got=seq_next_step(&seq,TRK_BASS0,p);
                    CHECK(got>=0 && got<new_len, "entire shrink matrix lookahead in bounds");
                    CHECK(seq.t[TRK_BASS0].pp_dir==dir, "shrink matrix lookahead has no side effects");
                }
            }
        }
    }
}

int main(void)
{
    test_lookahead();
    test_committed_audio_step();
    test_shrink_matrix();
    printf("pingpong linked: %u checks, %u failures (note on %u, off %u)\n",
           checks,failures,note_on_count,note_off_count);
    return failures!=0u;
}
