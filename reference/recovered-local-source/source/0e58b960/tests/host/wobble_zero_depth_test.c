/* SPDX-License-Identifier: GPL-3.0-only */
#include "wobble.h"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <time.h>
void baseline_wobble_init(wobble_t*,float);
int baseline_wobble_set(wobble_t*,unsigned,float);
int baseline_wobble_set_tempo(wobble_t*,float);
void baseline_wobble_reset(wobble_t*);
void baseline_wobble_note_on(wobble_t*,uint8_t,uint8_t);
void baseline_wobble_note_on_slide(wobble_t*,uint8_t,uint8_t,int);
void baseline_wobble_note_off(wobble_t*,uint8_t);
void baseline_wobble_all_notes_off(wobble_t*);
void baseline_wobble_render(wobble_t*,float*,float*,size_t);
static uint32_t random_state=6436;
static uint32_t rnd(void){random_state^=random_state<<13;random_state^=random_state>>17;random_state^=random_state<<5;return random_state;}
static unsigned long frames;
static void both_set(wobble_t*a,wobble_t*b,unsigned p,float v){assert(baseline_wobble_set(a,p,v)==wobble_set(b,p,v));}
static void compare(wobble_t*a,wobble_t*b,size_t n){float al[256],ar[256],bl[256],br[256];baseline_wobble_render(a,al,ar,n);wobble_render(b,bl,br,n);assert(!memcmp(al,bl,n*4)&&!memcmp(ar,br,n*4));assert(!memcmp(a,b,sizeof *a));frames+=n;}
static void edges(void){
 uint32_t values[]={0,0x80000000u,1,0x007fffffu,0x00800000u,0x3effffffu,0x3f000000u,0x3f000001u,0x3f7fffffu,0x3f800000u,0xbf800000u,0x7f800000u,0x7fc00000u};
 float depths[]={0,-0.0f,0x1p-149f,.5f,1};
 for(unsigned r=0;r<3;r++)for(unsigned shape=0;shape<2;shape++)for(unsigned v=0;v<13;v++)for(unsigned d=0;d<5;d++){
  wobble_t a,b;memset(&a,0,sizeof a);memset(&b,0,sizeof b);float cutoff;memcpy(&cutoff,&values[v],4);
  baseline_wobble_init(&a,r==0?8000:r==1?44100:192000);wobble_init(&b,r==0?8000:r==1?44100:192000);
  both_set(&a,&b,W_CUTOFF,cutoff);both_set(&a,&b,W_WOBBLE_DEPTH,depths[d]);both_set(&a,&b,W_WOBBLE_SHAPE,(float)shape);
  baseline_wobble_note_on(&a,127,127);wobble_note_on(&b,127,127);compare(&a,&b,256);
 }
}
static void transitions(void){
 baseline_wobble_render(NULL,NULL,NULL,0);wobble_render(NULL,NULL,NULL,0);
 float rates[]={8000,44100,48000,96000,192000,NAN,INFINITY,-1};
 for(unsigned rate=0;rate<8;rate++)for(unsigned scenario=0;scenario<64;scenario++){
  wobble_t a,b;memset(&a,0,sizeof a);memset(&b,0,sizeof b);baseline_wobble_init(&a,rates[rate]);wobble_init(&b,rates[rate]);
  for(unsigned p=0;p<W_PARAMETER_COUNT;p++)both_set(&a,&b,p,(float)(rnd()%129)/128);
  both_set(&a,&b,W_WOBBLE_DEPTH,scenario&1?-0.0f:0.0f);baseline_wobble_note_on(&a,48,100);wobble_note_on(&b,48,100);
  for(unsigned block=0;block<300;block++){
   if(block%11==0)both_set(&a,&b,W_WOBBLE_DEPTH,(block%33==0)?0.0f:(float)(rnd()%128+1)/128);
   if(block%7==0)both_set(&a,&b,W_CUTOFF,(float)(rnd()%257)/256);
   if(block%13==0){unsigned p=rnd()%W_PARAMETER_COUNT;both_set(&a,&b,p,(float)(rnd()%385)/128-1);}
   if(block%19==0){both_set(&a,&b,W_CUTOFF,NAN);both_set(&a,&b,W_WOBBLE_DEPTH,INFINITY);both_set(&a,&b,W_PARAMETER_COUNT,1);}
   if(block%23==0){float tempo=(float)(rnd()%400);assert(baseline_wobble_set_tempo(&a,tempo)==wobble_set_tempo(&b,tempo));}
   if(block%41==0){uint8_t note=(uint8_t)(rnd()%128);baseline_wobble_note_on_slide(&a,note,127,1);wobble_note_on_slide(&b,note,127,1);}
   if(block==231){baseline_wobble_note_off(&a,a.note);wobble_note_off(&b,b.note);}
   if(block==262){baseline_wobble_reset(&a);wobble_reset(&b);}
   if(block==270){baseline_wobble_note_on(&a,0,1);wobble_note_on(&b,0,1);}
   if(block==290){baseline_wobble_all_notes_off(&a);wobble_all_notes_off(&b);}
   size_t n=block%5==0?0:block%5==1?1:block%5==2?63:block%5==3?127:256;compare(&a,&b,n);
  }
 }
 printf("PASS %lu bit-identical stereo frames AND entire unchanged DSP state across controls/reset/note/tempo/nonfinite/rate/block boundaries\n",frames);
}
static void bench(void){for(unsigned depth=0;depth<2;depth++)for(unsigned square=0;square<2;square++)for(unsigned which=0;which<2;which++){
 wobble_t s;memset(&s,0,sizeof s);wobble_init(&s,44100);wobble_set(&s,W_CARRIER_WAVE,(float)square);wobble_set(&s,W_WOBBLE_SHAPE,(float)square);wobble_set(&s,W_WOBBLE_DEPTH,depth?.5f:0);wobble_note_on(&s,48,100);float l[256],r[256];clock_t t=clock();
 for(unsigned block=0;block<8192;block++){if(which)wobble_render(&s,l,r,256);else baseline_wobble_render(&s,l,r,256);}
 printf("BENCH depth%u square%u %s 2097152frames %.6f seconds tail%.9g\n",depth,square,which?"candidate":"baseline",(double)(clock()-t)/CLOCKS_PER_SEC,l[255]);
}}
int main(int argc,char**argv){(void)argv;if(argc>1)bench();else {edges();transitions();}return 0;}
