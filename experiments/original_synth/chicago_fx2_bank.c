/* SPDX-License-Identifier: GPL-3.0-only */
#include "chicago_fx2_bank.h"
#include <string.h>
static const uint8_t types[12]={0,3,4,5,10,12,13,15,16,18,22,24};
static void parameter(void *p,unsigned parameter_id,unsigned value){
 original_chicago_fx2_binding *b=p;original_chicago_fx2_bank *s=b->bank;
 switch(b->type){
 case 0:(void)original_chicago_bypass_parameter(parameter_id,(int32_t)value);break;
 case 3:(void)original_chicago_ensemble_set(&s->ensemble,parameter_id,value);break;
 case 4:(void)original_chicago_chorus_set(&s->chorus,parameter_id,value);break;
 case 5:(void)original_chicago_phaser_set(&s->phaser,parameter_id,value);break;
 case 10:(void)original_chicago_ring_set(&s->ring,parameter_id,value);break;
 case 12:(void)chfx_short_edit(&s->short_delay,parameter_id,(int)value);break;
 case 13:(void)chfx_bpm_edit(&s->bpm_delay,parameter_id,(int)value);break;
 case 15:(void)chfx_hall_edit(&s->hall,parameter_id,(int)value);break;
 case 16:(void)chfx_gate_edit(&s->gate,parameter_id,(int)value);break;
 case 18:(void)original_chicago_mastercomp_set(&s->mastercomp,parameter_id-2,(int32_t)value);break;
 case 22:(void)original_chicago_decimator_set(&s->decimator,parameter_id-2,(int32_t)value);break;
 case 24:(void)original_chicago_peaking_set(&s->peaking,parameter_id-2,value);break;
 }
}
static void reset(void *p){
 original_chicago_fx2_binding *b=p;original_chicago_fx2_bank *s=b->bank;
 switch(b->type){case 3:original_chicago_ensemble_reset(&s->ensemble);break;case 4:original_chicago_chorus_reset(&s->chorus);break;case 5:original_chicago_phaser_reset(&s->phaser);break;case 10:original_chicago_ring_reset(&s->ring);break;case 12:chfx_short_reset(&s->short_delay);break;case 13:chfx_bpm_reset(&s->bpm_delay);break;case 15:chfx_hall_reset(&s->hall);break;case 16:chfx_gate_reset(&s->gate);break;case 18:original_chicago_mastercomp_reset(&s->mastercomp);break;case 22:original_chicago_decimator_reset(&s->decimator);break;case 24:original_chicago_peaking_reset(&s->peaking);break;default:break;}
}
static void begin(void *p){original_chicago_fx2_binding *b=p;if(b->type==15)chfx_hall_begin(&b->bank->hall);}
static void retire(void *p){original_chicago_fx2_binding *b=p;if(b->type==15)chfx_hall_retire(&b->bank->hall);}
static void tempo(void *p,int32_t value){original_chicago_fx2_binding *b=p;if(b->type==13)chfx_bpm_tempo(&b->bank->bpm_delay,value);}
static void activity(void *p,unsigned value){
 original_chicago_fx2_binding *b=p;original_chicago_fx2_bank *s=b->bank;
 switch(b->type){case 4:s->chorus.activity=(uint8_t)value;break;case 5:s->phaser.activity=(uint8_t)value;break;case 10:s->ring.activity=(uint8_t)value;break;case 12:chfx_short_fast(&s->short_delay,(int)value);break;case 13:chfx_bpm_fast(&s->bpm_delay,(int)value);break;case 15:chfx_hall_fast(&s->hall,(int)value);break;case 16:chfx_gate_fast(&s->gate,(int)value);break;case 18:original_chicago_mastercomp_activity(&s->mastercomp,(uint8_t)value);break;case 24:original_chicago_peaking_activity(&s->peaking,(uint8_t)value);break;default:break;}
}
static int32_t si(uint32_t u){return u<=INT32_MAX?(int32_t)u:-1-(int32_t)(UINT32_MAX-u);}
static void before_frame(void *p,uint32_t local){
 original_chicago_fx2_binding *b=p;original_chicago_fx2_bank *s=b->bank;
 unsigned slot=b->type==12?0:b->type==13?1:3;
 original_chicago_drive_queue *q=s->history_queue+slot;
 while(q->cursor<q->count){
  const original_chicago_drive_event *e=q->events+q->cursor;
  int due=b->type==16?e->offset<=local:si(e->offset)<=si((uint32_t)s->history_event_origin+local);
  if(!due)break;
  if(b->type==12)chfx_short_queued_event(&s->short_delay,e->type);
  else if(b->type==13)chfx_bpm_queued_event(&s->bpm_delay,e->type);
  else chfx_gate_queued_event(&s->gate,e->type);
  q->cursor++;
 }
}
static void process(void *p,int16_t *l,int16_t *r,uint32_t n){
 original_chicago_fx2_binding *b=p;original_chicago_fx2_bank *s=b->bank;const chfx_hooks h={before_frame,b};
 switch(b->type){case 0:(void)original_chicago_bypass_process(l,r,n);break;case 3:(void)original_chicago_ensemble_process(&s->ensemble,l,r,n);break;case 4:(void)original_chicago_chorus_process(&s->chorus,l,r,n);break;case 5:(void)original_chicago_phaser_process(&s->phaser,l,r,n);break;case 10:(void)original_chicago_ring_process(&s->ring,l,r,n);break;case 12:chfx_short_process(&s->short_delay,l,r,n,&h);break;case 13:chfx_bpm_process(&s->bpm_delay,l,r,n,&h);break;case 15:chfx_hall_process(&s->hall,l,r,n,&s->hall_context);break;case 16:chfx_gate_process(&s->gate,l,r,n,&h);break;case 18:(void)original_chicago_mastercomp_process(&s->mastercomp,l,r,n);break;case 22:(void)original_chicago_decimator_process(&s->decimator,l,r,n);break;case 24:(void)original_chicago_peaking_process(&s->peaking,l,r,n);break;}
}
static int admitted(const original_chicago_fx2_storage *m){
 return m&&m->short_delay.data&&m->short_delay.slots>=CHFX_SHORT_SLOTS&&m->bpm_delay.data&&m->bpm_delay.slots>=CHFX_BPM_STEREO_SLOTS&&m->gate.data&&m->gate.slots>=CHFX_GATE_SLOTS&&m->hall.data&&m->hall.slots>=CHFX_HALL_SLOTS&&m->chorus.data&&m->chorus.slots>=2048&&m->ensemble_left&&m->ensemble_right&&(m->ensemble_slots==1024||m->ensemble_slots==4096);
}
int original_chicago_fx2_bank_init(original_chicago_fx2_bank *s,const original_chicago_fx2_storage *m){
 if(!s||!admitted(m))return 0;
 memset(s,0,sizeof *s);s->hall_context=m->hall_context;
 original_chicago_ring_init(&s->ring);original_chicago_phaser_init(&s->phaser);
 (void)original_chicago_chorus_init(&s->chorus,m->chorus.data,m->chorus.data+1024);
 (void)original_chicago_ensemble_init(&s->ensemble,m->ensemble_left,m->ensemble_right,m->ensemble_slots);
 (void)original_chicago_mastercomp_init(&s->mastercomp);(void)original_chicago_peaking_init(&s->peaking);(void)original_chicago_decimator_init(&s->decimator);
 chfx_short_construct(&s->short_delay);chfx_bpm_construct(&s->bpm_delay);chfx_gate_construct(&s->gate);chfx_hall_construct(&s->hall);
 (void)chfx_short_bind(&s->short_delay,m->short_delay.data,m->short_delay.slots);(void)chfx_bpm_bind(&s->bpm_delay,m->bpm_delay.data,m->bpm_delay.slots);(void)chfx_gate_bind(&s->gate,m->gate.data,m->gate.slots);(void)chfx_hall_bind(&s->hall,m->hall.data,m->hall.slots);
 for(unsigned i=0;i<12;i++){
  original_chicago_fx2_binding *b=s->binding+i;b->bank=s;b->type=types[i];
  b->kernel=(original_chicago_fx2_kernel){b,process,parameter,reset,begin,retire,tempo,activity};
 }
 return 1;
}
const original_chicago_fx2_kernel *original_chicago_fx2_bank_find(void *p,unsigned type){
 original_chicago_fx2_bank *s=p;if(!s)return NULL;for(unsigned i=0;i<12;i++)if(s->binding[i].type==type)return &s->binding[i].kernel;return NULL;
}
int original_chicago_fx2_bank_events(original_chicago_fx2_bank *s,const original_chicago_drive_event *e,uint32_t count,int32_t origin){
 if(!s||(count&&!e))return 0;for(uint32_t i=1;i<count;i++)if(e[i-1].offset>e[i].offset)return 0;
 /* Canonical identity and sorting were checked above once. The individual
  * bind APIs only repeat that check then replace these exact descriptors. */
 s->ring.queue=(original_mod_queue){e,count,0};s->chorus.queue=(original_mod_queue){e,count,0};s->phaser.queue=(original_mod_queue){e,count,0};s->ensemble.queue=(original_mod_queue){e,count,0};
 s->mastercomp.queue=(original_chicago_dyn_queue){e,count,0};s->peaking.queue=(original_chicago_dyn_queue){e,count,0};s->decimator.queue=(original_chicago_dyn_queue){e,count,0};
 for(unsigned i=0;i<4;i++)s->history_queue[i]=(original_chicago_drive_queue){e,count,0};s->history_event_origin=origin;return 1;
}
void original_chicago_fx2_bank_direct_event(original_chicago_fx2_bank *s,unsigned type){
 if(!s)return;original_chicago_ring_direct_event(&s->ring,type);original_chicago_chorus_direct_event(&s->chorus,type);original_chicago_phaser_direct_event(&s->phaser,type);original_chicago_ensemble_direct_event(&s->ensemble,type);original_chicago_mastercomp_direct_event(&s->mastercomp,type);original_chicago_peaking_direct_event(&s->peaking,type);original_chicago_decimator_direct_event(&s->decimator,type);chfx_short_event(&s->short_delay,type);chfx_bpm_event(&s->bpm_delay,type);chfx_gate_event(&s->gate,type);chfx_hall_event(&s->hall,type);
}
void original_chicago_fx2_bank_reset_public_projection(void *p){
 original_chicago_fx2_bank *s=p;if(!s)return;
 for(unsigned i=0;i<12;i++){original_chicago_fx2_kernel *k=&s->binding[i].kernel;
  if(s->binding[i].type==15){k->begin(k->context);k->reset(k->context);k->retire(k->context);}else k->reset(k->context);
 }
}
