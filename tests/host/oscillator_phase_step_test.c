/* SPDX-License-Identifier: GPL-3.0-only */
/* Include the implementation to exercise its internal bounded conversion. */
#include "../../experiments/original_synth/oscillator_motion.c"
#include <stdio.h>
#include <stdlib.h>

static unsigned checks;
static void check(uint32_t u){
 float value=original_math_float(u);
 int32_t actual=phase_step_int(value),expected=miami_fx_fcvtzs(u);
 if(actual!=expected){
  fprintf(stderr,"phase step %08x: %d != %d\n",u,actual,expected);
  exit(1);
 }
 checks++;
}
static void both(uint32_t u){check(u);check(u|UINT32_C(0x80000000));}
int main(void){
 /* Both signs of zero, subnormal/normal transitions, integer and exponent
  * boundaries, and every binary32 value near the saturating endpoints. */
 for(uint32_t u=0;u<4096;u++)both(u);
 for(unsigned exponent=1;exponent<=158;exponent++){
  uint32_t boundary=(uint32_t)exponent<<23;
  for(int offset=-16;offset<=16;offset++){
   uint32_t u=boundary+(uint32_t)offset;
   if(u<=UINT32_C(0x4f000000))both(u);
  }
 }
 for(uint32_t u=UINT32_C(0x4f000000)-4096;u<=UINT32_C(0x4f000000);u++)both(u);
 uint32_t random=UINT32_C(0x68479);
 for(unsigned i=0;i<1000000;i++){
  random^=random<<13;random^=random>>17;random^=random<<5;
  both(random%UINT32_C(0x4f000001));
 }
 printf("oscillator phase step: %u exact comparisons PASS\n",checks);
 return 0;
}
