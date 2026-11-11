/* SPDX-License-Identifier: GPL-3.0-only */
/* Build master.c a second time with master_settled_zero disabled as old oracle. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../../firmware/src/dsp/master.h"
void old_master_process_st(master_t *,float *,float *,int,float);
static master_t a,b;
static float al[513],ar[513],bl[513],br[513];
static void block(int n,float volume,int note)
{
    memset(al,0,sizeof al);memset(ar,0,sizeof ar);
    if(note < 0) {al[0]=-0.0f;ar[0]=-0.0f;}
    if(note > 0) for(int i=0;i<n && i<43;++i) {al[i]=(float)(i-20)*0.13f;ar[i]=(float)(12-i)*0.11f;}
    memcpy(bl,al,sizeof al);memcpy(br,ar,sizeof ar);
    master_process_st(&a,al,ar,n,volume);old_master_process_st(&b,bl,br,n,volume);
    assert(!memcmp(&a,&b,sizeof a));assert(!memcmp(al,bl,sizeof al));assert(!memcmp(ar,br,sizeof ar));
}
int main(void)
{
    const int spans[]={0,1,31,64,127,256,513};
    for(int scenario=0;scenario<8;++scenario) {
        master_init(&a);
        if(scenario&1) master_set(&a,MST_LIMIT,0);
        if(scenario&2) {master_set(&a,MST_COMP1,95);master_set(&a,MST_PUMP,90);}
        if(scenario&4) master_set(&a,MST_MODE,3);
        b=a;
        for(int i=0;i<100;++i) block(spans[i%7],0.7f,0);
        block(127,0.7f,-1);
        block(513,0.7f,1);
        for(int i=0;i<1200;++i) block(spans[i%7],i<300?0.3f:0.7f,0);
        master_key(&a,0,1.0f);master_key(&b,0,1.0f);
        block(513,0.7f,1);
        /* Signed zeros and a stale limiter ring must also remain exact. */
        a.la[13]=b.la[13]=-0.0f;block(127,0.7f,0);
    }
    puts("Master idle identity PASS: silence, tail, volume slew, filter/comp/pump, limiter, restart; all state/audio bytes equal");
}
