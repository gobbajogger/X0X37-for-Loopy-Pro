/* SPDX-License-Identifier: GPL-3.0-only */
#include "chicago_key_admission.h"
#include "chicago_scheduler.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef struct {unsigned enabled[4],vca[4];uint64_t selected;} voices;
static unsigned enabled(void *p,unsigned i){return ((voices *)p)->enabled[i];}
static unsigned vca(void *p,unsigned i){return ((voices *)p)->vca[i];}
static void state(void *p,unsigned i,int key,int filter,int amp){(void)key;(void)filter;if(amp>=0)((voices *)p)->vca[i]=(unsigned)amp;}
static void note(void *p,unsigned i,const original_voice_record *r,unsigned full,unsigned mask,int32_t anchor){(void)r;(void)full;(void)mask;(void)anchor;((voices *)p)->enabled[i]=1;((voices *)p)->vca[i]=0;}
static void tick(void *p,original_chicago_allocator *a){(void)p;(void)a;}
static const original_allocator_callbacks voice_callbacks={enabled,vca,state,note};
static const original_chicago_scheduler_callbacks callbacks={&voice_callbacks,tick,tick};
static uint64_t checks;
static void valid(const chicago_arp_control *k){assert(original_chicago_canonical_keys_valid(k));checks++;}
int main(void){
 chicago_arp_control keys;original_held_record held[128];original_voice_record records[4];original_chicago_allocator a;original_chicago_selector selector;chicago_arp_clock clock={0,110250,0};chicago_arp_clock_input ci={120,1};chicago_arp_control_input control={0,0,0};original_chicago_scheduler scheduler;voices v;
 for(unsigned mode=0;mode<4;mode++)for(unsigned arp=0;arp<4;arp++)for(unsigned range=1;range<=6;range++)for(unsigned flags=0;flags<16;flags++){
  memset(&keys,0,sizeof keys);memset(&v,0,sizeof v);memset(records,0,sizeof records);for(unsigned i=0;i<4;i++)v.vca[i]=6;
  assert(original_chicago_allocator_bind(&a,&keys,held,records,4,4,2,mode,flags&1u,(flags>>1)&1u));
  a.arp_mode=arp;a.arp_range=range;a.scan_index=-1;a.previous_index=60;a.octave=1;
  selector=(original_chicago_selector){-1,60,1,0,0x6d70};control=(chicago_arp_control_input){(flags>>2)&1u,(flags>>3)&1u,0};
  assert(original_chicago_scheduler_bind(&scheduler,&a,&selector,&clock,&ci,&control,&callbacks,&v));assert(original_chicago_scheduler_prepare(&scheduler));valid(&keys);
  for(unsigned step=0;step<256;step++){
   unsigned key=(step*73+flags*11)%128;
   assert(original_chicago_scheduler_note(&scheduler,(int32_t)key,step%3?127:1));valid(&keys);
   if(step%7==0){assert(original_chicago_scheduler_note(&scheduler,(int32_t)key,83));valid(&keys);}
   assert(original_chicago_scheduler_select(&scheduler)==1);valid(&keys);
   if(step%4==0){assert(original_chicago_scheduler_note(&scheduler,(int32_t)key,0));valid(&keys);}
   if(step%13==0){control.latch^=1;assert(original_chicago_scheduler_prepare(&scheduler));valid(&keys);}
   if(step%17==0){assert(original_chicago_scheduler_damper(&scheduler,127));valid(&keys);assert(original_chicago_scheduler_damper(&scheduler,0));valid(&keys);}
   if(step%29==0){control.enable^=1;assert(original_chicago_scheduler_prepare(&scheduler));valid(&keys);}
   if(step%31==0){original_chicago_allocator_clear_keys(&a,&voice_callbacks,&v);valid(&keys);}
   if(step%37==0){original_chicago_allocator_all_notes_off(&a,&voice_callbacks,&v);valid(&keys);}
   if(step%41==0){original_chicago_allocator_reset_lists(&a);valid(&keys);}
  }
 }
 /* The count invariant is about cells, not held-record count. Test every
  * array cardinality, both edited-cell states and distinct latched contents. */
 for(unsigned n=0;n<=128;n++)for(unsigned key=0;key<128;key++){
  memset(&keys,0,sizeof keys);memset(&v,0,sizeof v);for(unsigned i=0;i<4;i++)v.vca[i]=6;
  for(unsigned i=0;i<n;i++)keys.physical[i]=1+i%127;
  for(unsigned i=n;i<128;i++)keys.latched[i]=127;
  keys.physical_count=n;keys.latched_count=128-n;
  assert(original_chicago_allocator_bind(&a,&keys,held,records,4,4,1,1,0,1));valid(&keys);
  assert(original_chicago_allocator_canonical_note(&a,(int32_t)key,127,&voice_callbacks,&v));valid(&keys);
  assert(original_chicago_allocator_canonical_note(&a,(int32_t)key,0,&voice_callbacks,&v));valid(&keys);
 }
 /* Guarded generated-current lookup, covering all raw adjacent aliases. */
 for(unsigned alias=128;alias<256;alias++)for(unsigned present=0;present<2;present++){
  memset(&keys,0,sizeof keys);memset(&v,0,sizeof v);for(unsigned i=0;i<4;i++)v.vca[i]=6;
  keys.physical[alias-128]=present?127:0;keys.physical_count=present;keys.latched[0]=100;keys.latched_count=1;keys.committed_enable=keys.effective_latch=1;
  assert(original_chicago_allocator_bind(&a,&keys,held,records,4,4,1,1,0,0));keys.current_note=(int32_t)alias;
  selector=(original_chicago_selector){-1,60,1,0,0x6d70};control=(chicago_arp_control_input){1,0,0};
  assert(original_chicago_scheduler_bind(&scheduler,&a,&selector,&clock,&ci,&control,&callbacks,&v));assert(original_chicago_scheduler_prepare(&scheduler));valid(&keys);
 }
 /* Show why the broader low-level entry is outside the exclusion proof. */
 memset(&keys,0,sizeof keys);memset(&v,0,sizeof v);for(unsigned i=0;i<4;i++)v.vca[i]=6;
 assert(original_chicago_allocator_bind(&a,&keys,held,records,4,4,1,1,0,0));
 assert(original_chicago_allocator_key_event(&a,60,0,&voice_callbacks,&v));assert(!original_chicago_canonical_keys_valid(&keys));
 chicago_arp_control bad=keys;assert(!original_chicago_canonical_keys_valid(&keys)&&!memcmp(&bad,&keys,sizeof keys));
 printf("PASS %llu canonical count-invariant boundaries,1536 real allocator/scheduler sequences,all128 keys/cardinalities and256 generated-current latch exits;24 selector cycles excluded only in closed canonical domain; low-level zero continuation remains rejected by admission\n",(unsigned long long)checks);return 0;
}
