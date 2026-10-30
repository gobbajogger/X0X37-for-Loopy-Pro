/* SPDX-License-Identifier: GPL-3.0-only */
#include "chicago_tuning.h"
#include "binary32_classify.h"
#include "binary32_widen.h"
#include "master_tune_rne.h"
#include "chicago_tuning_private.h"
#include <math.h>
#include <string.h>
#if DBL_MANT_DIG != 53 || DBL_MAX_EXP != 1024
#error Original MasterTune conversion requires IEEE binary64 double
#endif
#ifdef ORIGINAL_USE_SOFT_FMA
#include "fused_reference.h"
#define ref_fma original_rne_fmaf
#else
#define ref_fma fmaf
#endif
#define TUNING_READY 0x43545531u
static float fl(uint32_t u){float f;memcpy(&f,&u,4);return f;}
static int32_t sign8(uint32_t u){u&=255;return u<128?(int32_t)u:(int32_t)u-256;}
static int32_t sign16(uint32_t u){u&=65535;return u<32768?(int32_t)u:(int32_t)u-65536;}
static uint32_t asr(uint32_t x,unsigned shift){return (x>>shift)|((x&0x80000000u)?(~UINT32_C(0)<<(32-shift)):0);}
int original_chicago_tuning_bind(original_chicago_tuning *out,unsigned scale,unsigned tonic,const float *table,uint16_t random){
 if(!out||scale>15||tonic>11||(!table&&scale>=4))return 0;
 if(table)for(unsigned i=0;i<144;i++)if(!original_binary32_isfinite(table[i]))return 0;
 original_chicago_tuning n;memset(&n,0,sizeof n);n.custom_offsets=table;n.ready=TUNING_READY;n.random=random;n.scale=(uint8_t)scale;n.tonic=(uint8_t)tonic;*out=n;return 1;
}
int original_chicago_tuning_target(original_chicago_tuning *s,int32_t raw,float *out){
 if(!s||!out||s->ready!=TUNING_READY||s->scale>15||s->tonic>11||raw<0||raw>187)return 0;
 float offset=0;
 if(s->scale==1){
  /* Exact32-bit register operations, explicit sign extension and low-bit
   * extraction. No signed C shifts/overflow or alternative RNG polynomial. */
  uint32_t a=(uint32_t)sign16(s->random),h=asr(a,8),sum=a+(a>>2);sum+=h>>3;uint32_t feedback=sum+(h>>7),twice=a<<1;
  uint32_t seven=(twice>>9)&127u,sample=(uint32_t)(seven<64?(int32_t)seven:(int32_t)seven-128);
  uint32_t next=(twice&~1u)|(feedback&1u);s->random=(uint16_t)next;
  uint32_t pair=((sample&0x00ffffffu)<<8)|(sample&255u),negative=((256u-pair)&0xffffff00u)|(sample&255u);
  uint32_t selected=sign16(next)<0?negative:pair;
  uint32_t upper=((uint32_t)sign16(selected))>>8,product=(uint32_t)sign8(selected)*upper;
  int32_t value=sign8(product>>8);offset=(float)value/99.0f;
 }else if(s->scale==2||s->scale==3){
  int32_t index=(raw-(int32_t)s->tonic)%12;
  offset=fl((s->scale==2?original_chicago_scale2_bits:original_chicago_scale3_bits)[index+11]);
 }else if(s->scale>=4){
  if(!s->custom_offsets)return 0;
  offset=s->custom_offsets[((unsigned)s->scale-4)*12+(unsigned)(raw%12)];
 }
 *out=(float)raw+offset;return 1;
}
int original_chicago_master_tune_convert(float normalized,original_chicago_logf_provider logarithm,float *converted){
 if(!converted||!logarithm||!original_binary32_isfinite(normalized)||normalized<0||normalized>1)return 0;
 float shifted=normalized+(-.5f),hz=ref_fma(shifted,40,440),ratio=hz/440.0f,value=logarithm(ratio);if(!original_binary32_isfinite(value))return 0;
#ifdef ORIGINAL_USE_SOFT_FMA
 *converted=original_master_tune_semitones_rne(value);return 1;
#else
 uint64_t bits=UINT64_C(0x3fe62e42fefa39ef);double ln2;typedef char binary64_size[sizeof ln2==sizeof bits?1:-1];(void)sizeof(binary64_size);memcpy(&ln2,&bits,8);double pitch=original_binary32_to_binary64_finite(value)/ln2;pitch=pitch*12.0;*converted=(float)pitch;return 1;
#endif
}
