/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ORIGINAL_CHICAGO_CLOCK_SPAN_H
#define ORIGINAL_CHICAGO_CLOCK_SPAN_H
#include "binary32_math.h"
/* Advance repeated positive binary32 additions strictly before a clock edge.
 * Within a binade, x+bpm rounds to a constant integer-ULP increment. One
 * literal addition within that binade resolves a possible halfway tie to an
 * even mantissa; the following increment then stays constant until the exponent changes.
 * Stop short of that boundary and do its addition literally too. This is NOT
 * x + bpm*n: the rounding at every original sample is preserved. The caller
 * supplies finite nonnegative accumulators and finite positive BPM/threshold.
 * The literal path also handles negative accumulators left by borrowed state.
 */
static inline uint32_t original_chicago_clock_span(float *value,float bpm,float threshold,uint32_t frames){
 float x=*value;uint32_t n=0;
 while(n<frames){
  float next=x+bpm;
  if(next>=threshold)break;
  x=next;++n;
  uint32_t u=original_math_bits(x),v=original_math_bits(x+bpm);
  if(x>=bpm&&u<0x7f800000u&&(u>>23)==(v>>23)){
   if(n==frames||original_math_float(v)>=threshold)break;
   x=original_math_float(v);++n;
   u=v;v=original_math_bits(x+bpm);
   if((u>>23)!=(v>>23))continue;
   uint32_t delta=v-u;
   if(!delta){n=frames;break;}
   uint32_t limit=((u>>23)+1u)<<23;
   uint32_t edge=original_math_bits(threshold);
   if(edge<limit)limit=edge;
   uint32_t room=limit-1u-u,remaining=frames-n;
   /* A settled, event-free 256-frame block fits one ULP-domain jump.
    * Avoid a target software integer divide when the whole span fits.
    * 4096*0xfffff fits uint32_t; larger/legal spans use the proof above. */
   uint32_t k;
   if(remaining<=4096u&&delta<=UINT32_C(0xfffff)&&remaining*delta<=room){
    k=remaining;
   }else{
    k=room/delta;
    if(k>remaining)k=remaining;
   }
   x=original_math_float(u+k*delta);n+=k;
  }
 }
 *value=x;return n;
}
#endif
