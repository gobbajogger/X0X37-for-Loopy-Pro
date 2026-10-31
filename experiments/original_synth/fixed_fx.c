/* SPDX-License-Identifier: GPL-3.0-only */
#include "fixed_fx.h"
#include "../wobble/miami_fx_conversion.h"
#include <stddef.h>
static int32_t signed32(uint32_t x) {
 return x<=INT32_MAX?(int32_t)x:-1-(int32_t)(UINT32_MAX-x);
}
static int32_t asr15(int32_t x) {
 return x>=0?x/32768:-1-(int32_t)((uint32_t)(-(int64_t)x)-1)/32768;
}
static int32_t product_shift(int32_t a,int32_t b) {
 return asr15(signed32((uint32_t)a*(uint32_t)b));
}
static int32_t mix(int32_t dry,int16_t processed,int32_t gain) {
 int32_t sum=product_shift((int32_t)processed,gain)+product_shift(dry,32767-gain);
 return sum>32767?32767:sum< -32767?-32767:sum;
}
void original_fixed_fx_init(original_fixed_fx *s) {
 if(!s)return;
 s->target=0;s->gain_state=0;s->active_enable=1;s->pending_enable=1;s->dirty=1;
}
void original_fixed_fx_internal_enable(original_fixed_fx *s,uint8_t value) {
 if(!s)return;
 s->pending_enable=value;s->dirty=1;
}
void original_fixed_fx_parameter16(original_fixed_fx *s) { original_fixed_fx_internal_enable(s,1); }
void original_fixed_fx_reset(original_fixed_fx *s,original_effect_reset_all reset,void *context) {
 if(!s||!reset)return;
 s->target=32767;s->gain_state=1073709056;reset(context);
}
int original_fixed_fx_process(original_fixed_fx *s,const int32_t *dryL,const int32_t *dryR,
 int16_t *scratchL,int16_t *scratchR,int32_t *outL,int32_t *outR,uint32_t n,
 original_effect_process effect,void *context) {
 if(!s||!effect||n>ORIGINAL_FX_CAPACITY)return 0;
 if(n&&(!dryL||!dryR||!scratchL||!scratchR||!outL||!outR))return 0;
 if(s->dirty){s->active_enable=s->pending_enable;s->target=s->active_enable?32767:0;s->dirty=0;}
 for(uint32_t i=0;i<n;i++){scratchL[i]=miami_fx_low16(dryL[i]);scratchR[i]=miami_fx_low16(dryR[i]);}
 effect(context,scratchL,scratchR,n);
 for(uint32_t i=0;i<n;i++) {
  int32_t g=asr15(s->gain_state);
  if(g!=s->target){s->gain_state=signed32((uint32_t)s->gain_state+(uint32_t)(s->target-g)*32511u);g=asr15(s->gain_state);}
  outL[i]=mix(dryL[i],scratchL[i],g);outR[i]=mix(dryR[i],scratchR[i],g);
 }
 return 1;
}
