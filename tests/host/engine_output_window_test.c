/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual host engine differential: split output windows and CPU2 lifecycle. */
#define main output_window_simulator_main
#include "../../../host/x0x_host.c"
#undef main
#include <assert.h>
static unsigned split_index, split_mode, cpu_mode, jobs, joins, deferred;
static void (*pending_job)(uint32_t);
static uint32_t pending_arg;
uint32_t output_window_until_event(seq_t *s, uint32_t n)
{
    static const unsigned cuts[] = {1,17,3,64,2,127,42,5,31};
    uint32_t real = seq_until_event(s,n);
    if (split_mode) { unsigned k=cuts[split_index++ % 9]; if(real>k)real=k; }
    assert(real && real<=n); return real;
}
int output_window_cpu2_run(void (*fn)(uint32_t), uint32_t arg)
{
    assert(!pending_job); ++jobs;
    if(cpu_mode==0)return 0;
    pending_job=fn;pending_arg=arg;++deferred;
    return 1;
}
int output_window_cpu2_wait(void)
{
    void (*fn)(uint32_t)=pending_job;uint32_t a=pending_arg;
    assert(fn);pending_job=0;++joins;
    if(cpu_mode==2)return -1; /* no completion, synchronous fallback */
    if(cpu_mode==3){ /* partial completion, then existing fallback policy */
        unsigned n=(a&255u)?(a&255u):256u;
        if(n>1)n/=2;
        fn((a&~255u)|n);
    }else fn(a);
    return cpu_mode==3?-1:0;
}
int main(int argc,char **argv)
{
    assert(argc==2);FILE *f=fopen(argv[1],"wb");assert(f);
    boot();engine_guard_enable(0);seq.playing=0;
    unsigned nonzero=0,blocks=0;
    const unsigned lens[]={256,1,255,3,129,17,300,0,64,251};
    for(cpu_mode=0;cpu_mode<4;cpu_mode++)for(split_mode=0;split_mode<2;split_mode++){
        split_index=0;
        for(unsigned b=0;b<100;b++){
            unsigned n=lens[b%10],written=n>256?256:n;
            struct {uint32_t pre[4];int32_t pcm[600];uint32_t post[4];} out;
            for(unsigned i=0;i<4;i++)out.pre[i]=out.post[i]=0x6b3d84f1u;
            for(unsigned i=0;i<600;i++)out.pcm[i]=(int32_t)0x13579bdf;
            if(b%7==0){engine_drum(1,b%11,0.8f);engine_drum(0,b%11,0.6f);}
            if(b%11==0){engine_bass_on_velocity(0,48+(b%24),1,0,110);engine_bass_on_velocity(1,60+(b%12),0,0,90);}
            if(b%11==7){engine_bass_off(0);engine_bass_off(1);}
            engine_render(out.pcm,n);assert(!pending_job);
            for(unsigned i=0;i<4;i++)assert(out.pre[i]==0x6b3d84f1u&&out.post[i]==0x6b3d84f1u);
            for(unsigned i=2*written;i<600;i++)assert(out.pcm[i]==(int32_t)0x13579bdf);
            for(unsigned i=0;i<2*written;i++)nonzero+=out.pcm[i]!=0;
            assert(fwrite(out.pcm,sizeof(int32_t),2*written,f)==2*written);++blocks;
        }
    }
    assert(nonzero&&jobs&&joins&&deferred);assert(!fclose(f));
    printf("PASS engine output windows: %u blocks; %u jobs/%u joins; nonzero=%u; 0/1/255/256/cap300, uneven splits, fallback/deferred/partial CPU2, guard words\n",blocks,jobs,joins,nonzero);return 0;
}
