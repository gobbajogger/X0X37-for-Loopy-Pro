/* SPDX-License-Identifier: GPL-3.0-only */
/* Run unchanged sources and candidate as separate programs. The runner byte
 * compares every PCM sample plus pointer-free state snapshots after blocks. */
#include "original_chicago_backend.h"
#include "chicago_fx2_compact.h"
#include "binary32_math.h"
#include "app/original_sound.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static original_chicago_backend backend;
static original_chicago_fx2_compact fx;
static original_chicago_tuning tuning;
static uint32_t core[2][256];
static int16_t converted[2][256], scratch[4][256];
static float pcm[2][256];
static original_chicago_backend saved_backend;
static original_chicago_fx2_compact saved_fx;
static original_chicago_tuning saved_tuning;
static int measuring;
static uint64_t calls, master_calls, frame_calls;
void __attribute__((no_instrument_function)) __cyg_profile_func_enter(void *fn,void *from)
{ (void)from; if(measuring){++calls;if(fn==(void *)original_master_process)++master_calls;if(fn==(void *)original_chicago_backend_frame)++frame_calls;} }
void __attribute__((no_instrument_function)) __cyg_profile_func_exit(void *fn,void *from)
{ (void)fn; (void)from; }
#define CHECK(x) do {if(!(x)){fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);exit(1);}}while(0)
static void put(FILE *f,const void *p,size_t n){CHECK(fwrite(p,1,n,f)==n);}
#define PUT(f,x) put(f,&(x),sizeof(x))
static void state(FILE *f){
 PUT(f,backend.program);PUT(f,backend.controls);PUT(f,backend.voice);
 PUT(f,backend.pending);PUT(f,backend.extra);PUT(f,backend.keys);
 PUT(f,backend.held);PUT(f,backend.record);
 original_chicago_allocator a=backend.allocator;a.held=NULL;a.voice=NULL;a.keys=NULL;PUT(f,a);
 PUT(f,backend.global);PUT(f,backend.global_input);PUT(f,backend.clock1);PUT(f,backend.clock2);
 PUT(f,backend.global_stage.helper_entries);PUT(f,backend.global_stage.full_head_seeds);PUT(f,backend.global_stage.status);
 PUT(f,backend.voice_owner.status);PUT(f,backend.note_context.status);PUT(f,backend.control_owner.status);
 PUT(f,backend.selector);PUT(f,backend.clock_input);PUT(f,backend.control_input);PUT(f,backend.scheduler.dispatch_result);
 PUT(f,backend.output.output);
 original_chicago_fx2_wrapper w=backend.output.fx2;
 memset(&w.owner,0,sizeof w.owner);memset(&w.cold_owner,0,sizeof w.cold_owner);
 unsigned active=w.active!=NULL,old=w.old!=NULL;w.active=w.old=NULL;PUT(f,w);PUT(f,active);PUT(f,old);
 original_chicago_fx2_compact_state s=fx.state;
 if(fx.selected_type==4){s.chorus.left=NULL;s.chorus.right=NULL;}
 PUT(f,s);PUT(f,fx.history);PUT(f,fx.queue.count);PUT(f,fx.queue.cursor);PUT(f,fx.ready);PUT(f,fx.selected_type);PUT(f,fx.selected);
 PUT(f,backend.events.count);PUT(f,backend.events.cursor);PUT(f,backend.events.frames);PUT(f,backend.events.next_frame);PUT(f,backend.events.status);
 PUT(f,backend.ready);PUT(f,backend.block_frames);PUT(f,backend.status);PUT(f,backend.block_open);PUT(f,backend.block_prepared);
}
int main(int argc,char **argv){
 CHECK(argc==2);FILE *f=fopen(argv[1],"wb");CHECK(f);
 original_chicago_backend_workspace w={core[0],core[1],converted[0],converted[1],
     {scratch[0],scratch[1],scratch[2],scratch[3]},256};
 const unsigned factories[]={0,7,19,33,54,0,0,0,0,0,0};
 const unsigned fx_macro[]={0,5,7,4,2,3,1};
 const float tempos[]={120,20,400,123.456f,99.75f};
 uint64_t idle_calls=0,idle_master=0,idle_frames=0;unsigned idle_blocks=0;
 for(unsigned scenario=0;scenario<11;scenario++){
  CHECK(original_chicago_tuning_bind(&tuning,0,0,NULL,0x6d70));
  CHECK(original_chicago_fx2_compact_init(&fx));
  original_chicago_fx2_cold_owner owner=original_chicago_fx2_compact_owner(&fx);
  unsigned raw=(unsigned)original_sound_factory_source(ORIGINAL_SOUND_CHICAGO,factories[scenario]);
  CHECK(original_chicago_backend_init_compact(&backend,raw,tempos[scenario%5],scenario%2,&tuning,&owner)==1);
  if(scenario>=5)CHECK(original_chicago_backend_set(&backend,17,(float)fx_macro[scenario-4]/11)==1);
  unsigned audible=0;
  uint64_t scenario_calls=0;
  for(unsigned b=0;b<2400;b++){
   unsigned n=256;original_engine_event e[4];unsigned ne=0;
   /* >10 seconds of uninterrupted silence before a mid-block note. Then
    * release, allow the tail to retire, and play again after a long gap. */
   if(b==2048){e[ne++]=(original_engine_event){127,ORIGINAL_EVENT_NOTE,48,100};}
   if(b==2072){e[ne++]=(original_engine_event){255,ORIGINAL_EVENT_NOTE,48,0};}
   if(b==2080){e[ne++]=(original_engine_event){0,ORIGINAL_EVENT_ALL_OFF,0,0};}
   if(b==2100){e[ne++]=(original_engine_event){63,ORIGINAL_EVENT_MUTE,0,1};}
   if(b==2110){e[ne++]=(original_engine_event){0,ORIGINAL_EVENT_MUTE,0,0};}
   if(b==2200){e[ne++]=(original_engine_event){255,ORIGINAL_EVENT_MACRO,5,0};}
   if(b==2210){e[ne++]=(original_engine_event){1,ORIGINAL_EVENT_MACRO,5,0x3f800000u};}
   if(b==2300){e[ne++]=(original_engine_event){0,ORIGINAL_EVENT_NOTE,60,110};}
   if(b==2320){e[ne++]=(original_engine_event){63,ORIGINAL_EVENT_NOTE,60,0};}
   if(b>2320)n=(b%5==0)?1:(b%5==1)?63:(b%5==2)?64:(b%5==3)?65:256;
   calls=master_calls=frame_calls=0;measuring=b>=1024&&b<1088;
   CHECK(original_chicago_backend_render(&backend,&w,e,ne,pcm[0],pcm[1],n)==1);
   if(measuring){idle_calls+=calls;scenario_calls+=calls;idle_master+=master_calls;idle_frames+=frame_calls;idle_blocks++;}measuring=0;
   if(b>=2048&&b<2072)for(unsigned i=0;i<n;i++)audible|=(original_math_bits(pcm[0][i])&0x7fffffffu)!=0;
   put(f,pcm[0],n*sizeof(float));put(f,pcm[1],n*sizeof(float));
   /* State is checked throughout silence, each event and after retirement. */
   if(b%64==0||ne||b>=2320)state(f);
  }
  CHECK(audible);
  if(scenario_calls)printf("scenario %u idle entries/block %.2f\n",scenario,(double)scenario_calls/64);
  CHECK(original_chicago_backend_retire(&backend)==1);
  /* Probe every silence length, without replaying its entire prefix. Save
   * and RESTORE at the same addresses so every internal borrow remains valid.
   * Each fork starts from the next untouched idle prefix, plays a fresh note
   * and compares all output samples plus DSP state. No copied graph is run
   * at a different address. Scratch is deliberately reused between forks. */
  CHECK(original_chicago_fx2_compact_init(&fx));
  owner=original_chicago_fx2_compact_owner(&fx);
  CHECK(original_chicago_backend_init_compact(&backend,raw,tempos[scenario%5],scenario%2,&tuning,&owner)==1);
  if(scenario>=5)CHECK(original_chicago_backend_set(&backend,17,(float)fx_macro[scenario-4]/11)==1);
  for(unsigned gap=1;gap<=2000;gap++){
   CHECK(original_chicago_backend_render(&backend,&w,NULL,0,pcm[0],pcm[1],256)==1);
   put(f,pcm,sizeof pcm);state(f);
   saved_backend=backend;saved_fx=fx;saved_tuning=tuning;
   unsigned sound=0;
   for(unsigned block=0;block<4;block++){
    original_engine_event note={gap%256,ORIGINAL_EVENT_NOTE,48,100};
    CHECK(original_chicago_backend_render(&backend,&w,block?NULL:&note,block?0:1,pcm[0],pcm[1],256)==1);
    for(unsigned i=0;i<256;i++)sound|=(original_math_bits(pcm[0][i])&0x7fffffffu)!=0;
    put(f,pcm,sizeof pcm);state(f);
   }
   CHECK(sound);
   backend=saved_backend;fx=saved_fx;tuning=saved_tuning;
  }
  CHECK(original_chicago_backend_retire(&backend)==1);
 }
 CHECK(!fclose(f));
 if(idle_calls){
  printf("idle proxy: %llu instrumented C function entries / %u blocks = %.2f entries per 256-frame block\n",
   (unsigned long long)idle_calls,idle_blocks,(double)idle_calls/idle_blocks);
  printf("idle entries: backend_frame %llu, master_process %llu\n",(unsigned long long)idle_frames,(unsigned long long)idle_master);
 }else puts("uninstrumented PCM/state transcript complete");
 return 0;
}
