/* SPDX-License-Identifier: GPL-3.0-only */
#include "miami_geometry.h"
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint64_t hashword(uint64_t h,uint32_t v){for(unsigned j=0;j<4;j++){h=(h^(uint8_t)v)*UINT64_C(1099511628211);v>>=8;}return h;}
#define FNV UINT64_C(1469598103934665603)
static uint32_t bits(float v){uint32_t u;memcpy(&u,&v,4);return u;}
static uint64_t relevant(uint64_t h,const original_miami_geometry*g,const original_chicago_allocator*a){
 const original_allocator_geometry*v=&g->value;int32_t x[]={v->polyphony,v->unison,v->groups,v->capacity,v->simd_groups,(int32_t)a->mode,(int32_t)g->chord_count};for(unsigned i=0;i<7;i++)h=hashword(h,(uint32_t)x[i]);
 const original_record_list *ls[4]={&a->held_active,&a->held_free,&a->voice_active,&a->voice_free};for(unsigned i=0;i<4;i++){h=hashword(h,ls[i]->count);h=hashword(h,(uint32_t)ls[i]->head);h=hashword(h,(uint32_t)ls[i]->tail);}
 uint32_t y[]={a->keys->physical_count,a->keys->latched_count,(uint32_t)a->keys->current_note,(uint32_t)a->last_note,(uint32_t)a->gate_remaining};for(unsigned i=0;i<5;i++)h=hashword(h,y[i]);for(unsigned j=0;j<2;j++)for(unsigned i=0;i<128;i++)h=hashword(h,j?a->keys->latched[i]:a->keys->physical[i]);return h;
}
static uint64_t data(const original_miami_geometry*g,const original_chicago_allocator*a){
 uint64_t h=relevant(FNV,g,a);for(unsigned i=0;i<128;i++)h=hashword(h,g->by_key[i]);for(unsigned i=0;i<128;i++){h=hashword(h,(uint32_t)g->pairs[i].note);h=hashword(h,(uint32_t)g->pairs[i].velocity);}
 for(unsigned i=0;i<128;i++){
  const original_held_record*r=a->held+i;int32_t q[]={r->note,r->velocity,r->marked,r->prev,r->next};for(unsigned j=0;j<5;j++)h=hashword(h,(uint32_t)q[j]);
  const original_voice_record*v=a->voice+i;uint32_t z[]={(uint32_t)v->note,(uint32_t)v->velocity,v->marked,v->unison_flag,bits(v->coordinate),(uint32_t)v->prev,(uint32_t)v->next};for(unsigned j=0;j<7;j++)h=hashword(h,z[j]);
 }return h;
}
typedef struct{original_miami_geometry*g;original_chicago_allocator*a;uint64_t phase;unsigned calls,fail;} observer;
static int reset(void*p){observer*o=p;o->phase=relevant(o->phase,o->g,o->a);o->calls++;return o->calls!=o->fail;}
static unsigned corpus(const char*path){
 FILE*f=fopen(path,"r");assert(f);unsigned cases=0;char kind;
 while(fscanf(f," %c",&kind)==1){
  int32_t op,ou,og,oc,os,om,oldcount,rp,ru;unsigned mode,n;assert(fscanf(f,"%"SCNd32"%"SCNd32"%"SCNd32"%"SCNd32"%"SCNd32"%"SCNd32"%"SCNd32"%"SCNd32"%"SCNd32"%u%u",&op,&ou,&og,&oc,&os,&om,&oldcount,&rp,&ru,&mode,&n)==11);
  chicago_arp_control keys;memset(&keys,0,sizeof keys);original_held_record held[128];original_voice_record voices[128];original_chicago_allocator a;memset(held,0,sizeof held);memset(voices,0,sizeof voices);assert(original_chicago_allocator_bind(&a,&keys,held,voices,128,128,(unsigned)ou,(unsigned)om,0,0));a.capacity=(uint32_t)oc;original_chicago_allocator_reset_lists(&a);
  for(unsigned i=0;i<n;i++){int k,v,m;assert(fscanf(f,"%d%d%d",&k,&v,&m)==3);original_chicago_allocator_physical(&a,k,v?v:1);held[a.held_active.tail].velocity=(uint8_t)v;held[a.held_active.tail].marked=(uint8_t)m;}
  uint8_t raw_map[128];if(kind=='R')for(unsigned i=0;i<128;i++){unsigned value;assert(fscanf(f,"%u",&value)==1&&value<256);raw_map[i]=(uint8_t)value;}
  uint64_t expected,phase;unsigned calls;assert(fscanf(f,"%"SCNu64"%"SCNu64"%u",&expected,&phase,&calls)==3);
  uint32_t by_key[128];original_allocator_chord pairs[128];for(unsigned i=0;i<128;i++){by_key[i]=7;pairs[i]=(original_allocator_chord){-17,33};keys.physical[i]=i%19==0?1+i%127:0;keys.latched[i]=i%11==0?1+(i*13)%127:0;keys.physical_count+=(keys.physical[i]!=0);keys.latched_count+=(keys.latched[i]!=0);}
  keys.current_note=42;a.last_note=57;a.gate_remaining=9;
  original_miami_geometry g={{op,ou,og,oc,os},by_key,pairs,(uint32_t)oldcount};observer o={&g,&a,FNV,0,0};original_miami_geometry_progress progress;memset(&progress,0xa5,sizeof progress);
  original_miami_geometry_progress saved_progress=progress;int status=kind=='L'?original_miami_geometry_learn(&g,&a,rp,ru,reset,&o,&progress):kind=='P'?original_miami_geometry_prepare(&g,&a,rp,ru,mode,reset,&o,&progress):original_miami_geometry_restore_map(&g,&a,kind=='Z'?NULL:raw_map,rp,ru,reset,&o,&progress);if(kind=='Z')assert(!status&&!memcmp(&progress,&saved_progress,sizeof progress)&&!o.calls);else assert(status&&progress.status&&progress.stage==ORIGINAL_MIAMI_GEOMETRY_COMPLETE&&progress.completed_resets==calls&&o.calls==calls);
  if(kind!='Z')assert(progress.learned_chord==(unsigned)(kind=='L'?n!=0:kind=='P'&&rp==op&&ru==ou&&om==3&&mode==0&&n!=0));
  if(data(&g,&a)!=expected||o.phase!=phase){fprintf(stderr,"case%u kind%c data%"PRIu64"/%"PRIu64" phase%"PRIu64"/%"PRIu64" resets%u/%u\n",cases,kind,data(&g,&a),expected,o.phase,phase,o.calls,calls);abort();}cases++;
 }
 assert(!ferror(f));fclose(f);return cases;
}
static void negative(void){
 chicago_arp_control k;memset(&k,0,sizeof k);original_held_record h[128];original_voice_record v[2];memset(h,0,sizeof h);memset(v,0,sizeof v);original_chicago_allocator a;assert(original_chicago_allocator_bind(&a,&k,h,v,2,2,2,1,0,0));uint32_t map[128];original_allocator_chord pair[128];memset(map,0x11,sizeof map);memset(pair,0x22,sizeof pair);original_miami_geometry g={{16,2,1,2,1},map,pair,3};observer o={&g,&a,FNV,0,0};original_miami_geometry_progress p,saved;memset(&p,0xa5,sizeof p);saved=p;original_chicago_allocator aa=a;original_miami_geometry gg=g;
 /* Changing to mode3 needs16owned voices;do not silently clamp to2,partly
  *learn,reset or alter the current geometry on failed resource admission. */
 assert(!original_miami_geometry_prepare(&g,&a,16,2,3,reset,&o,&p)&&!memcmp(&p,&saved,sizeof p)&&!memcmp(&a,&aa,sizeof a)&&!memcmp(&g,&gg,sizeof g)&&!o.calls);
 assert(!original_miami_geometry_prepare(&g,&a,16,2,4,reset,&o,&p)&&!memcmp(&p,&saved,sizeof p)&&!o.calls);
 uint8_t raw_map[128];memset(raw_map,0xff,sizeof raw_map);a.mode=3;g.value=(original_allocator_geometry){2,2,1,2,1};aa=a;gg=g;
 uint32_t saved_map[128];original_allocator_chord saved_pair[128];memcpy(saved_map,map,sizeof map);memcpy(saved_pair,pair,sizeof pair);
 assert(!original_miami_geometry_restore_map(&g,&a,raw_map,16,2,reset,&o,&p)&&!memcmp(&a,&aa,sizeof a)&&!memcmp(&g,&gg,sizeof g)&&!memcmp(map,saved_map,sizeof map)&&!memcmp(pair,saved_pair,sizeof pair)&&!memcmp(&p,&saved,sizeof p)&&!o.calls);
 assert(!original_miami_geometry_restore_map(&g,&a,NULL,16,2,reset,&o,&p)&&!memcmp(&p,&saved,sizeof p)&&!o.calls);a.mode=1;g.value=(original_allocator_geometry){16,2,1,2,1};
 /* Valid controls/mode unchanged preserve every resource and need no list
  *scan. An action on a cyclic restored list fails before any mutation. */
 assert(original_miami_geometry_prepare(&g,&a,16,2,1,reset,&o,&p)&&!o.calls);
 original_chicago_allocator_physical(&a,60,100);h[a.held_active.head].next=a.held_active.head;p=saved;aa=a;
 assert(!original_miami_geometry_prepare(&g,&a,15,2,1,reset,&o,&p)&&!memcmp(&a,&aa,sizeof a)&&!memcmp(&p,&saved,sizeof p)&&!o.calls);
 h[a.held_active.head].next=-1;original_chicago_allocator_reset_lists(&a);
 /* Callback failure occurs AFTER real geometry/list effects, and reports
  * the exact incomplete phase rather than claiming rollback/success. */
 o.fail=1;p=saved;assert(!original_miami_geometry_prepare(&g,&a,15,2,1,reset,&o,&p)&&!p.status&&p.stage==ORIGINAL_MIAMI_GEOMETRY_CONFIGURE&&!p.completed_resets&&o.calls==1&&g.value.polyphony==15&&a.held_active.count==0&&a.held_free.count==128);
}
int main(int argc,char**argv){assert(argc==2);unsigned n=corpus(argv[1]);negative();printf("PASS %u actual Miami BFD0/prepare geometry value-list-reset-order witnesses;duplicates/marked/zero/empty,all256 signed restore bytevalues/null/allzero-force,3to0 double reset,geometry-before-mode suppression,raw clamp comparison,resource/cyclic-state negatives and partial callback failure;B0738 numeric body is separate concrete-owner proof;PENDING_REVIEW\n",n);return 0;}
