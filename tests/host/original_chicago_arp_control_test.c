#include "chicago_arp_control.h"
#include "original_chicago_arp_witnesses.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#define FNV UINT64_C(1469598103934665603)
static uint64_t word(uint64_t h,uint32_t v)
{for(unsigned i=0;i<4;++i){h=(h^(uint8_t)v)*UINT64_C(1099511628211);v>>=8;}return h;}
static uint32_t bits(float f){uint32_t v;memcpy(&v,&f,4);return v;}
static uint32_t val(unsigned c,unsigned i,int lat)
{return (i+c+(lat?2u:0u))%5u==0?0u:(i*(lat?19u:37u)+c*(lat?7u:13u))&127u;}
typedef struct{chicago_arp_control *s;uint64_t hash;unsigned events;} ctlctx;
static void emit(ctlctx *c,uint32_t tag,uint32_t arg)
{
 uint32_t v[]={tag,arg,c->s->physical_count,c->s->latched_count,c->s->committed_enable,c->s->effective_latch,c->s->physical[60],c->s->latched[60]};
 for(unsigned i=0;i<sizeof(v)/sizeof(*v);++i)c->hash=word(c->hash,v[i]);++c->events;
}
static void retire_held(void *v){ctlctx*c=v;assert(c->s->committed_enable==0);emit(c,1,0);}
static void off(void *v,int32_t note){emit(v,2,(uint32_t)note);}
static void retire_voices(void *v){ctlctx*c=v;assert(c->s->physical_count==0&&c->s->latched_count==0);emit(c,3,0);}
static uint32_t velocity(void *v,int32_t note)
{
 ctlctx*c=v;emit(c,4,(uint32_t)note);assert(note>=0&&note<256);
 return note<128?c->s->physical[note]:c->s->latched[note-128];
}
static uint64_t statehash(const chicago_arp_control*s)
{
 uint64_t h=FNV;for(unsigned i=0;i<128;++i)h=word(h,s->physical[i]);for(unsigned i=0;i<128;++i)h=word(h,s->latched[i]);
 h=word(h,s->physical_count);h=word(h,s->latched_count);h=word(h,s->committed_enable);h=word(h,s->effective_latch);return word(h,(uint32_t)s->current_note);
}
typedef struct{ctlctx ctl;chicago_arp_control_input *in;unsigned cleanup_sequence;uint8_t expected_byte;} damperctx;
static void cleanup_held(void*v){damperctx*d=v;assert(d->in->damper_byte==d->expected_byte);d->cleanup_sequence=d->cleanup_sequence*10u+1u;}
static void cleanup_voices(void*v){damperctx*d=v;assert(d->in->damper_byte==d->expected_byte);d->cleanup_sequence=d->cleanup_sequence*10u+2u;}
typedef struct{uint64_t hash;unsigned steps,notify;chicago_arp_clock_input *mutate;unsigned sequence;} clkctx;
static void step(void*v){clkctx*c=v;c->hash=word(c->hash,1);++c->steps;c->sequence=c->sequence*10u+1;if(c->mutate&&c->steps==1)c->mutate->bpm=200.0f;}
static void notify(void*v){clkctx*c=v;c->hash=word(c->hash,2);++c->notify;c->sequence=c->sequence*10u+2;}
int main(void)
{
 const chicago_arp_control_callbacks cb={retire_held,off,retire_voices,velocity};
 const int notes[]={0,60,127,128,187,255};const uint8_t pedals[]={0,1,128,255};
 uint64_t control_digest=FNV,clock_digest=FNV;
 for(unsigned c=0;c<384;++c){
  unsigned f=c%64;chicago_arp_control s={0};
  for(unsigned i=0;i<128;++i){s.physical[i]=val(c,i,0);s.latched[i]=val(c,i,1);s.physical_count+=s.physical[i]!=0;s.latched_count+=s.latched[i]!=0;}
  s.committed_enable=f&1;s.effective_latch=(f>>1)&1;s.current_note=notes[c/64];
  chicago_arp_control_input in={(f>>2)&1,(f>>3)&1,pedals[f>>4]};ctlctx ctx={&s,FNV,0};
  chicago_arp_control_prepare(&s,&in,&cb,&ctx);
  assert(statehash(&s)==control_expected[c].state);assert(ctx.hash==control_expected[c].trace);assert(ctx.events==control_expected[c].events);
  control_digest=word(control_digest,(uint32_t)ctx.hash);control_digest=word(control_digest,(uint32_t)(ctx.hash>>32));
 }
 const chicago_arp_clock_callbacks ccb={step,notify};
 const float bases[]={96,48,36,24,16,12,8,6,4,3};const float bpms[]={20,81,120,170,400};
 for(unsigned c=0;c<160;++c){
  float fs=c%2==0?44100.0f:48000.0f,base=bases[c/16],times=(float)(16-c%16);
  chicago_arp_clock s={0};clkctx ctx={FNV,0,0,0,0};chicago_arp_clock_input in={0};
  chicago_arp_clock_prepare(&s,fs,base,times);assert(bits(s.threshold)==clock_expected[c].threshold);
  for(unsigned frame=0;frame<5000;++frame){
   if(frame%977==0)chicago_arp_clock_key(&s,1,1);
   else chicago_arp_clock_key(&s,1,2); /* Overlapping keys do not reset. */
   if(frame%64==0)chicago_arp_clock_prepare(&s,fs,base,times);
   in.bpm=bpms[(frame/113+c)%5];in.tempo_sync=(frame+c)%19==0?0:1;
   chicago_arp_clock_tick(&s,&in,&ccb,&ctx);ctx.hash=word(ctx.hash,bits(s.accumulator));ctx.hash=word(ctx.hash,s.key_reset);
  }
  assert(ctx.hash==clock_expected[c].trace);assert(ctx.steps==clock_expected[c].steps);assert(ctx.notify==clock_expected[c].notify);assert(bits(s.accumulator)==clock_expected[c].acc);
  clock_digest=word(clock_digest,(uint32_t)ctx.hash);clock_digest=word(clock_digest,(uint32_t)(ctx.hash>>32));
 }
 /* Ordered callback operand reload and single subtraction, including a case
  * that still exceeds threshold afterward. This does not require fast-math. */
 chicago_arp_clock s={999,50,1};chicago_arp_clock_input in={100,1};clkctx ctx={FNV,0,0,&in,0};
 chicago_arp_clock_tick(&s,&in,&ccb,&ctx);assert(s.accumulator==150&&s.key_reset==0&&ctx.sequence==121&&ctx.steps==2&&ctx.notify==1);
 in.tempo_sync=0;chicago_arp_clock_tick(&s,&in,&ccb,&ctx);assert(s.accumulator==300&&ctx.steps==2&&ctx.notify==1);
 chicago_arp_clock_key(&s,0,1);assert(s.key_reset==0);
 s.accumulator=NAN;chicago_arp_clock_tick(&s,&in,&ccb,&ctx);assert(isnan(s.accumulator)&&ctx.steps==2&&ctx.notify==1);
 const chicago_arp_damper_callbacks dcb={cleanup_held,cleanup_voices};
 unsigned damper_cases=0;
 for(unsigned v=0;v<128;++v)for(unsigned hold=0;hold<2;++hold)
 for(unsigned en=0;en<2;++en)for(unsigned latch=0;latch<2;++latch)
 for(unsigned old=0;old<4;++old){
  chicago_arp_control ds={0};chicago_arp_control_input di={en,latch,pedals[old]};
  ds.committed_enable=en;ds.effective_latch=latch|(pedals[old]!=0);ds.current_note=60;
  ds.physical[64]=99;ds.physical_count=1;ds.latched[60]=100;ds.latched_count=1;
  damperctx d={{&ds,FNV,0},&di,0,(uint8_t)(v<64?0:127)};
  unsigned previous=ds.effective_latch,wanted=latch|(v>=64);
  chicago_arp_damper_apply(&ds,&di,v,hold,&dcb,&cb,&d);
  assert(di.damper_byte==d.expected_byte&&ds.effective_latch==wanted);
  assert(d.cleanup_sequence==((v<64&&hold==0)?12u:0u));
  if(previous!=wanted&&(wanted||en))assert(ds.latched[60]==0&&ds.latched[64]==99&&ds.latched_count==1);
  if(previous&&!wanted&&en)assert(d.ctl.events==2); /* lookup then NoteOff60 */
  else assert(d.ctl.events==0);
  unsigned events=d.ctl.events;
  chicago_arp_damper_apply(&ds,&di,v,hold,&dcb,&cb,&d);
  assert(d.cleanup_sequence==((v<64&&hold==0)?1212u:0u));
  assert(d.ctl.events==events); /* Same-byte apply still cleans up; ARP remains stable. */
  damper_cases+=2;
 }
 assert(damper_cases==8192);
 printf("{\"control_cases\":384,\"clock_cases\":160,\"clock_frames\":800000,\"damper_setter_calls\":8192,\"control_digest\":\"%016llx\",\"clock_digest\":\"%016llx\",\"control_state_host_bytes\":%zu,\"clock_state_host_bytes\":%zu,\"all_pass\":true}\n",(unsigned long long)control_digest,(unsigned long long)clock_digest,sizeof(chicago_arp_control),sizeof(chicago_arp_clock));
 return 0;
}
