/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "../../firmware/src/dsp/juno6.h"
static unsigned gates(const juno6_t *s) {
 unsigned n=0;for(unsigned i=0;i<6;i++)n+=s->voice[i].gate!=0;return n;
}
int main(void) {
 juno6_t s;juno6_chorus_t c;float mono[512],l[512],r[512];
 juno6_init(&s,44100);
 for(unsigned i=0;i<6;i++)assert(juno6_note_on(&s,(uint8_t)(48+i*3),100));
 assert(gates(&s)==6);
 juno6_render(&s,mono,512);
 float energy=0;
 for(unsigned i=0;i<512;i++){assert(isfinite(mono[i])&&fabsf(mono[i])<1);energy+=fabsf(mono[i]);}
 assert(energy>0);
 /* All held: the oldest note, not an arbitrary newer note, is stolen. */
 assert(juno6_note_on(&s,72,110));assert(s.voice[0].note==72&&gates(&s)==6);
 juno6_note_off(&s,48);assert(gates(&s)==6); /* old stolen note cannot end 72 */
 assert(juno6_set(&s,17,1));juno6_note_off(&s,72);
 assert(s.voice[0].stage!=4&&!s.voice[0].gate);
 assert(juno6_set(&s,17,0));assert(s.voice[0].stage==4);
 float increment=s.voice[1].increment;
 assert(juno6_set(&s,16,1));assert(s.voice[1].increment>increment);
 assert(!juno6_set(&s,0,NAN)&&!juno6_set(&s,18,0));
 juno6_all_off(&s);assert(!juno6_active(&s)&&!s.damper);
 juno6_render(&s,mono,512);for(unsigned i=0;i<512;i++)assert(mono[i]==0);
 juno6_chorus_init(&c,44100);juno6_chorus_set(&c,.8f,5.4f,.25f);
 assert(c.depth_ms==5.4f&&sizeof c.delay==512);
 memset(l,0,sizeof l);memset(r,0,sizeof r);l[0]=r[0]=.5f;
 juno6_chorus_process(&c,l,r,512);
 /* At phase zero, delay is ~156 samples; no premature wet signal below 1.7ms. */
 for(unsigned i=1;i<74;i++)assert(l[i]==0&&r[i]==0);
 energy=0;for(unsigned i=74;i<240;i++)energy+=fabsf(l[i]);assert(energy>0);
 for(unsigned i=0;i<512;i++)assert(isfinite(l[i])&&isfinite(r[i]));
 assert(c.write==0);
 puts("juno6: six voices, deterministic stealing, sustain, bend, panic, bounded shared chorus PASS");
}
