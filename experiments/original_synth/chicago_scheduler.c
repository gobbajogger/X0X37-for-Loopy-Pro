/* SPDX-License-Identifier: GPL-3.0-only */
#include "chicago_scheduler.h"
#include "chicago_clock_span.h"
typedef struct {
 original_chicago_scheduler *owner;
 original_chicago_gate gate;
 original_chicago_selector_controls controls;
} selector_dispatch;
static void load_selector(selector_dispatch *d){
 original_chicago_allocator *a=d->owner->allocator;
 original_chicago_selector *p=d->owner->selector;
 p->cursor=a->scan_index;p->last_key=a->previous_index;
 p->octave=a->octave;p->direction=(int32_t)a->direction;
 d->gate=(original_chicago_gate){a->gate_duration,a->gate_remaining,a->last_note};
}
static void save_selector(selector_dispatch *d){
 original_chicago_allocator *a=d->owner->allocator;
 const original_chicago_selector *p=d->owner->selector;
 a->scan_index=p->cursor;a->previous_index=p->last_key;
 a->octave=p->octave;a->direction=(uint32_t)p->direction;
 a->gate_duration=d->gate.duration;a->gate_remaining=d->gate.remaining;a->last_note=d->gate.last_note;
}
static void release_selected(void *context){
 selector_dispatch *d=context;original_chicago_scheduler *s=d->owner;
 save_selector(d);
 original_chicago_allocator_release(s->allocator,-1,s->callbacks->voice,s->context);
 load_selector(d);
}
static void spawn_selected(void *context,int32_t note,uint32_t velocity){
 selector_dispatch *d=context;original_chicago_scheduler *s=d->owner;
 save_selector(d);
 original_chicago_allocator_spawn(s->allocator,note,(int32_t)velocity,s->callbacks->voice,s->context);
 load_selector(d);
}
static const original_chicago_selector_callbacks select_callbacks={release_selected,spawn_selected};
static void held_cleanup(void *context){
 original_chicago_scheduler *s=context;original_chicago_allocator_cleanup_held(s->allocator);
}
static void voice_cleanup(void *context){
 original_chicago_scheduler *s=context;
 original_chicago_allocator_cleanup_voices(s->allocator,s->callbacks->voice,s->context);
}
static void note_off(void *context,int32_t note){
 original_chicago_scheduler *s=context;
 (void)original_chicago_allocator_note_off(s->allocator,note,s->callbacks->voice,s->context);
}
static void finish_disable(void *context){
 original_chicago_scheduler *s=context;
 original_chicago_allocator_finish_disable(s->allocator,s->callbacks->voice,s->context);
}
static uint32_t current_velocity(void *context,int32_t note){
 original_chicago_scheduler *s=context;const chicago_arp_control *k=s->allocator->keys;
 /* prepare/damper reject out-of-contract raw addresses before mutation. */
 return note<128?k->physical[(unsigned)note]:k->latched[(unsigned)note-128];
}
static const chicago_arp_control_callbacks control_callbacks={held_cleanup,note_off,finish_disable,current_velocity};
static const chicago_arp_damper_callbacks damper_callbacks={held_cleanup,voice_cleanup};
static int valid_control(const original_chicago_scheduler *s){
 const chicago_arp_control_input *in=s->control_input;
 const chicago_arp_control *k=s->allocator->keys;
 return in->enable<=1&&in->latch<=1&&k->committed_enable<=1&&k->effective_latch<=1&&k->current_note>=0&&k->current_note<=255;
}
int original_chicago_scheduler_bind(original_chicago_scheduler *s,original_chicago_allocator *a,original_chicago_selector *selector,chicago_arp_clock *clock,const chicago_arp_clock_input *clock_input,chicago_arp_control_input *control_input,const original_chicago_scheduler_callbacks *cb,void *context){
 if(!s||!a||!a->keys||!selector||!clock||!clock_input||!control_input||!cb||!cb->voice||!cb->voice->enabled||!cb->voice->vca_state||!cb->voice->set_voice_state||!cb->voice->note||!cb->mg1_tick||!cb->mg2_tempo_notify)return 0;
 *s=(original_chicago_scheduler){a,selector,clock,clock_input,control_input,cb,context,1};return 1;
}
int original_chicago_scheduler_prepare(original_chicago_scheduler *s){
 if(!valid_control(s))return 0;
 s->allocator->pedal_byte=s->control_input->damper_byte;
 chicago_arp_control_prepare(s->allocator->keys,s->control_input,&control_callbacks,s);return 1;
}
int original_chicago_scheduler_damper(original_chicago_scheduler *s,uint32_t value){
 if(value>127||!valid_control(s))return 0;
 s->allocator->pedal_byte=value<64?0:127;
 chicago_arp_damper_apply(s->allocator->keys,s->control_input,value,s->allocator->hold,&damper_callbacks,&control_callbacks,s);return 1;
}
int original_chicago_scheduler_note(original_chicago_scheduler *s,int32_t key,int32_t velocity){
 return original_chicago_allocator_canonical_note(s->allocator,key,velocity,s->callbacks->voice,s->context);
}
int original_chicago_scheduler_select(original_chicago_scheduler *s){
 selector_dispatch d;d.owner=s;load_selector(&d);
 d.controls=(original_chicago_selector_controls){s->allocator->arp_mode,s->allocator->arp_range,s->allocator->mode};
 int result=original_chicago_selector_step(s->selector,s->allocator->keys,&d.gate,&d.controls,&select_callbacks,&d);
 save_selector(&d);return result;
}
static void clock_step(void *context){
 original_chicago_scheduler *s=context;s->allocator->mg2_key_reset=s->clock->key_reset;
 int result=original_chicago_scheduler_select(s);
 if(result<=0&&s->dispatch_result>0)s->dispatch_result=result;
 s->clock->key_reset=s->allocator->mg2_key_reset;
}
static void clock_notify(void *context){
 original_chicago_scheduler *s=context;s->allocator->mg2_key_reset=s->clock->key_reset;
 s->callbacks->mg2_tempo_notify(s->context,s->allocator);
 s->clock->key_reset=s->allocator->mg2_key_reset;
}
static const chicago_arp_clock_callbacks clock_callbacks={clock_step,clock_notify};
int original_chicago_scheduler_after_events(original_chicago_scheduler *s){
 original_chicago_allocator *a=s->allocator;
 original_chicago_gate g={a->gate_duration,a->gate_remaining,a->last_note};
 original_chicago_gate_tick(&g,a->keys->current_note);
 a->gate_duration=g.duration;a->gate_remaining=g.remaining;a->last_note=g.last_note;
 s->callbacks->mg1_tick(s->context,a);
 s->clock->key_reset=a->mg2_key_reset;s->dispatch_result=1;
 chicago_arp_clock_tick(s->clock,s->clock_input,&clock_callbacks,s);
 a->mg2_key_reset=s->clock->key_reset;return s->dispatch_result;
}
int original_chicago_scheduler_quiet(original_chicago_scheduler *s,chicago_arp_clock *c1,uint32_t frames){
 original_chicago_allocator *a=s->allocator;chicago_arp_clock *c2=s->clock;
 float bpm=s->clock_input->bpm;
 while(frames){
  /* No callback observes interior frames. Jump exact rounded additions up
   * to the first edge of either clock, then dispatch the original sample. */
  uint32_t n=0;float x=c1->accumulator,y=c2->accumulator;
  if(!a->mg1_key_reset&&!a->mg2_key_reset){
   n=original_chicago_clock_span(&x,bpm,c1->threshold,frames);
   uint32_t m=original_chicago_clock_span(&y,bpm,c2->threshold,n);
   if(m<n){x=c1->accumulator;n=original_chicago_clock_span(&x,bpm,c1->threshold,m);}
  }
  if(n){
   c1->accumulator=x;c2->accumulator=y;c1->key_reset=c2->key_reset=0;
   if(a->gate_remaining>0){
    if((uint32_t)a->gate_remaining<=n){a->gate_remaining=0;a->last_note=a->keys->current_note;}
    else a->gate_remaining-=(int32_t)n;
   }
   s->dispatch_result=1;frames-=n;
  }
  if(frames){if(original_chicago_scheduler_after_events(s)<=0)return 0;--frames;}
 }
 return 1;
}
int original_chicago_scheduler_voice_action(original_chicago_scheduler *s,unsigned id,unsigned action){
 if(id>=s->allocator->storage_voices||action>2)return 0;
 if(action==0)return 1;
 if(action==1)return original_chicago_allocator_return_voice(s->allocator,id,s->callbacks->voice,s->context);
 for(int16_t i=s->allocator->voice_active.head;i>=0;i=s->allocator->voice[i].next)if((unsigned)i==id)return 1;
 return 0;
}
