/* SPDX-License-Identifier: GPL-3.0-only */
#include "chicago_fx2_bank.h"
#include "chicago_engine_output.h"
#include "chicago_control_owner.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint64_t digest=UINT64_C(1469598103934665603);static unsigned frames,blocks;
static void word(uint32_t v){for(unsigned j=0;j<4;j++){digest=(digest^(uint8_t)v)*UINT64_C(1099511628211);v>>=8;}}
static int16_t input_signal(unsigned i,unsigned k){uint32_t v=(i*7919u+k*131071u)^(i*17u+k*53u);v&=65535u;return v<32768u?(int16_t)v:(int16_t)((int32_t)v-65536);}
static void noop(void *p){(void)p;}static void nt(void *p,int32_t t){(void)p;(void)t;}
typedef struct{original_chicago_drive_wrapper state;original_chicago_fx2_kernel kernel;} drive;
static void dp(void *p,int16_t *l,int16_t *r,uint32_t n){drive *d=p;original_chicago_drive_queue*q=&d->state.queue;for(uint32_t i=0;i<n;i++){while(q->cursor<q->count&&q->events[q->cursor].offset<=i){if(q->events[q->cursor].type==4)original_chicago_drive_reset(&d->state.drive);q->cursor++;}assert(original_chicago_drive_process(&d->state.drive,l+i,r+i,l+i,r+i,1));}}
static void ds(void *p,unsigned a,unsigned b){drive*d=p;assert(original_chicago_drive_set(&d->state.drive,a,b));}
static void dr(void *p){drive*d=p;original_chicago_drive_reset(&d->state.drive);}
static void da(void*p,unsigned a){drive*d=p;assert(original_chicago_drive_activity(&d->state.drive,a));}
static const original_chicago_fx2_kernel *df(void*p,unsigned t){return t==19?&((drive*)p)->kernel:NULL;}
static void fixed_drive_equivalence(void){
 int16_t a[4][257],b[2][257],sc[4][257];
 original_chicago_fx2_workspace workspace={sc[0],sc[1],sc[2],sc[3],257};
 for(unsigned test=0;test<24;test++){
  unsigned e1=test*17u&127u,e2=test*31u&127u,on=test&1;
  original_chicago_drive_wrapper old;assert(original_chicago_drive_wrapper_init(&old,e1,e2,on));drive d;memset(&d,0,sizeof d);original_chicago_drive_init(&d.state.drive);d.kernel=(original_chicago_fx2_kernel){&d,dp,ds,dr,noop,noop,nt,da};
  original_chicago_fx2_owner owner={&d,df,dr};original_chicago_fx2_wrapper next;assert(original_chicago_fx2_wrapper_init(&next,&owner));assert(original_chicago_fx2_wrapper_set(&next,0,19));assert(original_chicago_fx2_wrapper_set(&next,1,on));assert(original_chicago_fx2_wrapper_set(&next,2,e1));assert(original_chicago_fx2_wrapper_set(&next,3,e2));
  for(unsigned block=0;block<97;block++){
   if(block%7==0){unsigned v=(test*37+block*11)&127;assert(original_chicago_drive_wrapper_set(&old,2,v));assert(original_chicago_fx2_wrapper_set(&next,2,v));}
   if(block%13==0){unsigned v=block&1;assert(original_chicago_drive_wrapper_set(&old,1,v));assert(original_chicago_fx2_wrapper_set(&next,1,v));}
   if(block==23||block==61){original_chicago_drive_wrapper_reset(&old);original_chicago_fx2_wrapper_reset(&next);}
   original_chicago_drive_event event[]={{0,9},{3,4},{3,5},{63,4},{128,1}};
   assert(original_chicago_drive_wrapper_events(&old,event,5));d.state.queue=(original_chicago_drive_queue){event,5,0};
   uint32_t n=block%19==0?0:(test*11+block*19)%257+1;
   for(unsigned i=0;i<n;i++){a[0][i]=b[0][i]=input_signal(i,test+block);a[1][i]=b[1][i]=input_signal(i,test+block+1);}
   assert(original_chicago_drive_wrapper_process(&old,a[0],a[1],a[2],a[3],n,257));assert(original_chicago_fx2_wrapper_process(&next,&workspace,b[0],b[1],n));
   assert(!memcmp(a[0],b[0],n*2)&&!memcmp(a[1],b[1],n*2));assert(!memcmp(&old.drive,&d.state.drive,sizeof old.drive)&&old.queue.cursor==d.state.queue.cursor);
   assert(old.wet_target==next.wet_target&&old.wet_accumulator==next.wet_accumulator&&old.progress==next.progress&&old.activity_counter==next.activity_counter&&old.switch_current==next.switch_current&&old.switch_pending==next.switch_pending&&old.switch_dirty==next.switch_dirty&&old.transitioning==next.transitioning);
   for(unsigned i=0;i<n;i++){word((uint32_t)(int32_t)b[0][i]);word((uint32_t)(int32_t)b[1][i]);}frames+=n;blocks++;
  }
 }
}
static void *allocate(size_t n){void*p=malloc(n);assert(p);memset(p,0xa5,n);return p;}
/* Empty original voice pool at this control/output boundary. These total
 * callbacks observe no live DSP voices; actual voice integration has its own
 * source-derived owner test, not a substituted effect body. */
