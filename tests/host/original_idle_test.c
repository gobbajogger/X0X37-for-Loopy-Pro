/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual owners versus their unshortened kernel/output paths. */
#include "original_chicago_backend.h"
#include "chicago_fx2_compact.h"
#include "original_miami_backend.h"
#include "miami_fx_conversion.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define OK ORIGINAL_CHICAGO_BACKEND_OK
static const unsigned types[] = {0,4,5,10,18,22,24};
static const unsigned macro_indices[] = {0,5,7,4,2,3,1};
static void hash_bytes(uint64_t *h,const void *p,size_t n)
{
    const unsigned char *b = p;
    for (size_t i = 0; i < n; ++i) *h = (*h ^ b[i]) * UINT64_C(1099511628211);
}

static uint32_t bits(float x) { uint32_t u; memcpy(&u,&x,4); return u; }

typedef struct {
    uint32_t core[2][256],expected[2][256];
    int16_t converted[2][256],scratch[4][256];
    int16_t ref_converted[2][256],ref_scratch[4][256];
    float out[2][256];
    original_chicago_backend_workspace workspace;
} audio_workspace;

static void workspace_init(audio_workspace *w)
{
    memset(w,0,sizeof *w);
    w->workspace = (original_chicago_backend_workspace){w->core[0],w->core[1],
        w->converted[0],w->converted[1],
        {w->scratch[0],w->scratch[1],w->scratch[2],w->scratch[3]},256};
}

static unsigned idle_checks, phase_attempts, phase_hits, miami_probe;
static uint64_t idle_pcm=UINT64_C(1469598103934665603);
#define REQUIRE(x) do { ++idle_checks; if(!(x)){fprintf(stderr,"idle FAIL %d: %s\n",__LINE__,#x);exit(1);} } while(0)

/* Linker interposition counts actual cross-unit processing entries. It is
 * not elapsed time, target cycles or a hardware CPU percentage. */
enum {WORK_DRIVE,WORK_FX2,WORK_FIXED,WORK_DECIMATOR,WORK_MASTER,WORK_COUNT};
typedef struct {uint64_t calls,frames;} work_count;
static work_count work[2][WORK_COUNT];
static int proof_side=-1;
static void record_work(unsigned id,uint32_t n){
 if(proof_side>=0){work[proof_side][id].calls++;work[proof_side][id].frames+=n;}
}
int __real_original_chicago_drive_process(original_chicago_drive *,const int16_t *,const int16_t *,int16_t *,int16_t *,size_t);
int __wrap_original_chicago_drive_process(original_chicago_drive *s,const int16_t *l,const int16_t *r,int16_t *ol,int16_t *orr,size_t n){
 record_work(WORK_DRIVE,(uint32_t)n);return __real_original_chicago_drive_process(s,l,r,ol,orr,n);
}
int __real_original_chicago_fx2_wrapper_process(original_chicago_fx2_wrapper *,original_chicago_fx2_workspace *,int16_t *,int16_t *,uint32_t);
int __wrap_original_chicago_fx2_wrapper_process(original_chicago_fx2_wrapper *s,original_chicago_fx2_workspace *w,int16_t *l,int16_t *r,uint32_t n){
 record_work(WORK_FX2,n);return __real_original_chicago_fx2_wrapper_process(s,w,l,r,n);
}
int __real_original_fixed_fx_process(original_fixed_fx *,const int32_t *,const int32_t *,int16_t *,int16_t *,int32_t *,int32_t *,uint32_t,original_effect_process,void *);
int __wrap_original_fixed_fx_process(original_fixed_fx *s,const int32_t *l,const int32_t *r,int16_t *sl,int16_t *sr,int32_t *ol,int32_t *orr,uint32_t n,original_effect_process effect,void *p){
 record_work(WORK_FIXED,n);return __real_original_fixed_fx_process(s,l,r,sl,sr,ol,orr,n,effect,p);
}
int __real_miami_decimator_frame(miami_decimator_component_t *,uint64_t,int16_t,int16_t,int32_t,int16_t [2]);
int __wrap_miami_decimator_frame(miami_decimator_component_t *s,uint64_t i,int16_t l,int16_t r,int32_t wet,int16_t out[2]){
 record_work(WORK_DECIMATOR,1);return __real_miami_decimator_frame(s,i,l,r,wet,out);
}
int __real_original_master_process(original_master *,float,const uint32_t *,const uint32_t *,uint32_t *,uint32_t *,uint32_t);
int __wrap_original_master_process(original_master *s,float v,const uint32_t *l,const uint32_t *r,uint32_t *ol,uint32_t *orr,uint32_t n){
 record_work(WORK_MASTER,n);return __real_original_master_process(s,v,l,r,ol,orr,n);
}
static void work_report(const char *engine,unsigned id,const char *phase){
 static const char *names[]={"drive","fx2","fixed","decimator","master"};
 REQUIRE(work[0][WORK_MASTER].frames==work[1][WORK_MASTER].frames);
 REQUIRE(work[0][WORK_MASTER].calls==work[1][WORK_MASTER].calls);
 for(unsigned k=0;k<WORK_COUNT;k++){
  REQUIRE(work[0][k].frames<=work[1][k].frames);
  printf("WORK %s %u %s %s calls %llu/%llu frames %llu/%llu\n",engine,id,phase,names[k],
   (unsigned long long)work[0][k].calls,(unsigned long long)work[1][k].calls,
   (unsigned long long)work[0][k].frames,(unsigned long long)work[1][k].frames);
 }
}

