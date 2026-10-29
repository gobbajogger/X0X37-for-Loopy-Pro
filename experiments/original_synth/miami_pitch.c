/* SPDX-License-Identifier: GPL-3.0-only */
#include "miami_pitch.h"
#include "binary32_classify.h"
#include <math.h>
#include <string.h>
#ifdef ORIGINAL_USE_SOFT_FMA
#include "fused_reference.h"
#define ref_fma original_rne_fmaf
#else
#define ref_fma fmaf
#endif
static float clamp(float x,float lo,float hi){return x<lo?lo:x>hi?hi:x;}
int original_miami_pitch_init(original_miami_pitch *s,unsigned last){
 if(!s||last>127)return 0;
 memset(s,0,sizeof *s);for(unsigned i=0;i<4;i++)s->current[i]=s->target[i]=(float)last;return 1;
}
int original_miami_pitch_note(original_miami_pitch *s,unsigned note,unsigned last,unsigned mask,unsigned full){
 if(!s||note>127||last>127||mask>15||full>1)return 0;
 for(unsigned i=0;i<4;i++)if(mask&(1u<<i)){s->target[i]=(float)note;if(full)s->current[i]=(float)last;}
 return 1;
}
int original_miami_pitch_prepare(original_miami_pitch *s,const original_miami_pitch_controls *c,const int32_t table[128],float fs,uint32_t D,uint32_t out[4]){
 if(!s||!c||!table||!out||c->modulation_on>1||c->transpose< -120||c->transpose>120)return 0;
 const float values[]={c->master_tune,c->unison_coordinate,c->unison_detune,c->mod_master_pitch,c->bend_range,c->bend,c->detune,c->mod_detune,c->frequency_mod,c->mod_slave_pitch,c->portamento,c->mod_portamento};
 for(unsigned i=0;i<sizeof values/sizeof values[0];i++)if(!original_binary32_isfinite(values[i]))return 0;
 for(unsigned i=0;i<4;i++)if(!original_binary32_isfinite(c->fine_tune[i])||!original_binary32_isfinite(c->mod_pitch[i])||c->semitone[i]< -120||c->semitone[i]>120)return 0;
 original_miami_pitch next=*s;if(!original_glide(next.current,next.target,c->portamento,c->mod_portamento,fs,D))return 0;
 float common=c->master_tune+0.0f;common=ref_fma(c->unison_coordinate,c->unison_detune,common);common=ref_fma(c->mod_master_pitch,48,common);common=common+(float)c->transpose;common=ref_fma(c->bend_range,c->bend,common);
 float dt=c->detune+c->mod_detune;if(!original_binary32_isfinite(common)||!original_binary32_isfinite(dt))return 0;dt=clamp(dt,0,1)*.3499999940395355f;
 uint32_t increments[4];
 for(unsigned i=0;i<4;i++){
  float p=common+next.current[i];p=p+(float)c->semitone[i];p=ref_fma(c->mod_pitch[i],48,p);
  if(i){p=c->fine_tune[i]+p;if(i==1)p=p-dt;else if(i==3)p=dt+p;}
  if(c->modulation_on&&(i==1||i==3)){p=c->frequency_mod+p;p=ref_fma(c->mod_slave_pitch,48,p);}
  if(!original_binary32_isfinite(p)||!original_pitch_increment(p,table,&increments[i]))return 0;
 }
 *s=next;memcpy(out,increments,sizeof increments);return 1;
}