static unsigned cv(void*p,unsigned id){(void)p;(void)id;return 0;}
static unsigned cs(void*p,unsigned id){(void)p;(void)id;return 6;}
static void cw(void*p,unsigned id,int a,int b,int c){(void)p;(void)id;(void)a;(void)b;(void)c;}
static void cn(void*p,unsigned id,const original_voice_record*r,unsigned full,unsigned mask,int32_t last){(void)p;(void)id;(void)r;(void)full;(void)mask;(void)last;assert(0);}
static void cm(void*p,original_chicago_allocator*a){(void)p;(void)a;}
static void bank_join(void){
 original_chicago_fx2_bank *bank=allocate(sizeof *bank);
 original_chicago_fx2_storage m={{allocate(2*CHFX_SHORT_SLOTS),CHFX_SHORT_SLOTS},{allocate(2*CHFX_BPM_STEREO_SLOTS),CHFX_BPM_STEREO_SLOTS},{allocate(2*CHFX_GATE_SLOTS),CHFX_GATE_SLOTS},{allocate(2*CHFX_HALL_SLOTS),CHFX_HALL_SLOTS},{allocate(4096),2048},allocate(4096),allocate(4096),1024,{NULL,NULL}};
 original_chicago_fx2_storage bad=m;bad.bpm_delay.slots--;original_chicago_fx2_bank saved=*bank;assert(!original_chicago_fx2_bank_init(bank,&bad));assert(!memcmp(bank,&saved,sizeof saved));assert(original_chicago_fx2_bank_init(bank,&m));
 original_chicago_fx2_owner owner={bank,original_chicago_fx2_bank_find,original_chicago_fx2_bank_reset_public_projection};
 const unsigned types[]={0,24,18,22,10,4,3,5,12,13,15,16};for(unsigned t=0;t<30;t++){int present=0;for(unsigned j=0;j<12;j++)present|=types[j]==t;assert((original_chicago_fx2_bank_find(bank,t)!=NULL)==present);}
 uint32_t il[67],ir[67],ol[67],orr[67];int16_t input[2][67],scratch[4][67];
 original_chicago_output_workspace w={input[0],input[1],scratch[0],scratch[1],67};original_chicago_fx2_workspace fw={scratch[0],scratch[1],scratch[2],scratch[3],67};
 for(unsigned factory=0;factory<81;factory++){
  original_chicago_program p;original_chicago_routing routing;assert(original_chicago_program_init(&p,factory)&&original_chicago_program_routing(&p,&routing));original_chicago_engine_output s;assert(original_chicago_engine_output_init(&s,&routing,&owner));original_chicago_output_mute(&s.output,0);
  original_chicago_voice_controls compiled;assert(original_chicago_voice_compile(&compiled,&p));
  chicago_arp_control keys;memset(&keys,0,sizeof keys);original_held_record held[128];original_voice_record voice_record[1];original_chicago_allocator allocator;
  assert(original_chicago_allocator_bind(&allocator,&keys,held,voice_record,1,1,1,1,0,0));original_chicago_allocator_reset_lists(&allocator);allocator.octave=1;
  original_chicago_selector selector={-1,60,1,0,0x6d70};chicago_arp_clock clock={0,12345,0};chicago_arp_clock_input clock_input={137,1};chicago_arp_control_input ci={routing.arp_enable,routing.arp_latch,0};
  const original_allocator_callbacks voice_callbacks={cv,cs,cw,cn};const original_chicago_scheduler_callbacks scheduler_callbacks={&voice_callbacks,cm,cm};original_chicago_scheduler scheduler;
  assert(original_chicago_scheduler_bind(&scheduler,&allocator,&selector,&clock,&clock_input,&ci,&scheduler_callbacks,NULL));
  original_chicago_global_mg_input gi={44100,44100,compiled.mg_frequency[0],compiled.mg_frequency[1],compiled.mg_shape[0],compiled.mg_shape[1],compiled.mg_tempo[0],compiled.mg_tempo[1]};
  original_chicago_control_owner control;assert(original_chicago_control_owner_bind(&control,&p,&compiled,&s,&scheduler,&gi));
  /* No all30/original arena reset claim: this test explicitly supplies the
   * public projection and dedicated host histories. All actual bodies run. */
  for(unsigned block=0;block<48;block++){
   unsigned macro=block%22;float value=(float)((factory*19u+block*31u)&127u)/127;
   uint32_t committed=keys.committed_enable;float threshold=clock.threshold;
   assert(original_chicago_control_owner_edit(&control,macro,value));routing=compiled.routing;
   assert(p.macro[macro]==value&&clock.threshold==threshold);
   if(macro!=21)assert(keys.committed_enable==committed);
   assert(allocator.arp_mode==routing.arp_mode&&allocator.arp_range==routing.arp_range&&ci.enable==routing.arp_enable&&ci.latch==routing.arp_latch);
   assert(gi.rate1==compiled.mg_frequency[0]&&gi.rate2==compiled.mg_frequency[1]&&gi.tempo_sync2==compiled.mg_tempo[1]);
   assert(original_chicago_fx2_wrapper_set(&s.fx2,0,types[block%12]));
   if(block%12==1){/* A->B->A before rendering must retain typeDirty. */unsigned a=types[block%12];assert(original_chicago_fx2_wrapper_set(&s.fx2,0,types[(block+1)%12]));assert(original_chicago_fx2_wrapper_set(&s.fx2,0,a));assert(s.fx2.type_dirty);}
   original_chicago_drive_event e[]={{0,9},{3,4},{3,5},{65,4}};assert(original_chicago_drive_wrapper_events(&s.output.fx1,e,4));assert(original_chicago_fx2_bank_events(bank,e,4,0));
   assert(original_chicago_fx2_wrapper_tempo(&s.fx2,200+(int32_t)((factory*23u+block*107u)%3801u)));
   for(unsigned i=0;i<67;i++){float l=(float)input_signal(i,factory+block)/65536,r=(float)input_signal(i,factory+block+1)/65536;memcpy(il+i,&l,4);memcpy(ir+i,&r,4);}
   assert(original_chicago_engine_output_process(&s,&w,&fw,&routing,il,ir,ol,orr,67,0));
   assert(s.fx2.active==original_chicago_fx2_bank_find(bank,types[block%12]));
   for(unsigned i=0;i<67;i++){word(ol[i]);word(orr[i]);}frames+=67;blocks++;
  }
  if(factory%2==0)original_chicago_fx2_bank_direct_event(bank,2);else original_chicago_fx2_bank_direct_event(bank,5);
 }
 free(m.short_delay.data);free(m.bpm_delay.data);free(m.gate.data);free(m.hall.data);free(m.chorus.data);free(m.ensemble_left);free(m.ensemble_right);free(bank);
}
/* Explicit lifecycle check independent of audio arithmetic. Old/current can
 * alias: both calls advance the same object before retire. A full chunk runs
 * after its audible weight reacheszero. Inactive targets must survive. */