int __real_miami_decimator_advance_zero(miami_decimator_component_t *,uint32_t);
int __wrap_miami_decimator_advance_zero(miami_decimator_component_t *s,uint32_t n){
 int result=__real_miami_decimator_advance_zero(s,n);
 if(miami_probe){phase_attempts++;phase_hits+=(unsigned)result;}
 return result;
}

/* The separate backend object uses these same unchanged engine operations,
 * but links output to the independent, unshortened FX chain below. */
int reference_miami_backend_init(original_miami_backend *,unsigned,float);
int reference_miami_backend_prepare(original_miami_backend *);
int reference_miami_backend_note_on(original_miami_backend *,unsigned,unsigned);
int reference_miami_backend_note_off(original_miami_backend *,unsigned);
int reference_miami_backend_set_macro(original_miami_backend *,unsigned,float);
int reference_miami_backend_set_tempo(original_miami_backend *,float);
int reference_miami_backend_damper(original_miami_backend *,unsigned);
int reference_miami_backend_panic(original_miami_backend *);
int reference_miami_backend_load_factory(original_miami_backend *,unsigned);
int reference_miami_backend_render_event_words(original_miami_backend *,original_miami_fx_workspace *,uint32_t *,uint32_t *,uint32_t,const original_engine_event *,uint32_t);

typedef struct {miami_decimator_adapter_t *effect;const uint32_t *events;uint32_t count;} reference_resets;
static void reference_decimate(void *p,int16_t *l,int16_t *r,uint32_t n){
 reference_resets *s=p;REQUIRE(miami_decimator_adapter_process(s->effect,l,r,n,s->events,s->count));
}
static void reference_bypass(void *p,int16_t *l,int16_t *r,uint32_t n){(void)p;(void)l;(void)r;(void)n;}
int reference_miami_output_process(original_miami_output *s,const original_miami_program *p,
 original_miami_fx_workspace *w,const uint32_t *il,const uint32_t *ir,uint32_t *ol,uint32_t *orr,
 uint32_t n,unsigned fz,const uint32_t *events,uint32_t ne){
 float volume;REQUIRE(original_miami_program_native(p,96,&volume));
 if(!ne)REQUIRE(original_miami_fx_process(&s->fx,w,il,ir,ol,orr,n,fz));
 else{
  /* The existing chain has no event argument. Keep one full wrapper
   * invocation and supply the real adapter's immutable local reset list. */
  for(uint32_t i=0;i<n;i++){
   uint32_t scaled;
   REQUIRE(miami_fx_scaled_rne(miami_fx_cleanup_rne(il[i]),fz,&scaled));w->dryL[i]=miami_fx_fcvtzs(scaled);
   REQUIRE(miami_fx_scaled_rne(miami_fx_cleanup_rne(ir[i]),fz,&scaled));w->dryR[i]=miami_fx_fcvtzs(scaled);
  }
  reference_resets resets={&s->fx.decimator,events,ne};
  REQUIRE(original_fixed_fx_process(&s->fx.fx1,w->dryL,w->dryR,w->scratchL,w->scratchR,w->dryL,w->dryR,n,reference_decimate,&resets));
  REQUIRE(original_fixed_fx_process(&s->fx.fx2,w->dryL,w->dryR,w->scratchL,w->scratchR,w->dryL,w->dryR,n,reference_bypass,NULL));
  for(uint32_t i=0;i<n;i++){ol[i]=miami_fx_output_rne((int16_t)w->dryL[i]);orr[i]=miami_fx_output_rne((int16_t)w->dryR[i]);}
 }
 return original_master_process(&s->master,volume*volume,ol,orr,ol,orr,n);
}

