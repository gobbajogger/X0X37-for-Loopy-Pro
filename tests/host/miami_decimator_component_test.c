/* SPDX-License-Identifier: GPL-3.0-only */
#include "miami_decimator_component.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <inttypes.h>
/* Independent wider reference: quotient/remainder correction rather than
 * production's unsigned-magnitude ceiling. No implementation-defined signed shift. */
static int64_t shift(int64_t x){int64_t q=x/32768;return q-(x%32768<0);}
static int16_t oracle(int held,int input,int wet){
 int64_t v=shift((int64_t)held*wet)+shift((int64_t)input*(32767-wet));
 return v < -32767 ? -32767 : v > 32767 ? 32767 : (int16_t)v;
}
static int16_t sample(unsigned i){return (int16_t)((int)((i*1664525u+1013904223u)&65535u)-32768);}
static void run(unsigned block,int16_t *output,miami_decimator_component_t *final){
 miami_decimator_component_t s;memset(&s,0,sizeof s);assert(miami_decimator_seed(&s,65535,-123,456,1486));
 for(unsigned start=0;start<8193;start+=block){
  unsigned count=8193-start;if(count>block)count=block;
  for(unsigned j=0;j<count;j++){
   unsigned i=start+j;
   if(i==1||i==63||i==64||i==127||i==8192)assert(miami_decimator_stage_step(&s,(i*97)&65535));
   assert(miami_decimator_frame(&s,i,sample(i),sample(i+19),(i*131)&32767,output+2*i));
  }
 }*final=s;
}
int main(int argc,char **argv){
 (void)argv;
 if(argc>1){
  miami_decimator_component_t s;assert(miami_decimator_seed(&s,0,0,0,6187));int16_t out[2];volatile int64_t checksum=0;
  clock_t t=clock();for(unsigned i=0;i<10000000;i++){assert(miami_decimator_frame(&s,i,sample(i),sample(i+1),16512,out));checksum+=out[0];}
  printf("sizeof_state=%zu frames=10000000 cpu_seconds=%.9f checksum=%" PRId64 "\n",sizeof s,(double)(clock()-t)/CLOCKS_PER_SEC,checksum);return 0;
 }
 const int held[]={-32767,-1,0,1,32767},wet[]={0,1,258,8256,16384,16512,32766,32767};
 unsigned long checks=0;
 for(unsigned h=0;h<5;h++)for(unsigned w=0;w<8;w++){
  miami_decimator_component_t s;assert(miami_decimator_seed(&s,0,held[h],-held[h],0));
  for(int x=-32768;x<=32767;x++){
   int16_t out[4]={12345,0,0,12345};int y=-1-x;
   assert(miami_decimator_frame(&s,1,(int16_t)x,(int16_t)y,wet[w],out+1));
   assert(out[1]==oracle(held[h],x,wet[w])&&out[2]==oracle(-held[h],y,wet[w]));
   assert(out[0]==12345&&out[3]==12345&&out[1]>=-32767&&out[2]>=-32767);checks++;
  }
 }
 const int edges[]={-32768,-1,0,1,32767};
 for(int w=0;w<=32767;w++)for(unsigned h=0;h<5;h++)for(unsigned e=0;e<5;e++){
  miami_decimator_component_t s;assert(miami_decimator_seed(&s,0,held[h],-held[h],0));int16_t out[2];
  assert(miami_decimator_frame(&s,1,(int16_t)edges[e],(int16_t)edges[4-e],w,out));
  assert(out[0]==oracle(held[h],edges[e],w)&&out[1]==oracle(-held[h],edges[4-e],w));checks++;
 }
 const unsigned phases[]={0,1,32767,65534,65535};
 for(unsigned p=0;p<5;p++)for(unsigned step=0;step<=65535;step++){
  miami_decimator_component_t s;assert(miami_decimator_seed(&s,phases[p],17,-19,step));int16_t out[2];
  assert(miami_decimator_frame(&s,0,INT16_MIN,INT16_MAX,32767,out));unsigned sum=phases[p]+step;
  assert(s.phase==(uint16_t)sum);assert(s.held_left==((sum&65536)?-32767:17));assert(s.held_right==((sum&65536)?32767:-19));
  assert(out[0]==oracle(s.held_left,INT16_MIN,32767));assert(out[1]==oracle(s.held_right,INT16_MAX,32767));checks++;
 }
 miami_decimator_component_t s,before;memset(&s,0,sizeof s);assert(miami_decimator_seed(&s,0,1,2,0));before=s;
 int16_t out[2]={44,55};assert(miami_decimator_frame(&s,0,2,1,16384,out));assert(out[0]==0);/* combined-shift would1 */
 assert(miami_decimator_seed(&s,0,1,2,0));assert(miami_decimator_stage_step(&s,100));
 assert(miami_decimator_frame(&s,63,3,4,0,out));assert(s.step==0&&s.pending==1);
 assert(miami_decimator_stage_step(&s,200));assert(miami_decimator_frame(&s,64,3,4,0,out));assert(s.step==200&&!s.pending&&s.phase==200);
 before=s;out[0]=44;out[1]=55;
 assert(!miami_decimator_frame(&s,128,1,2,-1,out));assert(!memcmp(&s,&before,sizeof s)&&out[0]==44&&out[1]==55);
 assert(!miami_decimator_frame(&s,128,1,2,32768,out));assert(!memcmp(&s,&before,sizeof s));
 assert(!miami_decimator_frame(&s,128,1,2,0,NULL));assert(!memcmp(&s,&before,sizeof s));
 assert(!miami_decimator_seed(&s,65536,0,0,0));assert(!memcmp(&s,&before,sizeof s));
 assert(!miami_decimator_seed(&s,0,-32768,0,0));assert(!memcmp(&s,&before,sizeof s));
 assert(!miami_decimator_seed(&s,0,0,32768,0));assert(!memcmp(&s,&before,sizeof s));
 assert(!miami_decimator_seed(&s,0,0,0,65536));assert(!memcmp(&s,&before,sizeof s));
 assert(!miami_decimator_stage_step(&s,UINT32_MAX));assert(!memcmp(&s,&before,sizeof s));
 assert(!miami_decimator_seed(NULL,0,0,0,0)&&!miami_decimator_stage_step(NULL,0));
 assert(!miami_decimator_frame(NULL,0,0,0,0,out));
 /* Reset is explicit seeding: replaces dirty/held/phase, not claimed original defaults. */
 assert(miami_decimator_seed(&s,7,-8,9,10));assert(!s.pending&&s.phase==7&&s.held_left==-8&&s.held_right==9&&s.step==10);
 int16_t *base=malloc(8193*2*sizeof *base),*candidate=malloc(8193*2*sizeof *candidate);assert(base&&candidate);
 miami_decimator_component_t a,b;run(1,base,&a);
 for(unsigned block=1;block<=256;block++){run(block,candidate,&b);assert(!memcmp(base,candidate,8193*2*sizeof *base));assert(!memcmp(&a,&b,sizeof a));}
 free(base);free(candidate);
 /* Independent stereo states interleaved: no shared storage, channel bleed or globals. */
 miami_decimator_component_t c,d;assert(miami_decimator_seed(&a,65530,0,0,127));b=a;c=a;d=a;
 for(unsigned i=0;i<1000;i++){
  int16_t x[2],y[2],u[2],v[2];
  assert(miami_decimator_frame(&a,i,sample(i),0,32767,x));assert(miami_decimator_frame(&b,i,0,sample(i+3),32767,y));
  assert(miami_decimator_frame(&c,i,sample(i),0,32767,u));assert(miami_decimator_frame(&d,i,0,sample(i+3),32767,v));
  assert(!memcmp(x,u,sizeof x)&&!memcmp(y,v,sizeof y)&&x[1]==0&&y[0]==0);
 }
 printf("PASS %lu integer oracle/phase cases;blocks1..256;checked state/output;stereo;sizeof=%zu\n",checks,sizeof s);return 0;
}
