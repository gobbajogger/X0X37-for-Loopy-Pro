/* SPDX-License-Identifier: GPL-3.0-only */
#include "miami_ms20.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
static float fl(uint32_t u){float f;memcpy(&f,&u,4);return f;}
static uint32_t bits(float f){uint32_t u;memcpy(&u,&f,4);return u;}
int main(int argc,char **argv){assert(argc==3);FILE *v=fopen(argv[1],"r"),*render=fopen(argv[2],"wb");assert(v&&render);uint32_t in,out;unsigned count=0;float f;
 while(fscanf(v,"%"SCNx32" %"SCNx32,&in,&out)==2){assert(original_miami_nonlinear(fl(in),&f)&&bits(f)==out);count++;}fclose(v);
 float untouched=123;assert(!original_miami_nonlinear(NAN,&untouched)&&untouched==123);
 float b,u;assert(original_miami_mg_shape(3,0,&b,&u)&&b== -1&&u==0);assert(original_miami_mg_shape(3,-1,&b,&u)&&b==1&&u==1);
 for(unsigned shape=0;shape<4;shape++)for(unsigned j=0;j<=8192;j++){float p=(float)j/4096-1;assert(original_miami_mg_shape(shape,p,&b,&u));assert(b>=-1&&b<=1&&u>=0&&u<=1);assert(fwrite(&b,4,1,render)==1);assert(fwrite(&u,4,1,render)==1);}
 /* Synthetic explicit coefficients exercise state arithmetic only. They are
  * NOT recovered A/B tables and must never be labeled original rendered audio. */
 float a[129],B[129],r[129],cap[129];for(unsigned j=0;j<129;j++){a[j]=-.8f+.2f*(float)j/128;B[j]=.8f-.2f*(float)j/128;r[j]=.5f*(float)j/128;cap[j]=1.1f;}
 original_ms20_tables tables={a,B,r,cap};original_miami_ms20 s;assert(original_miami_ms20_init(&s,0,a[0],B[0],cap[0]));assert(s.noise==1&&bits(s.noise_gain.current)==0x3727c5ac);
 assert(!original_miami_ms20_sample(&s,0,1,&f));
 for(unsigned frame=0;frame<32768;frame++){
  if(frame%64==0)assert(original_miami_ms20_prepare(&s,&tables,(float)(frame%128)/128,.5f,0,1,0,0,0,0,0,64));
  if(frame==1000){uint32_t seed=s.noise;float ng=s.noise_gain.current;assert(original_miami_ms20_note_reset(&s,a[0]));assert(s.noise==seed&&s.noise_gain.current==ng&&!s.prepared);assert(original_miami_ms20_prepare(&s,&tables,.5f,.5f,0,1,0,0,0,0,0,64));}
  uint32_t old=s.noise;unsigned choice=(old&0x40000000u)==0;float osc=(float)(frame%101)/101-.5f;
  assert(original_miami_ms20_sample(&s,osc,1,&f));assert(s.noise==((old+old)^(choice?8393u:0)));assert(isfinite(f));assert(fwrite(&f,4,1,render)==1);
 }
 original_miami_ms20 saved=s;assert(!original_miami_ms20_prepare(&s,&tables,.5f,-1,0,1,0,0,0,0,0,64));assert(!memcmp(&s,&saved,sizeof s));
 assert(fclose(render)==0);printf("PASS %u exact N513 oracle cases;32772MG shape points;32768 synthetic state frames; state%zu bytes\n",count,sizeof s);return 0;
}