typedef struct {const void *a,*b;size_t bytes;} pair_region;
static original_chicago_backend ca,cb;
/* Compare every byte, allowing only same-offset addresses into the explicitly
 * enumerated disjoint owner/workspace regions. One callback deliberately
 * differs to select the unshortened Chicago reference; no DSP field is masked. */
static void same_region(const void *aa,const void *bb,size_t n,const pair_region *r,
 unsigned nr,size_t callback_offset){
 const unsigned char *a=aa,*b=bb;
 for(size_t i=0;i<n;){
  /* events_bind assigns a local C struct whose trailing padding is not
   * logical state. Compare every named field, not these unspecified bytes. */
  if(aa==&ca&&i==offsetof(original_chicago_backend,events)+offsetof(original_engine_events,status)+sizeof(int)){
   i=offsetof(original_chicago_backend,events)+sizeof(original_engine_events);continue;
  }
  if(i==callback_offset){i+=sizeof(void *);continue;}
  if(i%sizeof(uintptr_t)==0&&n-i>=sizeof(uintptr_t)){
   uintptr_t x,y;memcpy(&x,a+i,sizeof x);memcpy(&y,b+i,sizeof y);
   if(x!=y){
    unsigned mapped=0;
    for(unsigned k=0;k<nr;k++)if(x>=(uintptr_t)r[k].a&&x-(uintptr_t)r[k].a<r[k].bytes&&
       y>=(uintptr_t)r[k].b&&y-(uintptr_t)r[k].b==x-(uintptr_t)r[k].a){mapped=1;break;}
    if(mapped){i+=sizeof(uintptr_t);continue;}
   }
  }
  if(a[i]!=b[i]){fprintf(stderr,"state mismatch byte %zu: %u != %u\n",i,a[i],b[i]);REQUIRE(a[i]==b[i]);}
  i++;
 }
 idle_checks++;
}

