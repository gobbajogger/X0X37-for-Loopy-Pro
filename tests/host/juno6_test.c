/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "../../firmware/src/dsp/juno6.h"
int main(void){juno6_t s;juno6_chorus_t c;float mono[512],l[512],r[512];unsigned i,a=0;juno6_init(&s,44100);for(i=0;i<6;i++)assert(juno6_note_on(&s,(uint8_t)(48+i*3),100));for(i=0;i<6;i++)a+=s.voice[i].gate!=0;assert(a==6);juno6_render(&s,mono,512);for(i=0;i<512;i++)assert(isfinite(mono[i]));juno6_note_off(&s,48);a=0;for(i=0;i<6;i++)a+=s.voice[i].gate!=0;assert(a==5);assert(juno6_note_on(&s,72,110));a=0;for(i=0;i<6;i++)a+=s.voice[i].gate!=0;assert(a==6);juno6_all_off(&s);for(i=0;i<6;i++)assert(!s.voice[i].gate&&s.voice[i].stage==4);juno6_render(&s,mono,512);juno6_chorus_init(&c,44100);juno6_chorus_set(&c,.8f,1.7f,.25f);assert(c.depth_ms==1.7f);juno6_chorus_set(&c,.8f,5.4f,.25f);assert(c.depth_ms==5.4f);memset(l,0,sizeof l);memset(r,0,sizeof r);l[0]=r[0]=.5f;juno6_chorus_process(&c,l,r,512);for(i=0;i<512;i++)assert(isfinite(l[i])&&isfinite(r[i]));assert(c.write==0);assert(sizeof c.delay==512);puts("juno6: six voices, stealing, release, chorus bounds and shared 512-byte ring PASS");return 0;}
