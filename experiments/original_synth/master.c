/* SPDX-License-Identifier: GPL-3.0-only */
#include "master.h"
#include "binary32_classify.h"
#include "binary32_math.h"
#include "../wobble/miami_fx_conversion.h"
#include <math.h>
#include <string.h>
static float fl(uint32_t u){return original_math_float(u);}
static uint32_t bits(float f){return original_math_bits(f);}
void original_master_init(original_master *s){if(s){memset(s,0,sizeof *s);s->committed_mute=1;}}
void original_master_mute(original_master *s,uint8_t value){if(s)s->desired_mute=value;}
int original_master_process(original_master *s,float volume,const uint32_t *il,const uint32_t *ir,uint32_t *ol,uint32_t *orr,uint32_t n){
 if(!s||!original_binary32_isfinite(volume)||volume<0||volume>1||!original_binary32_isfinite(s->current)||s->current<0||s->current>4)return 0;
 if(!n)return 1;
 if(!il||!ir||!ol||!orr)return 0;
 float target=s->desired_mute?0:volume*4,current=s->current,step;
 if(s->desired_mute==s->committed_mute)step=(target-current)/(float)n;
 else if(s->desired_mute)step=(-current)/(float)n;
 else {current=target;step=0;}
 s->committed_mute=s->desired_mute;s->target=target;s->step=step;
 for(uint32_t i=0;i<n;i++) {
  current=current+step;
  uint32_t l=il[i],r=ir[i];
  ol[i]=miami_fx_cleanup_fast(bits(current*fl(l)));orr[i]=miami_fx_cleanup_fast(bits(current*fl(r)));
 }
 s->current=target;return 1;
}
/* original_master_process on the int16 FX output: the same as converting
 * every sample with miami_fx_output_rne into out_*, then
 * original_master_process(out_* in place). One pass, no second read. */
#define MASTER_I16_BODY(T) \
 if(!s||!original_binary32_isfinite(volume)||volume<0||volume>1||!original_binary32_isfinite(s->current)||s->current<0||s->current>4){ \
  for(uint32_t i=0;i<n;i++){ol[i]=miami_fx_output_rne_inline((int16_t)il[i]);orr[i]=miami_fx_output_rne_inline((int16_t)ir[i]);} \
  return 0;} \
 if(!n)return 1; \
 float target=s->desired_mute?0:volume*4,current=s->current,step; \
 if(s->desired_mute==s->committed_mute)step=(target-current)/(float)n; \
 else if(s->desired_mute)step=(-current)/(float)n; \
 else {current=target;step=0;} \
 s->committed_mute=s->desired_mute;s->target=target;s->step=step; \
 for(uint32_t i=0;i<n;i++){ \
  current=current+step; \
  uint32_t l=miami_fx_output_rne_inline((int16_t)il[i]),r=miami_fx_output_rne_inline((int16_t)ir[i]); \
  ol[i]=miami_fx_cleanup_fast(bits(current*fl(l)));orr[i]=miami_fx_cleanup_fast(bits(current*fl(r))); \
 } \
 s->current=target;return 1;
int original_master_process_i16(original_master *s,float volume,const int16_t *il,const int16_t *ir,uint32_t *ol,uint32_t *orr,uint32_t n){MASTER_I16_BODY(int16_t)}
int original_master_process_i32(original_master *s,float volume,const int32_t *il,const int32_t *ir,uint32_t *ol,uint32_t *orr,uint32_t n){MASTER_I16_BODY(int32_t)}