static int (*compact_zero)(void *,uint32_t,int16_t [2]);
static unsigned chi_attempts,chi_hits;
static int count_chicago_zero(void *p,uint32_t n,int16_t constant[2]){
 chi_attempts++;int ok=compact_zero(p,n,constant);chi_hits+=(unsigned)ok;return ok;
}
static original_chicago_fx2_compact fa,fb;
static audio_workspace cwa,cwb;
static original_chicago_tuning tuning;
static void chicago_equal(unsigned n){
 pair_region r[]={{&ca,&cb,sizeof ca},{&fa,&fb,sizeof fa},{&cwa,&cwb,sizeof cwa}};
 const size_t callback=offsetof(original_chicago_backend,output)+offsetof(original_chicago_engine_output,fx2)+
  offsetof(original_chicago_fx2_wrapper,cold_owner)+offsetof(original_chicago_fx2_cold_owner,advance_zero);
 same_region(&ca,&cb,sizeof ca,r,3,callback);
 same_region(&fa,&fb,sizeof fa,r,3,SIZE_MAX);
 same_region(&cwa,&cwb,sizeof cwa,r,3,SIZE_MAX);
 hash_bytes(&idle_pcm,cwa.out[0],n*sizeof(float));hash_bytes(&idle_pcm,cwa.out[1],n*sizeof(float));
}
static void chicago_block(unsigned n,const original_engine_event *e,unsigned ne){
 proof_side=0;
 REQUIRE(original_chicago_backend_render(&ca,&cwa.workspace,e,ne,cwa.out[0],cwa.out[1],n)==OK);
 proof_side=1;
 REQUIRE(original_chicago_backend_render(&cb,&cwb.workspace,e,ne,cwb.out[0],cwb.out[1],n)==OK);
 proof_side=-1;
 chicago_equal(n);
}
static void chicago_note(unsigned key,unsigned velocity){
 REQUIRE(original_chicago_backend_prepare(&ca)==OK&&original_chicago_backend_prepare(&cb)==OK);
 REQUIRE(original_chicago_backend_note(&ca,key,velocity)==OK&&original_chicago_backend_note(&cb,key,velocity)==OK);
}
static void chicago_set(unsigned p,float v){
 REQUIRE(original_chicago_backend_set(&ca,p,v)==OK&&original_chicago_backend_set(&cb,p,v)==OK);
}
static void chicago_output_zeros(unsigned n,unsigned fz){
 for(unsigned i=0;i<n;i++){
  cwa.core[0][i]=cwb.core[0][i]=(i&1)?UINT32_C(0x80000000):0;
  cwa.core[1][i]=cwb.core[1][i]=(i&1)?0:UINT32_C(0x80000000);
 }
 original_chicago_output_workspace aw={cwa.converted[0],cwa.converted[1],cwa.scratch[0],cwa.scratch[1],256};
 original_chicago_output_workspace bw={cwb.converted[0],cwb.converted[1],cwb.scratch[0],cwb.scratch[1],256};
 original_chicago_fx2_workspace af={cwa.scratch[0],cwa.scratch[1],cwa.scratch[2],cwa.scratch[3],256};
 original_chicago_fx2_workspace bf={cwb.scratch[0],cwb.scratch[1],cwb.scratch[2],cwb.scratch[3],256};
 REQUIRE(original_chicago_engine_output_process(&ca.output,&aw,&af,&ca.controls.routing,cwa.core[0],cwa.core[1],cwa.core[0],cwa.core[1],n,fz));
 REQUIRE(original_chicago_engine_output_process(&cb.output,&bw,&bf,&cb.controls.routing,cwb.core[0],cwb.core[1],cwb.core[0],cwb.core[1],n,fz));
 for(unsigned ch=0;ch<2;ch++){
  memcpy(cwa.out[ch],cwa.core[ch],n*sizeof(float));memcpy(cwb.out[ch],cwb.core[ch],n*sizeof(float));
 }
 chicago_equal(n);
}
static void chicago_refusals(unsigned type){
 original_chicago_fx2_wrapper saved=ca.output.fx2;
 static original_chicago_fx2_compact history;
 memcpy(&history,&fa,sizeof fa);
 for(unsigned k=0;k<7;k++){
  memcpy(&ca.output.fx2,&saved,sizeof saved);
  uint32_t n=256;
  if(k==0)ca.output.fx2.type_dirty=1;
  if(k==1)ca.output.fx2.switch_dirty=1;
  if(k==2)ca.output.fx2.transitioning=1;
  if(k==3)ca.output.fx2.activity_counter=43;
  if(k==4)ca.output.fx2.wet_accumulator=ca.output.fx2.wet_target?0:32768;
  if(k==5)n=0;
  if(k==6)n=4097;
  original_chicago_fx2_wrapper before=ca.output.fx2;
  int16_t raw[2]={123,-456},mixed[2]={789,-321};
  REQUIRE(!original_chicago_fx2_wrapper_advance_zero(&ca.output.fx2,n,raw,mixed));
  REQUIRE(!memcmp(&before,&ca.output.fx2,sizeof before)&&!memcmp(&history,&fa,sizeof fa));
  REQUIRE(raw[0]==123&&raw[1]==-456&&mixed[0]==789&&mixed[1]==-321);
 }
 memcpy(&ca.output.fx2,&saved,sizeof saved);
 if(type){
  switch(type){
   case 4:fa.history[0]=1;break;
   case 5:fa.state.phaser.history[0][0]=1;break;
   case 10:fa.state.ring.frequency_acc^=32768;break;
   case 18:fa.state.mastercomp.highpass[0]=1;break;
   case 22:fa.state.decimator.adapter.kernel.held_left=1;break;
   case 24:fa.state.peaking.history[0][2]=512;break;
  }
  static original_chicago_fx2_compact changed;memcpy(&changed,&fa,sizeof fa);
  int16_t raw[2]={123,-456},mixed[2]={789,-321};
  REQUIRE(!original_chicago_fx2_wrapper_advance_zero(&ca.output.fx2,256,raw,mixed));
  REQUIRE(!memcmp(&changed,&fa,sizeof fa)&&!memcmp(&saved,&ca.output.fx2,sizeof saved));
  REQUIRE(raw[0]==123&&raw[1]==-456&&mixed[0]==789&&mixed[1]==-321);
 }
 memcpy(&fa,&history,sizeof fa);
 /* Every unconsumed record blocks admission, including a non-reset event
  * outside this short invocation. Bypass deliberately consumes none. */
 static const original_chicago_drive_event events[]={{0,8},{63,4},{64,8},{65,4}};
 REQUIRE(original_chicago_fx2_compact_events(&fa,events,4)&&original_chicago_fx2_compact_events(&fb,events,4));
 if(type){
  static original_chicago_fx2_compact before;memcpy(&before,&fa,sizeof fa);
  int16_t raw[2]={123,-456},mixed[2]={789,-321};
  REQUIRE(!original_chicago_fx2_wrapper_advance_zero(&ca.output.fx2,1,raw,mixed));
  REQUIRE(!memcmp(&before,&fa,sizeof fa)&&raw[0]==123&&mixed[1]==-321);
 }
 chicago_block(63,NULL,0);chicago_block(1,NULL,0);chicago_block(64,NULL,0);chicago_block(65,NULL,0);chicago_block(66,NULL,0);
 REQUIRE(fa.queue.cursor==(type?4u:0u));
 REQUIRE(original_chicago_fx2_compact_events(&fa,NULL,0)&&original_chicago_fx2_compact_events(&fb,NULL,0));
 /* The same invocation-local reset records exercise Drive independently. */
 REQUIRE(original_chicago_drive_wrapper_events(&ca.output.output.fx1,events,4)&&original_chicago_drive_wrapper_events(&cb.output.output.fx1,events,4));
 chicago_block(66,NULL,0);
 REQUIRE(original_chicago_drive_wrapper_events(&ca.output.output.fx1,NULL,0)&&original_chicago_drive_wrapper_events(&cb.output.output.fx1,NULL,0));
 for(unsigned slot=0;slot<2;slot++){
  REQUIRE(original_chicago_output_suspend(&ca.output.output,slot,1,44100)&&original_chicago_output_suspend(&cb.output.output,slot,1,44100));
  /* A reachable late countdown snapshot; disabled slots must retain it. */
  ca.output.output.reset_countdown[slot]=cb.output.output.reset_countdown[slot]=63;
  REQUIRE(original_chicago_output_enable(&ca.output.output,slot,0)&&original_chicago_output_enable(&cb.output.output,slot,0));
  chicago_block(65,NULL,0);REQUIRE(ca.output.output.reset_countdown[slot]==63);
  REQUIRE(original_chicago_output_enable(&ca.output.output,slot,1)&&original_chicago_output_enable(&cb.output.output,slot,1));
  chicago_block(63,NULL,0);REQUIRE(ca.output.output.reset_countdown[slot]==-1);
  REQUIRE(original_chicago_output_suspend(&ca.output.output,slot,0,44100)&&original_chicago_output_suspend(&cb.output.output,slot,0,44100));
  chicago_block(65,NULL,0);
 }
 /* Activity's44-frame edge remains owned by the normal wrapper. */
 ca.output.fx2.activity_counter=cb.output.fx2.activity_counter=43;
 ca.output.fx2.active->activity(ca.output.fx2.active->context,1);
 cb.output.fx2.active->activity(cb.output.fx2.active->context,1);
 chicago_block(1,NULL,0);REQUIRE(ca.output.fx2.activity_counter==-1);
 static const unsigned sizes[]={0,1,31,63,64,65,255,256};
 for(unsigned fz=0;fz<2;fz++)for(unsigned k=0;k<sizeof sizes/sizeof sizes[0];k++)chicago_output_zeros(sizes[k],fz);
 original_chicago_output_mute(&ca.output.output,1);original_chicago_output_mute(&cb.output.output,1);
 chicago_output_zeros(63,0);
 original_chicago_output_mute(&ca.output.output,0);original_chicago_output_mute(&cb.output.output,0);
 chicago_output_zeros(65,1);
 printf("PASS Chicago boundaries type%u\n",type);
}
static void chicago_arp_wake(void){
 chicago_set(13,1);chicago_set(14,0);chicago_set(15,0);chicago_set(16,1);
 REQUIRE(ca.controls.routing.arp_enable);
 chicago_note(60,97);chicago_note(64,101);chicago_note(67,113);
 unsigned seen=0;
 for(unsigned k=0;k<256;k++){
  chicago_block(256,NULL,0);
  if(ca.voice.active&&ca.voice.raw_note>=0&&ca.voice.raw_note<128)seen|=1u<<((unsigned)ca.voice.raw_note&31u);
 }
 REQUIRE(seen&&(seen&(seen-1)));
 chicago_note(60,0);chicago_note(64,0);chicago_note(67,0);chicago_set(13,0);
 for(unsigned k=0;k<32;k++)chicago_block(256,NULL,0);
}
static void chicago_actual(void){
 static const unsigned sizes[]={1,63,64,65,255,256};
 for(unsigned t=0;t<7;t++){
  memset(&ca,0,sizeof ca);memset(&cb,0,sizeof cb);memset(&cwa,0,sizeof cwa);memset(&cwb,0,sizeof cwb);
  REQUIRE(original_chicago_fx2_compact_init(&fa)&&original_chicago_fx2_compact_init(&fb));
  original_chicago_fx2_cold_owner oa=original_chicago_fx2_compact_owner(&fa),ob=original_chicago_fx2_compact_owner(&fb);
  compact_zero=oa.advance_zero;oa.advance_zero=count_chicago_zero;ob.advance_zero=NULL;
  REQUIRE(original_chicago_tuning_bind(&tuning,0,0,NULL,0x6d70));workspace_init(&cwa);workspace_init(&cwb);
  REQUIRE(original_chicago_backend_init_compact(&ca,0,120,0,&tuning,&oa)==OK);
  REQUIRE(original_chicago_backend_init_compact(&cb,0,120,0,&tuning,&ob)==OK);
  chicago_set(17,(float)macro_indices[t]/11.0f);
  chi_attempts=chi_hits=0;
  memset(work,0,sizeof work);
  for(unsigned k=0;k<128;k++)chicago_block(256,NULL,0);
  unsigned cold=chi_hits;REQUIRE(cold>0);
  work_report("Chicago",types[t],"cold");
  chicago_note(60,127);
  for(unsigned k=0;k<24;k++)chicago_block(sizes[k%6],NULL,0);
  chicago_note(60,0);unsigned before=chi_hits,release_blocks=0;
  for(;release_blocks<4096;release_blocks++){
   chicago_block(256,NULL,0);
   if(!original_chicago_backend_active(&ca)&&chi_hits>before)break;
  }
  fprintf(stderr,"Chicago type%u cold_hits%u post_hits%u release_blocks%u active%d\n",types[t],cold,chi_hits-before,release_blocks,original_chicago_backend_active(&ca));
  REQUIRE(release_blocks<4096);REQUIRE(chi_hits>before);
  memset(work,0,sizeof work);
  for(unsigned k=0;k<32;k++)chicago_block(256,NULL,0);
  REQUIRE(!work[0][WORK_DRIVE].frames&&!work[0][WORK_FX2].frames);
  REQUIRE(work[1][WORK_DRIVE].frames==8192&&work[1][WORK_FX2].frames==8192);
  work_report("Chicago",types[t],"settled");
  if(types[t]==24){
   REQUIRE(fa.state.peaking.history[0][0]==-543&&fa.state.peaking.history[1][0]==-543);
   REQUIRE(bits(cwa.out[0][0])==UINT32_C(0xb86de833)&&bits(cwa.out[1][0])==UINT32_C(0xb86de833));
  }
  chicago_refusals(types[t]);
  chicago_arp_wake();
  /* Wake after actual played/released state. */
  chicago_note(67,111);
  for(unsigned k=0;k<12;k++)chicago_block(sizes[k%6],NULL,0);
  chicago_note(67,0);
  chicago_set(18,.73123455f);chicago_set(19,.18234569f);
  REQUIRE(original_chicago_backend_tempo(&ca,217.5f)==OK&&original_chicago_backend_tempo(&cb,217.5f)==OK);
  original_engine_event events[]={{0,ORIGINAL_EVENT_NOTE,64,101},{63,ORIGINAL_EVENT_NOTE,64,0}};
  chicago_block(64,events,2);
  REQUIRE(original_chicago_backend_damper(&ca,127)==OK&&original_chicago_backend_damper(&cb,127)==OK);
  chicago_note(72,99);chicago_block(65,NULL,0);chicago_note(72,0);chicago_block(255,NULL,0);
  REQUIRE(original_chicago_backend_damper(&ca,0)==OK&&original_chicago_backend_damper(&cb,0)==OK);
  for(unsigned k=0;k<64;k++)chicago_block(256,NULL,0);
  REQUIRE(original_chicago_backend_reset(&ca,0)==OK&&original_chicago_backend_reset(&cb,0)==OK);
  chicago_set(17,(float)macro_indices[(t+1)%7]/11.0f);
  for(unsigned k=0;k<64;k++)chicago_block(256,NULL,0);
  printf("PASS Chicago type%u cold_hits%u post_hits%u\n",types[t],cold,chi_hits-before);
 }
}

