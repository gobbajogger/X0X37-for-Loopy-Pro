/* SPDX-License-Identifier: GPL-3.0-only */
#include "engine_events.h"
#include "miami_geometry.h"
#include "miami_voice_snapshot.h"
#include "motion_tables_private.h"
#include "binary32_math.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint64_t hash=UINT64_C(1469598103934665603);static unsigned captures;
static void word(uint32_t u){for(unsigned j=0;j<4;j++){hash=(hash^(uint8_t)u)*UINT64_C(1099511628211);u>>=8;}}
typedef struct{original_miami_note_context *context;original_miami_geometry *geometry;unsigned calls;} observed;
static int direct_reset(void*p){
 observed*o=p;original_chicago_allocator*a=o->context->owner->allocator;o->calls++;
 assert(a->held_active.count==0&&a->held_free.count==128&&a->voice_active.count==0&&a->voice_free.count==2&&a->capacity==2&&a->unison_count==2&&o->geometry->value.polyphony==15);
 assert(a->keys->physical[60]==100&&a->keys->latched[60]==100&&a->keys->physical_count==1&&a->keys->latched_count==1&&a->keys->current_note==60&&a->last_note==60&&!a->gate_remaining);
#ifdef MIAMI_GEOMETRY_WIDER_RESET_BASELINE
 return original_miami_note_context_core_reset(o->context);
#else
 return original_miami_note_context_geometry_reset(o->context);
#endif
}
static int run(unsigned factory,const original_miami_voice_tables*t){
 original_miami_program p;original_miami_voice_controls c;assert(original_miami_program_init(&p,factory)&&original_miami_voice_compile(&c,&p));original_miami_program saved_program=p;original_miami_voice_controls saved_controls=c;
 chicago_arp_control keys;memset(&keys,0,sizeof keys);original_held_record held[128];original_voice_record records[2];original_chicago_allocator a;assert(original_chicago_allocator_bind(&a,&keys,held,records,2,2,2,1,0,1));a.mg1_key_sync=a.mg2_key_sync=1;
 original_miami_selected_voice v[2];for(unsigned i=0;i<2;i++)assert(original_miami_selected_voice_init(v+i,t,i,60));original_miami_pending_note pending[2];original_miami_extra_sources extra[2];memset(pending,0,sizeof pending);memset(extra,0,sizeof extra);
 original_chicago_global_mg global;memset(&global,0,sizeof global);global.mg1.phase=global.mg2.phase=-1;global.mg1.random=global.mg2.random=0x6d70;
 float rate1,rate2;assert(original_miami_program_native(&p,44,&rate1)&&original_miami_program_native(&p,50,&rate2));original_chicago_global_mg_input gi={44100,44100,rate1,rate2,c.mg_shape[0],c.mg_shape[1],1,1};chicago_arp_clock clock1={.25f,17,1},clock2={.75f,31,1};
 original_miami_note_context context;memset(&context,0,sizeof context);original_miami_note_producers producers=original_miami_note_context_callbacks(&context);original_miami_voice_owner owner;assert(original_miami_voice_owner_bind(&owner,v,pending,extra,2,&a,&c,t,&producers,137));assert(original_miami_note_context_bind(&context,&owner,&global,&gi,&clock1,&clock2,0,0,1,1));
 original_chicago_allocator_spawn(&a,60,100,original_miami_voice_owner_callbacks(),&owner);assert(owner.status);
 for(unsigned frame=0;frame<17;frame++){
  assert(original_miami_voice_owner_control_all(&owner));for(unsigned id=0;id<2;id++){float l,r;assert(original_miami_selected_voice_render_frame(v+id,&c,&l,&r));word(original_math_bits(l));word(original_math_bits(r));}
 }
 original_chicago_allocator_physical(&a,60,100);held[a.held_active.head].marked=1;keys.physical[60]=keys.latched[60]=100;keys.physical_count=keys.latched_count=1;keys.current_note=65;a.last_note=67;a.gate_remaining=9;a.pedal_byte=127;keys.effective_latch=1;
 uint32_t phases[2][4];float velocity[2];original_miami_extra_sources old_extra[2];unsigned char packets[2][ORIGINAL_MIAMI_VOICE_SNAPSHOT_BYTES];
 for(unsigned i=0;i<2;i++){memcpy(phases[i],v[i].pitch.oscillator.phase,16);velocity[i]=v[i].velocity;old_extra[i]=extra[i];original_miami_voice_snapshot state;assert(original_miami_voice_snapshot_capture(&state,&owner,i)&&original_miami_voice_snapshot_encode(&state,packets[i],sizeof packets[i]));}
 const uint32_t old_helpers=context.helper_entries,old_seeds=context.head_seeds;chicago_arp_clock before1=clock1,before2=clock2;
 uint32_t map[128];original_allocator_chord pairs[128];for(unsigned i=0;i<128;i++){map[i]=7;pairs[i]=(original_allocator_chord){-17,33};}
 original_miami_geometry g={{16,2,1,2,1},map,pairs,3};observed o={&context,&g,0};original_miami_geometry_progress progress;
 assert(original_miami_geometry_prepare(&g,&a,15,2,1,direct_reset,&o,&progress)&&o.calls==1&&progress.status&&progress.completed_resets==1&&!progress.learned_chord);
 if(keys.physical[60]!=100||keys.latched[60]!=100||keys.physical_count!=1||keys.latched_count!=1){fprintf(stderr,"EXPECTED_GEOMETRY_BOUNDARY_FAILURE: D444/wider-core callback cleared key arrays/counts; B0738 must preserve them\n");return 0;}
 assert(a.pedal_byte==127&&keys.effective_latch==1&&!memcmp(&p,&saved_program,sizeof p)&&!memcmp(&c,&saved_controls,sizeof c));assert(context.helper_entries==old_helpers&&context.head_seeds==old_seeds&&clock1.accumulator==before1.accumulator&&clock1.threshold==before1.threshold&&clock2.accumulator==before2.accumulator&&clock2.threshold==before2.threshold&&!clock1.key_reset&&!clock2.key_reset);
 for(unsigned i=0;i<128;i++)assert(map[i]==7&&pairs[i].note==-17&&pairs[i].velocity==33);
 for(unsigned i=0;i<2;i++){
  assert(!memcmp(phases[i],v[i].pitch.oscillator.phase,16)&&velocity[i]==v[i].velocity&&!memcmp(extra+i,old_extra+i,sizeof *extra)&&!v[i].raw_velocity&&!v[i].active&&v[i].vca.state==6&&v[i].vcf.state==6&&pending[i].mask==15);
  original_miami_voice_snapshot old,after;unsigned char again[ORIGINAL_MIAMI_VOICE_SNAPSHOT_BYTES];assert(ORIGINAL_MIAMI_VOICE_SNAPSHOT_WORDS==130&&ORIGINAL_MIAMI_VOICE_SNAPSHOT_BYTES==536);assert(original_miami_voice_snapshot_decode(&old,packets[i],sizeof packets[i])&&original_miami_voice_snapshot_encode(&old,again,sizeof again)&&!memcmp(again,packets[i],sizeof again));assert(original_miami_voice_snapshot_capture(&after,&owner,i)&&original_miami_voice_snapshot_encode(&after,again,sizeof again));captures+=2;for(unsigned j=0;j<sizeof again;j++)word(again[j]);
 }
 /* The actual restore caller sign-extends raw bytes,does not7bitclamp,
  *and always forces geometry,includingan empty map. Still no clock/helper. */
 uint8_t raw_map[128]={0};raw_map[60]=0x80;raw_map[72]=0xff;
 assert(original_miami_geometry_restore_map(&g,&a,raw_map,15,2,direct_reset,&o,&progress)&&o.calls==2&&!progress.learned_chord&&progress.completed_resets==1);
 assert(g.chord_count==2&&map[60]==UINT32_C(0xffffff80)&&map[72]==UINT32_C(0xffffffff)&&pairs[0].note==60&&pairs[0].velocity==-128&&pairs[1].note==72&&pairs[1].velocity==-1&&pairs[2].note==-128&&!pairs[2].velocity);
 memset(raw_map,0,sizeof raw_map);assert(original_miami_geometry_restore_map(&g,&a,raw_map,15,2,direct_reset,&o,&progress)&&o.calls==3&&!g.chord_count&&progress.completed_resets==1);
 assert(original_miami_geometry_learn(&g,&a,129,1,direct_reset,&o,&progress)&&o.calls==3&&!progress.completed_resets&&!progress.learned_chord&&g.value.polyphony==15);
 assert(keys.physical[60]==100&&keys.latched[60]==100&&context.helper_entries==old_helpers&&context.head_seeds==old_seeds&&clock1.accumulator==before1.accumulator&&clock2.accumulator==before2.accumulator);
 /* Existing D444 entry remains a wider reset and must still clear keys. */
 assert(original_miami_note_context_core_reset(&context)&&!keys.physical_count&&!keys.latched_count&&!keys.physical[60]&&!keys.latched[60]);return 1;
}
int main(void){const original_ms20_tables f={original_miami_a44100,original_miami_b44100,original_miami_r,original_miami_cap};original_miami_voice_tables t;assert(original_miami_voice_tables_bind_immutable(&t,&f,original_pitch44100));for(unsigned i=0;i<30;i++)if(!run(i,&t))return 42;printf("PASS all30 Miami concrete geometry->D5E0->B0738 owner paths plus signed/native restore and empty-learn ordering;key-array/phase/source/program/clock preservation,no extra helper/seed,retained D444 clearing,%u compatible130-field snapshot checks,hash%016llx;explicit selected resources,no mode0audio/native/adoption claim;PENDING_REVIEW\n",captures,(unsigned long long)hash);return 0;}
