/* SPDX-License-Identifier: GPL-3.0-only */
#include "program_envelope.h"
#include "miami_voice_snapshot.h"
#include "chicago_voice_snapshot.h"
#include "binary32_math.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int baseline_original_miami_program_encode(const original_miami_program *,void *,size_t);
int baseline_original_chicago_program_encode(const original_chicago_program *,void *,size_t);
int baseline_original_miami_program_decode(original_miami_program *,const void *,size_t);
int baseline_original_chicago_program_decode(original_chicago_program *,const void *,size_t);
int baseline_original_miami_voice_snapshot_encode(const original_miami_voice_snapshot *,void *,size_t);
int baseline_original_miami_voice_snapshot_decode(original_miami_voice_snapshot *,const void *,size_t);
int baseline_original_chicago_voice_snapshot_encode(const original_chicago_voice_snapshot *,void *,size_t);
int baseline_original_chicago_voice_snapshot_decode(original_chicago_voice_snapshot *,const void *,size_t);
int main(void){
 unsigned char a[556],b[556];unsigned cases=0;
 for(unsigned kind=0;kind<2;kind++)for(unsigned factory=0;factory<(kind?81u:30u);factory++){
  original_miami_program m,ma,mb;original_chicago_program c,ca,cb;assert(kind?original_chicago_program_init(&c,factory):original_miami_program_init(&m,factory));
  for(unsigned id=0;id<(kind?22u:18u);id++)for(unsigned endpoint=0;endpoint<3;endpoint++){
   float x=endpoint==0?-0.0f:endpoint==1?.123456791f:1.0f;size_t size=kind?224u:188u;
   if(kind){assert(original_chicago_program_set(&c,id,x));assert(baseline_original_chicago_program_encode(&c,a,sizeof a)==original_chicago_program_encode(&c,b,sizeof b));assert(!memcmp(a,b,size));assert(baseline_original_chicago_program_decode(&ca,a,size)&&original_chicago_program_decode(&cb,b,size)&&!memcmp(&ca,&cb,sizeof ca));}
   else{assert(original_miami_program_set(&m,id,x));assert(baseline_original_miami_program_encode(&m,a,sizeof a)==original_miami_program_encode(&m,b,sizeof b));assert(!memcmp(a,b,size));assert(baseline_original_miami_program_decode(&ma,a,size)&&original_miami_program_decode(&mb,b,size)&&!memcmp(&ma,&mb,sizeof ma));}
   cases++;
  }
 }
 original_miami_program p;assert(original_miami_program_init(&p,0));uint32_t random=0xa5a5f00d;
 for(unsigned i=0;i<1000000;i++){
  random=random*1664525u+1013904223u;p.macro[0]=original_math_float(random);memset(a,0xa5,sizeof a);memset(b,0xa5,sizeof b);
  assert(baseline_original_miami_program_encode(&p,a,sizeof a)==original_miami_program_encode(&p,b,sizeof b)&&!memcmp(a,b,sizeof a));cases++;
 }
 for(unsigned fill=0;fill<256;fill++){
  original_miami_voice_snapshot m,ma,mb;original_chicago_voice_snapshot c,ca,cb;memset(&m,(int)fill,sizeof m);memset(&c,(int)fill,sizeof c);memset(a,0xa5,sizeof a);memset(b,0xa5,sizeof b);
  int old=baseline_original_miami_voice_snapshot_encode(&m,a,sizeof a),next=original_miami_voice_snapshot_encode(&m,b,sizeof b);assert(old==next&&!memcmp(a,b,sizeof a));
  if(old){assert(baseline_original_miami_voice_snapshot_decode(&ma,a,ORIGINAL_MIAMI_VOICE_SNAPSHOT_BYTES)&&original_miami_voice_snapshot_decode(&mb,b,ORIGINAL_MIAMI_VOICE_SNAPSHOT_BYTES)&&!memcmp(&ma,&mb,sizeof ma));}
  memset(a,0xa5,sizeof a);memset(b,0xa5,sizeof b);old=baseline_original_chicago_voice_snapshot_encode(&c,a,sizeof a);next=original_chicago_voice_snapshot_encode(&c,b,sizeof b);assert(old==next&&!memcmp(a,b,sizeof a));
  if(old){assert(baseline_original_chicago_voice_snapshot_decode(&ca,a,sizeof a)&&original_chicago_voice_snapshot_decode(&cb,b,sizeof b)&&!memcmp(&ca,&cb,sizeof ca));}
  cases+=2;
 }
 printf("PASS %u unshared-vs-shared codec return/complete-buffer/value cases,all111factory edits,1000000 raw32 program inputs,all256 byte fills for both130/135field voice schemas; wire bytes unchanged; generated baseline,not original runtime/review\n",cases);return 0;
}
