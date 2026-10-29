/* SPDX-License-Identifier: GPL-3.0-only */
#include "miami_selected_voice.h"
#include "miami_output.h"
#include "motion_tables_private.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static int32_t dryl[128],dryr[128];static int16_t scratchl[128],scratchr[128];
static original_miami_fx_workspace work={dryl,dryr,scratchl,scratchr,128};
static uint32_t bits(float f){uint32_t u;memcpy(&u,&f,4);return u;}
static float fl(uint32_t u){float f;memcpy(&f,&u,4);return f;}
int main(int argc,char **argv){
 assert(argc==2);FILE *stream=fopen(argv[1],"wb");assert(stream);
 const original_ms20_tables filter={original_miami_a44100,original_miami_b44100,original_miami_r,original_miami_cap};
 original_miami_voice_tables tables;assert(original_miami_voice_tables_bind_immutable(&tables,&filter,original_pitch44100));
 unsigned frames=0;double energy=0;
 for(unsigned factory=0;factory<30;factory++) {
  original_miami_program program;original_miami_voice_controls control;original_miami_selected_voice voice[2];original_miami_output output={0};
  assert(original_miami_program_init(&program,factory)&&original_miami_voice_compile(&control,&program));
  original_miami_output_init(&output);assert(original_miami_output_apply(&output,&program,10));
  for(unsigned side=0;side<2;side++) {
   assert(original_miami_selected_voice_init(&voice[side],&tables,side,60));
   original_miami_voice_note n;memset(&n,0,sizeof n);n.note_key=48;n.velocity=100;n.last_note=60;n.tuned_pitch=48;n.unison_coordinate=side?-.5f:.5f;n.unison_flag=side?0:2;
   n.mg[0].phase=n.mg[1].phase=-1;n.mg[0].noise=n.mg[1].noise=0x6d70;
   /* Original voice order is explicit: side1 copies side0 AFTER its forced
    * note helper. This is an assigned-note graph, not an allocator emulator. */
   if(side)memcpy(n.mg,voice[0].mg,sizeof n.mg);
   assert(original_miami_selected_voice_full_note(&voice[side],&control,&tables,&n,137));
  }
  for(unsigned block=0;block<48;block++) {
   uint32_t left[128],right[128];
   if(block>=8&&block<26) {
    unsigned macro=block-8;assert(original_miami_program_set(&program,macro,(macro&1)?1:0));
    assert(original_miami_voice_compile(&control,&program)&&original_miami_output_apply(&output,&program,macro));
    for(unsigned side=0;side<2;side++)assert(original_miami_selected_voice_pedal(&voice[side],&control));
   }
   if(block==4)for(unsigned side=0;side<2;side++)assert(original_miami_selected_voice_partial_note(&voice[side],&control,&tables,60,110,60,15,137));
   if(block==30)for(unsigned side=0;side<2;side++)assert(original_miami_selected_voice_note_off(&voice[side],&control));
   if(block==36){assert(original_miami_program_set(&program,17,0)&&original_miami_voice_compile(&control,&program));for(unsigned side=0;side<2;side++)assert(original_miami_selected_voice_pedal(&voice[side],&control));}
   for(unsigned i=0;i<128;i++) {
    float L=0,R=0;
    for(unsigned side=0;side<2;side++) {float l,r;unsigned action;assert(original_miami_selected_voice_sample(&voice[side],&control,&tables,137,&l,&r,&action));L=L+l;R=R+r;}
    left[i]=bits(L);right[i]=bits(R);
   }
   assert(original_miami_output_process(&output,&program,&work,left,right,left,right,128,0,NULL,0));
   for(unsigned i=0;i<128;i++){assert(isfinite(fl(left[i]))&&isfinite(fl(right[i])));energy+=(double)fl(left[i])*fl(left[i])+(double)fl(right[i])*fl(right[i]);}
   assert(fwrite(left,sizeof left,1,stream)==1&&fwrite(right,sizeof right,1,stream)==1);frames+=128;
  }
  assert(!voice[0].active&&!voice[1].active);
 }
 assert(fclose(stream)==0&&energy>0);
 printf("PASS joined30-factory assigned two-voice graph, all18 controls, ordered unison MG, partial notes, damper/release, Decimator and master; %u stereoframes, energy%.9f. Allocator/startup/firmware integration remain excluded.\n",frames,energy);return 0;
}
