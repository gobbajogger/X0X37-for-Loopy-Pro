/* SPDX-License-Identifier: GPL-3.0-only */
#include "master.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static uint32_t bits(float f){uint32_t u;memcpy(&u,&f,4);return u;}
int main(void){
 original_master s;original_master_init(&s);assert(s.current==0&&s.target==0&&s.step==0&&s.committed_mute==1&&s.desired_mute==0);
 uint32_t input[4]={0x3d000000,0x3d000000,0x3d000000,0x3d000000},l[4],r[4];
 original_master saved=s;assert(original_master_process(&s,1,NULL,NULL,NULL,NULL,0)&&!memcmp(&s,&saved,sizeof s));
 float v=.716f*.716f;assert(original_master_process(&s,v,input,input,l,r,4));
 for(unsigned i=0;i<4;i++)assert(l[i]==0x3d833d6d&&r[i]==l[i]);
 assert(bits(s.current)==0x40033d6d&&s.step==0&&s.committed_mute==0);
 original_master_mute(&s,1);assert(original_master_process(&s,v,input,input,l,r,4));
 const uint32_t expected[]={0x3d44dc24,0x3d033d6e,0x3c833d6f,0x31800000};
 for(unsigned i=0;i<4;i++)assert(l[i]==expected[i]&&r[i]==l[i]);
 assert(s.current==0&&s.target==0&&s.committed_mute==1);
 original_master_mute(&s,0);assert(original_master_process(&s,1,input,input,l,r,4));
 for(unsigned i=0;i<4;i++)assert(l[i]==0x3e000000&&r[i]==l[i]);
 /* Native compares mute bytes, not normalized booleans. */
 original_master_mute(&s,2);assert(original_master_process(&s,1,input,input,l,r,4)&&s.committed_mute==2);
 original_master_mute(&s,255);assert(original_master_process(&s,1,input,input,l,r,4)&&s.committed_mute==255);
 saved=s;assert(!original_master_process(&s,NAN,input,input,l,r,4)&&!memcmp(&s,&saved,sizeof s));
 puts("PASS core master first-unmute jump, mute-down rounding residual, end-block force, empty/invalid state and exact mute bytes");return 0;
}
