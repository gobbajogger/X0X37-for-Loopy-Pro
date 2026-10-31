/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ORIGINAL_MASTER_TUNE_RNE_H
#define ORIGINAL_MASTER_TUNE_RNE_H
#include "binary32_math.h"
/* Exact finite RNE specialization of float((double(value)/ln2)*12.0).
 * ln2 is the original binary64 0x3fe62e42fefa39ef. Each binary64 operation is
 * rounded separately before the final binary32 rounding. No approximation,
 * reciprocal substitution, double-runtime archive or altered logf provider.
 * At most53 restoring division steps; no dynamic allocation or integer divide.
 * This is the existing software-RNE arithmetic path, not FP-status emulation. */
static inline uint64_t original_round_shift_rne(uint64_t x,unsigned shift){
 uint64_t q=x>>shift,mask=(UINT64_C(1)<<shift)-1u,r=x&mask,half=UINT64_C(1)<<(shift-1u);
 return q+(r>half||(r==half&&(q&1u)));
}
static inline float original_master_tune_semitones_rne(float value){
 uint32_t raw=original_math_bits(value),sign=raw&0x80000000u,m=raw&0x7fffffu,e=(raw>>23)&255u;
 if(!e&&!m)return value;
 int exponent;
 if(e){m|=0x800000u;exponent=(int)e-127;}
 else{exponent=-126;while(!(m&0x800000u)){m<<=1;exponent--;}}
 const uint64_t divisor=UINT64_C(0x162e42fefa39ef);
 uint64_t remainder=(uint64_t)m<<29,q=0;unsigned steps=53;
 if(remainder>=divisor){remainder-=divisor;q=1;steps=52;exponent++;}
 for(unsigned i=0;i<steps;i++){
  remainder<<=1;q<<=1;if(remainder>=divisor){remainder-=divisor;q|=1;}
 }
 uint64_t twice=remainder<<1;
 if(twice>divisor||(twice==divisor&&(q&1u)))q++;
 if(q==(UINT64_C(1)<<53)){q>>=1;exponent++;}
 uint64_t product=q*3u;unsigned shift=product>=(UINT64_C(1)<<54)?2u:1u;
 q=original_round_shift_rne(product,shift);exponent+=(int)shift+2;
 if(q==(UINT64_C(1)<<53)){q>>=1;exponent++;}
 if(exponent>127)return original_math_float(sign|0x7f800000u);
 if(exponent< -126){
  uint32_t fraction=(uint32_t)original_round_shift_rne(q,(unsigned)(-exponent-97));
  return original_math_float(sign|fraction);
 }
 uint32_t rounded=(uint32_t)original_round_shift_rne(q,29);
 if(rounded==0x1000000u){rounded>>=1;exponent++;}
 if(exponent>127)return original_math_float(sign|0x7f800000u);
 return original_math_float(sign|((uint32_t)(exponent+127)<<23)|(rounded&0x7fffffu));
}
#endif
