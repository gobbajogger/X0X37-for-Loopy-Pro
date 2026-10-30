/* SPDX-License-Identifier: GPL-3.0-only */
#include "miami_fx_chain.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"failed line %d: %s\n",__LINE__,#x); exit(1); } } while (0)
static void bypass(void *p,int16_t *l,int16_t *r,uint32_t n){(void)p;(void)l;(void)r;(void)n;}
static int32_t dry_a[2][256],dry_b[2][256];
static int16_t scratch_a[2][256],scratch_b[2][256];
static original_miami_fx_workspace workspace={dry_a[0],dry_a[1],scratch_a[0],scratch_a[1],256};
static unsigned checks;
static void compare(original_fixed_fx initial,int first,unsigned n){
 original_fixed_fx a=initial,b=initial;
 for(unsigned i=0;i<256;i++){
  int32_t value=first+(int)i;if(value>32767)value=32767;
  dry_a[0][i]=dry_b[0][i]=value;dry_a[1][i]=dry_b[1][i]=-value;
  scratch_a[0][i]=scratch_b[0][i]=1234;scratch_a[1][i]=scratch_b[1][i]=-1234;
 }
 original_miami_fx_bypass_process(&a,&workspace,n);
 CHECK(original_fixed_fx_process(&b,dry_b[0],dry_b[1],scratch_b[0],scratch_b[1],dry_b[0],dry_b[1],n,bypass,NULL));
 CHECK(!memcmp(&a,&b,sizeof a));CHECK(!memcmp(dry_a,dry_b,sizeof dry_a));
 CHECK(!memcmp(scratch_a,scratch_b,sizeof scratch_a));checks++;
}
int main(void){
 /* Every admitted dry value in each channel, with both settled endpoints and
  * the low, middle and high fractional gain-state remainders. */
 const unsigned residues[]={0,1,16384,32767};
 for(unsigned endpoint=0;endpoint<2;endpoint++)for(unsigned r=0;r<4;r++){
  original_fixed_fx s={0};s.target=endpoint?32767:0;
  s.gain_state=s.target*32768+(int32_t)residues[r];s.active_enable=s.pending_enable=(uint8_t)endpoint;
  for(int first=-32767;first<=32767;first+=256){
   unsigned n=(unsigned)(32768-first);if(n>256)n=256;
   compare(s,first,n);
  }
 }
 /* Initial ramp, near-settled values, dirty toggles, disabled wrappers,
  * arbitrary signed gain states and zero-length calls retain checked code. */
 const int32_t states[]={INT32_MIN,-32769,-1,0,1,32767,32768,1073709055,1073709056,INT32_MAX};
 const unsigned sizes[]={0,1,31,63,64,65,255,256};
 for(unsigned k=0;k<sizeof states/sizeof states[0];k++)for(unsigned pending=0;pending<2;pending++)
 for(unsigned dirty=0;dirty<2;dirty++)for(unsigned endpoint=0;endpoint<2;endpoint++)
 for(unsigned n=0;n<sizeof sizes/sizeof sizes[0];n++){
  original_fixed_fx s={0};s.target=endpoint?32767:0;s.gain_state=states[k];
  s.active_enable=(uint8_t)endpoint;s.pending_enable=(uint8_t)pending;s.dirty=(uint8_t)dirty;
  compare(s,-128,sizes[n]);compare(s,-32767,sizes[n]);compare(s,32512,sizes[n]);
 }
 printf("PASS Miami bypass: %u complete state/scratch/output comparisons; exhaustive signed dry range\n",checks);
 return 0;
}
