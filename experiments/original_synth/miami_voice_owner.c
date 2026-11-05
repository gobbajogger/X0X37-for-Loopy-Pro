/* SPDX-License-Identifier: GPL-3.0-only */
#include "miami_voice_owner.h"
#include "binary32_classify.h"
#include <string.h>
static unsigned enabled(void *p,unsigned id){original_miami_voice_owner *s=p;return id<s->count?s->voices[id].active:0;}
static unsigned vca(void *p,unsigned id){original_miami_voice_owner *s=p;return id<s->count?s->voices[id].vca.state:6;}
static void state(void *p,unsigned id,int key,int filter,int amp){
 original_miami_voice_owner *s=p;if(id>=s->count){s->status=0;return;}original_miami_selected_voice *v=s->voices+id;
 if(key>=0)v->key_down=(uint8_t)key;
 if(filter>=0)v->vcf.state=(uint8_t)filter;
 if(amp>=0)v->vca.state=(uint8_t)amp;
}
static int inputs(original_miami_voice_owner *s,unsigned id,const original_voice_record *r,unsigned full,unsigned mask,int32_t anchor,original_miami_voice_note *n){
 if(r->note<0||r->note>127||r->velocity<0||r->velocity>127||r->unison_flag>2||anchor<0||anchor>127||!original_binary32_isfinite(r->coordinate))return 0;
 memset(n,0,sizeof *n);n->note_key=(unsigned)r->note;n->velocity=(unsigned)r->velocity;n->last_note=(unsigned)anchor;n->unison_flag=r->unison_flag;n->unison_coordinate=r->coordinate;
 original_miami_selected_voice *v=s->voices+id;
 v->active=1;v->key_down=1;v->note_key=(uint8_t)r->note;v->raw_velocity=(uint32_t)r->velocity;v->unison_flag=(uint8_t)r->unison_flag;v->unison_coordinate=r->coordinate;v->velocity=(float)r->velocity/127.0f;
 s->extra[id]=(original_miami_extra_sources){(float)(r->velocity-64)/63.0f,(float)r->note*.0078125f,(float)(r->note-64)*.015625f};
 if(!s->producers.tune(s->producers.context,id,r->note,&n->tuned_pitch))return 0;
 if(!original_binary32_isfinite(n->tuned_pitch)||n->tuned_pitch<0||n->tuned_pitch>127)return 0;
 for(unsigned i=0;i<4;i++)if(mask&(1u<<i)){v->pitch.target[i]=n->tuned_pitch;v->activation[i]=-1;if(full)v->pitch.current[i]=(float)anchor;}
 if(full){if(!s->producers.full_mg(s->producers.context,id,n->mg))return 0;s->producers.full_helper_seed(s->producers.context,id);}
 return 1;
}
static int resume(original_miami_voice_owner *s,unsigned id){
 const original_miami_pending_note *p=s->pending+id;unsigned mask=p->mask;if(!mask||mask>15)return 0;
 original_voice_record r={p->note,p->velocity,0,p->unison_flag,p->coordinate,-1,-1};
 if(mask==15){original_miami_voice_note n;if(!inputs(s,id,&r,1,15,s->allocator->last_note,&n))return 0;return original_miami_selected_voice_resume_part(s->voices+id,s->controls,s->tables,&n,15,1,s->bpm)&&s->producers.helper_exit(s->producers.context);}
 for(unsigned bit=1;bit<=8;bit<<=1)if(mask&bit){
  unsigned i=bit==1?0:bit==2?1:bit==4?2:3;r.note=p->oscillator_note[i];original_miami_voice_note n;
  if(!inputs(s,id,&r,1,bit,s->allocator->last_note,&n))return 0;
  unsigned last=(mask&~((bit<<1)-1u))==0;
  if(!original_miami_selected_voice_resume_part(s->voices+id,s->controls,s->tables,&n,bit,last,s->bpm)||!s->producers.helper_exit(s->producers.context))return 0;
 }
 return 1;
}
static int action(original_miami_voice_owner *s,unsigned id,unsigned result){
 if(result==1){(void)original_chicago_allocator_return_voice(s->allocator,id,original_miami_voice_owner_callbacks(),s);return 1;}
 if(result==2){
  for(int16_t i=s->allocator->voice_active.head;i>=0;i=s->allocator->voice[i].next)if((unsigned)i==id)return resume(s,id);
  return 0;
 }
 return result==0;
}
static void note(void *p,unsigned id,const original_voice_record *r,unsigned full,unsigned mask,int32_t anchor){
 original_miami_voice_owner *s=p;if(!s->status)return;if(id>=s->count||!mask||mask>15){s->status=0;return;}
 unsigned kind=original_miami_selected_voice_request_kind(s->voices+id,full);
 if(kind==ORIGINAL_MIAMI_REQUEST_STAGE){original_chicago_allocator_stage_pending(s->pending+id,r,mask,s->voices[id].vca.state);s->status=original_miami_selected_voice_stage_raw(s->voices+id);return;}
 original_miami_voice_note n;if(!kind||!inputs(s,id,r,full,mask,anchor,&n)){s->status=0;return;}
 unsigned result;
 if(!original_miami_selected_voice_immediate_raw(s->voices+id,s->controls,s->tables,&n,full,mask,s->bpm,&result)||!action(s,id,result)){s->status=0;return;}
 s->status=s->producers.helper_exit(s->producers.context);
}
static const original_allocator_callbacks callbacks={enabled,vca,state,note};
const original_allocator_callbacks *original_miami_voice_owner_callbacks(void){return &callbacks;}
int original_miami_voice_owner_bind(original_miami_voice_owner *s,original_miami_selected_voice *v,original_miami_pending_note *p,original_miami_extra_sources *e,unsigned count,original_miami_allocator *a,const original_miami_voice_controls *c,const original_miami_voice_tables *t,const original_miami_note_producers *producer,float bpm){
 if(!s||!v||!p||!e||!count||!a||count!=a->storage_voices||!c||!t||!producer||!producer->tune||!producer->full_mg||!producer->full_helper_seed||!producer->helper_exit||!original_binary32_isfinite(bpm)||bpm<20||bpm>400)return 0;
 *s=(original_miami_voice_owner){v,p,e,a,c,t,*producer,count,bpm,1};return 1;
}
static int control_all(original_miami_voice_owner *s,unsigned prevalidated){
 if(!s||!s->status||s->allocator->capacity>s->count)return 0;
 for(unsigned i=0;i<s->allocator->capacity;i++){
  unsigned result;
  int ok=prevalidated?original_miami_selected_voice_control_prevalidated(s->voices+i,s->controls,s->tables,s->bpm,&result):original_miami_selected_voice_control(s->voices+i,s->controls,s->tables,s->bpm,&result);
  if(!ok||!action(s,i,result)){s->status=0;return 0;}
 }
 s->status=s->producers.helper_exit(s->producers.context);return s->status;
}
int original_miami_voice_owner_control_all(original_miami_voice_owner *s){return control_all(s,0);}
int original_miami_voice_owner_control_prevalidated(original_miami_voice_owner *s){return control_all(s,1);}
int original_miami_voice_owner_block_valid(const original_miami_voice_owner *s){
 if(!s||!s->status||s->allocator->capacity>s->count)return 0;
 for(unsigned i=0;i<s->count;i++)if(!original_miami_selected_voice_block_valid(s->voices+i,s->controls,s->tables,s->bpm))return 0;
 return 1;
}
