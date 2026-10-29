/* SPDX-License-Identifier: GPL-3.0-only */
#include "chicago_fx2_bank.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
int baseline_original_chicago_fx2_bank_events(original_chicago_fx2_bank *,const original_chicago_drive_event *,uint32_t,int32_t);
int main(int argc,char**argv){
 original_chicago_fx2_bank a,b;original_chicago_drive_event e[1025];for(unsigned i=0;i<1025;i++)e[i]=(original_chicago_drive_event){i/3,i%9};
 if(argc==4){unsigned n=(unsigned)strtoul(argv[2],NULL,10),iterations=(unsigned)strtoul(argv[3],NULL,10);assert(n<=1025);memset(&a,0,sizeof a);int (*fn)(original_chicago_fx2_bank *,const original_chicago_drive_event *,uint32_t,int32_t)=strcmp(argv[1],"baseline")?original_chicago_fx2_bank_events:baseline_original_chicago_fx2_bank_events;unsigned sum=0;clock_t t=clock();for(unsigned k=0;k<iterations;k++)sum+=(unsigned)fn(&a,e,n,(int32_t)k);double seconds=(double)(clock()-t)/CLOCKS_PER_SEC;assert(sum==iterations&&a.history_event_origin==(int32_t)(iterations-1));printf("%.9f\n",seconds);return 0;}
 assert(argc==1);
 for(unsigned test=0;test<32768;test++){
  memset(&a,0xa5,sizeof a);b=a;uint32_t n=test%1026;
  for(unsigned i=0;i<1025;i++)e[i]=(original_chicago_drive_event){i/3,(i+test)%9};
  if(test%17==0&&n>2)e[n/2].offset=UINT32_MAX;
  const original_chicago_drive_event *events=test%31==0?NULL:e;
  int32_t origin=test&1?(int32_t)test:-(int32_t)test;
  int old=baseline_original_chicago_fx2_bank_events(&a,events,n,origin),next=original_chicago_fx2_bank_events(&b,events,n,origin);
  assert(old==next&&!memcmp(&a,&b,sizeof a));
 }
 assert(!original_chicago_fx2_bank_events(NULL,e,1,0));
 puts("PASS32768 complete bank-state comparisons including queue ties,ignored types,negative origins,null/zero and unsorted rejection; one validation pass preserves every field; PENDING_REVIEW");return 0;
}