static original_miami_backend ma,mb;
static uint32_t ml[256],mr[256],rl[256],rr[256];
static int32_t md[2][256],rd[2][256];
static int16_t ms[2][256],rs[2][256];
static original_miami_fx_workspace mw={md[0],md[1],ms[0],ms[1],256},rw={rd[0],rd[1],rs[0],rs[1],256};
static void miami_equal(unsigned n){
 pair_region regions[]={{&ma,&mb,sizeof ma},
  {ma.tables.filter.a,mb.tables.filter.a,129*sizeof(float)},
  {ma.tables.filter.b,mb.tables.filter.b,129*sizeof(float)},
  {ma.tables.filter.r,mb.tables.filter.r,129*sizeof(float)},
  {ma.tables.filter.cap,mb.tables.filter.cap,129*sizeof(float)},
  {ma.tables.pitch,mb.tables.pitch,128*sizeof(int32_t)}};
 /* The separately compiled backend owns duplicate immutable flash arrays.
  * Compare their complete bytes, then normalize only same-offset pointers. */
 for(unsigned k=1;k<sizeof regions/sizeof regions[0];k++)REQUIRE(!memcmp(regions[k].a,regions[k].b,regions[k].bytes));
 same_region(&ma,&mb,sizeof ma,regions,6,SIZE_MAX);
 REQUIRE(!memcmp(ml,rl,sizeof ml)&&!memcmp(mr,rr,sizeof mr));
 REQUIRE(!memcmp(md,rd,sizeof md)&&!memcmp(ms,rs,sizeof ms));
 hash_bytes(&idle_pcm,ml,n*sizeof *ml);hash_bytes(&idle_pcm,mr,n*sizeof *mr);
}
static void miami_block(unsigned n,const original_engine_event *e,unsigned ne){
 proof_side=0;
 miami_probe=1;REQUIRE(original_miami_backend_render_event_words(&ma,&mw,ml,mr,n,e,ne));miami_probe=0;
 proof_side=1;
 REQUIRE(reference_miami_backend_render_event_words(&mb,&rw,rl,rr,n,e,ne));
 proof_side=-1;miami_equal(n);
}
static void miami_note(unsigned key,unsigned velocity){
 REQUIRE(original_miami_backend_prepare(&ma)&&reference_miami_backend_prepare(&mb));
 if(velocity){REQUIRE(original_miami_backend_note_on(&ma,key,velocity)&&reference_miami_backend_note_on(&mb,key,velocity));}
 else{REQUIRE(original_miami_backend_note_off(&ma,key)&&reference_miami_backend_note_off(&mb,key));}
}
static void miami_output_zeros(unsigned n,unsigned fz,const uint32_t *events,unsigned ne){
 for(unsigned i=0;i<n;i++){
  ml[i]=rl[i]=(i&1)?UINT32_C(0x80000000):0;
  mr[i]=rr[i]=(i&1)?0:UINT32_C(0x80000000);
 }
 REQUIRE(original_miami_output_process(&ma.output,&ma.program,&mw,ml,mr,ml,mr,n,fz,events,ne));
 REQUIRE(reference_miami_output_process(&mb.output,&mb.program,&rw,rl,rr,rl,rr,n,fz,events,ne));
 miami_equal(n);
}
static void miami_boundaries(void){
 static const unsigned sizes[]={0,1,31,63,64,65,255,256};
 for(unsigned fz=0;fz<2;fz++)for(unsigned k=0;k<sizeof sizes/sizeof sizes[0];k++)miami_output_zeros(sizes[k],fz,NULL,0);
 original_fixed_fx_internal_enable(&ma.output.fx.fx1,0);original_fixed_fx_internal_enable(&mb.output.fx.fx1,0);
 miami_output_zeros(1,0,NULL,0);
 original_fixed_fx_internal_enable(&ma.output.fx.fx1,1);original_fixed_fx_internal_enable(&mb.output.fx.fx1,1);
 miami_output_zeros(63,1,NULL,0);miami_output_zeros(65,0,NULL,0);
 REQUIRE(miami_decimator_stage_step(&ma.output.fx.decimator.kernel,65535)&&miami_decimator_stage_step(&mb.output.fx.decimator.kernel,65535));
 miami_output_zeros(1,1,NULL,0);miami_output_zeros(65,0,NULL,0);
 ma.output.fx.decimator.kernel.held_left=mb.output.fx.decimator.kernel.held_left=31;
 ma.output.fx.decimator.kernel.held_right=mb.output.fx.decimator.kernel.held_right=-29;
 miami_output_zeros(1,0,NULL,0);
 static const uint32_t resets[]={0,63,64};
 miami_output_zeros(65,1,resets,3);
 original_master_mute(&ma.output.master,1);original_master_mute(&mb.output.master,1);miami_output_zeros(63,0,NULL,0);
 original_master_mute(&ma.output.master,0);original_master_mute(&mb.output.master,0);miami_output_zeros(65,1,NULL,0);
 printf("PASS Miami output boundaries\n");
}
static void miami_actual(void){
 static const unsigned sizes[]={1,63,64,65,255,256};
 for(unsigned factory=0;factory<30;factory+=7){
  memset(&ma,0,sizeof ma);memset(&mb,0,sizeof mb);
  memset(ml,0,sizeof ml);memset(mr,0,sizeof mr);memset(rl,0,sizeof rl);memset(rr,0,sizeof rr);
  memset(md,0,sizeof md);memset(rd,0,sizeof rd);memset(ms,0,sizeof ms);memset(rs,0,sizeof rs);
  REQUIRE(original_miami_backend_init(&ma,factory,120)&&reference_miami_backend_init(&mb,factory,120));
  phase_hits=phase_attempts=0;
  memset(work,0,sizeof work);
  for(unsigned k=0;k<128;k++)miami_block(256,NULL,0);
  unsigned cold=phase_hits;REQUIRE(cold>0);
  work_report("Miami",factory,"cold");
  miami_note(60,127);for(unsigned k=0;k<24;k++)miami_block(sizes[k%6],NULL,0);
  miami_note(60,0);unsigned before=phase_hits,release_blocks=0;
  for(;release_blocks<4096;release_blocks++){
   miami_block(256,NULL,0);if(!original_miami_backend_active(&ma)&&phase_hits>before)break;
  }
  REQUIRE(release_blocks<4096&&phase_hits>before);
  memset(work,0,sizeof work);
  for(unsigned k=0;k<32;k++)miami_block(256,NULL,0);
  REQUIRE(!work[0][WORK_FIXED].frames&&!work[0][WORK_DECIMATOR].frames);
  REQUIRE(work[1][WORK_FIXED].frames==16384&&work[1][WORK_DECIMATOR].frames==8192);
  work_report("Miami",factory,"settled");
  miami_boundaries();
  miami_note(67,111);for(unsigned k=0;k<12;k++)miami_block(sizes[k%6],NULL,0);miami_note(67,0);
  REQUIRE(original_miami_backend_set_macro(&ma,10,.73123455f)&&reference_miami_backend_set_macro(&mb,10,.73123455f));
  REQUIRE(original_miami_backend_set_tempo(&ma,217.5f)&&reference_miami_backend_set_tempo(&mb,217.5f));
  original_engine_event e[]={{0,ORIGINAL_EVENT_NOTE,64,101},{63,ORIGINAL_EVENT_NOTE,64,0}};miami_block(64,e,2);
  REQUIRE(original_miami_backend_damper(&ma,127)&&reference_miami_backend_damper(&mb,127));
  miami_note(72,99);miami_block(65,NULL,0);miami_note(72,0);miami_block(255,NULL,0);
  REQUIRE(original_miami_backend_damper(&ma,0)&&reference_miami_backend_damper(&mb,0));
  for(unsigned k=0;k<64;k++)miami_block(256,NULL,0);
  REQUIRE(original_miami_backend_panic(&ma)&&reference_miami_backend_panic(&mb));
  for(unsigned k=0;k<64;k++)miami_block(256,NULL,0);
  REQUIRE(original_miami_backend_load_factory(&ma,(factory+1)%30)&&reference_miami_backend_load_factory(&mb,(factory+1)%30));
  miami_note(55,91);for(unsigned k=0;k<12;k++)miami_block(sizes[k%6],NULL,0);miami_note(55,0);
  printf("PASS Miami factory%u cold_hits%u post_hits%u\n",factory,cold,phase_hits-before);
 }
}

