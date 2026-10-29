#include "chicago_arp_control.h"
#include <assert.h>
#include <string.h>
static void copy_physical(chicago_arp_control *s)
{ memcpy(s->latched,s->physical,sizeof s->physical);s->latched_count=s->physical_count; }
void chicago_arp_control_prepare(chicago_arp_control *s,const chicago_arp_control_input *in,
 const chicago_arp_control_callbacks *cb,void *ctx)
{
 assert(s&&in&&cb&&cb->retire_held_records&&cb->note_off&&cb->retire_voices_and_finish&&cb->current_velocity);
 assert(in->enable<=1u&&in->latch<=1u&&s->committed_enable<=1u&&s->effective_latch<=1u);
 if(s->committed_enable!=in->enable){
  if(s->committed_enable==1u){
   s->committed_enable=0;
   cb->retire_held_records(ctx);
   memset(s->physical,0,sizeof s->physical);memset(s->latched,0,sizeof s->latched);
   s->physical_count=0;s->latched_count=0;
   for(int32_t key=0;key<128;++key)cb->note_off(ctx,key);
   cb->retire_voices_and_finish(ctx);
  }else{s->committed_enable=1;copy_physical(s);}
 }
 uint32_t latch=in->latch|(in->damper_byte!=0);
 if(s->effective_latch!=latch){
  if(s->effective_latch==1u){
   s->effective_latch=0;
   if(s->committed_enable==1u){
    copy_physical(s);
    int32_t note=s->current_note;
    if(cb->current_velocity(ctx,note)==0)cb->note_off(ctx,note);
   }
  }else{s->effective_latch=1;copy_physical(s);}
 }
}
void chicago_arp_damper_apply(chicago_arp_control *s,chicago_arp_control_input *in,
 uint32_t value,uint32_t hold,const chicago_arp_damper_callbacks *dc,
 const chicago_arp_control_callbacks *cc,void *ctx)
{
 assert(value<=127u&&s&&in&&dc&&dc->cleanup_held&&dc->cleanup_voices);
 in->damper_byte=value<64u?0u:127u;
 if(value<=63u&&hold==0u){dc->cleanup_held(ctx);dc->cleanup_voices(ctx);}
 chicago_arp_control_prepare(s,in,cc,ctx);
}
void chicago_arp_clock_prepare(chicago_arp_clock *s,float fs,float base,float times)
{ float a=fs*2.5f;float b=a*base;s->threshold=b*times; }
void chicago_arp_clock_key(chicago_arp_clock *s,uint32_t key_sync,uint32_t held_count)
{ if(key_sync==1u&&held_count==1u)s->key_reset=1; }
void chicago_arp_clock_tick(chicago_arp_clock *s,const chicago_arp_clock_input *in,
 const chicago_arp_clock_callbacks *cb,void *ctx)
{
 assert(s&&in&&cb&&cb->step&&cb->tempo_sync_notify);
 if(s->key_reset){s->accumulator=0;s->key_reset=0;cb->step(ctx);}
 s->accumulator=s->accumulator+in->bpm;
 if(s->accumulator>=s->threshold){
  s->accumulator=s->accumulator-s->threshold;
  if(in->tempo_sync==1u){cb->tempo_sync_notify(ctx);cb->step(ctx);}
 }
}
