/* SPDX-License-Identifier: GPL-3.0-only */
#include "master_unmuted.h"
#include "binary32_classify.h"
#include "../wobble/miami_fx_conversion.h"
#include <math.h>
#include <string.h>
static float fl(uint32_t u){float x;memcpy(&x,&u,4);return x;}
static uint32_t bits(float x){uint32_t u;memcpy(&u,&x,4);return u;}
int original_master_unmuted_seed(original_master_unmuted *s,float gain){if(!s||!original_binary32_isfinite(gain)||gain<0||gain>4)return 0;s->current_gain=gain;return 1;}
int original_master_unmuted_process(original_master_unmuted *s,float control,
 const uint32_t *inL,const uint32_t *inR,uint32_t *outL,uint32_t *outR,uint32_t n){
 if(!s||!original_binary32_isfinite(control)||control<0||control>1||!original_binary32_isfinite(s->current_gain)||s->current_gain<0||s->current_gain>4)return 0;
 if(!n)return 1;
 if(!inL||!inR||!outL||!outR)return 0;
 float target=4.0f*control,delta=(target-s->current_gain)/(float)n,g=s->current_gain;
 for(uint32_t i=0;i<n;i++){g=g+delta;outL[i]=miami_fx_cleanup_rne(bits(g*fl(inL[i])));outR[i]=miami_fx_cleanup_rne(bits(g*fl(inR[i])));}
 s->current_gain=target;return 1;
}
