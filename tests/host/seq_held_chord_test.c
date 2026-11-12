/* SPDX-License-Identifier: GPL-3.0-only */
/* Real sequencer + sparse serialized lane store. Only the physical-key source
 * and DSP/MIDI sinks are supplied by this host fixture. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../../firmware/src/seq/sequencer.h"
#include "../../firmware/src/seq/held_chord.h"
static pattern_t pats[NPAT];static lane_t lanes[NLANE];static seq_t s;
static uint8_t held[6];static int nh,enabled=1,switch_kind=-1,ons,offs,midi_on,midi_off;
int seq_bass_poly(seq_t *q,int b){(void)q;return b==0&&enabled;}
int seq_bass_chord_keys(seq_t*q,int b,uint8_t*n){(void)q;(void)b;memcpy(n,held,nh);return nh;}
int seq_bass_chord_set(seq_t*q,int b,int p,int st,const uint8_t*n,int count){(void)q;return held_chord_set(lanes,p,b,st,n,count);}
int seq_bass_chord_get(seq_t*q,int b,int p,int st,uint8_t*n){return held_chord_get(lanes,p,b,st,q->pat[p].bass[b].step[st].note,n);}
void seq_bass_chord_on(seq_t*q,int b,int note,int velocity){(void)q;(void)b;(void)note;(void)velocity;ons++;}
static int bon(void*x,int b,int n,int a,int sl,int v){(void)x;(void)b;(void)n;(void)a;(void)sl;(void)v;if(switch_kind>=0)enabled=switch_kind;ons++;return 1;}
static void boff(void*x,int b){(void)x;(void)b;offs++;}
static void midi(void*x,uint8_t st,uint8_t n,uint8_t v){(void)x;(void)n;(void)v;if((st&0xf0)==0x90)midi_on++;if((st&0xf0)==0x80)midi_off++;}
static void brk(void*x,int a,int b,float c,int d){(void)x;(void)a;(void)b;(void)c;(void)d;}
static seq_sink_t out={.bass_on=bon,.bass_off=boff,.midi=midi,.brk=brk};
static void setup(void){memset(lanes,0,sizeof lanes);for(int p=0;p<NPAT;p++)pattern_init(&pats[p],1,2);seq_init(&s,pats);s.send_clock=0;s.send_notes=1;ons=offs=midi_on=midi_off=nh=0;enabled=1;switch_kind=-1;}
static void run(unsigned n){seq_advance(&s,0,&out);while(n){unsigned k=seq_until_event(&s,n>256?256:n);seq_advance(&s,k,&out);n-=k;}}
static void chord(void){uint8_t n[]={60,64,67};pats[0].bass[0].step[0]=(bstep_t){60,G_NOTE,100};pats[0].bass[0].step[1]=(bstep_t){60,G_TIE|0xf0,100};assert(held_chord_set(lanes,0,0,0,n,3));}
int main(void){
 setup();s.playing=1;s.rec_mask=1;s.t[TRK_BASS0].started=1;s.t[TRK_BASS0].to_next=seq_step_q8(&s,RATE_16);
 held[0]=60;nh=1;seq_rec_bass(&s,0,60,100,1,3,0);held[1]=67;nh=2;seq_rec_bass(&s,0,67,100,1,3,0);
 uint8_t notes[6];assert(pats[0].bass[0].step[0].note==60);assert(seq_bass_chord_get(&s,0,0,0,notes)==2&&notes[1]==67);
 nh=1;seq_rec_bass(&s,0,67,0,0,3,0);assert(seq_bass_chord_get(&s,0,0,0,notes)==1);nh=0;seq_rec_bass(&s,0,60,0,0,3,0);assert(!s.rec_bass[0].active);
 setup();chord();seq_start(&s);run(1);assert(ons==3&&midi_on==3);unsigned step=seq_step_q8(&s,RATE_16)>>8;run(step);assert(ons==3&&offs==0);s.mute|=1u<<MUTE_BASS0;run(1);assert(offs==1&&midi_off==3);run(step);assert(ons==3);
 setup();chord();seq_start(&s);run(1);seq_stop(&s);run(1);assert(offs==1&&midi_off==3);assert(!s.chord_out_n[0]);
 setup();chord();seq_start(&s);run(1);seq_notes_off(&s,&out);assert(offs==1&&midi_off==3);
 setup();chord();pats[0].drum[0].len=1;seq_start(&s);run(1);seq_cue_part(&s,TRK_BASS0,1);run(step+1);assert(s.ppat[TRK_BASS0]==1&&offs==1&&midi_off==3);
 setup();chord();pats[0].bass[0].step[0].flags=G_TIE;seq_start(&s);run(1);assert(ons==0); /* orphan tie is silent */
 setup();enabled=0;s.playing=1;s.rec_mask=1;seq_rec_bass(&s,0,60,100,1,3,0);seq_rec_bass(&s,0,67,100,1,3,0);assert(pats[0].bass[0].step[0].note==67); /* legacy mono unchanged */
 setup();s.playing=1;s.rec_mask=1;for(int k=0;k<NLANE;k++)lanes[k].used=1;held[0]=60;held[1]=64;nh=2;seq_rec_bass(&s,0,60,100,1,3,0);assert(s.rec_refused==3&&bstep_gate(&pats[0].bass[0].step[0])==G_REST);
 setup();chord();enabled=0;switch_kind=1;seq_start(&s);run(1);assert(ons==3&&midi_on==3);
 setup();chord();switch_kind=0;seq_start(&s);run(1);assert(ons==1&&midi_on==1);
 puts("PASS held-key capture, one-note, tie, mute, Stop, PANIC, cue, orphan tie, mono, capacity");return 0;
}
