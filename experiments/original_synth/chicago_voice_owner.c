/* SPDX-License-Identifier: GPL-3.0-only */
#include "chicago_voice_owner.h"
#include "binary32_classify.h"
#include "binary32_math.h"
#include "chicago_scheduler.h"
#include <string.h>
static unsigned enabled(void *p,unsigned id){original_chicago_voice_owner *s=p;return id<s->count?s->voices[id].active:0;}
static unsigned vca(void *p,unsigned id){original_chicago_voice_owner *s=p;return id<s->count?s->voices[id].vca.state:6;}
static void state(void *p,unsigned id,int key,int filter,int amp){
 original_chicago_voice_owner *s=p;if(id>=s->count){s->status=0;return;}original_chicago_voice *v=s->voices+id;
 if(key>=0)v->key_down=(uint8_t)key;
 if(filter>=0)v->vcf.state=(uint8_t)filter;
 if(amp>=0)v->vca.state=(uint8_t)amp;
}
static int inputs(original_chicago_voice_owner *s,unsigned id,const original_voice_record *record,unsigned full,unsigned mask,int32_t anchor,original_chicago_voice_note *out){
 original_chicago_voice *v=s->voices+id;original_chicago_voice_note n;memset(&n,0,sizeof n);
 n.raw_note=record->note;n.velocity=(unsigned)record->velocity;n.unison_flag=record->unison_flag;n.unison_coordinate=record->coordinate;
 original_chicago_voice_sources src;if(!original_chicago_voice_source_values(&n,&src))return 0;
 /* A10c..A19c / B22c..B2c4 precede either global resolver. These source
  * publications also happen when the lane was previously disabled. */
 v->active=1;v->key_down=1;v->raw_note=n.raw_note;v->raw_velocity=n.velocity;v->velocity=src.velocity_unipolar;v->keyboard_source=src.key_bipolar;v->unison_flag=(uint8_t)n.unison_flag;v->unison_coordinate=n.unison_coordinate;
 s->producers.sources(s->producers.context,id,&src);
 if(!s->producers.tune(s->producers.context,id,record,full,anchor,&n.tuned_pitch,&n.last_pitch))return 0;
 if(!original_binary32_isfinite(n.tuned_pitch)||(full&&!original_binary32_isfinite(n.last_pitch)))return 0;
 for(unsigned i=0;i<4;i++)if(mask&(1u<<i)){v->pitch.target[i]=n.tuned_pitch;v->activation[i]=-1;if(full)v->pitch.current[i]=n.last_pitch;}
 if(full&&!s->producers.full_mg(s->producers.context,id,n.mg))return 0;
 if(full)s->producers.full_helper_seed(s->producers.context,id);
 *out=n;return 1;
}
static int action(original_chicago_voice_owner *s,unsigned id,unsigned value){
 if(value==1){
  /* 8e7c/NoteOff can already have returned the record while VCA3 still
   * renders its release tail. Native A818 then finds no active record and is
   * a no-op. Its found/absent result is not a rendering success/failure bit. */
  (void)original_chicago_allocator_return_voice(s->allocator,id,original_chicago_voice_owner_callbacks(),s);return 1;
 }
 if(value==2){for(int16_t i=s->allocator->voice_active.head;i>=0;i=s->allocator->voice[i].next)if((unsigned)i==id)return 1;return 0;}
 return value==0;
}
static int resume(original_chicago_voice_owner *s,unsigned id){
 const original_pending_note *p=s->pending+id;unsigned mask=p->mask;if(!mask||mask>15)return 0;
 original_voice_record record={p->note,p->velocity,0,p->unison_flag,p->coordinate,-1,-1};
 if(mask==15){original_chicago_voice_note note;if(!inputs(s,id,&record,1,15,s->allocator->last_note,&note))return 0;return original_chicago_voice_resume_restart(s->voices+id,s->controls,s->tables,&note,s->bpm)&&s->producers.helper_exit(s->producers.context);}
 for(unsigned bit=1;bit<=8;bit<<=1)if(mask&bit){
  unsigned index=bit==1?0:bit==2?1:bit==4?2:3;record.note=p->oscillator_note[index];
  original_chicago_voice_note note;if(!inputs(s,id,&record,1,bit,s->allocator->last_note,&note))return 0;
  unsigned final_part=(mask&~((bit<<1)-1u))==0;
  if(!original_chicago_voice_resume_part(s->voices+id,s->controls,s->tables,&note,bit,final_part,s->bpm))return 0;
  if(!s->producers.helper_exit(s->producers.context))return 0;
 }
 return 1;
}
static void note(void *p,unsigned id,const original_voice_record *record,unsigned full,unsigned mask,int32_t anchor){
 original_chicago_voice_owner *s=p;if(!s->status)return;if(id>=s->count||!mask||mask>15){s->status=0;return;}
 original_chicago_voice *v=s->voices+id;unsigned kind=original_chicago_voice_request_kind(v,full);
 if(kind==ORIGINAL_CHICAGO_REQUEST_STAGE){original_chicago_allocator_stage_pending(s->pending+id,record,mask,v->vca.state);s->status=original_chicago_voice_stage_raw(v);return;}
 if(!kind){s->status=0;return;}
 original_chicago_voice_note n;if(!inputs(s,id,record,full,mask,anchor,&n)){s->status=0;return;}
 unsigned result;if(!original_chicago_voice_immediate_raw(v,s->controls,s->tables,&n,full,mask,s->bpm,&result)||!action(s,id,result)){s->status=0;return;}
 if(result==2)s->status=resume(s,id);
 if(s->status)s->status=s->producers.helper_exit(s->producers.context);
}
static const original_allocator_callbacks callbacks={enabled,vca,state,note};
const original_allocator_callbacks *original_chicago_voice_owner_callbacks(void){return &callbacks;}
int original_chicago_voice_owner_bind(original_chicago_voice_owner *s,original_chicago_voice *v,original_pending_note *p,uint32_t count,original_chicago_allocator *allocator,const original_chicago_voice_controls *c,const original_chicago_voice_tables *t,const original_chicago_note_producers *producers,float bpm){
 if(!s||!v||!p||!count||!allocator||count!=allocator->storage_voices||!c||!t||!producers||!producers->tune||!producers->full_mg||!producers->sources||!producers->full_helper_seed||!producers->helper_exit||!original_binary32_isfinite(bpm)||bpm<20||bpm>400)return 0;
 *s=(original_chicago_voice_owner){v,p,count,allocator,c,t,*producers,bpm,1};return 1;
}
static int control(original_chicago_voice_owner *s,unsigned id,unsigned admitted){
 unsigned result;int done=admitted?original_chicago_voice_control_admitted(s->voices+id,s->controls,s->tables,s->bpm,0,&result):original_chicago_voice_control(s->voices+id,s->controls,s->tables,s->bpm,0,&result);
 if(!done||!action(s,id,result)){s->status=0;return 0;}
 if(result==2&&!resume(s,id)){s->status=0;return 0;}
 return 1;
}
int original_chicago_voice_owner_sample(original_chicago_voice_owner *s,unsigned id,float *left,float *right){
 if(!s||!s->status||id>=s->count||!left||!right||left==right)return 0;
 return control(s,id,0)&&original_chicago_voice_render_frame(s->voices+id,s->controls,left,right);
}
int original_chicago_voice_owner_control_all(original_chicago_voice_owner *s){
 if(!s||!s->status||s->allocator->capacity>s->count)return 0;
 for(unsigned id=0;id<s->allocator->capacity;id++)if(!control(s,id,0))return 0;
 s->status=s->producers.helper_exit(s->producers.context);return s->status;
}
int original_chicago_voice_owner_render_block(original_chicago_voice_owner *s,original_chicago_scheduler *scheduler,uint32_t *left,uint32_t *right,uint32_t frames){
 if(!s||!s->status||!scheduler||!left||!right||left==right||s->count!=1||s->allocator->capacity>1)return 0;
 if(!original_chicago_voice_block_ready(s->voices,s->controls,s->tables,s->bpm))return 0;
 for(uint32_t i=0;i<frames;i++){
  float l,r;
  if(original_chicago_scheduler_after_events(scheduler)<=0||!s->status)return 0;
  for(unsigned id=0;id<s->allocator->capacity;id++)if(!control(s,id,1))return 0;
  s->status=s->producers.helper_exit(s->producers.context);
  if(!s->status||!original_chicago_voice_render_frame(s->voices,s->controls,&l,&r))return 0;
  left[i]=original_math_bits(l);right[i]=original_math_bits(r);
 }
 return 1;
}
