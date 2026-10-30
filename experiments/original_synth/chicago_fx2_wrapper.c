/* SPDX-License-Identifier: GPL-3.0-only */
#include "chicago_fx2_wrapper.h"
#include <limits.h>
#include <string.h>
static int32_t si(uint32_t u){return u<=INT32_MAX?(int32_t)u:-1-(int32_t)(UINT32_MAX-u);}
static int32_t asr15(int32_t x){uint32_t u=(uint32_t)x;return si((u>>15)|(x<0?UINT32_C(0xfffe0000):0));}
static int16_t blend(int16_t old,int16_t newer,int32_t new_weight){
 int32_t v=asr15(si((uint32_t)(int32_t)newer*(uint32_t)new_weight))+asr15(si((uint32_t)(int32_t)old*(uint32_t)(32767-new_weight)));
 return (int16_t)(v>32767?32767:v< -32767?-32767:v);
}
static int total(const original_chicago_fx2_kernel *k){return k&&k->process&&k->parameter&&k->reset&&k->begin&&k->retire&&k->tempo&&k->activity;}
int original_chicago_fx2_wrapper_init(original_chicago_fx2_wrapper *s,const original_chicago_fx2_owner *owner){
 if(!s||!owner||!owner->find||!owner->reset_all)return 0;
 memset(s,0,sizeof *s);s->owner=*owner;s->switch_current=s->switch_pending=s->switch_dirty=s->type_dirty=1;
 s->pending_type=12;s->activity_counter=-1;s->tempo_times10=1200;return 1;
}
int original_chicago_fx2_wrapper_init_cold(original_chicago_fx2_wrapper *s,const original_chicago_fx2_cold_owner *owner){
 if(!s||!owner||!owner->supports||!owner->select_cold||!owner->discard||!owner->supports(owner->context,0))return 0;
 memset(s,0,sizeof *s);s->cold_owner=*owner;s->cold_policy=1;
 s->switch_current=s->switch_pending=s->switch_dirty=s->type_dirty=1;
 s->activity_counter=-1;s->tempo_times10=1200;return 1;
}
int original_chicago_fx2_wrapper_supports(const original_chicago_fx2_wrapper *s,unsigned type){
 if(!s||type>29)return 0;
 if(s->cold_policy)return s->cold_owner.supports&&s->cold_owner.supports(s->cold_owner.context,type);
 return s->owner.find&&total(s->owner.find(s->owner.context,type));
}
int original_chicago_fx2_wrapper_set(original_chicago_fx2_wrapper *s,unsigned p,unsigned v){
 if(!s||p>3||(p==0&&!original_chicago_fx2_wrapper_supports(s,v))||(p==1&&v>1)||(p>=2&&v>127))return 0;
 if(p==0){if(s->pending_type!=v){s->pending_type=(uint8_t)v;s->type_dirty=1;}return 1;}
 if(p==1){s->switch_pending=(uint8_t)(v==1);s->switch_dirty=1;return 1;}
 if(p==2)s->edit1=(uint8_t)v;else s->edit2=(uint8_t)v;
 if(s->active)s->active->parameter(s->active->context,p,v);
 return 1;
}
int original_chicago_fx2_wrapper_tempo(original_chicago_fx2_wrapper *s,int32_t tempo){
 if(!s||tempo<200||tempo>4000)return 0;
 s->tempo_times10=tempo;
 if(s->active)s->active->tempo(s->active->context,tempo);
 return 1;
}
void original_chicago_fx2_wrapper_reset(original_chicago_fx2_wrapper *s){
 if(!s)return;
 s->transitioning=0;
 s->switch_current=s->switch_pending;
 s->wet_target=s->switch_current?32767:0;s->wet_accumulator=s->wet_target*32768;
 if(s->cold_policy){if(s->active)s->active->reset(s->active->context);}
 else s->owner.reset_all(s->owner.context);
}
int original_chicago_fx2_wrapper_process(original_chicago_fx2_wrapper *s,original_chicago_fx2_workspace *w,int16_t *left,int16_t *right,uint32_t n){
 if(!s||!w||n>w->capacity||w->capacity>4096||(n&&(!left||!right||left==right||!w->new_left||!w->new_right||!w->old_left||!w->old_right)))return 0;
 const original_chicago_fx2_kernel *next=s->active;
 if(s->type_dirty){
  if(s->cold_policy){
   if(!original_chicago_fx2_wrapper_supports(s,s->pending_type))return 0;
   /* select_cold retires before overwriting its single object. No pointer to
    * the discarded algorithm may remain in the wrapper or run afterwards. */
   s->active=s->old=NULL;s->transitioning=0;
   next=s->cold_owner.select_cold(s->cold_owner.context,s->pending_type);
  }else next=s->owner.find(s->owner.context,s->pending_type);
  if(!total(next))return 0;
 }
 if(s->switch_dirty){s->switch_current=s->switch_pending;s->wet_target=s->switch_current?32767:0;s->switch_dirty=0;}
 if(s->type_dirty){
  s->old=s->active;s->active=next;s->progress=32767;s->transitioning=1;
  next->begin(next->context);s->activity_counter=0;s->type_dirty=0;
  next->reset(next->context);next->parameter(next->context,2,s->edit1);next->parameter(next->context,3,s->edit2);
  next->tempo(next->context,s->tempo_times10);next->activity(next->context,1);
 }
 for(uint32_t i=0;i<n;i++){w->new_left[i]=w->old_left[i]=left[i];w->new_right[i]=w->old_right[i]=right[i];}
 if(!n&&!s->transitioning&&s->active)s->active->process(s->active->context,w->new_left,w->new_right,0);
 uint32_t offset=0;
 while(offset<n){
  uint32_t chunk=n-offset;
  if(s->transitioning){
   int32_t q=s->progress/743;if(q<=1)q=1;uint32_t cap=743u*((uint32_t)q+1u);if(chunk>cap)chunk=cap;
   if(s->old)s->old->process(s->old->context,w->old_left+offset,w->old_right+offset,chunk);
   if(s->active)s->active->process(s->active->context,w->new_left+offset,w->new_right+offset,chunk);
   for(uint32_t i=0;i<chunk;i++){
    int32_t old_weight=(int32_t)(((uint32_t)s->progress*(uint32_t)s->progress)>>15);
    w->new_left[offset+i]=blend(w->old_left[offset+i],w->new_left[offset+i],32767-old_weight);
    w->new_right[offset+i]=blend(w->old_right[offset+i],w->new_right[offset+i],32767-old_weight);
    s->progress-=743;if(s->progress<0)s->progress=0;
   }
   if(s->progress<1){s->transitioning=0;if(s->old)s->old->retire(s->old->context);}
  }else if(s->active)s->active->process(s->active->context,w->new_left+offset,w->new_right+offset,chunk);
  offset+=chunk;
 }
 for(uint32_t i=0;i<n;i++){
  int32_t g=asr15(s->wet_accumulator);
  if(g!=s->wet_target){s->wet_accumulator=si((uint32_t)s->wet_accumulator+(uint32_t)(s->wet_target-g)*102u);g=asr15(s->wet_accumulator);}
  left[i]=blend(left[i],w->new_left[i],g);right[i]=blend(right[i],w->new_right[i],g);
 }
 if(s->activity_counter>=0){s->activity_counter+=(int32_t)n;if(s->activity_counter>=44){s->activity_counter=-1;if(s->active)s->active->activity(s->active->context,0);}}
 return 1;
}
int original_chicago_fx2_wrapper_advance_zero(original_chicago_fx2_wrapper *s,uint32_t n,
 int16_t kernel[2],int16_t mixed[2]){
 if(!s||!n||n>4096||!kernel||!mixed||!s->cold_policy||!s->active||!s->cold_owner.advance_zero||
    s->type_dirty||s->switch_dirty||s->transitioning||s->activity_counter>=0||
    asr15(s->wet_accumulator)!=s->wet_target)return 0;
 int16_t value[2];
 if(!s->cold_owner.advance_zero(s->cold_owner.context,n,value))return 0;
 kernel[0]=value[0];kernel[1]=value[1];
 mixed[0]=blend(0,value[0],s->wet_target);mixed[1]=blend(0,value[1],s->wet_target);
 return 1;
}
