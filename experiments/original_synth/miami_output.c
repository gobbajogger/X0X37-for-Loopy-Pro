/* SPDX-License-Identifier: GPL-3.0-only */
#include "miami_output.h"
#include "miami_zero.h"
#include "binary32_classify.h"
#include "../wobble/miami_fx_conversion.h"
#include <math.h>
#include <string.h>
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
static int zero_state_ready(const original_miami_output *s){
 const original_fixed_fx *a=&s->fx.fx1,*b=&s->fx.fx2;
 if(a->dirty||b->dirty||a->gain_state/32768!=a->target||b->gain_state/32768!=b->target||
    a->gain_state<0||b->gain_state<0||s->fx.decimator.wet_gain<0||
    s->fx.decimator.kernel.held_left||s->fx.decimator.kernel.held_right)return 0;
 return 1;
}
static int zero_ready(const original_miami_output *s,const uint32_t *il,const uint32_t *ir,uint32_t n){
 if(!zero_state_ready(s))return 0;
 for(uint32_t i=0;i<n;i++)if((il[i]&0x7fffffffu)||(ir[i]&0x7fffffffu))return 0;
 return 1;
}
int original_miami_output_zero(original_miami_output *s,const original_miami_program *p,original_miami_fx_workspace *w,uint32_t *l,uint32_t *r,uint32_t n){
 float volume;
 if(!zero_state_ready(s)||!original_miami_program_native(p,96,&volume)||
    !original_binary32_isfinite(volume)||volume<0||volume>1||
    !original_binary32_isfinite(s->master.current)||s->master.current<0||s->master.current>4)return 0;
 (void)miami_decimator_advance_zero(&s->fx.decimator.kernel,n);
 original_miami_zero_words((uint32_t *)w->dryL,n);original_miami_zero_words((uint32_t *)w->dryR,n);
 original_miami_zero_halves((uint16_t *)w->scratchL,n);original_miami_zero_halves((uint16_t *)w->scratchR,n);
 original_miami_zero_words(l,n);original_miami_zero_words(r,n);
 volume=volume*volume;
 float target=s->master.desired_mute?0:volume*4;
 if(s->master.desired_mute==s->master.committed_mute&&s->master.current==target){
  /* No rounded recurrence is skipped while moving. Preserve even the
   * diagnostic step and target writes of the settled scalar master. */
  s->master.step=(target-s->master.current)/(float)n;
  s->master.target=target;s->master.current=target;
  return 1;
 }
 return original_master_process(&s->master,volume,l,r,l,r,n);
}
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
 if(!ne&&zero_ready(s,il,ir,n)&&miami_decimator_advance_zero(&s->fx.decimator.kernel,n)){
  /* Held samples and signal input are zero; both settled fixed-FX mixers
   * are identities on zero. Phase/pending step and original master survive. */
  original_miami_zero_words((uint32_t *)w->dryL,n);original_miami_zero_words((uint32_t *)w->dryR,n);
  original_miami_zero_halves((uint16_t *)w->scratchL,n);original_miami_zero_halves((uint16_t *)w->scratchR,n);
  original_miami_zero_words(ol,n);original_miami_zero_words(orr,n);
  return original_master_process(&s->master,volume*volume,ol,orr,ol,orr,n);
 }
 for(uint32_t i=0;i<n;i++) {
  /* Both binary32 zeros become int32 zero in either admitted FZ mode.
   * Keep channel access order and the complete nonzero conversion; both FX
   * wrappers, held decimator samples and master ramps still run below. */
  w->dryL[i]=miami_fx_input_fast(il[i]);w->dryR[i]=miami_fx_input_fast(ir[i]);
 }
 effect_context effect={&s->fx.decimator,events,ne};
 (void)original_fixed_fx_process(&s->fx.fx1,w->dryL,w->dryR,w->scratchL,w->scratchR,w->dryL,w->dryR,n,decimate,&effect);
 original_miami_fx_bypass_process(&s->fx.fx2,w,n);
 return original_master_process_i32(&s->master,volume*volume,w->dryL,w->dryR,ol,orr,n);
}
