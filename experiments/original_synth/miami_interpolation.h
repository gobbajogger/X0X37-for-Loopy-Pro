/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ORIGINAL_MIAMI_INTERPOLATION_H
#define ORIGINAL_MIAMI_INTERPOLATION_H
#include "binary32_math.h"
#include "fused_reference.h"

/* Exact RNE base + difference*fraction for a positive normal base, a small
 * negative normal difference and fraction in [0,1). Other domains retain the
 * complete software FMA. No coefficient values or approximations are stored.
 * Adjacent nonlinear table entries normally satisfy these exponent guards. */
static inline float original_miami_interpolate_rne(float difference,float fraction,float base){
 uint32_t a=original_math_bits(base),d=original_math_bits(difference),f=original_math_bits(fraction);
 unsigned ea=a>>23,ed=(d>>23)&255u,ef=f>>23;
 if(ea<2||ea>=255||!(d>>31)||!ed||ed+1>=ea||ef>=127)
  return original_rne_fmaf(difference,fraction,base);
 /* A subnormal fraction is far below even the narrower midpoint below a
  * binade boundary. The same bound applies to a sufficiently small product. */
 if(!ef)return base;
 unsigned shift=ea-ed-ef+150u;
 if(shift>=50)return base;
 uint64_t product=(uint64_t)((d&0x7fffffu)|0x800000u)*((f&0x7fffffu)|0x800000u);
 uint32_t mantissa=(a&0x7fffffu)|0x800000u;
 uint32_t q=mantissa-(uint32_t)(product>>shift);
 uint64_t remainder=product&((UINT64_C(1)<<shift)-1);
 /* |difference|<base/2 and fraction<1 keep the exact result above base/2.
  * It can cross at most one binade; then use half-sized output units. */
 if(q<0x800000u||(q==0x800000u&&remainder)){
  --ea;--shift;mantissa<<=1;
  q=mantissa-(uint32_t)(product>>shift);
  remainder=product&((UINT64_C(1)<<shift)-1);
 }
 uint64_t half=UINT64_C(1)<<(shift-1);
 if(remainder>half||(remainder==half&&(q&1)))--q;
 if(q==0x1000000u){q>>=1;++ea;}
 return original_math_float((ea<<23)|(q-0x800000u));
}
#endif
