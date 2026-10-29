/* SPDX-License-Identifier: GPL-3.0-only */
#include "chicago_program.h"
#include "chicago_arp_control.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef struct {chicago_arp_control *state;chicago_arp_control_input *in;unsigned cleanup,phase,off,steps,notify;} context;
static void held(void *p){(void)p;}
static void off(void *p,int32_t note){context *c=p;assert(note>=0&&note<=255);c->off++;}
static void voices(void *p){(void)p;}
static uint32_t velocity(void *p,int32_t n){context *c=p;assert(n>=0&&n<256);return n<128?c->state->physical[n]:c->state->latched[n-128];}
static void cleanup_held(void *p){context *c=p;assert(c->in->damper_byte==0);c->phase=1;}
static void cleanup_voices(void *p){context *c=p;assert(c->phase==1);c->phase=2;c->cleanup++;}
static void step(void *p){((context *)p)->steps++;}
static void notify(void *p){((context *)p)->notify++;}
int main(void){
 unsigned setters=0;
 for(unsigned factory=0;factory<81;factory++){
  original_chicago_program p;original_chicago_routing r;assert(original_chicago_program_init(&p,factory));
  /* Explicit fixture, not a claim about the unclosed native cold constructor.
   * Full allocator callbacks/selector remain owned by the real caller. */
  chicago_arp_control s;memset(&s,0,sizeof s);s.current_note=60;s.physical[60]=100;s.physical_count=1;
  chicago_arp_control_input in={0,0,0};context c={&s,&in,0,0,0,0,0};
  const chicago_arp_control_callbacks callbacks={held,off,voices,velocity};
  const chicago_arp_damper_callbacks damper={cleanup_held,cleanup_voices};
  const chicago_arp_clock_callbacks clock_callbacks={step,notify};
  assert(original_chicago_program_routing(&p,&r));in.enable=r.arp_enable;in.latch=r.arp_latch;
  chicago_arp_control_prepare(&s,&in,&callbacks,&c);
  for(unsigned i=0;i<6;i++){
   unsigned down=(i==1||i==4);assert(original_chicago_program_set(&p,21,(float)down));
   assert(original_chicago_program_routing(&p,&r));in.enable=r.arp_enable;in.latch=r.arp_latch;
   unsigned before=c.cleanup;c.phase=0;
   chicago_arp_damper_apply(&s,&in,r.damper,0,&damper,&callbacks,&c);setters++;
   assert(in.damper_byte==(down?127:0));assert(c.cleanup==before+(unsigned)!down);
   assert(s.effective_latch==(unsigned)(in.latch||down));
   /* Repeated pedal-up still runs cleanup immediately, without an audio tick. */
   if(!down)assert(c.phase==2);
  }
  chicago_arp_clock clock={0,0,0};chicago_arp_clock_prepare(&clock,44100,r.arp_base_note,r.arp_times);
  float expected=44100*2.5f;expected=expected*r.arp_base_note;expected=expected*r.arp_times;
  assert(clock.threshold==expected);
  chicago_arp_clock_key(&clock,r.arp_key_sync,1);assert(clock.key_reset==r.arp_key_sync);
  chicago_arp_clock_input clock_input={120,r.arp_tempo_sync};
  chicago_arp_clock_tick(&clock,&clock_input,&clock_callbacks,&c);
  assert(c.steps==r.arp_key_sync&&c.notify==0&&clock.accumulator==120);
 }
 printf("PASS81programs -> original ARP enable/latch/base/times/key/tempo routing; %u immediate canonical damper setters, allocator callbacks explicit\n",setters);
 return 0;
}
