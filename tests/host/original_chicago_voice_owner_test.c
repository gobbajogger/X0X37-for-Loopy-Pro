/* SPDX-License-Identifier: GPL-3.0-only */
#include "chicago_voice_owner.h"
#include "chicago_packed.h"
#include "chicago_global_stage.h"
#include "motion_tables_private.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef struct {original_chicago_voice_owner *owner;unsigned tunes,mgs,sources,partial,full;int32_t observed_notes[16];original_chicago_selected_global_stage global;unsigned exits,seeds;} observe;
static uint64_t hash=UINT64_C(1469598103934665603);static unsigned assertions;
#define CHECK(x) do{assertions++;assert(x);}while(0)
static uint32_t bits(float x){uint32_t u;memcpy(&u,&x,4);return u;}
static void word(uint32_t x){for(unsigned j=0;j<4;j++){hash=(hash^(uint8_t)x)*UINT64_C(1099511628211);x>>=8;}}
static void sources(void*p,unsigned id,const original_chicago_voice_sources *s){observe*x=p;const original_chicago_voice *v=x->owner->voices+id;CHECK(v->active&&v->key_down&&v->velocity==s->velocity_unipolar&&v->keyboard_source==s->key_bipolar);CHECK(s->key_unipolar==(float)v->raw_note*.0078125f);x->sources++;}
static int tune(void*p,unsigned id,const original_voice_record*r,unsigned full,int32_t anchor,float*t,float*a){observe*x=p;original_chicago_voice*v=x->owner->voices+id;CHECK(x->sources==x->tunes+1&&v->active&&v->key_down&&v->raw_note==r->note&&v->unison_flag==r->unison_flag&&v->unison_coordinate==r->coordinate);CHECK(v->velocity==(float)r->velocity/127&&v->raw_velocity==(uint32_t)r->velocity);if(x->tunes<16)x->observed_notes[x->tunes]=r->note;x->tunes++;x->full+=full;x->partial+=!full;*t=(float)r->note;*a=(float)anchor;return 1;}
/* Test-only explicit producer. These snapshots check ordering/lifetime; this
 * function is never a stand-in for original global MG in product code. */
