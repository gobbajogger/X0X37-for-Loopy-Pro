/* SPDX-License-Identifier: GPL-3.0-only */
#include "chicago_allocator.h"
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define FNV UINT64_C(1469598103934665603)
typedef struct {unsigned enabled,key,vcf,vca;original_pending_note pending;} mock_voice;
typedef struct {original_chicago_allocator *a;mock_voice voice[128];uint64_t trace;unsigned calls;} context;
static uint32_t bits(float f){uint32_t u;memcpy(&u,&f,4);return u;}
static uint64_t word(uint64_t h,uint32_t v){for(unsigned i=0;i<4;i++){h=(h^(v&255))*UINT64_C(1099511628211);v>>=8;}return h;}
static void listtrace(context *c){c->trace=word(c->trace,c->a->held_active.count);c->trace=word(c->trace,c->a->held_free.count);c->trace=word(c->trace,c->a->voice_active.count);c->trace=word(c->trace,c->a->voice_free.count);}
static unsigned enabled(void *p,unsigned id){return ((context *)p)->voice[id].enabled;}
static unsigned state(void *p,unsigned id){return ((context *)p)->voice[id].vca;}
static void set_voice(void *p,unsigned id,int key,int vcf,int vca){
 context *c=p;mock_voice *v=&c->voice[id];if(key>=0)v->key=(unsigned)key;if(vcf>=0)v->vcf=(unsigned)vcf;if(vca>=0)v->vca=(unsigned)vca;
 uint32_t a[]={1,id,v->key,v->vcf,v->vca};for(unsigned i=0;i<5;i++)c->trace=word(c->trace,a[i]);listtrace(c);c->calls++;
}
/* Explicit numeric-voice callback fixture. The allocator is compared against
 * actual static instructions around this boundary. This does not supply or
 * claim the rest of the C6A4 numeric synthesis/initialization implementation. */
