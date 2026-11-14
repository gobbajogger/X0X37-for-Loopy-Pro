/* SPDX-License-Identifier: GPL-3.0-only */
/* Compare the actual frozen hoist parent and candidate DSP, including state. */
#define _POSIX_C_SOURCE 200809L
#include "juno6.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void parent_juno6_init(juno6_t *, float);
int parent_juno6_note_on(juno6_t *, uint8_t, uint8_t);
void parent_juno6_note_off(juno6_t *, uint8_t);
void parent_juno6_all_off(juno6_t *);
void parent_juno6_render(juno6_t *, float *, unsigned);
int parent_juno6_set(juno6_t *, unsigned, float);
int parent_juno6_active(const juno6_t *);
void parent_juno6_chorus_init(juno6_chorus_t *, float);
void parent_juno6_chorus_set(juno6_chorus_t *, float, float, float);
void parent_juno6_chorus_process(juno6_chorus_t *, float *, float *, unsigned);

typedef struct { juno6_t synth; juno6_chorus_t chorus; } state_t;
static state_t a, b;
static unsigned long checks, frames;
static const char *scenario;
static void fail(const char *what) {
 fprintf(stderr,"FAIL scenario=%s check=%lu frames=%lu: %s\n",scenario,checks,frames,what);exit(1);
}
static void same(const void *x,const void *y,size_t n,const char *what) {
 checks++;if(memcmp(x,y,n))fail(what);
}
static void states(void) {
 same(&a,&b,sizeof a,"full engine/chorus state bytes");
 if(parent_juno6_active(&a.synth)!=juno6_active(&b.synth))fail("active result");
}
static void init(void) {
 memset(&a,0,sizeof a);memset(&b,0,sizeof b);
 parent_juno6_init(&a.synth,44100);juno6_init(&b.synth,44100);
 parent_juno6_chorus_init(&a.chorus,44100);juno6_chorus_init(&b.chorus,44100);states();
}
static void set(unsigned id,float v) {
 if(parent_juno6_set(&a.synth,id,v)!=juno6_set(&b.synth,id,v))fail("set result");
 states();
}
static void on(uint8_t n,uint8_t v) {
 if(parent_juno6_note_on(&a.synth,n,v)!=juno6_note_on(&b.synth,n,v))fail("note_on result");
 states();
}
static void off(uint8_t n) {parent_juno6_note_off(&a.synth,n);juno6_note_off(&b.synth,n);states();}
static void all_off(void) {parent_juno6_all_off(&a.synth);juno6_all_off(&b.synth);states();}
static void render(unsigned n) {
 float al[257],ar[257],bl[257],br[257];
 /* Also compare untouched output suffix (including all of the zero-frame call). */
 memset(al,0xa5,sizeof al);memcpy(bl,al,sizeof al);
 parent_juno6_render(&a.synth,al,n);juno6_render(&b.synth,bl,n);
 same(al,bl,sizeof al,"mono PCM bits/output guard");states();
 memcpy(ar,al,sizeof ar);memcpy(br,bl,sizeof br);
 parent_juno6_chorus_process(&a.chorus,al,ar,n);juno6_chorus_process(&b.chorus,bl,br,n);
 same(al,bl,sizeof al,"left PCM bits/output guard");same(ar,br,sizeof ar,"right PCM bits/output guard");states();
 frames+=n;
}
static void boundaries(void) {
 float values[]={0.0f,-0.0f,nextafterf(0,1),0x1p-126f,
  nextafterf(.000001f,0),.000001f,nextafterf(.000001f,1),
  nextafterf(.00001f,0),.00001f,nextafterf(.00001f,1),.0001f,.5f,1.0f};
 const float releases[]={0,.5f,1};
 scenario="release boundaries";
 for(unsigned r=0;r<3;r++)for(unsigned v=0;v<sizeof values/sizeof values[0];v++){
  init();set(8,releases[r]);
  for(unsigned i=0;i<6;i++)on((uint8_t)(36+12*i),100);
  render(37); /* Nonzero phase/filter and chorus history at release. */
  for(unsigned i=0;i<6;i++){
   a.synth.voice[i].gate=b.synth.voice[i].gate=0;
   a.synth.voice[i].stage=b.synth.voice[i].stage=4;
   a.synth.voice[i].envelope=b.synth.voice[i].envelope=values[v];
  }
  states();render(0);
  float phase=a.synth.voice[0].phase;
  render(1);
  if(values[v]<.00001f && (a.synth.voice[0].stage!=0 || a.synth.voice[0].envelope!=0))fail("tail did not retire");
  if(a.synth.voice[0].phase==phase)fail("retiring voice lost final phase update");
  render(1);render(17);render(256);render(257);
 }
}
static uint32_t rng=0x6a09e667;
static uint32_t random32(void) {rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
static void lifecycle(void) {
 static const unsigned blocks[]={0,1,2,17,63,256,257};
 scenario="note lifecycle";init();set(5,0);set(6,0);set(7,.72f);set(8,0);
 for(unsigned i=0;i<6;i++)on((uint8_t)(48+3*i),100);
 for(unsigned i=0;i<8;i++)render(256); /* Attack, decay, sustain. */
 on(72,110);off(48);on(72,60); /* Steal, stale off, retrigger. */
 set(17,1);off(72);off(51);render(257);set(17,0);render(17);
 set(16,0);render(1);set(16,1);render(63);on(70,0);
 for(unsigned i=0;i<128;i++)off((uint8_t)i);
 for(unsigned i=0;i<32;i++)render(256);
 if(juno6_active(&b.synth))fail("minimum release did not finish");
 all_off();render(0);render(257);
 for(unsigned id=0;id<18;id++){set(id,0);render(1);set(id,1);render(17);}
 parent_juno6_chorus_set(&a.chorus,.1f,1.7f,0);juno6_chorus_set(&b.chorus,.1f,1.7f,0);states();render(257);
 parent_juno6_chorus_set(&a.chorus,5,5.4f,.5f);juno6_chorus_set(&b.chorus,5,5.4f,.5f);states();
 scenario="deterministic event stream";
 for(unsigned k=0;k<3000;k++){
  uint32_t x=random32();uint8_t note=(uint8_t)(24+(x>>8)%85);
  switch(x%7){
   case 0:on(note,(uint8_t)(1+(x>>16)%127));break;
   case 1:off(note);break;
   case 2:set((x>>16)%18,(float)(random32()%129)/128);break;
   case 3:if((x>>16)%7==0)all_off();else on(note,0);break;
   default:break;
  }
  render(blocks[random32()%(sizeof blocks/sizeof blocks[0])]);
 }
 all_off();render(257);
}

/* Host wall/CPU times are evidence, never converted into device cycles. */
typedef void (*render_fn)(juno6_t *,float *,unsigned);
typedef void (*chorus_fn)(juno6_chorus_t *,float *,float *,unsigned);
typedef struct {uint64_t wall,cpu;} timing_t;
static volatile float consume;
static uint64_t now_ns(clockid_t clock) {
 struct timespec ts;if(clock_gettime(clock,&ts))exit(2);
 return (uint64_t)ts.tv_sec*UINT64_C(1000000000)+(uint64_t)ts.tv_nsec;
}
static timing_t timed(render_fn rf,chorus_fn cf,const state_t *seed) {
 float left[256],right[256];state_t s;
 uint64_t wall=now_ns(CLOCK_MONOTONIC),cpu=now_ns(CLOCK_THREAD_CPUTIME_ID);
 for(unsigned rep=0;rep<16;rep++){
  memcpy(&s,seed,sizeof s);
  for(unsigned block=0;block<64;block++){
   rf(&s.synth,left,256);memcpy(right,left,sizeof right);cf(&s.chorus,left,right,256);
  }
  consume+=left[255]+right[255]+s.synth.voice[0].envelope;
 }
 uint64_t elapsed_cpu=now_ns(CLOCK_THREAD_CPUTIME_ID)-cpu;
 timing_t result={now_ns(CLOCK_MONOTONIC)-wall,elapsed_cpu};return result;
}
static void benchmark(void) {
 const unsigned voices[]={0,1,6,1,6};
 const char *names[]={"idle","held-1","held-6","release-1","release-6"};
 for(unsigned c=0;c<5;c++){
  state_t seed;memset(&seed,0,sizeof seed);juno6_init(&seed.synth,44100);juno6_chorus_init(&seed.chorus,44100);
  juno6_set(&seed.synth,8,1);
  for(unsigned i=0;i<voices[c];i++){
   juno6_note_on(&seed.synth,(uint8_t)(48+3*i),100);
   seed.synth.voice[i].envelope=.8f;seed.synth.voice[i].stage=c>=3?4:3;
   seed.synth.voice[i].gate=c>=3?0:1;
  }
  (void)timed(parent_juno6_render,parent_juno6_chorus_process,&seed);
  (void)timed(juno6_render,juno6_chorus_process,&seed);
  for(unsigned pair=0;pair<9;pair++){
   timing_t a1=timed(parent_juno6_render,parent_juno6_chorus_process,&seed);
   timing_t b1=timed(juno6_render,juno6_chorus_process,&seed);
   timing_t b2=timed(juno6_render,juno6_chorus_process,&seed);
   timing_t a2=timed(parent_juno6_render,parent_juno6_chorus_process,&seed);
   printf("{\"kind\":\"timing\",\"case\":\"%s\",\"pair\":%u,\"voices\":%u,\"frames_per_run\":262144,\"parent_ns\":[%llu,%llu],\"candidate_ns\":[%llu,%llu],\"parent_cpu_ns\":[%llu,%llu],\"candidate_cpu_ns\":[%llu,%llu]}\n",names[c],pair,voices[c],(unsigned long long)a1.wall,(unsigned long long)a2.wall,(unsigned long long)b1.wall,(unsigned long long)b2.wall,(unsigned long long)a1.cpu,(unsigned long long)a2.cpu,(unsigned long long)b1.cpu,(unsigned long long)b2.cpu);
  }
 }
}
int main(int argc,char **argv) {
 if(argc==2&&!strcmp(argv[1],"--bench")){benchmark();return 0;}
 boundaries();lifecycle();
 printf("{\"kind\":\"equivalence\",\"status\":\"PASS\",\"checks\":%lu,\"frames\":%lu,\"sample_rate\":44100,\"juno_state_bytes\":%zu,\"chorus_state_bytes\":%zu}\n",checks,frames,sizeof(juno6_t),sizeof(juno6_chorus_t));return 0;
}
