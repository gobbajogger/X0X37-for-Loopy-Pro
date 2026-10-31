/* SPDX-License-Identifier: GPL-3.0-only */
#include "miami_program.h"
#include "binary32_classify.h"
#include "miami_program_private.h"
#include <math.h>
#include <stddef.h>
#include <string.h>
#ifdef ORIGINAL_USE_SOFT_FMA
#include "fused_reference.h"
#define ref_fma original_rne_fmaf
#else
#define ref_fma fmaf
#endif
static float canonical(float x,unsigned count) {
 if(!count)return x;
 float scale=(float)(count-1);int q=(int)ref_fma(x,scale,.5f);
 if(q<0)q=0;
 if(q>=(int)count)q=(int)count-1;
 return (float)q/scale;
}
static float map(float x,const original_miami_mapping *m) {
 float y;
 if(m->type==2) {
  unsigned i=(unsigned)ref_fma(x,(float)(m->length-1),.5f);
  if(i>=m->length)i=m->length-1;
  y=original_miami_lookup[m->lookup+i];
 } else {
  float d=m->b-m->a;
  if(m->type==3)d=d*x;
  y=ref_fma(d,x,m->a);
 }
 if(m->outer_count)y=canonical(y,m->outer_count);
 return canonical(y,m->canonical_count);
}
int original_miami_program_set(original_miami_program *p,unsigned parameter,float value) {
 if(!p||parameter>=18||!original_binary32_isfinite(value)||value<0||value>1)return 0;
 /* All mapping/metadata invariants are generation-checked. This loop visits
  * only the 1..3 targets owned by this public control. No 109-cell copy. */
 unsigned begin=original_miami_macro_start[parameter],end=original_miami_macro_start[parameter+1];
 for(unsigned i=begin;i<end;i++)p->target[i]=map(value,&original_miami_mappings[i]);
 p->macro[parameter]=value;p->revision++;return 1;
}
int original_miami_program_init(original_miami_program *p,unsigned factory) {
 if(!p||factory>=30)return 0;
 original_miami_program next;memset(&next,0,sizeof next);
 for(unsigned i=0;i<18;i++)(void)original_miami_program_set(&next,i,original_miami_factory_macro[factory][i]);
 next.revision=0;*p=next;return 1;
}
int original_miami_program_native(const original_miami_program *p,unsigned id,float *out) {
 if(!p||!out)return 0;
 for(unsigned i=0;i<24;i++)if(original_miami_mappings[i].native_id==id){*out=p->target[i];return 1;}
 if(id>=109)return 0;
 *out=original_miami_base[id];return 1;
}
const char *original_miami_program_name(unsigned factory) {
 return factory<30?original_miami_factory_name[factory]:NULL;
}
