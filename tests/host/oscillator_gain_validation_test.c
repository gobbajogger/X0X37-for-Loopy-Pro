/* SPDX-License-Identifier: GPL-3.0-only */
#include "../../experiments/original_synth/oscillator_motion.c"
#include <stdio.h>
#include <stdlib.h>

static unsigned checks;
static void check(uint32_t u){
 float gain=original_math_float(u);
 int expected=original_binary32_isfinite(gain)&&gain>=0&&gain<=ORIGINAL_OSCILLATOR_GAIN_MAX;
 if(oscillator_gain_valid(gain)!=expected){
  fprintf(stderr,"gain validation differs for %08x\n",u);exit(1);
 }
 checks++;
}
int main(void){
 for(unsigned exponent=0;exponent<256;exponent++){
  uint32_t base=(uint32_t)exponent<<23;
  for(unsigned i=0;i<32;i++){
   check(base+i);check(base+i+UINT32_C(0x80000000));
   check(base+UINT32_C(0x7fffff)-i);check(base+UINT32_C(0x807fffff)-i);
  }
 }
 uint32_t max=bits(ORIGINAL_OSCILLATOR_GAIN_MAX);
 for(int i=-4096;i<=4096;i++){check(max+(uint32_t)i);check((max+(uint32_t)i)|UINT32_C(0x80000000));}
 uint32_t random=UINT32_C(0x791684);
 for(unsigned i=0;i<1000000;i++){
  random^=random<<13;random^=random>>17;random^=random<<5;check(random);
 }
 original_osc_controls controls={0};
 for(unsigned i=0;i<4;i++){controls.increment[i]=UINT32_C(0x1234567);controls.shape[i]=(uint8_t)i;controls.width[i]=.5f;}
 original_prepared_oscillators prepared;
 if(!original_oscillator_prepare(&prepared,&controls))return 1;
 const uint32_t invalid[]={1u<<31|1u,UINT32_C(0xbf800000),UINT32_C(0x7f800000),UINT32_C(0xff800000),UINT32_C(0x7fc00001),UINT32_C(0x7f800001),max+1};
 for(unsigned i=0;i<4;i++)for(unsigned j=0;j<sizeof invalid/sizeof invalid[0];j++){
  original_oscillators phase={{0,UINT32_C(0x80000000),UINT32_C(0x7fffffff),UINT32_MAX}},old=phase;
  float gain[4]={.25f,.5f,.75f,1},out=123;
  gain[i]=original_math_float(invalid[j]);
  if(original_oscillator_sample_gains(&phase,&prepared,gain,1,&out)||memcmp(&phase,&old,sizeof phase)||out!=123){
   fprintf(stderr,"invalid gain changed oscillator state/output\n");return 1;
  }
 }
 original_oscillators phase={{0,0,0,0}};
 float gain[4]={0,original_math_float(UINT32_C(0x80000000)),ORIGINAL_OSCILLATOR_GAIN_MAX,1},out;
 if(!original_oscillator_sample_gains(&phase,&prepared,gain,1,&out))return 1;
 printf("oscillator gain: %u predicate comparisons and invalid-state guards PASS\n",checks);
 return 0;
}
