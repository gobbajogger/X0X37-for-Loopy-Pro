/* SPDX-License-Identifier: GPL-3.0-only */
#include "chicago_mp4.h"
#include "binary32_classify.h"
#include <math.h>
#include "binary32_math.h"
#include <string.h>
#ifdef ORIGINAL_USE_SOFT_FMA
#include "fused_reference.h"
#define ref_fma original_rne_fmaf
#else
#define ref_fma fmaf
#endif
static float clamp(float x,float lo,float hi){return x<lo?lo:x>hi?hi:x;}
static void stage(original_ramp *r,float target,float reciprocal){r->target=target;r->delta=(target-r->current)*reciprocal;}
static float advance(original_ramp *r){float q=r->current+r->delta;if(r->delta>0){if(q>=r->target)q=r->target;}else if(r->delta<0){if(q<=r->target)q=r->target;}else q=r->target;r->current=q;return q;}
int original_chicago_feedback(float input,float *out){
 if(!out||!original_binary32_isfinite(input))return 0;
 float u=original_binary32_abs(input)>=1?0:1-original_binary32_abs(input);float u2=u*u,u4=u2*u2,u5=u*u4;float v=1-u5;*out=input>=0?v:-v;return 1;
}
int original_chicago_mp4_init(original_chicago_mp4 *s,float a0,float e0){
 if(!s||!original_binary32_isfinite(a0)||!original_binary32_isfinite(e0)||a0<0||a0>1||e0< -1||e0>1)return 0;
 memset(s,0,sizeof *s);s->a.current=s->a.target=a0;s->e.current=s->e.target=e0;return 1;
}
int original_chicago_mp4_prepare_reader(original_chicago_mp4 *s,original_mp4_reader read,const void *context,
 float cutoff,float depth,float env,float md,float mc,int note,float track,float resonance,float mr,uint32_t duration,unsigned kick,float *postgain){
 if(!s||!read||!postgain||!duration||duration>2147482880u||kick>1||note<0||note>127)return 0;
 float controls[]={cutoff,depth,env,md,mc,track,resonance,mr};for(unsigned i=0;i<8;i++)if(!original_binary32_isfinite(controls[i]))return 0;
 float sum=depth+md;if(!original_binary32_isfinite(sum)||!original_binary32_isfinite(resonance+mr))return 0;
 float d=clamp(sum,-1,1),z=ref_fma(env,d,cutoff),q=z*16384.0f,key=(float)(note-48)*128.0f;
 q=ref_fma(key,track,q);q=ref_fma(mc,16384.0f,q);if(!original_binary32_isfinite(q))return 0;
 q=clamp(q,0,16384);unsigned j=(unsigned)q;float a,e;if(!read(context,j,&a,&e))return 0;
 if(!original_binary32_isfinite(a)||a<0||a>1||!original_binary32_isfinite(e)||e< -1||e>1)return 0;
 float R=clamp(resonance+mr,0,1),reciprocal=1.0f/(float)duration;
 original_chicago_mp4 next=*s;stage(&next.a,a,reciprocal);stage(&next.e,e,reciprocal);stage(&next.resonance,R,reciprocal);
 if(kick){float r2=R*R,r4=r2*r2,r8=r4*r4;next.history[0]=R>.2f?r8:0;}
 next.prepared=1;*s=next;*postgain=clamp(ref_fma(resonance+mr,4,1),1,4);return 1;
}
static int raw_table_read(const void *context,uint32_t index,float *a,float *e){
 const original_mp4_tables *t=context;if(!t||!t->a||!t->e||index>16384)return 0;*a=t->a[index];*e=t->e[index];return 1;
}
int original_chicago_mp4_prepare(original_chicago_mp4 *s,const original_mp4_tables *tables,
 float cutoff,float depth,float env,float md,float mc,int note,float track,float resonance,float mr,uint32_t duration,unsigned kick,float *postgain){
 return original_chicago_mp4_prepare_reader(s,raw_table_read,tables,cutoff,depth,env,md,mc,note,track,resonance,mr,duration,kick,postgain);
}
int original_chicago_mp4_sample(original_chicago_mp4 *s,float input,float *out){
 if(!s||!out||!s->prepared||!original_binary32_isfinite(input))return 0;
 original_chicago_mp4 n=*s;float A=advance(&n.a),E=advance(&n.e),R=advance(&n.resonance);
 float tmp=n.history[3]+E*n.history[4],d=n.history[4]-E*tmp,nonlinear;
 if(!original_chicago_feedback(d,&nonlinear))return 0;
 float v=input-R*nonlinear,z1=v+A*(n.history[0]-v),z2=z1+A*(n.history[1]-z1),z3=z2+A*(n.history[2]-z2),z4=z3+A*(n.history[3]-z3);
 if(!original_binary32_isfinite(z1)||!original_binary32_isfinite(z2)||!original_binary32_isfinite(z3)||!original_binary32_isfinite(z4)||!original_binary32_isfinite(tmp))return 0;
 n.history[0]=z1;n.history[1]=z2;n.history[2]=z3;n.history[3]=z4;n.history[4]=tmp;*s=n;*out=z4;return 1;
}
