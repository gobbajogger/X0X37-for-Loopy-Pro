/* SPDX-License-Identifier: GPL-3.0-only */
#include "engine_events.h"
#include "program_envelope.h"
#include "miami_voice_snapshot.h"
#include "chicago_key_admission.h"
#include "motion_tables_private.h"
#include "binary32_math.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint64_t hash=UINT64_C(1469598103934665603);static unsigned frames,restarts,snapshots;
static void word(uint32_t x){for(unsigned j=0;j<4;j++){hash=(hash^(uint8_t)x)*UINT64_C(1099511628211);x>>=8;}}
static void run(unsigned factory,const original_miami_voice_tables *tables){
 original_miami_program p,decoded;original_miami_voice_controls c;original_miami_output output;
 assert(original_miami_program_init(&p,factory)&&original_miami_voice_compile(&c,&p));
 unsigned char saved[ORIGINAL_MIAMI_PROGRAM_ENVELOPE_BYTES];assert(original_miami_program_encode(&p,saved,sizeof saved));assert(original_miami_program_decode(&decoded,saved,sizeof saved)&&!memcmp(&p,&decoded,sizeof p));
 unsigned char legacy[ORIGINAL_MIAMI_PROGRAM_ENVELOPE_BYTES]={0};decoded=p;assert(!original_miami_program_decode(&decoded,legacy,sizeof legacy)&&!memcmp(&p,&decoded,sizeof p));
 chicago_arp_control keys;memset(&keys,0,sizeof keys);original_held_record held[128];original_voice_record records[2];original_miami_allocator a;assert(original_chicago_allocator_bind(&a,&keys,held,records,2,2,2,1,0,1));a.mg1_key_sync=a.mg2_key_sync=1;a.arp_range=1;a.scan_index=-1;a.previous_index=60;a.octave=1;
 original_miami_selected_voice v[2];for(unsigned i=0;i<2;i++)assert(original_miami_selected_voice_init(v+i,tables,i,60));
 original_miami_pending_note pending[2];original_miami_extra_sources extra[2];memset(pending,0,sizeof pending);memset(extra,0,sizeof extra);
 original_chicago_global_mg global;memset(&global,0,sizeof global);global.mg1.phase=global.mg2.phase=-1;global.mg1.random=0x12346d70;global.mg2.random=0xabcd6d70;
 float rate1,rate2,key1,key2,tempo1,tempo2;assert(original_miami_program_native(&p,44,&rate1)&&original_miami_program_native(&p,50,&rate2));assert(original_miami_program_native(&p,45,&key1)&&original_miami_program_native(&p,51,&key2)&&original_miami_program_native(&p,46,&tempo1)&&original_miami_program_native(&p,52,&tempo2));assert(key1==1&&key2==1&&tempo1==1&&tempo2==1);
 original_chicago_global_mg_input gi={44100,44100,rate1,rate2,c.mg_shape[0],c.mg_shape[1],1,1};chicago_arp_clock clock1={0,10000,0},clock2={0,10000,0};
 original_miami_note_context context;memset(&context,0,sizeof context);original_miami_note_producers producers=original_miami_note_context_callbacks(&context);original_miami_voice_owner owner;
 assert(original_miami_voice_owner_bind(&owner,v,pending,extra,2,&a,&c,tables,&producers,137));assert(original_miami_note_context_bind(&context,&owner,&global,&gi,&clock1,&clock2,0,0,1,1));
 original_chicago_selector selector={-1,60,1,0,0x6d70};chicago_arp_clock_input clock_input={137,1};chicago_arp_control_input control_input={0,0,0};original_chicago_scheduler_callbacks cb=original_miami_note_context_scheduler_callbacks();original_chicago_scheduler scheduler;assert(original_chicago_scheduler_bind(&scheduler,&a,&selector,&clock2,&clock_input,&control_input,&cb,&owner));
 original_miami_output_init(&output);assert(original_miami_output_apply(&output,&p,10));original_miami_control_owner control;assert(original_miami_control_owner_bind(&control,&p,&c,&output,&scheduler,&gi));
 int32_t dry[2][256];int16_t scratch[2][256];original_miami_fx_workspace workspace={dry[0],dry[1],scratch[0],scratch[1],256};uint32_t left[256],right[256];
 for(unsigned block=0;block<36;block++){
  unsigned note=48+block%12;
  original_engine_event events[]={
   {0,ORIGINAL_EVENT_MACRO,block/2,original_math_bits((float)(block&1))},
   {0,ORIGINAL_EVENT_NOTE,note,100},{8,ORIGINAL_EVENT_NOTE,note,0},
   {9,ORIGINAL_EVENT_DAMPER,0,0},{10,ORIGINAL_EVENT_NOTE,note+12,127},
   {190,ORIGINAL_EVENT_NOTE,note+12,0},{210,ORIGINAL_EVENT_CLEAR_KEYS,0,0},
   {220,ORIGINAL_EVENT_ALL_OFF,0,0}};
  original_engine_events queue;assert(original_engine_events_bind(&queue,events,8,256,&control,NULL));
  original_engine_events before=queue;original_engine_event malformed=events[0];malformed.value=0x7fc00000;
  assert(!original_engine_events_bind(&queue,&malformed,1,256,&control,NULL)&&!memcmp(&queue,&before,sizeof queue));
  assert(original_miami_control_owner_block_prepare(&control,&clock1));
  for(unsigned frame=0;frame<256;frame++){
   assert(original_engine_events_frame(&queue,frame));assert(original_chicago_canonical_keys_valid(&keys));
   assert(original_chicago_scheduler_after_events(&scheduler));unsigned old_updates=v[0].control_updates+v[1].control_updates;
   assert(original_miami_voice_owner_control_all(&owner));unsigned updates=v[0].control_updates+v[1].control_updates;if(updates>old_updates+2)restarts++;
   float l=0,r=0;for(unsigned i=0;i<2;i++){float x,y;assert(original_miami_selected_voice_render_frame(v+i,&c,&x,&y));l=l+x;r=r+y;}
   left[frame]=original_math_bits(l);right[frame]=original_math_bits(r);
   for(unsigned i=0;i<2;i++){assert(extra[i].key_unipolar==(float)v[i].note_key*.0078125f||!v[i].active);}
   if(frame==0||frame==8||frame==10||frame==64||frame==128||frame==255)for(unsigned id=0;id<2;id++){
    original_miami_voice_snapshot snapshot,restored;unsigned char packet[ORIGINAL_MIAMI_VOICE_SNAPSHOT_BYTES];
    assert(original_miami_voice_snapshot_capture(&snapshot,&owner,id)&&original_miami_voice_snapshot_encode(&snapshot,packet,sizeof packet));
    assert(original_miami_voice_snapshot_decode(&restored,packet,sizeof packet)&&!memcmp(&snapshot,&restored,sizeof snapshot));
    snapshots++;original_miami_voice_snapshot before=restored;packet[16+frame%100]^=1;
    assert(!original_miami_voice_snapshot_decode(&restored,packet,sizeof packet)&&!memcmp(&before,&restored,sizeof restored));
   }
  }
  assert(queue.cursor==8&&owner.status&&context.status&&control.status);
  assert(original_miami_output_process(&output,&p,&workspace,left,right,left,right,256,0,NULL,0));
  for(unsigned i=0;i<256;i++){word(left[i]);word(right[i]);}frames+=256;
  assert(original_miami_program_encode(&p,saved,sizeof saved)&&original_miami_program_decode(&decoded,saved,sizeof saved)&&!memcmp(&decoded,&p,sizeof p));
 }
 /* Direct negative VCA5 edge pauses before the old tail with BOTH EGs+0.
  * No render is permitted until exact full-mask same-frame continuation. */
 original_miami_selected_voice raw=v[0];raw.active=1;raw.vca.state=5;raw.vca.level=-.25f;raw.counter=44100;raw.awaiting_restart=0;unsigned action;uint32_t phases[4];memcpy(phases,raw.pitch.oscillator.phase,sizeof phases);
 assert(original_miami_selected_voice_control(&raw,&c,tables,137,&action)&&action==2&&raw.awaiting_restart&&!original_math_bits(raw.vca.level)&&!original_math_bits(raw.vcf.level));float l=1,r=2;assert(!original_miami_selected_voice_render_frame(&raw,&c,&l,&r)&&l==1&&r==2);
 original_miami_voice_note n;memset(&n,0,sizeof n);n.note_key=72;n.velocity=0;n.last_note=60;n.tuned_pitch=72;n.unison_flag=2;n.unison_coordinate=.5f;n.mg[0].phase=n.mg[1].phase=-1;
 assert(original_miami_selected_voice_resume_part(&raw,&c,tables,&n,15,1,137)&&!raw.awaiting_restart&&raw.active&&raw.velocity==0&&!memcmp(phases,raw.pitch.oscillator.phase,sizeof phases));
 assert(original_miami_selected_voice_render_frame(&raw,&c,&l,&r));
 /* All actual bitwise pending masks, multiple staged entries, retained mask,
  * ascending parts and active-record ownership through negative state5. */
 for(unsigned mask=1;mask<15;mask++){
  for(unsigned i=0;i<2;i++){v[i].active=0;v[i].vca.state=6;v[i].awaiting_restart=0;}
  original_chicago_allocator_reset_lists(&a);original_chicago_allocator_spawn(&a,60,100,original_miami_voice_owner_callbacks(),&owner);assert(owner.status&&a.voice_active.count==2);
  unsigned parts=0;const original_allocator_callbacks *voice=original_miami_voice_owner_callbacks();
  for(unsigned bit=1,i=0;bit<=8;bit<<=1,i++)if(mask&bit){
   original_voice_record rec=records[0];rec.note=64+(int32_t)i;rec.velocity=0;voice->note(&owner,0,&rec,1,bit,60);parts++;
  }
  assert(pending[0].mask==mask&&v[0].vca.state==5&&owner.status);unsigned helpers=context.helper_entries;
  v[0].vca.level=-.25f;v[0].counter=44100;assert(original_miami_voice_owner_control_all(&owner));
  assert(context.helper_entries==helpers+parts+1&&pending[0].mask==mask&&a.voice_active.count==2&&v[0].active&&!v[0].awaiting_restart);
  for(unsigned i=0;i<4;i++)if(mask&(1u<<i))assert(v[0].pitch.target[i]==(float)(64+i));
 }
 /* Core reset preserves oscillator phase, separate normalized velocity/source
  * arrays, clock residual/threshold and master while clearing raw velocity. */
 original_chicago_allocator_all_notes_off(&a,original_miami_voice_owner_callbacks(),&owner);
 uint32_t old_phases[2][4];float old_velocity[2];original_miami_extra_sources old_extra[2];
 for(unsigned i=0;i<2;i++){memcpy(old_phases[i],v[i].pitch.oscillator.phase,16);old_velocity[i]=v[i].velocity;old_extra[i]=extra[i];}
 chicago_arp_clock old1=clock1,old2=clock2;original_master master=output.master;
 assert(original_miami_note_context_core_reset(&context));
 assert(!memcmp(&master,&output.master,sizeof master)&&clock1.accumulator==old1.accumulator&&clock1.threshold==old1.threshold&&clock2.accumulator==old2.accumulator&&clock2.threshold==old2.threshold&&!clock1.key_reset&&!clock2.key_reset);
 for(unsigned i=0;i<2;i++){
  assert(!memcmp(old_phases[i],v[i].pitch.oscillator.phase,16)&&v[i].velocity==old_velocity[i]&&!memcmp(extra+i,old_extra+i,sizeof *extra)&&!v[i].raw_velocity&&!v[i].active&&v[i].unison_flag==1&&v[i].vca.state==6&&v[i].vcf.state==6&&pending[i].mask==15);
  assert(original_math_bits(v[i].filter.a.current)==0x3a98fbd4u&&v[i].filter.noise==i+1&&v[i].filter.noise_gain.target==0);
  for(unsigned j=0;j<4;j++)assert(v[i].pitch.current[j]==60&&v[i].pitch.target[j]==0);
 }
 original_engine_events queue,saved_queue;memset(&queue,0xa5,sizeof queue);saved_queue=queue;
 keys.physical_count=1;assert(!original_engine_events_bind(&queue,NULL,0,1,&control,NULL)&&!memcmp(&queue,&saved_queue,sizeof queue));keys.physical_count=0;
 owner.status=0;assert(!original_engine_events_bind(&queue,NULL,0,1,&control,NULL)&&!memcmp(&queue,&saved_queue,sizeof queue));owner.status=1;
 assert(original_engine_events_bind(&queue,NULL,0,1,&control,NULL));owner.status=0;assert(!original_engine_events_frame(&queue,0)&&!queue.status&&!queue.next_frame);owner.status=1;
}
int main(void){
 const original_ms20_tables f={original_miami_a44100,original_miami_b44100,original_miami_r,original_miami_cap};original_miami_voice_tables t;assert(original_miami_voice_tables_bind_immutable(&t,&f,original_pitch44100));
 for(unsigned i=0;i<30;i++)run(i,&t);
 printf("PASS actual30-factory/all18 endpoint Miami typed events->allocator->Equal/fullMG->voice/globalMG->Decimator/master graph;%u stereo frames,hash%016llx,observed nested restarts%u;raw negativeVCA5/signed-zero/phase-preserving restart;program envelope roundtrip+legacy rejection,%u complete130-field per-voice/pending/source snapshots; explicit snapshots,not full startup/adoption/native fit; PENDING_REVIEW\n",frames,(unsigned long long)hash,restarts,snapshots);return 0;
}
