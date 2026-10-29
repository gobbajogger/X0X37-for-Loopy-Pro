/* SPDX-License-Identifier: GPL-3.0-only */
#include "oscillator_motion.h"
#include "chicago_mp4.h"
#include "miami_ms20.h"
#include "motion_tables_private.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
static float fl(unsigned u){float f;memcpy(&f,&u,4);return f;}
static unsigned bits(float f){unsigned u;memcpy(&u,&f,4);return u;}
static FILE *file(const char *dir,const char *name){char p[2048];assert(snprintf(p,sizeof p,"%s/%s",dir,name)>0);FILE *f=fopen(p,"r");assert(f);return f;}
static void waves(const char *dir){
 FILE *f=file(dir,"wave.txt");unsigned shape,p,d,w,expected,n=0;float out;
 while(fscanf(f,"%u %x %u %x %x",&shape,&p,&d,&w,&expected)==5){assert(original_waveform(shape,fl(p),d,fl(w),&out));assert(bits(out)==expected);n++;}
 fclose(f);assert(n==29344);puts("PASS 29344 original waveform oracle fixtures, strict pulse boundary and correction threshold");
}
static void phases(const char *dir){
 FILE *f=file(dir,"phase.txt");char tag;original_oscillators s,prepared_state;original_osc_controls c;original_prepared_oscillators prepared;unsigned sync=0,n=0;
 while(fscanf(f," %c",&tag)==1){
  if(tag=='I'){
   unsigned sh[4],u;
   assert(fscanf(f,"%u %u %u %u %u %u %u %u %u %u %u %u %u",&sync,&s.phase[0],&s.phase[1],&s.phase[2],&s.phase[3],&c.increment[0],&c.increment[1],&c.increment[2],&c.increment[3],&sh[0],&sh[1],&sh[2],&sh[3])==13);
   for(unsigned i=0;i<4;i++)c.shape[i]=(uint8_t)sh[i];
   for(unsigned i=0;i<4;i++){assert(fscanf(f,"%x",&u)==1);c.width[i]=fl(u);}
   for(unsigned i=0;i<4;i++){assert(fscanf(f,"%x",&u)==1);c.gain[i]=fl(u);}
   assert(fscanf(f,"%x",&u)==1);c.xmod=fl(u);prepared_state=s;assert(original_oscillator_prepare(&prepared,&c));
  }else{
   assert(tag=='S');unsigned expected[4],u;float out;
   assert(fscanf(f,"%u %u %u %u %x",&expected[0],&expected[1],&expected[2],&expected[3],&u)==5);
   assert(original_oscillator_sample(&s,&c,sync,&out));float cached;assert(original_oscillator_sample_prepared(&prepared_state,&prepared,sync,&cached)&&bits(cached)==bits(out)&&!memcmp(&s,&prepared_state,sizeof s));assert(!memcmp(expected,s.phase,sizeof expected)&&bits(out)==u);n++;
  }
 }
 fclose(f);assert(n==8192);
 original_oscillators saved=s;float out=123;c.increment[0]=0;assert(!original_oscillator_sample(&s,&c,0,&out)&&out==123&&!memcmp(&s,&saved,sizeof s));
 puts("PASS 8192 four-oscillator oracle frames, Miami XMod versus Chicago signed-wrap sync, invalid transaction");
}
static void mp4(const char *dir){
 original_chicago_mp4 s;original_mp4_tables t={original_chicago_a44100,original_chicago_e44100};
 assert(original_chicago_mp4_init(&s,t.a[0],t.e[0]));FILE *f=file(dir,"mp4.txt");char tag;unsigned frames=0;
 while(fscanf(f," %c",&tag)==1){
  if(tag=='P'){unsigned j,r,D,k;float gain;assert(fscanf(f,"%u %x %u %u",&j,&r,&D,&k)==4);assert(original_chicago_mp4_prepare(&s,&t,(float)j/16384,0,0,0,0,48,0,fl(r),0,D,k,&gain));assert(gain>=1&&gain<=4);}
  else{unsigned x,expected[5];float out;assert(tag=='S');assert(fscanf(f,"%x %x %x %x %x %x",&x,&expected[0],&expected[1],&expected[2],&expected[3],&expected[4])==6);assert(original_chicago_mp4_sample(&s,fl(x),&out));for(unsigned i=0;i<5;i++)assert(bits(s.history[i])==expected[i]);assert(bits(out)==expected[3]);frames++;}
 }
 fclose(f);assert(frames>4000);original_chicago_mp4 saved=s;float out=123;
 assert(!original_chicago_mp4_sample(&s,NAN,&out)&&out==123&&!memcmp(&s,&saved,sizeof s));
 assert(original_chicago_feedback(-0.0f,&out)&&bits(out)==0);assert(original_chicago_feedback(-2,&out)&&out==-1);assert(original_chicago_feedback(2,&out)&&out==1);
 printf("PASS Chicago MP4 oracle %u sample states, original derived tables, coefficient ramps, R^8 kick, nonlinearity and invalid transaction\n",frames);
}
static void glides(const char *dir){
 FILE *f=file(dir,"glide.txt");char tag;float current[4],target[4],port=0,mod=0;unsigned fs=0,D=0,count=0;
 while(fscanf(f," %c",&tag)==1){
  unsigned u;
  if(tag=='I'){
   for(unsigned i=0;i<4;i++){assert(fscanf(f,"%x",&u)==1);current[i]=fl(u);}
   for(unsigned i=0;i<4;i++){assert(fscanf(f,"%x",&u)==1);target[i]=fl(u);}
   assert(fscanf(f,"%x",&u)==1);port=fl(u);assert(fscanf(f,"%x",&u)==1);mod=fl(u);assert(fscanf(f,"%u %u",&fs,&D)==2);
  }else{
   assert(tag=='S');assert(original_glide(current,target,port,mod,(float)fs,D));
   for(unsigned i=0;i<4;i++){assert(fscanf(f,"%x",&u)==1);assert(bits(current[i])==u);}count++;
  }
 }
 fclose(f);assert(count==8192);puts("PASS 8192 exact fused four-pitch glide oracle ticks");
}
static void controls(void){
 for(unsigned i=0;i<=120;i++){uint32_t D;assert(original_pitch_increment((float)i,original_pitch44100,&D)&&D==(unsigned)original_pitch44100[i]);}
 for(unsigned rate=0;rate<8;rate++){
  static const float base[]={3,3,3,3,4,4,4,4},times[]={16,8,4,2,8,4,2,1};
  float p=-1;for(unsigned k=0;k<4096;k++){float old=p,expected=old+((120.0f*.8f)*64.0f)/((44100.0f*base[rate])*times[rate]);unsigned wrap=expected>=1;if(wrap)expected-=2;unsigned got=99;assert(original_mg_tempo(&p,rate,120,44100,64,&got));assert(bits(p)==bits(expected)&&got==wrap);}
 }
 float p=1;unsigned wrap;assert(original_mg_tempo(&p,0,120,44100,64,&wrap)&&wrap);float out;
 assert(original_pw_width(2,0,1,.9010000228881836f,0,0,&out)&&out==.5f);
 assert(original_pw_width(3,0,1,.9010000228881836f,0,0,&out)&&out==1.0f-.9010000228881836f);
 float current[]={0,40,80,120},target[]={120,80,40,0};assert(original_glide(current,target,0,0,44100,64));for(unsigned i=0;i<4;i++)assert(current[i]==target[i]);
 float saved[4];memcpy(saved,current,sizeof saved);assert(!original_glide(current,target,NAN,0,44100,64)&&!memcmp(current,saved,sizeof saved));
 assert(original_xmod_coefficient(1,1,&out)&&out==.9f);assert(original_xmod_coefficient(0,-1,&out)&&out==0);
 printf("PASS control rate order, 32768 phase ticks, original PW, zero-glide, pitch table and XMod clamp\n");
}
static void joined(void){
 original_ms20_tables t={original_miami_a44100,original_miami_b44100,original_miami_r,original_miami_cap};original_miami_ms20 ms;
 assert(bits(t.cap[108])==0x3f820834&&bits(t.cap[109])==0x3f8020c5);assert(original_miami_ms20_init(&ms,0,t.a[0],t.b[0],t.cap[0]));
 original_oscillators osc={{0x12345678,0x87654321,0xfedcba98,0xabcdef01}};original_osc_controls c={{42852281,45400410,21426140,22700205},{.5f,.5f,.5f,.5f},{0,0,.5f,.099f},{0,1,2,3},.2f};
 original_prepared_oscillators prepared;assert(original_oscillator_prepare(&prepared,&c));
 float mg=-1,out=0,sum=0;clock_t start=clock();
 for(unsigned i=0;i<44100;i++){
  if(i%64==0){float bipolar,unipolar;unsigned wrapped;assert(original_miami_mg_shape(0,mg,&bipolar,&unipolar));assert(original_mg_tempo(&mg,2,120,44100,64,&wrapped));assert(original_miami_ms20_prepare(&ms,&t,.5f,.5f,0,0,0,bipolar*.2f,0,0,0,64));}
  float mix;assert(original_oscillator_sample_prepared(&osc,&prepared,0,&mix));assert(original_miami_ms20_sample(&ms,mix,1,&out));assert(isfinite(out));sum+=out;
 }
 printf("PASS joined explicit-phase Miami oscillator/MG/MS20 44100 frames; host CPU %.6f s checksum %.9g; NOT original note or device CPU proof\n",(double)(clock()-start)/CLOCKS_PER_SEC,sum);
 printf("STATE oscillator=%zu ChicagoMP4=%zu MS20=%zu; private coefficient bytes=%zu\n",sizeof osc,sizeof(original_chicago_mp4),sizeof ms,sizeof original_chicago_a44100+sizeof original_chicago_e44100+sizeof original_miami_a44100+sizeof original_miami_b44100+sizeof original_miami_r+sizeof original_miami_cap+sizeof original_pitch44100);
}
static void cost(void){
 original_osc_controls c={{42852281,45400410,21426140,22700205},{.5f,.5f,.5f,.5f},{0,0,.5f,.099f},{0,1,2,3},.2f};
 original_prepared_oscillators prepared;assert(original_oscillator_prepare(&prepared,&c));
 original_oscillators a={{1,2,3,4}},b=a;float raw=0,cached=0,rawsum=0,cachedsum=0;clock_t start=clock();
 for(unsigned i=0;i<262144;i++){assert(original_oscillator_sample(&a,&c,1,&raw));rawsum+=raw;}
 double rawtime=(double)(clock()-start)/CLOCKS_PER_SEC;start=clock();
 for(unsigned i=0;i<262144;i++){assert(original_oscillator_sample_prepared(&b,&prepared,1,&cached));cachedsum+=cached;}
 double cachedtime=(double)(clock()-start)/CLOCKS_PER_SEC;
 assert(bits(rawsum)==bits(cachedsum)&&bits(raw)==bits(cached)&&!memcmp(&a,&b,sizeof a));
 printf("HOST ONLY 262144 identical frames: repeated preparation %.6f s, control-rate prepared %.6f s; prepared state %zu bytes. No device improvement claim.\n",rawtime,cachedtime,sizeof prepared);
}
int main(int argc,char **argv){assert(argc==2);waves(argv[1]);phases(argv[1]);mp4(argv[1]);glides(argv[1]);controls();joined();cost();return 0;}
