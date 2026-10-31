/* SPDX-License-Identifier: GPL-3.0-only */
/* Link the actual engine, observe its tempo call, and reuse product host setup. */
#include MIAMI_SETUP_SOURCE
static unsigned tempo_calls;
static float last_bpm;
int miami_observe_tempo(wobble_t *w, float bpm)
{
    ++tempo_calls; last_bpm=bpm;
    return wobble_set_tempo(w,bpm);
}
int main(void)
{
    int32_t out[512];
    assert(miami_wheel_regressions()==0);
    /* The existing wheel tests leave Miami selected. Free play still follows
     * effective tempo, even when sequencer transport is stopped. */
    assert(engine_bass_is_wobble(0));
    seq.playing=0; seq.ext=0; seq.bpm=90;
    tempo_calls=0; engine_render(out,256);
    assert(tempo_calls && last_bpm==90);
    seq.bpm=180; engine_render(out,256); assert(last_bpm==180);
    seq.ext=1; seq.ext_bpm=75; engine_render(out,256); assert(last_bpm==75);
    seq.ext_bpm=150; engine_render(out,256); assert(last_bpm==150);
    seq.ext=0; engine_render(out,256); assert(last_bpm==180);
    seq.playing=1; seq.bpm=120; engine_render(out,256); assert(last_bpm==120);
    puts("PASS actual engine Miami tempo: internal90/180, external75/150, internal fallback; stopped free play and running transport");
    return 0;
}