static void note(void *p,unsigned id,const original_voice_record *r,unsigned full,unsigned mask,int32_t last){
 context *c=p;mock_voice *v=&c->voice[id];uint32_t a[]={2,id,(uint32_t)r->note,(uint32_t)r->velocity,r->marked,r->unison_flag,bits(r->coordinate),full,mask,(uint32_t)last};
 for(unsigned i=0;i<10;i++)c->trace=word(c->trace,a[i]);listtrace(c);c->calls++;
 if(full&&v->vca!=6){original_chicago_allocator_stage_pending(&v->pending,r,mask,v->vca);v->vca=5;}
 else{v->enabled=1;v->key=1;if(full)v->vcf=v->vca=0;}
}
static original_allocator_callbacks cb={enabled,state,set_voice,note};
static unsigned values(const original_chicago_allocator *s,const context *c,uint32_t out[4096]){
 unsigned n=0;
#define V(x) out[n++]=(uint32_t)(x)
 V(s->capacity);V(s->unison_count);V(s->hold);V(s->legato);V(s->mode);V(s->pedal_byte);V(s->mg1_key_sync);V(s->mg2_key_sync);V(s->mg1_key_reset);V(s->mg2_key_reset);V(s->arp_mode);V(s->arp_range);V(s->scan_index);V(s->previous_index);V(s->octave);V(s->direction);V(s->last_note);V(s->gate_duration);V(s->gate_remaining);
 for(unsigned i=0;i<128;i++)V(s->keys->physical[i]);for(unsigned i=0;i<128;i++)V(s->keys->latched[i]);
 V(s->keys->physical_count);V(s->keys->latched_count);V(s->keys->committed_enable);V(s->keys->effective_latch);V(s->keys->current_note);
 const original_record_list *lists[]={&s->held_active,&s->held_free,&s->voice_active,&s->voice_free};
 for(unsigned i=0;i<4;i++){V(lists[i]->count);V(lists[i]->head);V(lists[i]->tail);}
 for(unsigned i=0;i<128;i++){const original_held_record *r=&s->held[i];V(r->note);V(r->velocity);V(r->marked);V(r->prev);V(r->next);}
 for(unsigned i=0;i<s->capacity;i++){
  const original_voice_record *r=&s->voice[i];const mock_voice *v=&c->voice[i];const original_pending_note *p=&v->pending;
  V(r->note);V(r->velocity);V(r->marked);V(r->unison_flag);V(bits(r->coordinate));V(r->prev);V(r->next);
  V(v->enabled);V(v->key);V(v->vcf);V(v->vca);V(p->note);V(p->velocity);V(p->unison_flag);V(bits(p->coordinate));V(p->mask);for(unsigned j=0;j<4;j++)V(p->oscillator_note[j]);
 }
#undef V
 assert(n<4096);return n;
}
static uint64_t digest(const original_chicago_allocator *s,const context *c){uint32_t words[4096];unsigned n=values(s,c,words);uint64_t h=FNV;for(unsigned i=0;i<n;i++)h=word(h,words[i]);return h;}
static void validate(const original_chicago_allocator *s){
 unsigned seen[128]={0};const original_record_list *lists[]={&s->held_active,&s->held_free};
 for(unsigned l=0;l<2;l++){unsigned n=0;int16_t prev=-1;for(int16_t i=lists[l]->head;i>=0;i=s->held[i].next){assert(i<128&&!seen[(unsigned)i]&&n<128);seen[(unsigned)i]=1;assert(s->held[i].prev==prev);prev=i;n++;}assert(n==lists[l]->count&&prev==lists[l]->tail);}
 for(unsigned i=0;i<128;i++)assert(seen[i]);memset(seen,0,sizeof seen);
 const original_record_list *vs[]={&s->voice_active,&s->voice_free};for(unsigned l=0;l<2;l++){unsigned n=0;int16_t prev=-1;for(int16_t i=vs[l]->head;i>=0;i=s->voice[i].next){assert((unsigned)i<s->capacity&&!seen[(unsigned)i]&&n<128);seen[(unsigned)i]=1;assert(s->voice[i].prev==prev);prev=i;n++;}assert(n==vs[l]->count&&prev==vs[l]->tail);}
 for(unsigned i=0;i<s->capacity;i++)assert(seen[i]);
}
int main(int argc,char **argv){
 assert(argc==2);FILE *f=fopen(argv[1],"r");assert(f);original_chicago_allocator s;original_held_record held[128];original_voice_record voice[128];chicago_arp_control keys;context c;unsigned row=0,caseid=0;char op,last='R';
 while(fscanf(f," %c",&op)==1){
  int32_t a,b;unsigned cap,u,mode,flags;
  if(op=='R'){
   assert(fscanf(f,"%u%u%u%u%u",&caseid,&cap,&u,&mode,&flags)==5);memset(&keys,0,sizeof keys);memset(&c,0,sizeof c);
   assert(original_chicago_allocator_bind(&s,&keys,held,voice,128,cap,u,mode,flags&1,(flags>>1)&1));c.a=&s;c.trace=FNV;s.pedal_byte=flags&4?127:0;keys.committed_enable=(flags>>3)&1;keys.effective_latch=(flags>>4)&1;
   s.mg1_key_sync=(caseid>>1)&1;s.mg2_key_sync=caseid&1;s.arp_mode=caseid%4;s.arp_range=1+caseid%6;s.gate_duration=123;s.scan_index=55;s.previous_index=51;s.octave=2;s.direction=1;
   for(unsigned i=0;i<cap;i++){c.voice[i].enabled=caseid%3==1||(caseid%3==2&&i%2==0);c.voice[i].key=1;c.voice[i].vca=(unsigned[]){6,3,0}[(caseid+i)%3];c.voice[i].vcf=(caseid+i)%7;c.voice[i].pending.mask=3;for(unsigned j=0;j<4;j++)c.voice[i].pending.oscillator_note[j]=17+(int32_t)j+(int32_t)i;}
  }else if(op=='W'){
   uint64_t expected,trace;unsigned calls;assert(fscanf(f,"%" SCNu64 "%" SCNu64 "%u",&expected,&trace,&calls)==3);validate(&s);uint64_t got=digest(&s,&c);
   if(got!=expected||trace!=c.trace||calls!=c.calls){fprintf(stderr,"FAIL case%u row%u after%c digest%016" PRIx64 "/%016" PRIx64 " trace%016" PRIx64 "/%016" PRIx64 " calls%u/%u\n",caseid,row,last,got,expected,c.trace,trace,c.calls,calls);FILE *d=fopen("allocator-actual-fields.bin","wb");uint32_t v[4096];unsigned n=values(&s,&c,v);assert(d);assert(fwrite(v,4,n,d)==n);fclose(d);return 1;}row++;
  }else{
   assert(fscanf(f,"%" SCNd32 "%" SCNd32,&a,&b)==2);last=op;
   switch(op){case 'E':assert(original_chicago_allocator_key_event(&s,a,b,&cb,&c));break;case'O':assert(original_chicago_allocator_note_off(&s,a,&cb,&c));break;case'Q':assert(original_chicago_allocator_canonical_note(&s,a,b,&cb,&c));break;case'K':original_chicago_allocator_clear_keys(&s,&cb,&c);break;case'T':assert(original_chicago_allocator_return_voice(&s,(unsigned)a,&cb,&c)==b);break;case'N':original_chicago_allocator_spawn(&s,a,b,&cb,&c);break;case'L':original_chicago_allocator_release(&s,a,&cb,&c);break;case'H':original_chicago_allocator_cleanup_held(&s);break;case'V':original_chicago_allocator_cleanup_voices(&s,&cb,&c);break;case'A':original_chicago_allocator_all_notes_off(&s,&cb,&c);break;case'F':original_chicago_allocator_reset_lists(&s);break;default:assert(0);}
  }
 }
 assert(!ferror(f));fclose(f);printf("PASS %u exact Miami canonical mode0/1/2/3 dispatch state+callback rows; physicalrecord%zu voicerecord%zu descriptor%zu hostbytes; C6A4 numeric callback explicit\n",row,sizeof(original_held_record),sizeof(original_voice_record),sizeof(original_chicago_allocator));return 0;
}
