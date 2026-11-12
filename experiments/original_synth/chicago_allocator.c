/* SPDX-License-Identifier: GPL-3.0-only */
#include "chicago_allocator.h"
#include <assert.h>
#include <string.h>
static original_record_list empty(void){original_record_list l={-1,-1,0};return l;}
static void hv_remove(original_chicago_allocator *s,original_record_list *l,int16_t i){
 original_held_record *r=&s->held[i];int16_t p=r->prev,n=r->next;
 if(p<0)l->head=n;else s->held[p].next=n;
 if(n<0)l->tail=p;else s->held[n].prev=p;
 r->prev=r->next=-1;l->count--;
}
static void hv_append(original_chicago_allocator *s,original_record_list *l,int16_t i){
 original_held_record *r=&s->held[i];r->prev=l->tail;r->next=-1;
 if(l->tail<0)l->head=i;else s->held[l->tail].next=i;l->tail=i;l->count++;
}
static void vv_remove(original_chicago_allocator *s,original_record_list *l,int16_t i){
 original_voice_record *r=&s->voice[i];int16_t p=r->prev,n=r->next;
 if(p<0)l->head=n;else s->voice[p].next=n;
 if(n<0)l->tail=p;else s->voice[n].prev=p;
 r->prev=r->next=-1;l->count--;
}
static void vv_append(original_chicago_allocator *s,original_record_list *l,int16_t i){
 original_voice_record *r=&s->voice[i];r->prev=l->tail;r->next=-1;
 if(l->tail<0)l->head=i;else s->voice[l->tail].next=i;l->tail=i;l->count++;
}
static void voice_to_free(original_chicago_allocator *s,int16_t i){vv_remove(s,&s->voice_active,i);vv_append(s,&s->voice_free,i);}
static void held_to_free(original_chicago_allocator *s,int16_t i){hv_remove(s,&s->held_active,i);hv_append(s,&s->held_free,i);}
static void request(original_chicago_allocator *s,int16_t i,unsigned full,const original_allocator_callbacks *cb,void *ctx){
 original_voice_record r=s->voice[i];cb->note(ctx,(unsigned)i,&r,full,15,s->last_note);
}
static void release_record(original_chicago_allocator *s,int16_t i,const original_allocator_callbacks *cb,void *ctx){
 if(cb->vca_state(ctx,(unsigned)i)!=6)cb->set_voice_state(ctx,(unsigned)i,0,3,3);
 voice_to_free(s,i);
}
void original_chicago_allocator_reset_lists(original_chicago_allocator *s){
 /* Remove list ownership before rebuilding; untouchedoutofcapacity records
  * remain unowned. No additional physical/latched clear is performed. */
 for(int16_t i=s->voice_active.head;i>=0;){int16_t n=s->voice[i].next;s->voice[i].prev=s->voice[i].next=-1;i=n;}
 for(int16_t i=s->voice_free.head;i>=0;){int16_t n=s->voice[i].next;s->voice[i].prev=s->voice[i].next=-1;i=n;}
 s->voice_active=empty();s->voice_free=empty();
 for(unsigned i=0;i<s->capacity;i++){
  original_voice_record *v=&s->voice[i];v->note=-1;v->velocity=0;v->marked=0;v->unison_flag=1;v->coordinate=0;vv_append(s,&s->voice_free,(int16_t)i);
 }
 for(int16_t i=s->held_active.head;i>=0;){int16_t n=s->held[i].next;s->held[i].prev=s->held[i].next=-1;i=n;}
 for(int16_t i=s->held_free.head;i>=0;){int16_t n=s->held[i].next;s->held[i].prev=s->held[i].next=-1;i=n;}
 s->held_active=empty();s->held_free=empty();
 for(unsigned i=0;i<128;i++){s->held[i].note=-1;s->held[i].velocity=0;s->held[i].marked=0;hv_append(s,&s->held_free,(int16_t)i);}
 s->keys->current_note=s->last_note=60;s->gate_remaining=0;
}
int original_chicago_allocator_bind(original_chicago_allocator *s,chicago_arp_control *keys,original_held_record held[128],original_voice_record *voice,unsigned storage,unsigned capacity,unsigned unison,unsigned mode,unsigned hold,unsigned legato){
 if(!s||!keys||!held||!voice||storage<1||storage>128||capacity<1||capacity>storage||unison<1||unison>16||unison>capacity||mode>3||hold>1||legato>1)return 0;
 original_chicago_allocator n;memset(&n,0,sizeof n);n.held=held;n.voice=voice;n.keys=keys;n.storage_voices=storage;n.capacity=capacity;n.unison_count=unison;n.mode=mode;n.hold=hold;n.legato=legato;
 n.held_active=n.held_free=n.voice_active=n.voice_free=empty();*s=n;original_chicago_allocator_reset_lists(s);return 1;
}
unsigned original_chicago_allocator_acquire(original_chicago_allocator *s,const original_allocator_callbacks *cb,void *ctx){
 assert(s&&cb&&cb->enabled&&cb->vca_state&&cb->set_voice_state&&cb->note);
 if(s->voice_free.head<0){
  int16_t i=s->voice_active.head;assert(i>=0);
  if(cb->enabled(ctx,(unsigned)i)&&cb->vca_state(ctx,(unsigned)i)!=6)cb->set_voice_state(ctx,(unsigned)i,0,-1,4);
  voice_to_free(s,i);
 }
 int16_t choice=-1;unsigned limit=s->capacity;
 for(int16_t i=s->voice_free.head;i>=0;i=s->voice[i].next)if(!cb->enabled(ctx,(unsigned)i)&&(unsigned)i<limit){choice=i;limit=(unsigned)i;}
 unsigned fallback=choice<0;if(fallback)choice=s->voice_free.head;
 vv_remove(s,&s->voice_free,choice);
 if(fallback&&cb->enabled(ctx,(unsigned)choice)&&cb->vca_state(ctx,(unsigned)choice)!=6)cb->set_voice_state(ctx,(unsigned)choice,0,-1,4);
 vv_append(s,&s->voice_active,choice);
 if(!fallback&&cb->enabled(ctx,(unsigned)choice)&&cb->vca_state(ctx,(unsigned)choice)!=6)cb->set_voice_state(ctx,(unsigned)choice,0,-1,4);
 return (unsigned)choice;
}
int original_chicago_allocator_return_voice(original_chicago_allocator *s,unsigned id,const original_allocator_callbacks *cb,void *ctx){
 for(int16_t i=s->voice_active.head;i>=0;i=s->voice[i].next)if((unsigned)i==id){
  if(cb->enabled(ctx,id)&&cb->vca_state(ctx,id)!=6)cb->set_voice_state(ctx,id,0,-1,4);
  voice_to_free(s,i);return 1;
 }
 return 0;
}
void original_chicago_allocator_spawn(original_chicago_allocator *s,int32_t note,int32_t velocity,const original_allocator_callbacks *cb,void *ctx){
 if((int32_t)s->unison_count<1)return;
 unsigned id=original_chicago_allocator_acquire(s,cb,ctx),count=s->unison_count;
 original_voice_record *r=&s->voice[id];r->note=note;r->velocity=velocity;r->marked=0;r->coordinate=count<2?0:.5f;r->unison_flag=(count&1)?1:2;
 request(s,(int16_t)id,1,cb,ctx);
 for(unsigned i=1;i<s->unison_count;i++){
  id=original_chicago_allocator_acquire(s,cb,ctx);count=s->unison_count;r=&s->voice[id];r->note=note;r->velocity=velocity;r->marked=0;
  r->coordinate=count<2?0:.5f-(float)i/(float)(count-1);r->unison_flag=2u&~(2u*i);request(s,(int16_t)id,1,cb,ctx);
 }
}
void original_chicago_allocator_release(original_chicago_allocator *s,int32_t match,const original_allocator_callbacks *cb,void *ctx){
 for(int16_t i=s->voice_active.head;i>=0;){int16_t next=s->voice[i].next;
  if(match<0||s->voice[i].note==match){
   /* f5 is independent of effective_latch. */
   if(!s->keys->committed_enable&&(s->hold==1||s->pedal_byte))s->voice[i].marked=1;
   else release_record(s,i,cb,ctx);
  }i=next;
 }
}
void original_chicago_allocator_physical(original_chicago_allocator *s,int32_t key,int32_t velocity){
 if(velocity){
  assert(key>=0&&key<=127&&velocity>=1&&velocity<=127);
  int16_t i=s->held_free.head;
  if(i>=0)hv_remove(s,&s->held_free,i);else{i=s->held_active.head;assert(i>=0);hv_remove(s,&s->held_active,i);}
  s->held[i].note=(int8_t)key;s->held[i].velocity=(uint8_t)velocity;s->held[i].marked=0;hv_append(s,&s->held_active,i);return;
 }
 for(int16_t i=s->held_active.head;i>=0;){int16_t next=s->held[i].next;
  if(s->held[i].note==key){if(s->pedal_byte||s->hold==1)s->held[i].marked=1;else{s->held[i].marked=0;held_to_free(s,i);}}i=next;
 }
}
void original_chicago_allocator_cleanup_held(original_chicago_allocator *s){
 for(int16_t i=s->held_active.head;i>=0;){int16_t next=s->held[i].next;if(s->held[i].marked){s->held[i].marked=0;held_to_free(s,i);}i=next;}
}
void original_chicago_allocator_cleanup_voices(original_chicago_allocator *s,const original_allocator_callbacks *cb,void *ctx){
 for(int16_t i=s->voice_active.head;i>=0;){int16_t next=s->voice[i].next;if(s->voice[i].marked)release_record(s,i,cb,ctx);i=next;}
}
void original_chicago_allocator_finish_disable(original_chicago_allocator *s,const original_allocator_callbacks *cb,void *ctx){
 original_chicago_allocator_cleanup_voices(s,cb,ctx);
 for(unsigned i=0;i<s->capacity;i++)if(cb->enabled(ctx,i)&&cb->vca_state(ctx,i)!=6)cb->set_voice_state(ctx,i,0,-1,4);
 original_chicago_allocator_reset_lists(s);
}
void original_chicago_allocator_all_notes_off(original_chicago_allocator *s,const original_allocator_callbacks *cb,void *ctx){
 original_chicago_allocator_cleanup_held(s);original_chicago_allocator_clear_keys(s,cb,ctx);
 original_chicago_allocator_finish_disable(s,cb,ctx);
}
void original_chicago_allocator_clear_keys(original_chicago_allocator *s,const original_allocator_callbacks *cb,void *ctx){
 memset(s->keys->physical,0,sizeof s->keys->physical);memset(s->keys->latched,0,sizeof s->keys->latched);s->keys->physical_count=s->keys->latched_count=0;
 for(int32_t i=0;i<128;i++)(void)original_chicago_allocator_note_off(s,i,cb,ctx);
}
static uint32_t raw_velocity(const chicago_arp_control *k,unsigned note){return note<128?k->physical[note]:k->latched[note-128];}
static void raw_velocity_zero(chicago_arp_control *k,unsigned note){if(note<128)k->physical[note]=0;else k->latched[note-128]=0;}
int original_chicago_allocator_note_off(original_chicago_allocator *s,int32_t note,const original_allocator_callbacks *cb,void *ctx){
 if(note<0||note>255)return 0;
 original_chicago_allocator_physical(s,note,0);
 if(raw_velocity(s->keys,(unsigned)note)){s->keys->physical_count=(int32_t)s->keys->physical_count>1?s->keys->physical_count-1:0;raw_velocity_zero(s->keys,(unsigned)note);}
 if(s->keys->committed_enable==1){if(!s->keys->physical_count&&!s->keys->effective_latch)original_chicago_allocator_release(s,-1,cb,ctx);return 1;}
 if(s->mode==3){original_chicago_allocator_release(s,note,cb,ctx);return 1;}
 if(s->mode!=1)return 1;
 int32_t key=-1,velocity=-1;
 for(int16_t i=s->held_active.head;i>=0;i=s->held[i].next)if(!s->held[i].marked){key=s->held[i].note;velocity=s->held[i].velocity;}
 if(key==-1){original_chicago_allocator_release(s,-1,cb,ctx);return 1;}
 if(s->keys->current_note==key)return 1;
 s->keys->current_note=key;if(!s->gate_remaining)s->gate_remaining=s->gate_duration;
 if(s->voice_active.head>=0&&s->legato){
  for(int16_t i=s->voice_active.head;i>=0;i=s->voice[i].next){s->voice[i].note=key;s->voice[i].velocity=velocity;request(s,i,0,cb,ctx);}
 }else original_chicago_allocator_spawn(s,key,velocity,cb,ctx);
 return 1;
}
int original_chicago_allocator_key_event(original_chicago_allocator *s,int32_t key,int32_t velocity,const original_allocator_callbacks *cb,void *ctx){
 if(key<0||key>127||velocity<0||velocity>127)return 0;
 if(!velocity)(void)original_chicago_allocator_note_off(s,key,cb,ctx);
 s->keys->current_note=key;if(!s->gate_remaining)s->gate_remaining=s->gate_duration;
 original_chicago_allocator_physical(s,key,velocity);
 if(s->held_active.count==1){if(s->mg1_key_sync==1)s->mg1_key_reset=1;if(s->mg2_key_sync==1)s->mg2_key_reset=1;}
 if(s->keys->committed_enable!=1){
  if(s->mode==3)original_chicago_allocator_spawn(s,key,velocity,cb,ctx);
  else if(s->mode==1){
   if(s->voice_active.head>=0&&s->legato){for(int16_t i=s->voice_active.head;i>=0;i=s->voice[i].next){s->voice[i].note=key;s->voice[i].velocity=velocity;request(s,i,0,cb,ctx);}}
   else original_chicago_allocator_spawn(s,key,velocity,cb,ctx);
  }
 }else if(!s->keys->physical_count){
  if(s->arp_mode==0||s->arp_mode==2){s->scan_index=0;s->previous_index=0;s->octave=1;s->direction=0;}
  else if(s->arp_mode==1){s->scan_index=127;s->previous_index=127;s->octave=(int32_t)s->arp_range;s->direction=1;}
  memset(s->keys->physical,0,sizeof s->keys->physical);memset(s->keys->latched,0,sizeof s->keys->latched);s->keys->physical_count=0;s->keys->latched_count=0;
 }
 if(!s->keys->physical[(unsigned)key])s->keys->physical_count++;
 if(!s->keys->latched[(unsigned)key])s->keys->latched_count++;
 s->keys->physical[(unsigned)key]=(uint32_t)velocity;s->keys->latched[(unsigned)key]=(uint32_t)velocity;return 1;
}
int original_chicago_allocator_canonical_note(original_chicago_allocator *s,int32_t key,int32_t velocity,const original_allocator_callbacks *cb,void *ctx){
 if(key<0||key>127||velocity<0||velocity>127)return 0;
 return velocity?original_chicago_allocator_key_event(s,key,velocity,cb,ctx):original_chicago_allocator_note_off(s,key,cb,ctx);
}
void original_chicago_allocator_stage_pending(original_pending_note *p,const original_voice_record *r,unsigned mask,unsigned old){
 assert(p&&r&&mask<=15);p->note=r->note;p->velocity=r->velocity;p->unison_flag=r->unison_flag;p->coordinate=r->coordinate;
 if(mask==15)p->mask=15;
 else{p->mask=old==5?p->mask|mask:mask;for(unsigned i=0;i<4;i++)if(mask&(1u<<i))p->oscillator_note[i]=r->note;}
}
void original_chicago_allocator_geometry(original_allocator_geometry *out,int32_t poly,int32_t unison,int32_t mode,int32_t chords){
 int32_t n;if(poly>128)poly=128;else if(poly<=0)poly=1;
 if(poly<=unison){unison=poly;n=1;}
 else{if(unison<=0)unison=1;if(mode==1||mode==2)n=1;else if(mode==0)n=chords<1?1:(chords<poly/unison?chords:poly/unison);else n=poly/unison;}
 *out=(original_allocator_geometry){poly,unison,n,unison*n,(unison*n+3)/4};
}
int original_chicago_allocator_chord_snapshot(const original_chicago_allocator *s,uint32_t by_key[128],original_allocator_chord pairs[128],unsigned *count){
 if(s->held_active.head<0)return 0;
 memset(by_key,0,128*sizeof *by_key);for(unsigned i=0;i<128;i++)pairs[i]=(original_allocator_chord){-128,0};
 for(int16_t i=s->held_active.head;i>=0;i=s->held[i].next){assert(s->held[i].note>=0);by_key[(unsigned)s->held[i].note]=(uint32_t)s->held[i].velocity;}
 unsigned n=0;for(unsigned i=0;i<128;i++)if(by_key[i])pairs[n++]=(original_allocator_chord){(int32_t)i,(int32_t)by_key[i]};*count=n;return 1;
}