static int mg(void*p,unsigned id,original_chicago_voice_mg out[2]){observe*x=p;original_chicago_voice*v=x->owner->voices+id;CHECK(v->active&&v->key_down);if(v->awaiting_restart&&v->vca.state==6){CHECK(bits(v->vca.level)==0&&bits(v->vcf.level)==0);}CHECK(v->pitch.target[0]==(float)v->raw_note||v->pitch.target[2]==(float)v->raw_note);out[0]=(original_chicago_voice_mg){-1,0,0,.13f,0x6d70,0};out[1]=(original_chicago_voice_mg){-1,0,0,-.27f,0x1234567,0};x->mgs++;return 1;}
static void seed_global(void*p,unsigned id){observe*x=p;original_chicago_selected_global_seed(&x->global,id);x->seeds++;}
static int exit_global(void*p){observe*x=p;x->exits++;return original_chicago_selected_global_exit(&x->global);}
static void init_voice(original_chicago_voice*v,const original_chicago_voice_tables*t){original_chicago_voice_seed seed;memset(&seed,0,sizeof seed);seed.noise=0x72aa;seed.noise_gain.current=seed.noise_gain.target=.00001f;seed.last_pitch=60;seed.mg[0].phase=seed.mg[1].phase=-1;seed.mg[0].noise=0x6d70;seed.mg[1].noise=0x1234567;CHECK(original_chicago_voice_init(v,t,&seed));}
static void run(unsigned factory){
 chicago_arp_control keys;memset(&keys,0,sizeof keys);original_held_record held[128];original_voice_record record[1];original_chicago_allocator a;CHECK(original_chicago_allocator_bind(&a,&keys,held,record,1,1,1,1,0,0));original_chicago_allocator_reset_lists(&a);
 original_chicago_voice_tables t;CHECK(original_chicago_voice_tables_bind(&t,original_chicago_packed_lookup,NULL,original_pitch44100));original_chicago_program p;original_chicago_voice_controls c;CHECK(original_chicago_program_init(&p,factory)&&original_chicago_voice_compile(&c,&p));original_chicago_voice v;init_voice(&v,&t);original_pending_note pending;memset(&pending,0,sizeof pending);original_chicago_voice_owner owner;observe ob;memset(&ob,0,sizeof ob);ob.owner=&owner;original_chicago_global_mg g;memset(&g,0,sizeof g);g.mg1.phase=g.mg2.phase=-1;g.mg1.random=g.mg2.random=0x6d70;original_chicago_global_mg_input gi={44100,44100,.5f,.5f,0,0,0,1};chicago_arp_clock c1={0,10000,0},c2={0,10000,0};CHECK(original_chicago_selected_global_bind(&ob.global,&g,&gi,&c1,&c2,&a));original_chicago_note_producers producers={&ob,tune,mg,sources,seed_global,exit_global};CHECK(original_chicago_voice_owner_bind(&owner,&v,&pending,1,&a,&c,&t,&producers,137));const original_allocator_callbacks*cb=original_chicago_voice_owner_callbacks();
 original_chicago_allocator_spawn(&a,60,100,cb,&owner);CHECK(owner.status&&v.active&&v.raw_note==60&&ob.tunes==1&&ob.mgs==1&&a.voice_active.count==1&&!a.voice_free.count);
 for(unsigned i=0;i<70;i++){float l,r;CHECK(original_chicago_voice_owner_control_all(&owner));CHECK(original_chicago_voice_render_frame(&v,&c,&l,&r));word(bits(l));word(bits(r));}
 /* Free-empty steal writes state4, then A088 stages without consuming any
  * tuning/MG/source producer or returning the newly installed active record. */
 unsigned tunes=ob.tunes;original_chicago_allocator_spawn(&a,72,127,cb,&owner);CHECK(owner.status&&v.vca.state==5&&v.raw_note==60&&pending.note==72&&pending.mask==15&&ob.tunes==tunes&&a.voice_active.count==1);
 v.vca.level=-.25f;v.counter=44100;float l=123,r=234;CHECK(original_chicago_voice_owner_control_all(&owner));CHECK(original_chicago_voice_render_frame(&v,&c,&l,&r));CHECK(owner.status&&!v.awaiting_restart&&v.active&&v.raw_note==72&&ob.tunes==tunes+1&&ob.mgs==2&&a.voice_active.count==1&&!a.voice_free.count&&pending.mask==15);word(bits(l));word(bits(r));CHECK(ob.exits==73&&ob.seeds==2&&ob.global.full_head_seeds==2);
 /* Disabled/VCA4 still stages. Partial/VCA5 then activates and exposes the
  * pending continuation in the same forced-helper entry. */
 v.active=0;v.vca.state=4;tunes=ob.tunes;original_voice_record q=record[0];q.note=187;q.velocity=0;q.unison_flag=2;q.coordinate=-.5f;cb->note(&owner,0,&q,1,1,60);CHECK(owner.status&&!v.active&&v.vca.state==5&&ob.tunes==tunes&&pending.mask==1&&pending.oscillator_note[0]==187);
 q.note=65;q.velocity=127;cb->note(&owner,0,&q,1,4,60);CHECK(pending.mask==5&&pending.oscillator_note[0]==187&&pending.oscillator_note[2]==65&&ob.tunes==tunes);
 v.counter=44100;v.vca.level=0;q.note=40;q.coordinate=.5f;unsigned mgs=ob.mgs;cb->note(&owner,0,&q,0,1,60);CHECK(owner.status&&v.active&&!v.awaiting_restart&&ob.tunes==tunes+3&&ob.mgs==mgs+2&&ob.partial==1&&v.raw_note==65&&v.pitch.target[0]==187&&v.pitch.target[2]==65&&pending.mask==5&&a.voice_active.count==1);
 CHECK(ob.exits==76&&ob.seeds==4&&ob.global.helper_entries==76);
 /* Every public control compiles into this live owner before another sample.
  * The fixed tuned producer and MG snapshots above are explicitly test-only. */
 for(unsigned macro=0;macro<22;macro++)for(unsigned endpoint=0;endpoint<2;endpoint++){
  CHECK(original_chicago_program_set(&p,macro,(float)endpoint)&&original_chicago_voice_compile(&c,&p));
  for(unsigned i=0;i<17;i++){CHECK(original_chicago_voice_owner_control_all(&owner));CHECK(original_chicago_voice_render_frame(&v,&c,&l,&r));word(bits(l));word(bits(r));}
 }
 keys.committed_enable=1;original_chicago_allocator_release(&a,-1,cb,&owner);CHECK(v.active&&v.vca.state==3&&!a.voice_active.count&&a.voice_free.count==1);v.vca.level=0;v.counter=44100;CHECK(original_chicago_voice_owner_control_all(&owner));CHECK(original_chicago_voice_render_frame(&v,&c,&l,&r));CHECK(!v.active&&v.vca.state==6&&a.voice_active.count==0&&a.voice_free.count==1&&l==0&&r==0&&pending.mask==5);
 /* Enabled/VCA6 full initialization is admitted without false staging. */
 v.active=1;q.note=60;q.velocity=100;tunes=ob.tunes;cb->note(&owner,0,&q,1,15,60);CHECK(owner.status&&v.active&&ob.tunes==tunes+1&&v.raw_note==60);
}
int main(void){for(unsigned factory=0;factory<81;factory++)run(factory);printf("PASS81 factory/all22 endpoint owner compositions, real globalMG normal/forced/nested entries and head seeds, source-before-tune/pitch-before-MG order, disabled/state5/full/partial/masked pending/action1-2 ownership; %u assertions,hash%016llx; test-only explicit global producers; PENDING_REVIEW\n",assertions,(unsigned long long)hash);return 0;}
