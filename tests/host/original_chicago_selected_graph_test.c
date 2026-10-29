/* SPDX-License-Identifier: GPL-3.0-only */
#include "chicago_note_context.h"
#include "chicago_control_owner.h"
#include "chicago_fx2_bank.h"
#include "chicago_packed.h"
#include "motion_tables_private.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint64_t hash=UINT64_C(1469598103934665603);static unsigned frames;
static void word(uint32_t u){for(unsigned j=0;j<4;j++){hash=(hash^(uint8_t)u)*UINT64_C(1099511628211);u>>=8;}}
static void *storage(size_t n){void*p=malloc(n);assert(p);memset(p,0xa5,n);return p;}
static void one(unsigned factory,original_chicago_fx2_bank *bank,original_chicago_tuning *tuning){
 original_chicago_program p;original_chicago_voice_controls c;assert(original_chicago_program_init(&p,factory)&&original_chicago_voice_compile(&c,&p));assert(c.mg_shape[0]==0&&c.mg_shape[1]==0&&c.mg_tempo[1]==1);
 chicago_arp_control keys;memset(&keys,0,sizeof keys);original_held_record held[128];original_voice_record record[1];original_chicago_allocator a;assert(original_chicago_allocator_bind(&a,&keys,held,record,1,1,1,1,0,0));original_chicago_allocator_reset_lists(&a);a.scan_index=-1;a.previous_index=60;a.octave=1;a.mg1_key_sync=0;a.mg2_key_sync=1;a.arp_mode=c.routing.arp_mode;a.arp_range=c.routing.arp_range;
 original_chicago_voice_tables tables;assert(original_chicago_voice_tables_bind(&tables,original_chicago_packed_lookup,NULL,original_pitch44100));original_chicago_voice v;original_chicago_voice_seed seed;memset(&seed,0,sizeof seed);seed.oscillator_phase[0]=0x40000001;seed.oscillator_phase[1]=0xa0000003;seed.noise=0x72aa;seed.noise_gain.current=seed.noise_gain.target=.00001f;seed.last_pitch=60;seed.mg[0].phase=seed.mg[1].phase=-1;seed.mg[0].noise=0x12346d70;seed.mg[1].noise=0xabcd1234;assert(original_chicago_voice_init(&v,&tables,&seed));
 original_chicago_global_mg globals;memset(&globals,0,sizeof globals);globals.mg1.phase=globals.mg2.phase=-1;globals.mg1.random=0x12346d70;globals.mg2.random=0xabcd1234;
 original_chicago_global_mg_input gi={44100,44100,c.mg_frequency[0],c.mg_frequency[1],c.mg_shape[0],c.mg_shape[1],c.mg_tempo[0],c.mg_tempo[1]};chicago_arp_clock clock1={0,12345,0},clock2={0,12345,0};original_chicago_selected_global_stage global_stage;assert(original_chicago_selected_global_bind(&global_stage,&globals,&gi,&clock1,&clock2,&a));
 original_chicago_voice_owner owner;original_pending_note pending;memset(&pending,0,sizeof pending);original_chicago_extra_sources extra={0,0};original_chicago_note_context context;memset(&context,0,sizeof context);original_chicago_note_producers producers=original_chicago_note_context_callbacks(&context);assert(original_chicago_voice_owner_bind(&owner,&v,&pending,1,&a,&c,&tables,&producers,137));assert(original_chicago_note_context_bind(&context,&owner,tuning,&global_stage,&extra,0,1));
 original_chicago_selector selector={-1,60,1,0,0x6d70};chicago_arp_clock_input clock_input={137,c.routing.arp_tempo_sync};chicago_arp_control_input control_input={c.routing.arp_enable,c.routing.arp_latch,0};original_chicago_scheduler_callbacks callbacks=original_chicago_note_context_scheduler_callbacks();original_chicago_scheduler scheduler;assert(original_chicago_scheduler_bind(&scheduler,&a,&selector,&clock2,&clock_input,&control_input,&callbacks,&owner));
 original_chicago_fx2_owner effects={bank,original_chicago_fx2_bank_find,original_chicago_fx2_bank_reset_public_projection};original_chicago_engine_output output;assert(original_chicago_engine_output_init(&output,&c.routing,&effects));original_chicago_output_mute(&output.output,0);
 original_chicago_control_owner control;assert(original_chicago_control_owner_bind(&control,&p,&c,&output,&scheduler,&gi));
 int16_t converted[2][31],scratch[4][31];uint32_t il[31],ir[31],ol[31],orr[31];original_chicago_output_workspace w={converted[0],converted[1],scratch[0],scratch[1],31};original_chicago_fx2_workspace fw={scratch[0],scratch[1],scratch[2],scratch[3],31};
 for(unsigned block=0;block<44;block++){
  assert(original_chicago_control_owner_edit(&control,block/2,(float)(block&1)));
  /* Explicit chosen block boundary, without pretending this test supplies
   * the native outer startup/geometry/restore/warmup owner. */
  chicago_arp_clock_prepare(&clock2,44100,c.routing.arp_base_note,c.routing.arp_times);assert(original_chicago_scheduler_prepare(&scheduler));
  original_chicago_drive_event events[]={{3,4},{3,5},{17,4}};assert(original_chicago_drive_wrapper_events(&output.output.fx1,events,3));assert(original_chicago_fx2_bank_events(bank,events,3,0));
  for(unsigned i=0;i<31;i++){
   if(i==0)assert(original_chicago_scheduler_note(&scheduler,60+(int32_t)(block%12),100));
   if(i==19)assert(original_chicago_scheduler_note(&scheduler,60+(int32_t)(block%12),0));
   assert(original_chicago_scheduler_after_events(&scheduler));if(!original_chicago_voice_owner_control_all(&owner)){fprintf(stderr,"control failure factory%u block%u frame%u owner%d context%d global%d active%u VCA%u pending%u awaiting%u lists%u/%u head%d note%d counter%u\n",factory,block,i,owner.status,context.status,global_stage.status,v.active,v.vca.state,pending.mask,v.awaiting_restart,a.voice_active.count,a.voice_free.count,a.voice_active.head,v.raw_note,v.counter);assert(0);}float l,r;assert(original_chicago_voice_render_frame(&v,&c,&l,&r));memcpy(il+i,&l,4);memcpy(ir+i,&r,4);
   original_chicago_voice_sources current;assert(original_chicago_note_context_sources(&context,0,&current));assert(current.velocity_unipolar==v.velocity&&current.key_bipolar==v.keyboard_source);
  }
  assert(original_chicago_engine_output_process(&output,&w,&fw,&c.routing,il,ir,ol,orr,31,0));for(unsigned i=0;i<31;i++){word(ol[i]);word(orr[i]);}frames+=31;
  assert(owner.status&&context.status&&global_stage.status&&control.status&&globals.mg1.random==0x12346d70&&globals.mg2.random==0xabcd1234&&tuning->random==0x6d70);
 }
}
int main(void){
 original_chicago_fx2_bank *bank=storage(sizeof *bank);original_chicago_fx2_storage m={{storage(2*CHFX_SHORT_SLOTS),CHFX_SHORT_SLOTS},{storage(2*CHFX_BPM_STEREO_SLOTS),CHFX_BPM_STEREO_SLOTS},{storage(2*CHFX_GATE_SLOTS),CHFX_GATE_SLOTS},{storage(2*CHFX_HALL_SLOTS),CHFX_HALL_SLOTS},{storage(4096),2048},storage(4096),storage(4096),1024,{NULL,NULL}};
 original_chicago_tuning tuning;assert(original_chicago_tuning_bind(&tuning,0,0,NULL,0x6d70));for(unsigned factory=0;factory<81;factory++){assert(original_chicago_fx2_bank_init(bank,&m));one(factory,bank,&tuning);}
 free(m.short_delay.data);free(m.bpm_delay.data);free(m.gate.data);free(m.hall.data);free(m.chorus.data);free(m.ensemble_left);free(m.ensemble_right);free(bank);
 printf("PASS actual selected Chicago macro->scheduler->allocator->tuning/fullMG->voice/globalMG->Drive->12FX2/master host composition:81factories/all22endpoints,%u stereo frames,hash%016llx; explicit snapshots,public-reset/dedicated-history projection; NOT original-all30 parity/device fit; PENDING_REVIEW\n",frames,(unsigned long long)hash);return 0;
}
