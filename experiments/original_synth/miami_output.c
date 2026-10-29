/* SPDX-License-Identifier: GPL-3.0-only */
#include "miami_output.h"
#include "binary32_classify.h"
#include "../wobble/miami_fx_conversion.h"
#include <math.h>
#ifdef ORIGINAL_USE_SOFT_FMA
#include "fused_reference.h"
#define ref_fma original_rne_fmaf
#else
#define ref_fma fmaf
#endif
typedef struct {miami_decimator_adapter_t *effect;const uint32_t *frames;uint32_t count;} effect_context;
static void decimate(void *context,int16_t *l,int16_t *r,uint32_t n){
 effect_context *c=context;(void)miami_decimator_adapter_process(c->effect,l,r,n,c->frames,c->count);
}
static void bypass(void *context,int16_t *l,int16_t *r,uint32_t n){(void)context;(void)l;(void)r;(void)n;}
void original_miami_output_init(original_miami_output *s){if(s){original_miami_fx_init(&s->fx);original_master_init(&s->master);}}
int original_miami_output_apply(original_miami_output *s,const original_miami_program *p,unsigned id){
 if(!s||!p||id>=18)return 0;
 if(id!=10)return 1;
 float frequency,wet;
 if(!original_miami_program_native(p,110,&frequency)||!original_miami_program_native(p,111,&wet)||!original_binary32_isfinite(frequency)||!original_binary32_isfinite(wet)||frequency<0||frequency>1||wet<0||wet>1)return 0;
 int32_t a=(int32_t)ref_fma(frequency,127,.5f),b=(int32_t)ref_fma(wet,127,.5f);
 (void)miami_decimator_adapter_set(&s->fx.decimator,1,a);
 (void)miami_decimator_adapter_set(&s->fx.decimator,2,b);return 1;
}
int original_miami_output_process(original_miami_output *s,const original_miami_program *p,original_miami_fx_workspace *w,const uint32_t *il,const uint32_t *ir,uint32_t *ol,uint32_t *orr,uint32_t n,unsigned fz,const uint32_t *events,uint32_t ne){
 if(!s||!p||!w||fz>1||n>ORIGINAL_FX_CAPACITY||(ne&&!events))return 0;
 if(!n)return ne==0;
 if(!il||!ir||!ol||!orr||n>w->capacity||!w->dryL||!w->dryR||!w->scratchL||!w->scratchR)return 0;
 for(uint32_t i=0;i<ne;i++)if(events[i]>=n||(i&&events[i]<events[i-1]))return 0;
 float volume;if(!original_miami_program_native(p,96,&volume)||!original_binary32_isfinite(volume)||volume<0||volume>1)return 0;
 if(!original_binary32_isfinite(s->master.current)||s->master.current<0||s->master.current>4)return 0;
 for(uint32_t i=0;i<n;i++) {
  uint32_t scaled;(void)miami_fx_scaled_rne(miami_fx_cleanup_rne(il[i]),fz,&scaled);w->dryL[i]=miami_fx_fcvtzs(scaled);
  (void)miami_fx_scaled_rne(miami_fx_cleanup_rne(ir[i]),fz,&scaled);w->dryR[i]=miami_fx_fcvtzs(scaled);
 }
 effect_context effect={&s->fx.decimator,events,ne};
 (void)original_fixed_fx_process(&s->fx.fx1,w->dryL,w->dryR,w->scratchL,w->scratchR,w->dryL,w->dryR,n,decimate,&effect);
 (void)original_fixed_fx_process(&s->fx.fx2,w->dryL,w->dryR,w->scratchL,w->scratchR,w->dryL,w->dryR,n,bypass,0);
 for(uint32_t i=0;i<n;i++){ol[i]=miami_fx_output_rne((int16_t)w->dryL[i]);orr[i]=miami_fx_output_rne((int16_t)w->dryR[i]);}
 return original_master_process(&s->master,volume*volume,ol,orr,ol,orr,n);
}
