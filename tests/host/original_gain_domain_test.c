/* SPDX-License-Identifier: GPL-3.0-only */
#include "miami_program.h"
#include "oscillator_motion.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static float fl(uint32_t u){float f;memcpy(&f,&u,4);return f;}
int main(void) {
 static const unsigned levels[4]={3,7,11,15};
 original_osc_controls c={{10000000,11000000,12000000,13000000},{0,0,0,0},{.5f,.5f,.5f,.5f},{0,1,2,3},.25f};
 unsigned beyond_old_bound=0;double output_sum=0;
 for(unsigned factory=0;factory<30;factory++) {
  original_miami_program p;assert(original_miami_program_init(&p,factory));
  for(unsigned i=0;i<4;i++) {
   float normalized;assert(original_miami_program_native(&p,levels[i],&normalized));
   /* Original curve0 at b4b34..b4d84: squared clamped level then
    * normalization bits3faaa993. This is a physical gain, not normalized. */
   c.gain[i]=(normalized*normalized)*fl(0x3faaa993);
   beyond_old_bound+=(c.gain[i]>1);
  }
  original_prepared_oscillators ready;assert(original_oscillator_prepare(&ready,&c));
  original_oscillators phase={{0,0,0,0}};
  for(unsigned sample=0;sample<512;sample++) {
   float y;assert(original_oscillator_sample_prepared(&phase,&ready,0,&y));
   output_sum+=y;
  }
 }
 assert(beyond_old_bound>0);
 printf("PASS all30 actual original level contexts render, %u physical gains exceed old normalized1 bound; checksum%.9f\n",beyond_old_bound,output_sum);
 return 0;
}
