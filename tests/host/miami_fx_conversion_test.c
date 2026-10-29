/* SPDX-License-Identifier: GPL-3.0-only */
#include "miami_fx_conversion.h"
#include "miami_decimator_component.h"
#include <assert.h>
#include <fenv.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
static int32_t floor15(int64_t x) { return (int32_t)(x>=0?x/32768:-((-x+32767)/32768)); }
static int16_t mix(int16_t in,int16_t held,int w) {
 int n=floor15((int64_t)in*(32767-w))+floor15((int64_t)held*w);
 if(n>32767)n=32767;
 if(n< -32767)n=-32767;
 return (int16_t)n;
}
static void composition(void) {
 for(unsigned size=1;size<=257;size++) {
  miami_decimator_component_t s; assert(miami_decimator_seed(&s,65535,0,0,6041));
  unsigned phase=65535,step=6041; int16_t held[2]={0,0};
  for(unsigned frame=0;frame<4096;frame++) {
   unsigned local=frame%size;
   if(!local) { step=(frame*29u)&65535u; assert(miami_decimator_stage_step(&s,step)); }
   int16_t input[2],output[2];
   uint32_t u=0x3e000000u+(frame*1597u)%0x01800000u;
   assert(miami_fx_input_rne(u,0,&input[0]));
   assert(miami_fx_input_rne(u^0x80000000u,1,&input[1]));
   unsigned sum=phase+step;phase=sum&65535u;
   if(sum&65536u){held[0]=input[0];held[1]=input[1];}
   int wet=frame%3==0?0:frame%3==1?32767:(int)(frame*71u%32768u);
   assert(miami_decimator_frame(&s,local,input[0],input[1],wet,output));
   for(unsigned ch=0;ch<2;ch++) {
    int16_t expected=mix(input[ch],held[ch],wet);
    assert(output[ch]==expected);
    assert(miami_fx_output_rne(output[ch])==miami_fx_output_rne(expected));
   }
  }
 }
 puts("PASS composed conversion/integer component: 257 invocation sizes x4096 stereo frames; explicit step/wet schedule, separate Q15 oracle");
}
int main(int argc,char **argv) {
 assert(miami_fx_rne_environment());
 if(argc==2 && strcmp(argv[1],"--bench")==0) {
  volatile uint32_t sink=0;clock_t t=clock();
  for(unsigned i=0;i<1000000;i++){int16_t q;assert(miami_fx_input_rne(0x3e800000u+i%0x1000000u,0,&q));sink^=miami_fx_output_rne(q);}
  printf("host one million mono conversions %.6f CPU seconds; sink=%"PRIu32"\n",(double)(clock()-t)/CLOCKS_PER_SEC,sink);return 0;
 }
 assert(argc==2);FILE *f=fopen(argv[1],"r");assert(f);char line[256];size_t count=0;
 while(fgets(line,sizeof line,f)) {
  uint32_t u,c,s,b,actual;int i,n;unsigned fz;
  if(line[0]=='C') {
   assert(sscanf(line,"C %u %"SCNx32" %"SCNx32" %"SCNx32" %d %d %"SCNx32,&fz,&u,&c,&s,&i,&n,&b)==7);
   assert(miami_fx_cleanup_rne(u)==c);assert(miami_fx_scaled_rne(c,fz,&actual)&&actual==s);
   assert(miami_fx_fcvtzs(actual)==i);assert(miami_fx_low16(i)==n);
   int16_t q;assert(miami_fx_input_rne(u,fz,&q)&&q==n);assert(miami_fx_output_rne(q)==b);
   assert(q>=-24575&&q<=24575);
  } else if(line[0]=='D') {
   assert(sscanf(line,"D %"SCNx32" %d",&u,&i)==2);assert(miami_fx_fcvtzs(u)==i);
  } else {assert(sscanf(line,"O %d %"SCNx32,&i,&b)==2);assert(miami_fx_output_rne((int16_t)i)==b);}
  count++;
 }
 assert(!ferror(f));fclose(f);
 int32_t vals[]={32767,32768,-32768,-32769,65535,65536,-65536,INT32_MAX,INT32_MIN};
 int expected[]={32767,-32768,-32768,32767,-1,0,0,-1,0};
 for(unsigned k=0;k<sizeof vals/sizeof *vals;k++)assert(miami_fx_low16(vals[k])==expected[k]);
 uint32_t sentinel=123;int16_t q=123;
 assert(!miami_fx_scaled_rne(0,2,&sentinel)&&sentinel==123);assert(!miami_fx_scaled_rne(0,0,NULL));
 assert(!miami_fx_input_rne(0,2,&q)&&q==123);assert(!miami_fx_input_rne(0,0,NULL));
 const uint32_t zeros[]={0,0x80000000u};
 for(unsigned z=0;z<2;z++) {
  for(unsigned fz=0;fz<2;fz++) {
   q=123;assert(miami_fx_input_rne(zeros[z],fz,&q)&&q==0);
   assert(!miami_fx_input_rne(zeros[z],fz,NULL));
  }
  q=123;assert(!miami_fx_input_rne(zeros[z],2,&q)&&q==123);
  assert(!miami_fx_input_rne(zeros[z],~0u,&q)&&q==123);
 }
 assert(miami_fx_output_rne(513)==0x3c804101u);assert(miami_fx_output_rne(-513)==0xbc804101u);
 assert(fesetround(FE_DOWNWARD)==0);assert(!miami_fx_rne_environment());assert(fesetround(FE_TONEAREST)==0);
 printf("PASS %zu rational-oracle vector rows; narrowing/invalid-input/RNE contracts\n",count);
 composition();return 0;
}
