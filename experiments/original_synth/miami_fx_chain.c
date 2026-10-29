/* SPDX-License-Identifier: GPL-3.0-only */
#include "miami_fx_chain.h"
#include "../wobble/miami_fx_conversion.h"
#include <stddef.h>
static void decimate(void *context,int16_t *l,int16_t *r,uint32_t n) {
 (void)miami_decimator_adapter_process(context,l,r,n,NULL,0);
}
static void bypass(void *context,int16_t *l,int16_t *r,uint32_t n) {(void)context;(void)l;(void)r;(void)n;}
/* Reference FX2 wrapper during the target sound-regression investigation.
 * The original mixer always advances and rounds samples, even when the DSP
 * algorithm selected inside that wrapper is a no-op. Idle bypass still uses
 * the independent exact-zero output seam and does not enter this path. */
void original_miami_fx_bypass_process(original_fixed_fx *s,original_miami_fx_workspace *w,uint32_t n) {
 (void)original_fixed_fx_process(s,w->dryL,w->dryR,w->scratchL,w->scratchR,w->dryL,w->dryR,n,bypass,NULL);
}
static void reset_decimator(void *context) {(void)miami_decimator_adapter_reset(context);}
static void reset_bypass(void *context) {(void)context;}
void original_miami_fx_init(original_miami_fx_chain *s) {
 if(!s)return;
 original_fixed_fx_init(&s->fx1);original_fixed_fx_init(&s->fx2);
 (void)miami_decimator_adapter_init(&s->decimator);
}
void original_miami_fx_reset(original_miami_fx_chain *s) {
 if(!s)return;
 /* This projected graph owns only its fixed selected effects. It does not
  * emulate the original wrapper's unused effect vector or switching flags. */
 original_fixed_fx_reset(&s->fx1,reset_decimator,&s->decimator);
 original_fixed_fx_reset(&s->fx2,reset_bypass,NULL);
}
int original_miami_fx_process(original_miami_fx_chain *s,original_miami_fx_workspace *w,
 const uint32_t *inL,const uint32_t *inR,uint32_t *outL,uint32_t *outR,uint32_t n,unsigned fz) {
 if(!s||!w||fz>1||n>ORIGINAL_FX_CAPACITY)return 0;
 if(!n)return 1;
 if(!inL||!inR||!outL||!outR||n>w->capacity||!w->dryL||!w->dryR||!w->scratchL||!w->scratchR)return 0;
 for(uint32_t i=0;i<n;i++) {
  uint32_t scaled;
  (void)miami_fx_scaled_rne(miami_fx_cleanup_rne(inL[i]),fz,&scaled);w->dryL[i]=miami_fx_fcvtzs(scaled);
  (void)miami_fx_scaled_rne(miami_fx_cleanup_rne(inR[i]),fz,&scaled);w->dryR[i]=miami_fx_fcvtzs(scaled);
 }
 (void)original_fixed_fx_process(&s->fx1,w->dryL,w->dryR,w->scratchL,w->scratchR,w->dryL,w->dryR,n,decimate,&s->decimator);
 original_miami_fx_bypass_process(&s->fx2,w,n);
 for(uint32_t i=0;i<n;i++){outL[i]=miami_fx_output_rne((int16_t)w->dryL[i]);outR[i]=miami_fx_output_rne((int16_t)w->dryR[i]);}
 return 1;
}
