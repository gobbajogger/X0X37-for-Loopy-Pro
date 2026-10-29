/* SPDX-License-Identifier: GPL-3.0-only */
/* Compare independently compiled immutable and candidate sample functions.
 * Tables are the existing authentic44100 fixtures. This checks preservation
 * of the admitted component, not a new original-plugin or device sound claim. */
#include "miami_ms20.h"
#include "motion_tables_private.h"
#include <fenv.h>
#include <float.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int reference_miami_ms20_sample(original_miami_ms20 *,float,float,float *);
typedef int (*sample_function)(original_miami_ms20 *,float,float,float *);
static unsigned long cases,accepted,rejected;
static uint32_t seed=UINT32_C(0x7bd156a9);
#define CHECK(x) do { if (!(x)) { \
 fprintf(stderr,"FAIL case%lu line%d: %s\n",cases,__LINE__,#x);exit(1); \
} } while (0)

static float value(uint32_t bits){float f;memcpy(&f,&bits,sizeof f);return f;}
static uint32_t bits(float f){uint32_t u;memcpy(&u,&f,sizeof u);return u;}
static uint32_t random_word(void){
 seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;
}
static float unit(void){return value(UINT32_C(0x3f800000)|(random_word()>>9))-1.0f;}
/* These are legal float subobjects, including the normally immutable fields.
 * Do not test type-punned writes into integer fields or padding as API aliasing. */
