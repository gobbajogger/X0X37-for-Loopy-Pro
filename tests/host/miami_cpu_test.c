/* SPDX-License-Identifier: GPL-3.0-only */
#define _POSIX_C_SOURCE 200809L
#include "original_miami_backend.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <time.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr,"failed line %d: %s\n",__LINE__,#x); exit(1); } } while (0)
static original_miami_backend engine;
static uint32_t left[256],right[256];
static int32_t dry[2][256];
static int16_t scratch[2][256];
static original_miami_fx_workspace workspace={dry[0],dry[1],scratch[0],scratch[1],256};
static FILE *trace;
static volatile uint32_t sink;
static void record(const void *p,size_t n){if(trace)CHECK(fwrite(p,1,n,trace)==n);}
#define RECORD(x) record(&(x),sizeof(x))
/* Record pointer-free state and every workspace word. The harness checks the
 * unchanged owner wiring separately; no process addresses enter the trace.
 * Zero automatic padding at compilation so unspecified bytes are not proof. */
static void snapshot(unsigned n){
 record(left,n*sizeof *left);record(right,n*sizeof *right);
 RECORD(dry);RECORD(scratch);
 RECORD(engine.program);RECORD(engine.controls);RECORD(engine.output);
 RECORD(engine.keys);RECORD(engine.held);RECORD(engine.records);
 record(&engine.allocator.held_active,sizeof engine.allocator-
     offsetof(original_chicago_allocator,held_active));
 RECORD(engine.voices);RECORD(engine.pending);RECORD(engine.extra);
 RECORD(engine.global);RECORD(engine.global_input);RECORD(engine.clock1);RECORD(engine.clock2);
 record(&engine.note_context.key_sync,sizeof engine.note_context-
     offsetof(original_miami_note_context,key_sync));
 record(&engine.voice_owner.count,sizeof engine.voice_owner-
     offsetof(original_miami_voice_owner,count));
 RECORD(engine.selector);RECORD(engine.clock_input);RECORD(engine.control_input);
 RECORD(engine.scheduler.dispatch_result);RECORD(engine.control_owner.status);
 RECORD(engine.ready);RECORD(engine.status);RECORD(engine.running);RECORD(engine.block_prepared);
 CHECK(engine.allocator.held==engine.held && engine.allocator.voice==engine.records &&
     engine.allocator.keys==&engine.keys && engine.voice_owner.voices==engine.voices &&
     engine.note_context.owner==&engine.voice_owner && engine.scheduler.context==&engine.voice_owner &&
     engine.control_owner.program==&engine.program);
}
static void block(unsigned n,const original_engine_event *e,unsigned count){
 CHECK(original_miami_backend_render_event_words(&engine,&workspace,left,right,n,e,count));
 if(n)sink^=left[n-1]^right[n-1];
 if(trace)snapshot(n);
}
static double seconds(void){struct timespec t;CHECK(!clock_gettime(CLOCK_MONOTONIC,&t));return (double)t.tv_sec+(double)t.tv_nsec*1e-9;}
static void workload(unsigned factory,unsigned blocks,int lifecycle){
 memset(left,0,sizeof left);memset(right,0,sizeof right);
 memset(dry,0,sizeof dry);memset(scratch,0,sizeof scratch);
 CHECK(original_miami_backend_init(&engine,factory,120));
 if(lifecycle){block(0,NULL,0);block(1,NULL,0);block(65,NULL,0);}
 CHECK(original_miami_backend_prepare(&engine));
 CHECK(original_miami_backend_note_on(&engine,60,111));
 for(unsigned k=0;k<blocks;k++){
  if(lifecycle){
   static const unsigned sizes[]={1,31,63,64,65,255,256};
   unsigned n=sizes[k%7];
   original_engine_event e[3];unsigned count=0;
   if(k%17==0){e[count++]=(original_engine_event){0,ORIGINAL_EVENT_NOTE,64,103};}
   if(k%17==1){e[count++]=(original_engine_event){0,ORIGINAL_EVENT_NOTE,64,0};}
   if(k%5==0){float value=(float)(k%9)*.125f;uint32_t bits;memcpy(&bits,&value,4);
    e[count++]=(original_engine_event){n-1,ORIGINAL_EVENT_MACRO,(k/5)%18,bits};}
   if(k%23<=1)e[count++]=(original_engine_event){n-1,ORIGINAL_EVENT_MUTE,0,k%23==0};
   block(n,e,count);
   if(k==20)CHECK(original_miami_backend_damper(&engine,127));
   if(k==30)CHECK(original_miami_backend_damper(&engine,0));
   if(k==40)CHECK(original_miami_backend_set_tempo(&engine,217.5f));
   if(k==50)CHECK(original_miami_backend_load_factory(&engine,(factory+1)%30));
  }else block(256,NULL,0);
 }
 if(lifecycle){
  CHECK(original_miami_backend_note_off(&engine,60));
  CHECK(original_miami_backend_all_off(&engine));
  for(unsigned k=0;k<32;k++)block(256,NULL,0);
  CHECK(original_miami_backend_panic(&engine));block(63,NULL,0);
  CHECK(original_miami_backend_note_on(&engine,0,1));block(64,NULL,0);
  CHECK(original_miami_backend_note_on(&engine,127,127));block(65,NULL,0);
  CHECK(original_miami_backend_transport(&engine,1));
  CHECK(original_miami_backend_transport(&engine,0));block(255,NULL,0);
 }
}
int main(int argc,char **argv){
 CHECK(argc>=2);
 if(!strcmp(argv[1],"proof")){
  CHECK(argc==3);trace=fopen(argv[2],"wb");CHECK(trace);
  for(unsigned factory=0;factory<30;factory++)workload(factory,96,1);
  CHECK(!fclose(trace));puts("PASS Miami all30 PCM/state trace");
 }else{
  unsigned repeats=argc>2?(unsigned)strtoul(argv[2],NULL,10):1;CHECK(repeats);
  for(unsigned factory=0;factory<30;factory++){
   double begin=seconds();
   for(unsigned r=0;r<repeats;r++)workload(factory,128,0);
   double elapsed=seconds()-begin;
   printf("factory %u ns/frame %.3f\n",factory,elapsed*1e9/(repeats*128.0*256.0));
  }
 }
 return 0;
}