static void decimator_zero_contract(void){
 static const unsigned phases[]={0,1,32767,65534,65535},steps[]={0,1,32768,65535};
 static const unsigned sizes[]={0,1,63,64,65,255,256,4096};
 for(unsigned p=0;p<sizeof phases/sizeof phases[0];p++)for(unsigned q=0;q<sizeof steps/sizeof steps[0];q++)
 for(unsigned k=0;k<sizeof sizes/sizeof sizes[0];k++)for(unsigned pending=0;pending<2;pending++){
  miami_decimator_component_t a,b;REQUIRE(miami_decimator_seed(&a,phases[p],0,0,steps[q]));b=a;
  if(pending){REQUIRE(miami_decimator_stage_step(&a,steps[(q+1)%4]));REQUIRE(miami_decimator_stage_step(&b,steps[(q+1)%4]));}
  REQUIRE(miami_decimator_advance_zero(&a,sizes[k]));
  for(unsigned i=0;i<sizes[k];i++){int16_t sample[2];REQUIRE(miami_decimator_frame(&b,i,0,0,32767,sample));REQUIRE(!sample[0]&&!sample[1]);}
  REQUIRE(!memcmp(&a,&b,sizeof a));
 }
 miami_decimator_component_t a,b;
 REQUIRE(miami_decimator_seed(&a,65535,1,-1,65535));b=a;
 REQUIRE(!miami_decimator_advance_zero(&a,256)&&!memcmp(&a,&b,sizeof a));
 printf("PASS Decimator zero-span local64/phase/pending/refusal\n");
}
int main(void){
 decimator_zero_contract();
 chicago_actual();miami_actual();
 printf("PASS exact idle PCM/state: %u checks PCM %016llx\n",idle_checks,(unsigned long long)idle_pcm);
 return 0;
}