static float *field(original_miami_ms20 *s,unsigned index){
 CHECK(index<16);
 if(index<5)return s->history+index;
 if(index==5)return &s->b;
 if(index==6)return &s->cap;
 original_ramp *r=index<10?&s->a:index<13?&s->resonance:&s->noise_gain;
 unsigned member=(index-7)%3;
 return member==0?&r->current:member==1?&r->target:&r->delta;
}
static void seed_filter(original_miami_ms20 *s,unsigned index,float cutoff,
 float resonance,float noise,uint32_t duration){
 original_ms20_tables t={original_miami_a44100,original_miami_b44100,
                        original_miami_r,original_miami_cap};
 CHECK(original_miami_ms20_init(s,index,original_miami_a44100[0],
       original_miami_b44100[0],original_miami_cap[0]));
 CHECK(original_miami_ms20_prepare(s,&t,cutoff,resonance,0,1,0,0,0,
                                 noise,0,duration));
 /* The transaction must preserve object padding as well as named fields. */
 for(size_t i=offsetof(original_miami_ms20,prepared)+sizeof s->prepared;
     i<sizeof *s;i++)((unsigned char *)s)[i]=(unsigned char)(0xa0u+i);
}
static void compare(const original_miami_ms20 *initial,float osc,float gain,
 int output_field,original_miami_ms20 *next){
 original_miami_ms20 a,b;
 memcpy(&a,initial,sizeof a);memcpy(&b,initial,sizeof b);
 float ao=value(UINT32_C(0xc3161000)),bo=ao;
 float *ap=output_field>=0?field(&a,(unsigned)output_field):
           output_field== -2?NULL:&ao;
 float *bp=output_field>=0?field(&b,(unsigned)output_field):
           output_field== -2?NULL:&bo;
 CHECK(feclearexcept(FE_ALL_EXCEPT)==0);
 int ar=reference_miami_ms20_sample(&a,osc,gain,ap);
 int af=fetestexcept(FE_ALL_EXCEPT);
 CHECK(feclearexcept(FE_ALL_EXCEPT)==0);
 int br=original_miami_ms20_sample(&b,osc,gain,bp);
 int bf=fetestexcept(FE_ALL_EXCEPT);
 cases++;CHECK(ar==br);CHECK(af==bf);
 CHECK(!memcmp(&a,&b,sizeof a));CHECK(bits(ao)==bits(bo));
 if(ar)accepted++;
 else{
  rejected++;CHECK(!memcmp(&a,initial,sizeof a));
  CHECK(bits(ao)==UINT32_C(0xc3161000));
 }
 if(next)memcpy(next,&b,sizeof b);
}
static void prepared_histories(void){
 static const uint32_t durations[]={1,2,63,64,256,65535,2147482880u};
 for(unsigned cut=0;cut<=128;cut++)for(unsigned res=0;res<=4;res++)
 for(unsigned d=0;d<sizeof durations/sizeof durations[0];d++){
  original_miami_ms20 s;
  seed_filter(&s,(cut+res)%128,(float)cut/128.0f,(float)res/4.0f,
              (float)(cut%9)/8.0f,durations[d]);
  for(unsigned frame=0;frame<32;frame++){
   float osc=(unit()-.5f)*4,gain=1+unit()*3;
   compare(&s,osc,gain,-1,&s);
  }
  for(unsigned alias=0;alias<16;alias++)compare(&s,.125f,1,(int)alias,NULL);
 }
}
static void edge_cases(void){
 static const uint32_t special[]={
  0,0x80000000u,1,0x80000001u,0x007fffffu,0x807fffffu,
  0x00800000u,0x80800000u,0x3f800000u,0xbf800000u,
  0x7f7fffffu,0xff7fffffu,0x7f800000u,0xff800000u,
  0x7fc12345u,0xffc12345u,0x7f812345u,0xff812345u
 };
 original_miami_ms20 initial;
 seed_filter(&initial,127,.5f,.75f,.125f,64);
 for(unsigned i=0;i<128;i++)compare(&initial,(unit()-.5f)*2,1,-1,&initial);
 for(unsigned f=0;f<16;f++)for(unsigned k=0;k<sizeof special/sizeof special[0];k++){
  original_miami_ms20 s;memcpy(&s,&initial,sizeof s);*field(&s,f)=value(special[k]);
  for(int alias= -2;alias<16;alias++)compare(&s,.25f,1,alias,NULL);
 }
 for(unsigned a=0;a<sizeof special/sizeof special[0];a++)
 for(unsigned b=0;b<sizeof special/sizeof special[0];b++)
 for(int alias= -2;alias<16;alias++)
  compare(&initial,value(special[a]),value(special[b]),alias,NULL);
 /* Different ramp directions, zero deltas, pending targets and LFSR words. */
 for(unsigned trial=0;trial<32768;trial++){
  original_miami_ms20 s;memcpy(&s,&initial,sizeof s);
  s.noise=random_word();s.prepared=(uint8_t)(trial%256);
  for(unsigned f=0;f<16;f++){
   float q=unit();
   if(f<5)q=(q-.5f)*4;
   if(f==9||f==12||f==15)q=(trial%3)?(q-.5f)*.25f:0;
   *field(&s,f)=q;
  }
  if((trial%8)==0)*field(&s,random_word()%16)=value(random_word());
  compare(&s,(unit()-.5f)*4,unit()*4,(int)(trial%18)-2,NULL);
 }
 float out=123;
 CHECK(!reference_miami_ms20_sample(NULL,0,1,&out)&&out==123);
 CHECK(!original_miami_ms20_sample(NULL,0,1,&out)&&out==123);
}
static void benchmark(sample_function fn,unsigned count,double *seconds,
 uint32_t *sum_bits,original_miami_ms20 *result){
 original_miami_ms20 s;seed_filter(&s,19,.57f,.4f,.013f,64);
 float inputs[256];for(unsigned i=0;i<256;i++)inputs[i]=(float)((int)i-128)/256.0f;
 float sum=0;unsigned success=0;clock_t start=clock();
 for(unsigned i=0;i<count;i++){
  float out=0;success+=(unsigned)fn(&s,inputs[i%256],1,&out);sum+=out;
 }
 *seconds=(double)(clock()-start)/CLOCKS_PER_SEC;
 CHECK(success==count);*sum_bits=bits(sum);memcpy(result,&s,sizeof s);
}
int main(int argc,char **argv){
 CHECK(fegetround()==FE_TONEAREST);
 if(argc==2&&!strcmp(argv[1],"--bench")){
  const unsigned count=1048576;
  for(unsigned run=0;run<8;run++){
   double a,b;uint32_t ah,bh;original_miami_ms20 as,bs;
   if(run&1){benchmark(original_miami_ms20_sample,count,&b,&bh,&bs);
             benchmark(reference_miami_ms20_sample,count,&a,&ah,&as);}
   else{benchmark(reference_miami_ms20_sample,count,&a,&ah,&as);
        benchmark(original_miami_ms20_sample,count,&b,&bh,&bs);}
   CHECK(ah==bh&&!memcmp(&as,&bs,sizeof as));
   printf("BENCH run=%u frames=%u baseline_seconds=%.6f candidate_seconds=%.6f checksum=%08x\n",
          run,count,a,b,ah);
  }
  return 0;
 }
 CHECK(argc==1);prepared_histories();edge_cases();
 CHECK(accepted>200000&&rejected>1000);
 printf("PASS Miami MS20 transaction: %lu exact state/output/status/host-FP-flag cases, %lu accepted, %lu rejected; all16 legal float-field aliases; state_bytes=%zu\n",
        cases,accepted,rejected,sizeof(original_miami_ms20));
 return 0;
}
