/* SPDX-License-Identifier: GPL-3.0-only */
#include "chicago_program.h"
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static uint32_t bits(float f){uint32_t u;memcpy(&u,&f,4);return u;}
static unsigned q(float x,unsigned n){int y=(int)fmaf(x,(float)(n-1),.5f);return y<0?0:y>=(int)n?n-1:(unsigned)y;}
static void check(const original_chicago_program *p,const float expected[117]){
 for(unsigned i=0;i<117;i++){
  float actual;assert(original_chicago_program_native(p,i,&actual));
  if(bits(actual)!=bits(expected[i])){fprintf(stderr,"native%u actual%08x expected%08x\n",i,bits(actual),bits(expected[i]));assert(0);}
 }
 original_chicago_routing r;assert(original_chicago_program_routing(p,&r));
 const float bases[10]={96,48,36,24,16,12,8,6,4,3};
 assert(r.arp_base_note==bases[q(expected[53],10)]);
 assert(r.arp_times==(float)(16-q(expected[54],16)));
 assert(r.arp_range==1+q(expected[60],6));assert(r.arp_mode==q(expected[61],4));
 assert(r.arp_enable==q(expected[62],2));assert(r.damper==q(expected[108],128));
 assert(r.arp_tempo_sync==q(expected[52],2));assert(r.arp_key_sync==q(expected[51],2));assert(r.arp_latch==q(expected[63],2));
 assert(r.vca_eg_mode==q(expected[107],2));
 assert(bits(r.bend_bipolar)==bits(fmaf(expected[105],2,-1)));
 assert(bits(r.master_squared)==bits(expected[96]*expected[96]));
 for(unsigned slot=0;slot<2;slot++)for(unsigned i=0;i<4;i++)assert(r.fx[slot][i]==q(expected[109+slot*4+i],i==0?30:i==1?2:128));
}
int main(int argc,char **argv){
 assert(argc==3);FILE *f=fopen(argv[1],"rb");assert(f);float expected[117];unsigned count=0;
 for(unsigned factory=0;factory<81;factory++){
  original_chicago_program p;assert(original_chicago_program_init(&p,factory));assert(p.revision==0);
  assert(original_chicago_program_name(factory));assert(fread(expected,sizeof expected,1,f)==1);check(&p,expected);
  float macros[22];assert(fread(macros,sizeof macros,1,f)==1);assert(!memcmp(macros,p.macro,sizeof macros));
  original_chicago_program held=p,saved=p;
  for(unsigned i=0;i<22;i++)assert(original_chicago_program_set(&p,i,1-p.macro[i]));
  assert(!memcmp(&held,&saved,sizeof held));assert(p.revision==22);saved=p;
  assert(!original_chicago_program_init(&p,81));assert(!original_chicago_program_set(&p,22,0));
  assert(!original_chicago_program_set(&p,0,NAN));assert(!original_chicago_program_set(&p,0,INFINITY));
  assert(!original_chicago_program_set(&p,0,-.01f));assert(!original_chicago_program_set(&p,0,1.01f));
  assert(!memcmp(&saved,&p,sizeof p));float sentinel=123;
  for(unsigned i=117;i<140;i++)assert(!original_chicago_program_native(&p,i,&sentinel)&&sentinel==123);
  count++;
 }
 assert(fgetc(f)==EOF);fclose(f);f=fopen(argv[2],"rb");assert(f);unsigned edits=0,factory,id;float value;
 while(fread(&factory,4,1,f)==1){
  assert(fread(&id,4,1,f)==1);assert(fread(&value,4,1,f)==1);assert(fread(expected,sizeof expected,1,f)==1);
  original_chicago_program p,held;assert(original_chicago_program_init(&p,factory));held=p;
  assert(original_chicago_program_set(&p,id,value));assert(p.revision==1);assert(bits(p.macro[id])==bits(value));
  for(unsigned i=0;i<22;i++)if(i!=id)assert(bits(p.macro[i])==bits(held.macro[i]));
  check(&p,expected);edits++;
 }
 assert(!ferror(f));fclose(f);
 assert(!original_chicago_program_name(81));assert(!original_chicago_program_init(NULL,0));
 original_chicago_program p;assert(original_chicago_program_init(&p,0));
 assert(!original_chicago_program_routing(&p,NULL));assert(!original_chicago_program_routing(NULL,NULL));
 assert(!original_chicago_program_native(&p,0,NULL));assert(!original_chicago_program_set(NULL,0,0));
 assert(sizeof p==208);
 printf("PASS %u original saved contexts (%u native fields), %u independent public edits/full117state/routing; snapshot%zu vs560 expanded\n",count,count*117,edits,sizeof p);
 return 0;
}
