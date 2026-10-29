/* SPDX-License-Identifier: GPL-3.0-only */
#include "engine_events.h"
#include "program_envelope.h"
#include "miami_voice_snapshot.h"
#include "chicago_key_admission.h"
#include "motion_tables_private.h"
#include "binary32_math.h"
#include "miami_geometry.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint64_t hash=UINT64_C(1469598103934665603);static unsigned frames,snapshots,profiles,audible,quiet;
static void word(uint32_t x){for(unsigned j=0;j<4;j++){hash=(hash^(uint8_t)x)*UINT64_C(1099511628211);x>>=8;}}
static void run(unsigned factory,unsigned profile,const original_miami_voice_tables *tables){
 unsigned note_mode=profile%4,arp=(profile>>2)&1,key_sync=(profile+factory)%4;
 original_miami_program p,decoded;original_miami_voice_controls c;original_miami_output output;
 assert(original_miami_program_init(&p,factory)&&original_miami_voice_compile(&c,&p));
 unsigned char saved[ORIGINAL_MIAMI_PROGRAM_ENVELOPE_BYTES];assert(original_miami_program_encode(&p,saved,sizeof saved));assert(original_miami_program_decode(&decoded,saved,sizeof saved)&&!memcmp(&p,&decoded,sizeof p));
 unsigned char legacy[ORIGINAL_MIAMI_PROGRAM_ENVELOPE_BYTES]={0};decoded=p;assert(!original_miami_program_decode(&decoded,legacy,sizeof legacy)&&!memcmp(&p,&decoded,sizeof p));
 chicago_arp_control keys;memset(&keys,0,sizeof keys);original_held_record held[128];original_voice_record records[2];original_miami_allocator a;assert(original_chicago_allocator_bind(&a,&keys,held,records,2,2,2,note_mode,(profile>>4)&1,(profile>>3)&1));a.mg1_key_sync=(key_sync>>1)&1;a.mg2_key_sync=key_sync&1;a.arp_mode=(profile>>3)&3;a.arp_range=1;keys.committed_enable=arp;a.scan_index=-1;a.previous_index=60;a.octave=1;
 original_miami_selected_voice v[2];for(unsigned i=0;i<2;i++)assert(original_miami_selected_voice_init(v+i,tables,i,60));
 original_miami_pending_note pending[2];original_miami_extra_sources extra[2];memset(pending,0,sizeof pending);memset(extra,0,sizeof extra);
 original_chicago_global_mg global;memset(&global,0,sizeof global);global.mg1.phase=global.mg2.phase=-1;global.mg1.random=0x12346d70;global.mg2.random=0xabcd6d70;
 float rate1,rate2,key1,key2,tempo1,tempo2;assert(original_miami_program_native(&p,44,&rate1)&&original_miami_program_native(&p,50,&rate2));assert(original_miami_program_native(&p,45,&key1)&&original_miami_program_native(&p,51,&key2)&&original_miami_program_native(&p,46,&tempo1)&&original_miami_program_native(&p,52,&tempo2));assert(key1==1&&key2==1&&tempo1==1&&tempo2==1);
 original_chicago_global_mg_input gi={44100,44100,rate1,rate2,c.mg_shape[0],c.mg_shape[1],1,1};chicago_arp_clock clock1={0,10000,0},clock2={0,10000,0};
 original_miami_note_context context;memset(&context,0,sizeof context);original_miami_note_producers producers=original_miami_note_context_callbacks(&context);original_miami_voice_owner owner;
 assert(original_miami_voice_owner_bind(&owner,v,pending,extra,2,&a,&c,tables,&producers,137));
#ifdef MIAMI_CLOCKED_BASELINE
 if(!original_miami_note_context_bind(&context,&owner,&global,&gi,&clock1,&clock2,0,0,a.mg1_key_sync,a.mg2_key_sync)){fprintf(stderr,"EXPECTED legacy selected binder rejects existing ARPon profile%u factory%u; no original audio bug inferred\n",profile,factory);exit(42);}
#else
 assert(original_miami_note_context_bind_clocked(&context,&owner,&global,&gi,&clock1,&clock2,0,0,a.mg1_key_sync,a.mg2_key_sync));
 /* All failed admissions preserve the current descriptor. */
 original_miami_note_context old_context=context;
 a.arp_range=2;assert(!original_miami_note_context_bind_clocked(&context,&owner,&global,&gi,&clock1,&clock2,0,0,0,0)&&!memcmp(&context,&old_context,sizeof context));a.arp_range=1;
 keys.physical_count=1;assert(!original_miami_note_context_bind_clocked(&context,&owner,&global,&gi,&clock1,&clock2,0,0,0,0)&&!memcmp(&context,&old_context,sizeof context));keys.physical_count=0;
 assert(!original_miami_note_context_bind_clocked(&context,&owner,&global,&gi,&clock1,&clock2,1,0,0,0)&&!memcmp(&context,&old_context,sizeof context));
 gi.sample_rate_integer=48000;assert(!original_miami_note_context_bind_clocked(&context,&owner,&global,&gi,&clock1,&clock2,0,0,0,0)&&!memcmp(&context,&old_context,sizeof context));gi.sample_rate_integer=44100;
 gi.tempo_sync2=0;assert(!original_miami_note_context_bind_clocked(&context,&owner,&global,&gi,&clock1,&clock2,0,0,0,0)&&!memcmp(&context,&old_context,sizeof context));gi.tempo_sync2=1;
 gi.waveform2=4;assert(!original_miami_note_context_bind_clocked(&context,&owner,&global,&gi,&clock1,&clock2,0,0,0,0)&&!memcmp(&context,&old_context,sizeof context));gi.waveform2=c.mg_shape[1];
 a.last_note=128;assert(!original_miami_note_context_bind_clocked(&context,&owner,&global,&gi,&clock1,&clock2,0,0,0,0)&&!memcmp(&context,&old_context,sizeof context));a.last_note=60;
#endif

 original_chicago_selector selector={-1,60,1,0,0x6d70};chicago_arp_clock_input clock_input={137,1};chicago_arp_control_input control_input={arp,(profile>>4)&1,0};original_chicago_scheduler_callbacks cb=original_miami_note_context_scheduler_callbacks();original_chicago_scheduler scheduler;assert(original_chicago_scheduler_bind(&scheduler,&a,&selector,&clock2,&clock_input,&control_input,&cb,&owner));
 original_miami_output_init(&output);assert(original_miami_output_apply(&output,&p,10));original_miami_control_owner control;assert(original_miami_control_owner_bind(&control,&p,&c,&output,&scheduler,&gi));
 int32_t dry[2][512];int16_t scratch[2][512];original_miami_fx_workspace workspace={dry[0],dry[1],scratch[0],scratch[1],512};uint32_t left[512],right[512];
 /* Exercise exact restore->geometry/list/direct-core reset before ingress.
  * Restored chord payload is not an instruction to spawn or transpose. */
 uint32_t by_key[128];original_allocator_chord pairs[128];original_miami_geometry geometry={{2,2,1,2,1},by_key,pairs,0};original_miami_geometry_progress progress;
 unsigned char map[128]={0};map[0]=127;map[60]=100;map[127]=255;
 assert(original_miami_geometry_restore_map(&geometry,&a,map,2,2,original_miami_note_context_geometry_reset,&context,&progress));
 assert(geometry.chord_count==3&&pairs[2].note==127&&pairs[2].velocity==-1&&a.voice_active.count==0);
 original_engine_event events[]={
  {0,ORIGINAL_EVENT_NOTE,0,1},{8,ORIGINAL_EVENT_NOTE,127,127},
  {16,ORIGINAL_EVENT_NOTE,60,100},{16,ORIGINAL_EVENT_NOTE,60,0},
  {24,ORIGINAL_EVENT_DAMPER,0,127},{32,ORIGINAL_EVENT_NOTE,0,0},
  {40,ORIGINAL_EVENT_DAMPER,0,0},{64,ORIGINAL_EVENT_NOTE,127,0},
  {96,ORIGINAL_EVENT_NOTE,60,64},{128,ORIGINAL_EVENT_NOTE,60,0},
  {160,ORIGINAL_EVENT_CLEAR_KEYS,0,0},{192,ORIGINAL_EVENT_ALL_OFF,0,0}};
 original_engine_events queue;assert(original_engine_events_bind(&queue,events,12,512,&control,NULL));
 assert(original_miami_control_owner_block_prepare(&control,&clock1));
 /* Explicit prior clock residuals force tempo crossings as well as key reset.
  * They are restored test inputs, not an added production event. */
 clock1.accumulator=clock1.threshold-137;clock2.accumulator=clock2.threshold-137;
 unsigned nonzero=0;
 for(unsigned frame=0;frame<512;frame++){
  assert(original_engine_events_frame(&queue,frame)&&original_chicago_canonical_keys_valid(&keys));
  assert(original_chicago_scheduler_after_events(&scheduler)&&original_miami_voice_owner_control_all(&owner));
  float l=0,r=0;for(unsigned i=0;i<2;i++){float x,y;assert(original_miami_selected_voice_render_frame(v+i,&c,&x,&y));l=l+x;r=r+y;}
  left[frame]=original_math_bits(l);right[frame]=original_math_bits(r);nonzero+=(l!=0||r!=0);
  if(note_mode==0||note_mode==2)assert(!a.voice_active.count&&!v[0].active&&!v[1].active&&l==0&&r==0);
  if(frame==0||frame==16||frame==64||frame==127||frame==255||frame==511)for(unsigned id=0;id<2;id++){
   original_miami_voice_snapshot snap,decoded_snapshot;unsigned char wire[ORIGINAL_MIAMI_VOICE_SNAPSHOT_BYTES];
   assert(original_miami_voice_snapshot_capture(&snap,&owner,id)&&original_miami_voice_snapshot_encode(&snap,wire,sizeof wire));
   assert(original_miami_voice_snapshot_decode(&decoded_snapshot,wire,sizeof wire)&&!memcmp(&snap,&decoded_snapshot,sizeof snap));snapshots++;
  }
 }
 assert(queue.cursor==12&&owner.status&&context.status&&control.status);
 if(note_mode==0||note_mode==2){assert(!nonzero);quiet++;}else if(nonzero)audible++;
 assert(original_miami_output_process(&output,&p,&workspace,left,right,left,right,512,0,NULL,0));
 for(unsigned i=0;i<512;i++){word(left[i]);word(right[i]);}frames+=512;profiles++;
 /* An explicitly preexisting record/tail is not erased by a mode0/2 event.
  * No test-only producer is installed in production dispatch. */
 if((note_mode==0||note_mode==2)&&!arp){
  original_chicago_allocator_spawn(&a,60,100,original_miami_voice_owner_callbacks(),&owner);assert(owner.status&&a.voice_active.count==2);
  original_record_list owned=a.voice_active;uint8_t state0=v[0].vca.state,state1=v[1].vca.state;
  assert(original_chicago_scheduler_note(&scheduler,60,0)&&!memcmp(&owned,&a.voice_active,sizeof owned)&&state0==v[0].vca.state&&state1==v[1].vca.state);
  assert(original_chicago_scheduler_note(&scheduler,72,100)&&!memcmp(&owned,&a.voice_active,sizeof owned)&&state0==v[0].vca.state&&state1==v[1].vca.state);
 }
}
int main(void){
 const original_ms20_tables f={original_miami_a44100,original_miami_b44100,original_miami_r,original_miami_cap};original_miami_voice_tables t;assert(original_miami_voice_tables_bind_immutable(&t,&f,original_pitch44100));
 for(unsigned i=0;i<30;i++)for(unsigned j=0;j<32;j++)run(i,j,&t);
 assert(profiles==960&&quiet==480&&audible>0);
 printf("PASS %u factory/profile clocked Miami graphs;canonical modes0/2 %u quiet contexts,no invented chord spawn; modes1/3 %u audible contexts;%u stereo frames hash%016llx;%u unchanged130-field snapshot roundtrips;ARPoff/on range1,selector0..3,key-sync0/1,hold/pedal/legato,boundarykeys0/127,duplicate/NoteOff and direct geometry reset;PENDING_REVIEW\n",profiles,quiet,audible,frames,(unsigned long long)hash,snapshots);return 0;
}
