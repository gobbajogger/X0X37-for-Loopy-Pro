/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../../firmware/src/dsp/fxbus.h"
void old_fxbus_process(fxbus_t *,const float *,const float *,const float *,float *,int);
void old_fxbus_process_st(fxbus_t *,const float *,const float *,const float *,const float *,float *,float *,int);
static fxbus_t a,b;
static int16_t da[8192],db[8192];
static float l[256],r[256],rev[256],dly[256],al[256],ar[256],bl[256],br[256];
static void block(int n,int stereo,int note)
{
    memset(l,0,sizeof l);memset(r,0,sizeof r);memset(rev,0,sizeof rev);memset(dly,0,sizeof dly);
    if(note < 0) {l[0]=-0.0f;r[0]=-0.0f;}
    if(note > 0) for(int i=0;i<n && i<41;++i) {l[i]=(float)(i-19)*0.1f;r[i]=(float)(13-i)*0.08f;rev[i]=l[i]*0.3f;dly[i]=r[i]*0.4f;}
    memset(al,0,sizeof al);memset(ar,0,sizeof ar);memset(bl,0,sizeof bl);memset(br,0,sizeof br);
    if(stereo) {
        fxbus_process_st(&a,l,r,rev,dly,al,ar,n);old_fxbus_process_st(&b,l,r,rev,dly,bl,br,n);
    } else {fxbus_process(&a,l,rev,dly,al,n);old_fxbus_process(&b,l,rev,dly,bl,n);}
    assert(!memcmp(al,bl,sizeof al));assert(!memcmp(ar,br,sizeof ar));
    fxbus_t copy=b;copy.dbuf=a.dbuf;
    assert(!memcmp(&a,&copy,sizeof a));assert(!memcmp(da,db,sizeof da));
}
int main(void)
{
    const int spans[]={0,1,31,127,256};
    for(int mode=0;mode<8;++mode) for(int stereo=0;stereo<2;++stereo) {
        fxbus_init(&a,da,8192);fxbus_set(&a,FX_DIST,mode);fxbus_set(&a,FX_COMP,mode*18+1);
        fxbus_set(&a,FX_DL_TYPE,mode&1);fxbus_set(&a,FX_DL_PING,1);
        b=a;b.dbuf=db;memcpy(db,da,sizeof da);
        for(int i=0;i<30;++i) block(spans[i%5],stereo,0);
        block(127,stereo,-1);
        block(256,stereo,1);
        fxbus_set_bpm(&a,217.0f);fxbus_set_bpm(&b,217.0f);
        for(int i=0;i<1200;++i) block(spans[i%5],stereo,0);
        block(256,stereo,1);
        a.comp_det=b.comp_det=-0.0f;block(127,stereo,0);
    }
    /* Exhaust all legal compressor settings at the exact zero fixed point,
     * including a negative-zero input and the first audible attack. */
    for(int comp=0;comp<=127;++comp) for(int stereo=0;stereo<2;++stereo) {
        fxbus_init(&a,da,8192);fxbus_set(&a,FX_COMP,comp);
        b=a;b.dbuf=db;memcpy(db,da,sizeof da);
        block(256,stereo,0);block(127,stereo,-1);block(256,stereo,1);
    }
    puts("FX bus idle identity PASS: mono/stereo, all distortion modes, DIGI/TAPE, tails, retime, restart; state/ring/audio bytes equal");
}
