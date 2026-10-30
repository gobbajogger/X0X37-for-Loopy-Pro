/* SPDX-License-Identifier: GPL-3.0-only */
#include "miami_note_context.h"
#include "chicago_key_admission.h"
static int tune(void *p,unsigned id,int32_t raw,float *out){
 original_miami_note_context *s=p;if(!s->status||id>=s->owner->count||raw<0||raw>127)return 0;
 *out=(float)raw+0.0f;return 1; /* Actual C978/DAE8 selected Equal branch. */
}
static int full_mg(void *p,unsigned id,original_miami_voice_mg out[2]){
 original_miami_note_context *s=p;if(!s->status||id>=s->owner->count)return 0;
 int16_t head=s->owner->allocator->voice_active.head;
 if(head<0||(unsigned)head>=s->owner->count)return 0;
 const original_chicago_global_mg_input *in=s->global_input;
 if(in->waveform1>3||in->waveform2>3||in->tempo_sync2!=1)return 0;
 for(unsigned j=0;j<2;j++){
  out[j]=s->owner->voices[id].mg[j];
  if((unsigned)head==id){
   original_chicago_global_mg_osc *global=j?&s->global->mg2:&s->global->mg1;
   if(s->key_sync[j]){
    /* Reset+global waveform evaluation has the same final selected fields as
     * notification for0..3:phase-1,wrap0,output0/1,hold/noise preserved. */
    original_chicago_global_mg2_notify(global,j?in->waveform2:in->waveform1);out[j].wrapped=1;
   }
   out[j].phase=global->phase;
  }else{
   const original_miami_voice_mg *source=s->owner->voices[head].mg+j;
   out[j].phase=source->phase;out[j].hold=source->hold;out[j].noise=source->noise;
  }
 }
 return 1;
}
static void seed(void *p,unsigned id){original_miami_note_context *s=p;if(s->owner->allocator->voice_active.head==(int16_t)id){s->global->counter=44100;s->head_seeds++;}}
static void unexpected_free_step(void *p){((original_miami_note_context *)p)->status=0;}
static int helper_exit(void *p){
 original_miami_note_context *s=p;if(!s->status||s->global_input->tempo_sync2!=1)return 0;
 (void)original_chicago_global_mg_after_voices(s->global,s->global_input,s->clock1,s->clock2,unexpected_free_step,s);s->helper_entries++;return s->status;
}
original_miami_note_producers original_miami_note_context_callbacks(original_miami_note_context *s){return (original_miami_note_producers){s,tune,full_mg,seed,helper_exit};}
static int bind_context(original_miami_note_context *s,original_miami_voice_owner *owner,original_chicago_global_mg *global,const original_chicago_global_mg_input *input,chicago_arp_clock *a,chicago_arp_clock *b,unsigned scale,unsigned tonic,unsigned key1,unsigned key2,unsigned clocked){
 if(!s||!owner||!global||!input||!a||!b||scale||tonic||key1>1||key2>1||input->sample_rate_integer!=44100||input->sample_rate!=44100||input->tempo_sync1!=1||input->tempo_sync2!=1||input->waveform1>3||input->waveform2>3||!owner->allocator||!owner->allocator->keys||(!clocked&&owner->allocator->keys->committed_enable))return 0;
 if(clocked&&(!original_chicago_canonical_keys_valid(owner->allocator->keys)||owner->allocator->arp_range!=1||owner->allocator->mode>3||owner->allocator->keys->current_note>127||owner->allocator->last_note<0||owner->allocator->last_note>127))return 0;
 *s=(original_miami_note_context){owner,global,input,a,b,{(uint8_t)key1,(uint8_t)key2},1,0,0};return 1;
}
int original_miami_note_context_bind(original_miami_note_context *s,original_miami_voice_owner *owner,original_chicago_global_mg *global,const original_chicago_global_mg_input *input,chicago_arp_clock *a,chicago_arp_clock *b,unsigned scale,unsigned tonic,unsigned key1,unsigned key2){
 return bind_context(s,owner,global,input,a,b,scale,tonic,key1,key2,0);
}
int original_miami_note_context_bind_clocked(original_miami_note_context *s,original_miami_voice_owner *owner,original_chicago_global_mg *global,const original_chicago_global_mg_input *input,chicago_arp_clock *a,chicago_arp_clock *b,unsigned scale,unsigned tonic,unsigned key1,unsigned key2){
 return bind_context(s,owner,global,input,a,b,scale,tonic,key1,key2,1);
}
static void mg1(void *p,original_chicago_allocator *a){
 original_miami_voice_owner *owner=p;original_miami_note_context *s=owner->producers.context;
 s->clock1->key_reset=a->mg1_key_reset;original_chicago_global_mg1_tick(&s->global->mg1,s->clock1,s->global_input->waveform1,s->global_input->tempo_sync1,owner->bpm);a->mg1_key_reset=s->clock1->key_reset;
}
static void mg2(void *p,original_chicago_allocator *a){
 original_miami_voice_owner *owner=p;original_miami_note_context *s=owner->producers.context;(void)a;
 original_chicago_global_mg2_notify(&s->global->mg2,s->global_input->waveform2);
}
original_chicago_scheduler_callbacks original_miami_note_context_scheduler_callbacks(void){return (original_chicago_scheduler_callbacks){original_miami_voice_owner_callbacks(),mg1,mg2};}
static int reset_ready(const original_miami_note_context *s){
 if(!s||!s->status||!s->owner||!s->owner->status)return 0;
 for(unsigned i=0;i<s->owner->count;i++)if(!original_miami_selected_voice_request_kind(s->owner->voices+i,0))return 0;
 return 1;
}
static int reset_owned(original_miami_note_context *s){
 original_miami_voice_owner *owner=s->owner;
 for(unsigned i=0;i<owner->count;i++){
  if(!original_miami_selected_voice_core_reset(owner->voices+i,owner->tables,i)){owner->status=0;return 0;}
  owner->pending[i]=(original_miami_pending_note){0,0,0,0,15,{0,0,0,0}};
 }
 s->global->counter=0;s->global->mg1=(original_chicago_global_mg_osc){-1,0,0,1,0x6d70};s->global->mg2=s->global->mg1;
 s->clock1->key_reset=s->clock2->key_reset=0;owner->allocator->mg1_key_reset=owner->allocator->mg2_key_reset=0;
 return 1;
}
int original_miami_note_context_geometry_reset(void *context){
 original_miami_note_context *s=context;return reset_ready(s)&&reset_owned(s);
}
int original_miami_note_context_core_reset(original_miami_note_context *s){
 if(!reset_ready(s))return 0;
 original_miami_voice_owner *owner=s->owner;
 original_chicago_allocator_all_notes_off(owner->allocator,original_miami_voice_owner_callbacks(),owner);
 return owner->status&&reset_owned(s);
}