typedef struct{unsigned id,reset,begin,retire,process,calls,parameters,activity,tempo;original_chicago_fx2_kernel k;} observed;
static unsigned sequence[512],ns;
static void op(void*p,int16_t*l,int16_t*r,uint32_t n){observed*x=p;assert(ns<512);sequence[ns++]=100+x->id;x->process+=n;x->calls++;(void)l;(void)r;}
static void os(void*p,unsigned a,unsigned b){observed*x=p;x->parameters++;(void)a;(void)b;}
static void ore(void*p){((observed*)p)->reset++;}static void ob(void*p){((observed*)p)->begin++;}static void ort(void*p){((observed*)p)->retire++;}
static void ot(void*p,int32_t t){((observed*)p)->tempo=(unsigned)t;}static void oa(void*p,unsigned a){((observed*)p)->activity=a;}
static const original_chicago_fx2_kernel *of(void*p,unsigned t){observed*x=p;return t<2?&x[t].k:NULL;}
static void orall(void*p){observed*x=p;ore(x);ore(x+1);}
static void lifecycle(void){
 observed o[2];memset(o,0,sizeof o);for(unsigned j=0;j<2;j++){o[j].id=j;o[j].k=(original_chicago_fx2_kernel){o+j,op,os,ore,ob,ort,ot,oa};}
 original_chicago_fx2_owner owner={o,of,orall};original_chicago_fx2_wrapper s;assert(original_chicago_fx2_wrapper_init(&s,&owner));assert(original_chicago_fx2_wrapper_set(&s,0,0));int16_t l[1490]={0},r[1490]={0},sc[4][1490];original_chicago_fx2_workspace w={sc[0],sc[1],sc[2],sc[3],1490};
 assert(original_chicago_fx2_wrapper_process(&s,&w,l,r,0));assert(o[0].reset==1&&o[0].begin==1&&!o[0].calls&&o[0].activity==1);
 assert(original_chicago_fx2_wrapper_process(&s,&w,l,r,67));assert(o[0].process==67&&!s.transitioning&&o[0].activity==0);
 assert(original_chicago_fx2_wrapper_set(&s,0,1));assert(original_chicago_fx2_wrapper_set(&s,2,91));assert(o[0].parameters==3&&!o[1].parameters);
 assert(original_chicago_fx2_wrapper_process(&s,&w,l,r,1));assert(sequence[ns-2]==100&&sequence[ns-1]==101&&o[1].process==1&&s.transitioning);
 /* Advance to a small progress then force a chunk/remainder split. */
 assert(original_chicago_fx2_wrapper_process(&s,&w,l,r,42));assert(s.progress==818);unsigned old_calls=o[0].calls,new_calls=o[1].calls;
 assert(original_chicago_fx2_wrapper_process(&s,&w,l,r,1490));assert(o[0].calls==old_calls+1&&o[1].calls==new_calls+2&&o[0].retire==1);
 assert(original_chicago_fx2_wrapper_set(&s,0,0));assert(original_chicago_fx2_wrapper_set(&s,0,1));unsigned before=o[1].process;
 assert(original_chicago_fx2_wrapper_process(&s,&w,l,r,67));assert(s.old==s.active&&o[1].process==before+134&&o[1].retire==1);
}
int main(void){fixed_drive_equivalence();lifecycle();bank_join();printf("PASS FX2 generic-vs-reviewed-fixedDrive full audio/state; explicit lifecycle and actual12-kernel/81-program output join: %u stereo frames,%u blocks,hash%016llx; public-reset/dedicated-host-history projection only; PENDING_REVIEW\n",frames,blocks,(unsigned long long)digest);return 0;}
