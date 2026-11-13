/* SPDX-License-Identifier: GPL-3.0-only */
#include "chicago_selector.h"
static int state_valid(const original_chicago_selector *s,const original_chicago_gate *g,const original_chicago_selector_controls *c){
 return s&&g&&c&&s->cursor>=-1&&s->cursor<=128&&s->last_key>=0&&s->last_key<=127&&s->octave>=1&&s->octave<=6&&(s->direction==0||s->direction==1)&&c->mode<=3&&c->range>=1&&c->range<=6&&g->duration>=0&&g->remaining>=0;
}
static uint32_t read(const uint32_t *v,int32_t key){return v[(uint32_t)key];}
static int32_t chosen(original_chicago_selector *s,int32_t key){s->last_key=key;return key+12*(s->octave-1);}
static int32_t choose(original_chicago_selector *s,const uint32_t *v,uint32_t count,uint32_t mode,uint32_t range,uint32_t *velocity){
 int32_t key=s->cursor;
 if(mode==3){
  uint32_t old=s->random,x=old<<1;
  x=(x&~1u)|((((old<<12)^(old<<4)^old^(old<<1))>>15)&1u);s->random=(uint16_t)x;
  /* Preserve SCVTF signed32 semantics even for a native underflowed count.
   * Canonical count<=128 uses an exact integer equivalent; the uncommon
   * wrapped/mismatched case keeps both original rounded binary32 products. */
  int32_t ordinal;
  if(count<=128)ordinal=(int32_t)((count*((x>>8)&255u))>>8);
  else{int32_t n=count<=INT32_MAX?(int32_t)count:-1-(int32_t)(UINT32_MAX-count);float q=(float)n*(float)((x>>8)&255u);q=q*.00390625f;ordinal=(int32_t)q;}
  uint32_t octave=(range*(x&255u))>>8;
  int32_t seen=0;for(unsigned k=0;k<128;k++)if(v[k]){if(seen==ordinal){*velocity=v[k];return (int32_t)(k+12*octave);}seen++;}
 }
 else if(mode==2){
  for(;;){
   if(key<127){key++;s->cursor=key;if(v[(uint32_t)key]){*velocity=v[(uint32_t)key];return chosen(s,key);}}
   else {key=0;s->cursor=0;s->octave=s->octave>=(int32_t)range?1:s->octave+1;if(v[0]){*velocity=v[0];return chosen(s,0);}}
  }
 }
 else if(mode==1){
  for(;;){
   if(key>0){key--;s->cursor=key;if(v[(uint32_t)key]){*velocity=v[(uint32_t)key];return chosen(s,key);}}
   else{key=127;s->cursor=127;if(--s->octave<=0)s->octave=(int32_t)range;if(v[127]){*velocity=v[127];return chosen(s,127);}}
  }
 }
 else if(s->direction){
  for(;;){
   key--;s->cursor=key;
   if(key>=0){if(v[(uint32_t)key]){*velocity=v[(uint32_t)key];return chosen(s,key);}}
   else{
    int32_t old=s->octave;s->octave=old-1;
    if(s->octave<=0){key=s->last_key;*velocity=read(v,key);s->cursor=key;s->octave=1;s->direction=0;return key+12*(old-1);}
    key=127;s->cursor=127;if(v[127]){*velocity=v[127];return chosen(s,127);}
   }
  }
 }
 else {
  for(;;){
   if(key<127){key++;s->cursor=key;if(v[(uint32_t)key]){*velocity=v[(uint32_t)key];return chosen(s,key);}}
   else {
    int32_t old=s->octave;s->octave=old+1;
    if(old>=(int32_t)range){key=s->last_key;*velocity=read(v,key);s->cursor=key;s->octave=(int32_t)range;s->direction=1;return key+12*(old-1);}
    key=0;s->cursor=0;if(v[0]){*velocity=v[0];return chosen(s,0);}
   }
  }
 }
 *velocity=0;return 0; /* Original random scan falls through on a count mismatch. */
}
int original_chicago_selector_step(original_chicago_selector *s,chicago_arp_control *keys,original_chicago_gate *g,const original_chicago_selector_controls *c,const original_chicago_selector_callbacks *cb,void *ctx){
 if(!keys)return 0;
 if(keys->committed_enable!=1)return 1;
 if(!state_valid(s,g,c)||!cb||!cb->release_active||!cb->spawn)return 0;
 const uint32_t *v=keys->effective_latch==1?keys->latched:keys->physical;
 uint32_t count=keys->effective_latch==1?keys->latched_count:keys->physical_count,actual=0;
 for(unsigned i=0;i<128;i++)actual+=(v[i]!=0);
 if(count&&!actual&&(c->mode==1||c->mode==2))return -1;
 if(!count){cb->release_active(ctx);return 1;}
 uint32_t velocity;int32_t note=choose(s,v,count,c->mode,c->range,&velocity);keys->current_note=note;
 if(!g->remaining)g->remaining=g->duration;
 if(c->note_mode==1||c->note_mode==3){cb->release_active(ctx);cb->spawn(ctx,note,velocity);}
 return 1;
}
void original_chicago_gate_tick(original_chicago_gate *g,int32_t note){if(g&&g->remaining>0&&!--g->remaining)g->last_note=note;}
