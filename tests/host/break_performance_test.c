/* SPDX-License-Identifier: GPL-3.0-only */
/* Real ADPCM render: pitch by zero crossings; grain audio against decoded source;
 * unchanged hit timing; arbitrary callback partitioning; snapshot replacement. */
#define X0X_HOST 1
#define X0X_IMA_TABLES_DEFINE
#include "../../firmware/src/dsp/breaks.c"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define N 65536
static uint8_t data[(N+1)/2], replacement[(N+1)/2];
static int16_t decoded[N];
static float whole[BRK_FS], chunked[BRK_FS];
static unsigned checks, failures, events;
#define C(x) do {checks++;if(!(x)){failures++;fprintf(stderr,"line %d: %s\n",__LINE__,#x);}}while(0)
static void encode(void)
{
    int32_t pred=0,idx=0;
    for(unsigned i=0;i<N;i++) {
        int value=(int)(18000.0*sin(2.0*3.141592653589793*440.0*i/BRK_FS));
        int step=IMA_STEP_T[idx],diff=value-pred,code=0,vd=step>>3;
        if(diff<0){code=8;diff=-diff;}
        if(diff>=step){code|=4;diff-=step;vd+=step;}
        if(diff>=step>>1){code|=2;diff-=step>>1;vd+=step>>1;}
        if(diff>=step>>2){code|=1;vd+=step>>2;}
        pred+=(code&8)?-vd:vd;
        if (pred > 32767) pred = 32767;
        if (pred < -32768) pred = -32768;
        idx+=IMA_IDX_T[code&7];if(idx<0)idx=0;if(idx>88)idx=88;
        data[i>>1]|=(uint8_t)(code<<((i&1)*4));
    }
    pred=idx=0;
    for(unsigned i=0;i<N;i++) decoded[i]=(int16_t)ima_decode(data,i,&pred,&idx);
    for(unsigned i=0;i<sizeof data;i++)replacement[i]=data[i]^0x55;
}
static void accepted(void *ctx,int sl,int bank,unsigned gen)
{(void)ctx;(void)sl;(void)bank;(void)gen;events++;}
static void setup(breaks_t *b,int count,int jungle,int rev,int bend)
{
    breaks_init(b);brk_loop_t l={data,N,44100,1,(uint8_t)count};
    breaks_set_loop(b,0,&l);breaks_set_loop(b,1,&l);
    b->sp16=(float)N/16.0f;breaks_bend(b,bend);
    breaks_live(b,9,jungle);breaks_live(b,8,rev);breaks_slice_on(b,0);
    b->accepted=accepted;
}
static void render(breaks_t *b,float *out,int n,int size)
{
    for(int i=0;i<n;) {
        int k=n-i;if(k>size)k=size;breaks_render(b,out+i,k);
        for(int q=0;q<k;q++) C(isfinite(out[i+q]));
        i+=k;
    }
}
static int crossings(const float *out,int start,int end)
{int count=0;for(int i=start+1;i<end;i++)if(out[i-1]<=0&&out[i]>0)count++;return count;}
int main(void)
{
    encode();
    for(int bend=-8192;bend<=8192;bend+=8192) {
        breaks_t b;setup(&b,4,0,0,bend);render(&b,whole,BRK_FS,256);
        int expected=bend<0?200:bend>0?800:400;
        int actual=crossings(whole,BRK_FS/11,BRK_FS);
        C(abs(actual-expected)<=2);C(b.set[BRK_PITCH]==12);
    }
    for(int rev=0;rev<2;rev++)for(int count=4;count<=32;count*=2) {
        breaks_t a,b;setup(&a,count,1,rev,0);setup(&b,count,1,rev,0);
        events=0;render(&a,whole,BRK_FS,256);render(&b,chunked,BRK_FS,17);
        C(!memcmp(whole,chunked,sizeof whole));C(events==0);
        // Grain two starts at half a grain's source distance, keeps source pitch.
        setup(&a,count,1,rev,0);render(&a,whole,BRK_GRAIN_HOP+256,256);
        unsigned span=N/(unsigned)count;
        unsigned origin=rev?(span-(BRK_GRAIN_HOP/2)%span)%span:(BRK_GRAIN_HOP/2)%span;
        C(a.grain_anchor==origin);
        float scale=(float)a.set[BRK_LEVEL]/127.0f;scale=scale*scale/32768.0f;
        for(unsigned i=BRK_FADE+2;i<256;i++) {
            unsigned source=rev?(origin+span-i%span)%span:(origin+i)%N;
            C(fabsf(whole[BRK_GRAIN_HOP+i]-(float)decoded[source]*scale)<1e-5f);
        }
        C(breaks_count(&a,0)==count);
        if (!rev) {
            // Even a short 32-slice loop must retain dry forward continuity
            // before the first crossfaded grain. The test's slice endpoints
            // differ in amplitude/phase; a raw hi->lo seek breaks this equality.
            setup(&a,count,1,0,0);setup(&b,count,0,0,0);
            render(&a,whole,BRK_GRAIN_HOP,256);render(&b,chunked,BRK_GRAIN_HOP,256);
            C(!memcmp(whole,chunked,BRK_GRAIN_HOP*sizeof(float)));
            // Enabling jungle after the dry voice has passed its slice end
            // must also preserve every sample until the scheduled grain.
            setup(&a,count,0,0,0);setup(&b,count,0,0,0);
            render(&a,whole,BRK_GRAIN_HOP,256);render(&b,chunked,BRK_GRAIN_HOP,256);
            breaks_live(&a,9,1);
            render(&a,whole,BRK_GRAIN_HOP,256);render(&b,chunked,BRK_GRAIN_HOP,256);
            C(!memcmp(whole,chunked,BRK_GRAIN_HOP*sizeof(float)));
        }
        // Reload cannot rewrite a grain's sounding immutable snapshot.
        const brk_bank_t *old=a.v[a.cur].snap;
        brk_loop_t l={replacement,N,44100,1,(uint8_t)count};breaks_set_loop(&a,0,&l);
        render(&a,whole,BRK_GRAIN_HOP+1,256);C(a.v[a.cur].snap==old);C(old->adpcm==data);
        breaks_live(&a,9,0);breaks_slice_hold(&a,0,0);breaks_stop(&a);
        render(&a,whole,256,256);C(breaks_silent(&a));
    }
    // Across tempo/count combinations jungle and dry produce the same top-level
    // event count, including 32nd-note events within a render block.
    for(int count=4;count<=32;count*=2)for(int tempo=90;tempo<=180;tempo+=90) {
        unsigned hits[2];
        for(int jungle=0;jungle<2;jungle++) {
            breaks_t b;setup(&b,count,jungle,0,0);breaks_slice_hold(&b,0,0);
            breaks_set(&b,BRK_ALEN,0);breaks_set(&b,BRK_COMPLEX,0);events=0;
            float sp16=(float)BRK_FS*15.0f/(float)tempo;
            for(int s=0;s<16;s++){breaks_step(&b,s,0,sp16,1);render(&b,whole,(int)sp16,211);}
            hits[jungle]=events;
        }
        C(hits[0]>16&&hits[0]==hits[1]);
    }
    printf("BREAK performance: %u checks, %u failures\n",checks,failures);
    return failures?1:0;
}
