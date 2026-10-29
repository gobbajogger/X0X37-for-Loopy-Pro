/* SPDX-License-Identifier: GPL-3.0-only */
#include "dsp/wobble_backend.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
int main(void)
{
    const float rates[] = {1,2,3,4,6,8,12};
    const float tempos[] = {20,60,90,120,180,240,300};
    const float sr[] = {44100,48000,96000};
    for (unsigned si=0;si<3;++si) for (unsigned rate=0;rate<128;++rate) {
        wobble_t w, before;
        float l[256],r[256];
        wobble_init(&w,sr[si]);
        wobble_set(&w,W_WOBBLE_RATE,(float)rate/127);
        unsigned bin=(unsigned)(((float)rate/127)*6+.5f);
        assert(w.lfo_step==rates[bin]/sr[si]); /* Exact old 120 BPM value. */
        wobble_note_on(&w,48,100);
        wobble_render(&w,l,r,256);
        for (unsigned t=0;t<7;++t) {
            uint8_t bank[16],after[16];
            wobble_bank_save(&w,bank);
            before=w;
            assert(wobble_set_tempo(&w,tempos[t]));
            assert(fabsf(w.lfo_step*sr[si]-rates[bin]*tempos[t]/120)<1e-5f);
            before.tempo_bpm=w.tempo_bpm; before.lfo_step=w.lfo_step;
            assert(memcmp(&before,&w,sizeof w)==0); /* No held-note/state changes. */
            wobble_bank_save(&w,after); assert(!memcmp(bank,after,16));
            float phase=w.lfo_phase;
            wobble_render(&w,l,r,1);
            assert(fabsf(w.lfo_phase-(phase+w.lfo_step-floorf(phase+w.lfo_step)))<1e-6f);
            before=w;
            assert(!wobble_set_tempo(&w,NAN)); assert(!wobble_set_tempo(&w,INFINITY));
            assert(!wobble_set_tempo(&w,0)); assert(!wobble_set_tempo(&w,-1));
            assert(!memcmp(&before,&w,sizeof w));
        }
        assert(wobble_set_tempo(&w,1) && w.tempo_bpm==20);
        assert(wobble_set_tempo(&w,999) && w.tempo_bpm==300);
        wobble_reset(&w); assert(w.tempo_bpm==300 && !w.active);
    }
    puts("PASS Miami tempo: 3 sample rates x128 controls x7 tempos; exact120BPM rates; phase, held state, bank and invalid-input preservation");
}
