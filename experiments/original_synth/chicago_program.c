/* SPDX-License-Identifier: GPL-3.0-only */
#include "chicago_program.h"
#include "binary32_classify.h"
#ifndef ORIGINAL_CHICAGO_PROGRAM_METADATA
#define ORIGINAL_CHICAGO_PROGRAM_METADATA "chicago_program_private.h"
#endif
#include ORIGINAL_CHICAGO_PROGRAM_METADATA
#include <math.h>
#include <stddef.h>
#include <string.h>
#ifdef ORIGINAL_USE_SOFT_FMA
#include "fused_reference.h"
#define ref_fma original_rne_fmaf
#else
#define ref_fma fmaf
#endif
static unsigned quantize(float x,unsigned n){
 int q=(int)ref_fma(x,(float)(n-1),.5f);
 return q<0?0:q>=(int)n?n-1:(unsigned)q;
}
static float canonical(float x,unsigned n){return (float)quantize(x,n)/(float)(n-1);}
static float mapped(float x,const original_chicago_mapping *m){
 float y;
 if(m->type==2)y=original_chicago_lookup[m->lookup+quantize(x,m->length)];
 else {
  float a=m->type==1?m->b:m->a,b=m->type==1?m->a:m->b;
  if(m->type==3)x=x*x;
  y=ref_fma(x,b-a,a);
 }
 if(m->outer_count)y=canonical(y,m->outer_count);
 return canonical(y,m->canonical_count);
}
int original_chicago_program_set(original_chicago_program *p,unsigned parameter,float value){
 if(!p||parameter>=22||!original_binary32_isfinite(value)||value<0||value>1)return 0;
 for(unsigned i=original_chicago_macro_start[parameter];i<original_chicago_macro_start[parameter+1];i++)
  p->target[i]=mapped(value,&original_chicago_mappings[i]);
 p->macro[parameter]=value;p->revision++;return 1;
}
int original_chicago_program_init(original_chicago_program *p,unsigned factory){
 if(!p||factory>=81)return 0;
 original_chicago_program next;memset(&next,0,sizeof next);
 for(unsigned i=0;i<22;i++){
#ifdef ORIGINAL_CHICAGO_EXPANDED_FACTORY
  float x=original_chicago_factory_macro[factory][i];
#else
  unsigned bit=(factory*22+i)*9,byte=bit>>3;
  unsigned code=((unsigned)original_chicago_factory_codes[byte]|((unsigned)original_chicago_factory_codes[byte+1]<<8))>>(bit&7);
  float x=original_chicago_factory_dictionary[code&511];
#endif
  (void)original_chicago_program_set(&next,i,x);
 }
 next.revision=0;*p=next;return 1;
}
int original_chicago_program_native(const original_chicago_program *p,unsigned id,float *out){
 if(!p||!out||id>=117)return 0;
 for(unsigned i=0;i<29;i++)if(original_chicago_mappings[i].native_id==id){*out=p->target[i];return 1;}
 *out=original_chicago_base[id];return 1;
}
const char *original_chicago_program_name(unsigned factory){
 if(factory>=81)return NULL;
#ifdef ORIGINAL_CHICAGO_EXPANDED_FACTORY
 return original_chicago_factory_name[factory];
#else
 return original_chicago_factory_names+original_chicago_factory_name_offset[factory];
#endif
}
int original_chicago_program_routing(const original_chicago_program *p,original_chicago_routing *out){
 if(!p||!out)return 0;
 original_chicago_routing r;memset(&r,0,sizeof r);float x;
 (void)original_chicago_program_native(p,53,&x);r.arp_base_note=original_chicago_arp_base[quantize(x,10)];
 (void)original_chicago_program_native(p,54,&x);r.arp_times=(float)(16-quantize(x,16));
 (void)original_chicago_program_native(p,60,&x);r.arp_range=(uint8_t)(1+quantize(x,6));
 (void)original_chicago_program_native(p,61,&x);r.arp_mode=(uint8_t)quantize(x,4);
 (void)original_chicago_program_native(p,62,&x);r.arp_enable=(uint8_t)quantize(x,2);
 (void)original_chicago_program_native(p,52,&x);r.arp_tempo_sync=(uint8_t)quantize(x,2);
 (void)original_chicago_program_native(p,51,&x);r.arp_key_sync=(uint8_t)quantize(x,2);
 (void)original_chicago_program_native(p,63,&x);r.arp_latch=(uint8_t)quantize(x,2);
 (void)original_chicago_program_native(p,108,&x);r.damper=(uint8_t)quantize(x,128);
 (void)original_chicago_program_native(p,107,&x);r.vca_eg_mode=(uint8_t)quantize(x,2);
 (void)original_chicago_program_native(p,105,&x);r.bend_bipolar=ref_fma(x,2,-1);
 (void)original_chicago_program_native(p,96,&x);r.master_squared=x*x;
 for(unsigned slot=0;slot<2;slot++)for(unsigned i=0;i<4;i++){
  (void)original_chicago_program_native(p,109+slot*4+i,&x);
  r.fx[slot][i]=(uint8_t)quantize(x,i==0?30:i==1?2:128);
 }
 *out=r;return 1;
}
