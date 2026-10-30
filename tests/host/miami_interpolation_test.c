/* SPDX-License-Identifier: GPL-3.0-only */
#include "miami_interpolation.h"
#include "miami_ms20.h"
#include "miami_n513_private.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
static uint32_t rng=0x68479u;
static unsigned checks;
static uint32_t random_word(void){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
static void check(uint32_t d,uint32_t f,uint32_t a){
 uint32_t want;
 if(!original_fma_finite_bits_reference(d,f,a,&want))abort();
 uint32_t got=original_math_bits(original_miami_interpolate_rne(original_math_float(d),original_math_float(f),original_math_float(a)));
 if(got!=want){fprintf(stderr,"interpolation %08x * %08x + %08x: %08x != %08x\n",d,f,a,got,want);exit(1);}checks++;
}
static void nonlinear(uint32_t bits){
 float x=original_math_float(bits),u=512.0f*original_binary32_abs(x),want;
 if(u<512){
  unsigned j=(unsigned)u;
  float a=original_math_float(original_miami_n513_bits[j]),b=original_math_float(original_miami_n513_bits[j+1]);
  uint32_t fused;
  if(!original_fma_finite_bits_reference(original_math_bits(b-a),original_math_bits(u-(float)j),original_math_bits(a),&fused))abort();
  want=x*original_math_float(fused);
 }else{float clamp=x< -1?-1:x>1?1:x;want=clamp*original_math_float(original_miami_n513_bits[512]);}
 float got;
 if(!original_miami_nonlinear(x,&got)||original_math_bits(got)!=original_math_bits(want)){
  fprintf(stderr,"nonlinear mismatch %08x\n",bits);exit(1);
 }
 checks++;
}
int main(void){
 /* Exact half-way ties on both mantissa parities, the lower binade seam,
  * zero/subnormal fractions and the shift49/50 shortcut boundary. */
 const uint32_t bases[]={0x01000000,0x3e800000,0x3f000000,0x3f000001,0x3f000002,0x3fffffff,0x7f7fffff};
 const uint32_t fractions[]={0,1,0x007fffff,0x00800000,0x33000000,0x33800000,0x3e800000,0x3f000000,0x3f400000,0x3f7fffff};
 for(unsigned a=0;a<sizeof bases/sizeof bases[0];a++)for(unsigned f=0;f<sizeof fractions/sizeof fractions[0];f++){
  check(0xb3800000,fractions[f],bases[a]);check(0xbe000000,fractions[f],bases[a]);
  check(0xb3000000,fractions[f],bases[a]);check(0x80000000,fractions[f],bases[a]);
 }
 for(unsigned k=0;k<300000;k++){
  /* Broad finite domains exercise every fallback sign/exponent class. */
  uint32_t a=random_word(),d=random_word(),f=random_word();
  if((a&0x7f800000u)==0x7f800000u)a^=0x00800000u;
  if((d&0x7f800000u)==0x7f800000u)d^=0x00800000u;
  if((f&0x7f800000u)==0x7f800000u)f^=0x00800000u;
  check(d,f,a);
  /* Guarded domains force both sides of binade/rounding/shift boundaries. */
  unsigned ea=3+random_word()%252,ed=1+random_word()%(ea-2),ef=random_word()%127;
  a=(ea<<23)|(random_word()&0x7fffffu);
  if(k%4==0)a=ea<<23;
  d=0x80000000u|(ed<<23)|(random_word()&0x7fffffu);
  f=(ef<<23)|(random_word()&0x7fffffu);check(d,f,a);
 }
 for(unsigned j=0;j<=512;j++){
  uint32_t x=original_math_bits((float)j/512.0f);
  for(int delta=-1;delta<=1;delta++){
   uint32_t p=x+(uint32_t)delta;
   if((p&0x7f800000u)!=0x7f800000u){nonlinear(p);nonlinear(p^0x80000000u);}
  }
 }
 for(unsigned k=0;k<300000;k++){
  uint32_t x=random_word();
  if((x&0x7f800000u)!=0x7f800000u)nonlinear(x);
 }
 printf("PASS Miami interpolation/nonlinear: %u exact 18-limb oracle comparisons\n",checks);
 return 0;
}
