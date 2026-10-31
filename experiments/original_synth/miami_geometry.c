/* SPDX-License-Identifier: GPL-3.0-only */
#include "miami_geometry.h"
static int basic(const original_miami_geometry *g,const original_chicago_allocator *a,original_miami_geometry_reset reset,const original_miami_geometry_progress *p){
 if(!g||!a||!p||!reset||!g->by_key||!g->pairs||!a->held||!a->voice||!a->keys||a->mode>3||!a->storage_voices||a->storage_voices>128||g->chord_count>128)return 0;
 const original_allocator_geometry *v=&g->value;
 return v->polyphony>=1&&v->polyphony<=128&&v->unison>=1&&v->unison<=v->polyphony&&v->groups>=1&&v->groups<=v->polyphony/v->unison&&v->capacity==v->groups*v->unison&&v->simd_groups==(v->capacity+3)/4&&a->capacity==(uint32_t)v->capacity&&a->unison_count==(uint32_t)v->unison&&a->capacity<=a->storage_voices;
}
static int list(const original_chicago_allocator *s,const original_record_list *l,unsigned voice,unsigned active,uint32_t seen[4]){
 int16_t previous=-1,i=l->head;unsigned count=0,limit=voice?s->storage_voices:128;
 if(l->head< -1||l->tail< -1||l->count>limit)return 0;
 while(i>=0){
  if((unsigned)i>=limit||count>=limit||(seen[(unsigned)i/32]&(1u<<((unsigned)i%32))))return 0;
  seen[(unsigned)i/32]|=1u<<((unsigned)i%32);
  int16_t before=voice?s->voice[i].prev:s->held[i].prev,next=voice?s->voice[i].next:s->held[i].next;
  if(before!=previous||next< -1||(!voice&&active&&(s->held[i].note<0||s->held[i].velocity>127)))return 0;
  previous=i;i=next;count++;
 }
 return previous==l->tail&&count==l->count;
}
static int lists(const original_chicago_allocator *a){
 uint32_t seen[4]={0};if(!list(a,&a->held_active,0,1,seen)||!list(a,&a->held_free,0,0,seen))return 0;
 for(unsigned i=0;i<4;i++)seen[i]=0;
 return list(a,&a->voice_active,1,1,seen)&&list(a,&a->voice_free,1,0,seen);
}
static unsigned learned_count(const original_chicago_allocator *a){
 uint32_t seen[4]={0};unsigned n=0;
 for(int16_t i=a->held_active.tail;i>=0;i=a->held[i].prev){unsigned key=(unsigned)a->held[i].note,bit=1u<<(key%32);if(!(seen[key/32]&bit)){seen[key/32]|=bit;n+=(a->held[i].velocity!=0);}}
 return n;
}
static int fits(const original_allocator_geometry *v,const original_chicago_allocator *a){return v->capacity>=1&&(uint32_t)v->capacity<=a->storage_voices;}
static int phase(original_miami_geometry *g,original_chicago_allocator *a,const original_allocator_geometry *v,unsigned stage,original_miami_geometry_reset reset,void *context,original_miami_geometry_progress *p){
 g->value=*v;a->capacity=(uint32_t)v->capacity;a->unison_count=(uint32_t)v->unison;original_chicago_allocator_reset_lists(a);p->stage=stage;
 if(!reset(context))return 0;
 p->completed_resets++;
 return 1;
}
int original_miami_geometry_learn(original_miami_geometry *g,original_chicago_allocator *a,int32_t poly,int32_t unison,original_miami_geometry_reset reset,void *context,original_miami_geometry_progress *p){
 if(!basic(g,a,reset,p))return 0;
 if(a->held_active.head<0){*p=(original_miami_geometry_progress){ORIGINAL_MIAMI_GEOMETRY_COMPLETE,0,0,1};return 1;}
 if(!lists(a))return 0;
 unsigned count=learned_count(a);
 original_allocator_geometry v;
 original_chicago_allocator_geometry(&v,poly,unison,(int32_t)a->mode,(int32_t)count);
 if(!fits(&v,a))return 0;
 *p=(original_miami_geometry_progress){ORIGINAL_MIAMI_GEOMETRY_READY,0,0,0};unsigned actual;
 if(!original_chicago_allocator_chord_snapshot(a,g->by_key,g->pairs,&actual))return 0;
 g->chord_count=actual;
 p->learned_chord=1;
 if(!phase(g,a,&v,ORIGINAL_MIAMI_GEOMETRY_CHORD,reset,context,p))return 0;
 p->stage=ORIGINAL_MIAMI_GEOMETRY_COMPLETE;p->status=1;return 1;
}
int original_miami_geometry_prepare(original_miami_geometry *g,original_chicago_allocator *a,int32_t poly,int32_t unison,unsigned mode,original_miami_geometry_reset reset,void *context,original_miami_geometry_progress *p){
 if(!basic(g,a,reset,p)||mode>3)return 0;
 unsigned configure=poly!=g->value.polyphony||unison!=g->value.unison,change=mode!=a->mode;
 if(!configure&&!change){*p=(original_miami_geometry_progress){ORIGINAL_MIAMI_GEOMETRY_COMPLETE,0,0,1};return 1;}
 if(!lists(a))return 0;
 unsigned learn=!configure&&change&&a->mode==3&&mode==0&&a->held_active.head>=0,count=learn?learned_count(a):g->chord_count;
 original_allocator_geometry first=g->value,last;
 if(configure||learn)original_chicago_allocator_geometry(&first,poly,unison,(int32_t)a->mode,(int32_t)count);
 last=first;if(change)original_chicago_allocator_geometry(&last,first.polyphony,first.unison,(int32_t)mode,(int32_t)count);
 if(!fits(&first,a)||!fits(&last,a))return 0;
 *p=(original_miami_geometry_progress){ORIGINAL_MIAMI_GEOMETRY_READY,0,0,0};
 if(learn){unsigned actual;if(!original_chicago_allocator_chord_snapshot(a,g->by_key,g->pairs,&actual))return 0;g->chord_count=actual;p->learned_chord=1;}
 if((configure||learn)&&!phase(g,a,&first,learn?ORIGINAL_MIAMI_GEOMETRY_CHORD:ORIGINAL_MIAMI_GEOMETRY_CONFIGURE,reset,context,p))return 0;
 if(change){a->mode=mode;if(!phase(g,a,&last,ORIGINAL_MIAMI_GEOMETRY_MODE,reset,context,p))return 0;}
 p->stage=ORIGINAL_MIAMI_GEOMETRY_COMPLETE;p->status=1;return 1;
}
int original_miami_geometry_restore_map(original_miami_geometry *g,original_chicago_allocator *a,const uint8_t bytes[128],int32_t poly,int32_t unison,original_miami_geometry_reset reset,void *context,original_miami_geometry_progress *p){
 if(!bytes||!basic(g,a,reset,p)||!lists(a))return 0;
 unsigned count=0;for(unsigned i=0;i<128;i++)count+=(bytes[i]!=0);
 original_allocator_geometry v;original_chicago_allocator_geometry(&v,poly,unison,(int32_t)a->mode,(int32_t)count);if(!fits(&v,a))return 0;
 *p=(original_miami_geometry_progress){ORIGINAL_MIAMI_GEOMETRY_READY,0,0,0};
 for(unsigned i=0;i<128;i++)g->pairs[i]=(original_allocator_chord){-128,0};
 unsigned n=0;for(unsigned i=0;i<128;i++){int32_t velocity=bytes[i]<128?(int32_t)bytes[i]:(int32_t)bytes[i]-256;g->by_key[i]=(uint32_t)velocity;if(bytes[i])g->pairs[n++]=(original_allocator_chord){(int32_t)i,velocity};}g->chord_count=n;
 if(!phase(g,a,&v,ORIGINAL_MIAMI_GEOMETRY_RESTORE,reset,context,p))return 0;
 p->stage=ORIGINAL_MIAMI_GEOMETRY_COMPLETE;p->status=1;return 1;
}
