/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ORIGINAL_CHICAGO_CHORUS_SPAN_H
#define ORIGINAL_CHICAGO_CHORUS_SPAN_H
#include <stdint.h>
#include <limits.h>
/* Exact triangle/modulator recurrence on zero history. Between signed phase
 * wrap/turn points, magnitude is affine. If the modulator's integer delta d
 * stays constant, its rounding condition is
 *   2^24*d <= magnitude(k) - 2^16*y(k-1) < 2^24*(d+1).
 * Both sides are affine in k: checking the endpoints proves the whole span.
 * Jump to the first rounding/phase/clipping boundary and repeat there. No
 * geometric approximation or float smoother replacement is used. */
static inline int32_t original_chicago_chorus_span(int32_t phase,int32_t speed,int32_t y,uint32_t n){
 while(n){
  uint32_t bits=(uint32_t)phase+(uint32_t)speed;
  phase=bits<=INT32_MAX?(int32_t)bits:-1-(int32_t)(UINT32_MAX-bits);
  uint32_t magnitude=phase<0?0u-(uint32_t)phase:(uint32_t)phase;
  int32_t z=y*8160+(int32_t)(magnitude>>11);
  int32_t next=z< -268427264?-32767:z>=268435456?32767:
      (z>=0?z/8192:-1-(int32_t)((uint32_t)(-1-z)/8192));
  int32_t delta=next-y;y=next;--n;
  if(!n)break;
  int64_t p=phase,step=speed;
  int64_t lo=p<0?INT32_MIN:0,hi=p<0?-1:INT32_MAX;
  uint32_t k=n;
  int64_t end=p+step*k;
  if(end<lo||end>hi){
   /* A signed half-cycle is at most 2^31 samples wide. Both magnitudes
    * fit uint32_t, including INT32_MIN speed; AC79 needs no 64-bit divide. */
   uint32_t distance=(uint32_t)(step>0?hi-p:p-lo);
   uint32_t stride=step>0?(uint32_t)speed:0u-(uint32_t)speed;
   k=distance/stride;
  }
  if(!k)continue;
  int64_t ey=(int64_t)y+(int64_t)delta*k;
  if(ey< -32767||ey>32767)k=(uint32_t)(delta>0?(32767-y)/delta:(y+32767)/-delta);
  if(!k)continue;
  int64_t slope=(p<0?-step:step)-(int64_t)65536*delta;
  int64_t start=(int64_t)magnitude-(int64_t)65536*y+(int64_t)65536*delta;
  int64_t lower=(int64_t)16777216*delta,upper=lower+16777215;
  int64_t first=start+slope,last=start+slope*k;
  if(first<lower||first>upper)continue;
  if(last<lower||last>upper){
   /* first is already inside the 24-bit rounding interval. Count further
    * steps from there, so the dividend and any dividing stride fit 32 bits. */
   uint32_t room=(uint32_t)(slope>0?upper-first:first-lower);
   uint64_t stride=(uint64_t)(slope>0?slope:-slope);
   k=1u+(stride>room?0u:room/(uint32_t)stride);
  }
  phase=(int32_t)(p+step*k);y+=(int32_t)((int64_t)delta*k);n-=k;
 }
 return y;
}
#endif
