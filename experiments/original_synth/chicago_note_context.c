/* SPDX-License-Identifier: GPL-3.0-only */
#include "chicago_note_context.h"
#include <string.h>
static uint32_t bits(float x){uint32_t u;memcpy(&u,&x,4);return u;}
static float fl(uint32_t u){float x;memcpy(&x,&u,4);return x;}
static int tune(void *p,unsigned id,const original_voice_record *record,unsigned full,int32_t anchor,float *target,float *current){
 original_chicago_note_context *s=p;(void)full;
 if(!s->status||id>=s->owner->count||!original_chicago_tuning_target(s->tuning,record->note,target)){s->status=0;return 0;}
 /* Actual integer last-note anchor; selected additional Analog/secondary
  * pitch term is original+0. Do not inject an unobserved tuning offset. */
 *current=(float)anchor;return 1;
}
static original_chicago_mg_voice_bits voice_bits(const original_chicago_voice_mg *v){
 return (original_chicago_mg_voice_bits){bits(v->phase),bits(v->unipolar),bits(v->bipolar),bits(v->hold),v->noise,v->wrapped};
}
static original_chicago_mg_global_bits global_bits(const original_chicago_global_mg_osc *v){
 return (original_chicago_mg_global_bits){bits(v->phase),bits(v->output),bits(v->held),v->random,v->wrapped};
}
static int mg(void *p,unsigned id,original_chicago_voice_mg out[2]){
 original_chicago_note_context *s=p;if(!s->status||id>=s->owner->count)return 0;
 int16_t head=s->owner->allocator->voice_active.head;if(head<0||(uint32_t)head>=s->owner->count){s->status=0;return 0;}
 original_chicago_full_mg_input in;memset(&in,0,sizeof in);in.voice_id=id;in.head_id=(unsigned)head;
 original_chicago_global_mg_osc *g[2]={&s->global->state->mg1,&s->global->state->mg2};
 for(unsigned i=0;i<2;i++){
  in.key_sync[i]=s->key_sync[i];in.shape[i]=s->owner->controls->mg_shape[i];
  in.current[i]=voice_bits(&s->owner->voices[id].mg[i]);in.head[i]=voice_bits(&s->owner->voices[(unsigned)head].mg[i]);in.global[i]=global_bits(g[i]);
 }
 original_chicago_full_mg_output n;if(!original_chicago_full_mg_resolve(&in,&n)||n.voice[0].wrapped>255||n.voice[1].wrapped>255){s->status=0;return 0;}
 for(unsigned i=0;i<2;i++){
  g[i]->phase=fl(n.global[i].phase);g[i]->output=fl(n.global[i].sample);g[i]->held=fl(n.global[i].hold);g[i]->random=n.global[i].noise;g[i]->wrapped=n.global[i].wrapped;
  out[i]=(original_chicago_voice_mg){fl(n.voice[i].phase),fl(n.voice[i].unipolar),fl(n.voice[i].bipolar),fl(n.voice[i].hold),n.voice[i].noise,(uint8_t)n.voice[i].wrapped};
 }
 return 1;
}
static void sources(void *p,unsigned id,const original_chicago_voice_sources *v){
 original_chicago_note_context *s=p;if(!s->status||id>=s->owner->count){s->status=0;return;}
 s->extra[id]=(original_chicago_extra_sources){v->velocity_bipolar,v->key_unipolar};
}
static void seed(void *p,unsigned id){original_chicago_note_context *s=p;if(s->status)original_chicago_selected_global_seed(s->global,id);}
static int exit_helper(void *p){original_chicago_note_context *s=p;return s->status&&original_chicago_selected_global_exit(s->global);}
original_chicago_note_producers original_chicago_note_context_callbacks(original_chicago_note_context *s){return (original_chicago_note_producers){s,tune,mg,sources,seed,exit_helper};}
static void mg1_tick(void *p,original_chicago_allocator *a){
 original_chicago_voice_owner *owner=p;original_chicago_note_context *s=owner->producers.context;
 if(!s->status){owner->status=0;return;}
 chicago_arp_clock *clock=s->global->clock1;clock->key_reset=a->mg1_key_reset;
 original_chicago_global_mg1_tick(&s->global->state->mg1,clock,s->global->input->waveform1,s->global->input->tempo_sync1,owner->bpm);a->mg1_key_reset=clock->key_reset;
}
static void mg2_notify(void *p,original_chicago_allocator *a){
 original_chicago_voice_owner *owner=p;original_chicago_note_context *s=owner->producers.context;(void)a;
 if(!s->status){owner->status=0;return;}
 original_chicago_global_mg2_notify(&s->global->state->mg2,s->global->input->waveform2);
}
original_chicago_scheduler_callbacks original_chicago_note_context_scheduler_callbacks(void){return (original_chicago_scheduler_callbacks){original_chicago_voice_owner_callbacks(),mg1_tick,mg2_notify};}
int original_chicago_note_context_bind(original_chicago_note_context *s,original_chicago_voice_owner *owner,original_chicago_tuning *tuning,original_chicago_selected_global_stage *global,original_chicago_extra_sources *extra,unsigned k1,unsigned k2){
 typedef char global_noise32[sizeof(global->state->mg1.random)==sizeof(uint32_t)?1:-1];(void)sizeof(global_noise32);
 if(!s||!owner||!owner->voices||!owner->allocator||!owner->controls||!tuning||!global||!global->status||!extra||k1>1||k2>1||owner->controls->mg_shape[0]!=0||owner->controls->mg_shape[1]!=0||owner->producers.context!=s)return 0;
 *s=(original_chicago_note_context){owner,tuning,global,extra,{(uint8_t)k1,(uint8_t)k2},1};return 1;
}
int original_chicago_note_context_sources(const original_chicago_note_context *s,unsigned id,original_chicago_voice_sources *out){
 if(!s||!s->status||!out||id>=s->owner->count)return 0;
 const original_chicago_voice *v=s->owner->voices+id;
 *out=(original_chicago_voice_sources){v->velocity,s->extra[id].velocity_bipolar,s->extra[id].key_unipolar,v->keyboard_source};return 1;
}
