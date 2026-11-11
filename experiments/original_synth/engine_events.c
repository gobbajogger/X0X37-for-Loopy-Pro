/* SPDX-License-Identifier: GPL-3.0-only */
#include "engine_events.h"
#include "binary32_math.h"
#include "binary32_classify.h"
#include "chicago_key_admission.h"
static int event_valid(const original_engine_event *e,unsigned macros,uint32_t frames){
 if(e->frame>=frames||e->kind>ORIGINAL_EVENT_MUTE)return 0;
 if(e->kind==ORIGINAL_EVENT_NOTE)return e->parameter<128&&e->value<128;
 if(e->kind==ORIGINAL_EVENT_MACRO){float x=original_math_float(e->value);return e->parameter<macros&&original_binary32_isfinite(x)&&x>=0&&x<=1;}
 if(e->kind==ORIGINAL_EVENT_DAMPER)return !e->parameter&&e->value<128;
 if(e->kind==ORIGINAL_EVENT_MUTE)return !e->parameter&&e->value<256;
 return !e->parameter&&!e->value;
}
int original_engine_events_bind(original_engine_events *s,const original_engine_event *events,uint32_t n,uint32_t frames,original_miami_control_owner *miami,original_chicago_control_owner *chicago){
 if(!s||((miami==0)==(chicago==0))||(n&&!events)||(!frames&&n)||(miami&&!miami->status)||(chicago&&!chicago->status))return 0;
 original_chicago_scheduler *scheduler=miami?miami->scheduler:chicago->scheduler;
 if(!scheduler||!scheduler->context||!scheduler->callbacks||scheduler->callbacks->voice!=(miami?original_miami_voice_owner_callbacks():original_chicago_voice_owner_callbacks()))return 0;
 if(miami?!((original_miami_voice_owner *)scheduler->context)->status:!((original_chicago_voice_owner *)scheduler->context)->status)return 0;
 if(!scheduler->allocator||!original_chicago_canonical_keys_valid(scheduler->allocator->keys))return 0;
 for(uint32_t i=0;i<n;i++)if(!event_valid(events+i,miami?18u:22u,frames)||(i&&events[i].frame<events[i-1].frame))return 0;
 *s=(original_engine_events){events,n,0,frames,0,miami,chicago,1};return 1;
}
int original_engine_events_frame(original_engine_events *s,uint32_t frame){
 if(!s||!s->status||frame!=s->next_frame||frame>=s->frames)return 0;
 original_chicago_scheduler *scheduler=s->miami?s->miami->scheduler:s->chicago->scheduler;
 if((s->miami?!s->miami->status:!s->chicago->status)||(s->miami?!((original_miami_voice_owner *)scheduler->context)->status:!((original_chicago_voice_owner *)scheduler->context)->status)){s->status=0;return 0;}
 while(s->cursor<s->count&&s->events[s->cursor].frame==frame){
  const original_engine_event *e=s->events+s->cursor;int result=1;
  switch(e->kind){
   case ORIGINAL_EVENT_NOTE:result=original_chicago_scheduler_note(scheduler,(int32_t)e->parameter,(int32_t)e->value);break;
   case ORIGINAL_EVENT_MACRO:result=s->miami?original_miami_control_owner_edit(s->miami,e->parameter,original_math_float(e->value)):original_chicago_control_owner_edit(s->chicago,e->parameter,original_math_float(e->value));break;
   case ORIGINAL_EVENT_DAMPER:result=original_chicago_scheduler_damper(scheduler,e->value);break;
   case ORIGINAL_EVENT_ALL_OFF:original_chicago_allocator_all_notes_off(scheduler->allocator,scheduler->callbacks->voice,scheduler->context);break;
   case ORIGINAL_EVENT_CLEAR_KEYS:original_chicago_allocator_clear_keys(scheduler->allocator,scheduler->callbacks->voice,scheduler->context);break;
   case ORIGINAL_EVENT_MUTE:original_master_mute(s->miami?&s->miami->output->master:&s->chicago->output->output.master,(uint8_t)e->value);break;
   default:result=0;break;
  }
  if(!result||(s->miami?!((original_miami_voice_owner *)scheduler->context)->status:!((original_chicago_voice_owner *)scheduler->context)->status)){s->status=0;return 0;}
  s->cursor++;
 }
 s->next_frame++;return 1;
}
