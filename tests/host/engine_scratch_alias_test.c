/* SPDX-License-Identifier: GPL-3.0-only */
/* Software-only full-engine PCM equivalence. Serial, inline-second-core and pthread-second-core. */
#include <pthread.h>
#define plat_cpu2_run original_cpu2_run
#define plat_cpu2_wait original_cpu2_wait
#define main simulator_main
#include HOST_SOURCE
#undef main
#undef plat_cpu2_run
#undef plat_cpu2_wait
static int mode;
static pthread_t worker;
static void (*job_fn)(uint32_t);
static uint32_t job_arg;
static void *job(void *unused) { (void)unused; job_fn(job_arg); return NULL; }
int plat_cpu2_run(void (*fn)(uint32_t), uint32_t arg) {
    if (!mode) return 0;
    if (mode==1) { fn(arg); return 1; }
    job_fn=fn;job_arg=arg;
    if (pthread_create(&worker,NULL,job,NULL)) abort();
    return 1;
}
int plat_cpu2_wait(void) { if (mode==2 && pthread_join(worker,NULL)) abort(); return 0; }
#include ENGINE_SOURCE
int main(int argc,char **argv) {
    if(argc!=2)return 2;mode=atoi(argv[1]);boot();
    int32_t pcm[512];uint32_t rng=0x12345678;
    for(unsigned b=0;b<4096;b++) {
        rng=rng*1664525u+1013904223u;
        unsigned n=b<256 ? b+1 : (rng%256)+1;
        if(b==256)seq_start(&seq);
        if(b==3072)seq_stop(&seq);
        if((b%7)==0) {drum909_trigger(&d909,(b/7)%NDRUM,0.7f);drum808_trigger(&d808,(b/7)%NDRUM,0.9f);}
        if((b%97)==0)engine_smc_solo((b&1)?(1u<<PART_909):0);
        if((b%101)==0)seq.vmute=(b&1)?0x15555u:0;
        memset(pcm,0xA5,sizeof pcm);engine_render(pcm,n);
        if(fwrite(pcm,sizeof pcm,1,stdout)!=1)return 3;
#define DUMP(x) if(fwrite((const void *)&(x),sizeof(x),1,stdout)!=1)return 3
        DUMP(dry_l); DUMP(dry_r); DUMP(rev); DUMP(dly);
        DUMP(t0); DUMP(t1); DUMP(t2); DUMP(tr); DUMP(kbuf);
        DUMP(mix_glide); DUMP(eng_peak); DUMP(eng_scope); DUMP(part_peak);
#undef DUMP
    }
    return 0;
}
