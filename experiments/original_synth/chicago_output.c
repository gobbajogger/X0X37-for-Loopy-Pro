/* SPDX-License-Identifier: GPL-3.0-only */
#include "chicago_output.h"
#include "miami_fx_conversion.h"
#include "binary32_classify.h"
#include "binary32_math.h"
#include <limits.h>
#include <math.h>
#include <string.h>
static int32_t signed32(uint32_t u){return u<=INT32_MAX?(int32_t)u:(int32_t)((int64_t)u-4294967296LL);}
static int32_t asr15(int32_t x){uint32_t u=(uint32_t)x;return signed32((u>>15)|(x<0?0xfffe0000u:0));}
static int32_t product(int32_t a,int32_t b){return asr15(signed32((uint32_t)a*(uint32_t)b));}
static int16_t mix(int16_t old,int16_t newer,int32_t new_weight){
 int32_t value=product(newer,new_weight)+product(old,32767-new_weight);
 if(value>32767)value=32767;
 if(value< -32767)value=-32767;
 return (int16_t)value;
}
int original_chicago_drive_wrapper_init(original_chicago_drive_wrapper *s,unsigned d,unsigned g,unsigned on){
 if(!s||d>127||g>127||on>1)return 0;
 memset(s,0,sizeof *s);original_chicago_drive_init(&s->drive);
 s->edit1=(uint8_t)d;s->edit2=(uint8_t)g;s->switch_current=1;s->switch_pending=(uint8_t)on;s->switch_dirty=1;
 s->activity_counter=-1;s->initial_selection_pending=1;return 1;
}
int original_chicago_drive_wrapper_set(original_chicago_drive_wrapper *s,unsigned p,unsigned v){
 if(!s||p>3||(p==0&&v!=19)||(p==1&&v>1)||(p>=2&&v>127))return 0;
 if(p==0)return 1;
 if(p==1){s->switch_pending=(uint8_t)v;s->switch_dirty=1;return 1;}
 if(p==2)s->edit1=(uint8_t)v;else s->edit2=(uint8_t)v;
 if(s->selected)return original_chicago_drive_set(&s->drive,p,v);
 return 1;
}
int original_chicago_drive_wrapper_events(original_chicago_drive_wrapper *s,const original_chicago_drive_event *events,uint32_t count){
 if(!s||(count&&!events))return 0;
 for(uint32_t i=1;i<count;i++)if(events[i-1].offset>events[i].offset)return 0;
 s->queue=(original_chicago_drive_queue){events,count,0};return 1;
}
void original_chicago_drive_wrapper_direct_event(original_chicago_drive_wrapper *s,unsigned type){if(s&&type==5)original_chicago_drive_reset(&s->drive);}
void original_chicago_drive_wrapper_reset(original_chicago_drive_wrapper *s){
 if(!s)return;
 s->transitioning=0;s->switch_current=s->switch_pending;s->wet_target=s->switch_current?32767:0;s->wet_accumulator=s->wet_target*32768;
 original_chicago_drive_reset(&s->drive);
}
static void drive_invocation_from(original_chicago_drive_wrapper *s,const int16_t *in_left,const int16_t *in_right,int16_t *left,int16_t *right,uint32_t n){
 uint32_t local=0;
 while(local<n){
  while(s->queue.cursor<s->queue.count&&s->queue.events[s->queue.cursor].offset<=local){
   if(s->queue.events[s->queue.cursor].type==4)original_chicago_drive_reset(&s->drive);
   s->queue.cursor++;
  }
  uint32_t end=n;
  if(s->queue.cursor<s->queue.count&&s->queue.events[s->queue.cursor].offset<end)end=s->queue.events[s->queue.cursor].offset;
  (void)original_chicago_drive_process(&s->drive,in_left+local,in_right+local,left+local,right+local,end-local);local=end;
 }
}
static void drive_invocation(original_chicago_drive_wrapper *s,int16_t *left,int16_t *right,uint32_t n){drive_invocation_from(s,left,right,left,right,n);}
int original_chicago_drive_wrapper_process(original_chicago_drive_wrapper *s,int16_t *left,int16_t *right,int16_t *wet_left,int16_t *wet_right,uint32_t n,uint32_t capacity){
 if(!s||n>capacity||capacity>4096||(n&&(!left||!right||left==right||!wet_left||!wet_right||wet_left==wet_right)))return 0;
 if(s->switch_dirty){s->switch_current=s->switch_pending;s->wet_target=s->switch_current?32767:0;s->switch_dirty=0;}
 if(s->initial_selection_pending){
  s->progress=32767;s->transitioning=1;s->selected=1;s->initial_selection_pending=0;s->activity_counter=0;
  original_chicago_drive_reset(&s->drive);
  (void)original_chicago_drive_set(&s->drive,2,s->edit1);(void)original_chicago_drive_set(&s->drive,3,s->edit2);
  (void)original_chicago_drive_activity(&s->drive,1);
 }
 uint32_t offset=0;
 if(!s->transitioning&&s->selected&&n){
  /* The whole block is one chunk: the drive reads the dry input and
   * writes wet directly, the same as copying dry to wet first. */
  drive_invocation_from(s,left,right,wet_left,wet_right,n);offset=n;
 }else for(uint32_t i=0;i<n;i++){wet_left[i]=left[i];wet_right[i]=right[i];}
 while(offset<n){
  uint32_t chunk=n-offset;
  if(s->transitioning){
   int32_t q=s->progress/743;if(q<=1)q=1;uint32_t cap=743u*((uint32_t)q+1u);if(chunk>cap)chunk=cap;
   drive_invocation(s,wet_left+offset,wet_right+offset,chunk);
   for(uint32_t i=0;i<chunk;i++){
    int32_t old_weight=(int32_t)(((uint32_t)s->progress*(uint32_t)s->progress)>>15);
    wet_left[offset+i]=mix(left[offset+i],wet_left[offset+i],32767-old_weight);
    wet_right[offset+i]=mix(right[offset+i],wet_right[offset+i],32767-old_weight);
    s->progress-=743;if(s->progress<0)s->progress=0;
   }
   if(s->progress<1)s->transitioning=0;
  }else if(s->selected)drive_invocation(s,wet_left+offset,wet_right+offset,chunk);
  offset+=chunk;
 }
 int32_t settled=asr15(s->wet_accumulator);
 if(settled==s->wet_target&&(settled==0||settled==32767)){
  /* A settled weight does not change in this block. With a weight of 0 or
   * 32767, one of the two products in mix() is product(x,0)==0, so only
   * the other product is needed. The result is the same. */
  const int16_t *src_l=settled?wet_left:left,*src_r=settled?wet_right:right;
  for(uint32_t i=0;i<n;i++){
   int32_t a=product(src_l[i],32767),b=product(src_r[i],32767);
   left[i]=(int16_t)(a< -32767?-32767:a);right[i]=(int16_t)(b< -32767?-32767:b);
  }
 }else for(uint32_t i=0;i<n;i++){
  int32_t g=asr15(s->wet_accumulator);
  if(g!=s->wet_target){s->wet_accumulator=signed32((uint32_t)s->wet_accumulator+(uint32_t)(s->wet_target-g)*102u);g=asr15(s->wet_accumulator);}
  left[i]=mix(left[i],wet_left[i],g);right[i]=mix(right[i],wet_right[i],g);
 }
 if(s->activity_counter>=0){s->activity_counter+=(int32_t)n;if(s->activity_counter>=44){s->activity_counter=-1;(void)original_chicago_drive_activity(&s->drive,0);}}
 return 1;
}
int original_chicago_output_init(original_chicago_output *s,unsigned d,unsigned g,unsigned on){
 if(!s||d>127||g>127||on>1)return 0;
 memset(s,0,sizeof *s);(void)original_chicago_drive_wrapper_init(&s->fx1,d,g,on);original_master_init(&s->master);
 s->reset_countdown[0]=s->reset_countdown[1]=-1;s->process_slot[0]=s->process_slot[1]=s->enable_slot[0]=s->enable_slot[1]=1;return 1;
}
static uint32_t positive_fcvtzu(float x){
 uint32_t u;memcpy(&u,&x,4);if(u>=0x4f800000u)return UINT32_MAX;if(u<0x3f800000u)return 0;
 unsigned e=(u>>23)-127u;uint32_t v=(u&0x7fffffu)|0x800000u;return e>=23?v<<(e-23):v>>(23-e);
}
int original_chicago_output_suspend(original_chicago_output *s,unsigned slot,unsigned suspend,float fs){
 if(!s||slot>1||!original_binary32_isfinite(fs)||fs<8000)return 0;
 if(suspend){float n=(fs*20.0f)/5.0f;s->reset_countdown[slot]=signed32(positive_fcvtzu(n)&0xfffffff0u);s->process_slot[slot]=0;}
 else {s->reset_countdown[slot]=-1;s->process_slot[slot]=1;}
 return 1;
}
int original_chicago_output_enable(original_chicago_output *s,unsigned slot,unsigned enabled){if(!s||slot>1||enabled>1)return 0;s->enable_slot[slot]=(uint8_t)enabled;return 1;}
void original_chicago_output_mute(original_chicago_output *s,uint8_t byte){if(s)original_master_mute(&s->master,byte);}
static int crosses(int32_t count,uint32_t n){return count>0&&count<=(int32_t)n;}
int original_chicago_output_zero_state_ready(const original_chicago_output *s,
 const original_chicago_output_workspace *w,float volume,const uint32_t *il,
 const uint32_t *ir,uint32_t *ol,uint32_t *orr,uint32_t n,unsigned fz){
 if(!s||!n||!w||!w->left||!w->right||!w->wet_left||!w->wet_right||
    !il||!ir||!ol||!orr||ol==orr||n>w->capacity||w->capacity>4096||fz>1||
    !original_binary32_isfinite(volume)||volume<0||volume>1||
    !original_binary32_isfinite(s->master.current)||s->master.current<0||s->master.current>4)return 0;
 for(unsigned i=0;i<2;i++)if(s->enable_slot[i]!=1||s->process_slot[i]!=1||s->reset_countdown[i]>0)return 0;
 const original_chicago_drive_wrapper *x=&s->fx1;
 if(x->switch_dirty||x->initial_selection_pending||!x->selected||x->transitioning||
    x->activity_counter>=0||x->queue.cursor<x->queue.count||asr15(x->wet_accumulator)!=x->wet_target)return 0;
 const original_chicago_drive *d=&x->drive;
 int32_t threshold=d->target_drive<19660?19660-d->target_drive:0;
 if(asr15(d->drive)!=d->target_drive||asr15(d->gain)!=d->target_gain||asr15(d->threshold)!=threshold||
    d->previous_driven[0]||d->previous_driven[1]||d->previous_nonlinear[0]||d->previous_nonlinear[1])return 0;
 return 1;
}
int original_chicago_output_zero_ready(const original_chicago_output *s,
 const original_chicago_output_workspace *w,float volume,const uint32_t *il,
 const uint32_t *ir,uint32_t *ol,uint32_t *orr,uint32_t n,unsigned fz){
 if(!original_chicago_output_zero_state_ready(s,w,volume,il,ir,ol,orr,n,fz))return 0;
 for(uint32_t i=0;i<n;i++)if((il[i]&0x7fffffffu)||(ir[i]&0x7fffffffu))return 0;
 return 1;
}
int original_chicago_output_process(original_chicago_output *s,original_chicago_output_workspace *w,const original_chicago_fx2 *fx2,float volume,const uint32_t *il,const uint32_t *ir,uint32_t *ol,uint32_t *orr,uint32_t n,unsigned fz){
 if(!s)return 0;
 if(!n)return 1;
 if(!w||!w->left||!w->right||!w->wet_left||!w->wet_right||!il||!ir||!ol||!orr||ol==orr||n>w->capacity||w->capacity>4096||fz>1||!original_binary32_isfinite(volume)||volume<0||volume>1)return 0;
 if(s->enable_slot[1]&&((s->process_slot[1]&&(!fx2||!fx2->process))||(crosses(s->reset_countdown[1],n)&&(!fx2||!fx2->reset))))return 0;
 for(uint32_t i=0;i<n;i++){w->left[i]=(int16_t)miami_fx_input_fast(il[i]);w->right[i]=(int16_t)miami_fx_input_fast(ir[i]);}
 for(unsigned slot=0;slot<2;slot++){
  if(!s->enable_slot[slot])continue;
  if(s->reset_countdown[slot]>0){s->reset_countdown[slot]-=(int32_t)n;if(s->reset_countdown[slot]<=0){s->reset_countdown[slot]=-1;if(slot==0)original_chicago_drive_wrapper_reset(&s->fx1);else fx2->reset(fx2->context);}}
  if(!s->process_slot[slot])continue;
  if(slot==0){if(!original_chicago_drive_wrapper_process(&s->fx1,w->left,w->right,w->wet_left,w->wet_right,n,w->capacity))return 0;}
  else fx2->process(fx2->context,w->left,w->right,n);
 }
 return original_master_process_i16(&s->master,volume,w->left,w->right,ol,orr,n);
}
