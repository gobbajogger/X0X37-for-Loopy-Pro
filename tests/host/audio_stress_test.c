/* SPDX-License-Identifier: GPL-3.0-only */
/* Whole production app. Radio, DMA, IRQ nesting and SPI are host substitutes. */
#define CHICAGO_PRESENTATION_NO_MAIN
#include "chicago_presentation_test.c"
#include "app/original_runtime.h"
#include "dsp/busfx.h"

static uint64_t synth_cpu[2];
static unsigned max_active[2], note_events[2][2][2], measured, runtime_faults, drop_one;
static uint64_t cpu_ns(void)
{
    struct timespec t;
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t);
    return (uint64_t)t.tv_sec * 1000000000u + (uint64_t)t.tv_nsec;
}
static void charge(original_runtime_part *p, uint64_t start, int rc)
{
    if (!measured) return;
    if (p->kind < 2) synth_cpu[p->kind] += cpu_ns() - start;
    if (rc != ORIGINAL_RUNTIME_OK) runtime_faults++;
}
int stress_real_original_runtime_prepare(original_runtime_part *, float);
int original_runtime_prepare(original_runtime_part *p, float bpm)
{ uint64_t t=cpu_ns(); int rc=stress_real_original_runtime_prepare(p,bpm); charge(p,t,rc); return rc; }
int stress_real_original_runtime_param(original_runtime_part *, unsigned, float);
int original_runtime_param(original_runtime_part *p, unsigned id, float value)
{ uint64_t t=cpu_ns(); int rc=stress_real_original_runtime_param(p,id,value); charge(p,t,rc); return rc; }
int stress_real_original_runtime_key(original_runtime_part *, unsigned, unsigned, unsigned);
int original_runtime_key(original_runtime_part *p,unsigned source,unsigned key,unsigned velocity)
{
    uint64_t t=cpu_ns(); int rc=stress_real_original_runtime_key(p,source,key,velocity); charge(p,t,rc);
    if(measured&&rc==ORIGINAL_RUNTIME_OK&&source<2)note_events[p->kind][source][velocity!=0]++;
    return rc;
}
int stress_real_original_runtime_init(original_runtime_part *,const uint8_t *,float,original_chicago_tuning *,const original_chicago_fx2_cold_owner *);
int original_runtime_init(original_runtime_part *p,const uint8_t *cell,float bpm,original_chicago_tuning *tuning,const original_chicago_fx2_cold_owner *fx)
{ uint64_t t=cpu_ns(); int rc=stress_real_original_runtime_init(p,cell,bpm,tuning,fx); charge(p,t,rc); return rc; }
int stress_real_original_runtime_release_source(original_runtime_part *,unsigned);
int original_runtime_release_source(original_runtime_part *p,unsigned source)
{ uint64_t t=cpu_ns(); int rc=stress_real_original_runtime_release_source(p,source); charge(p,t,rc); return rc; }
int stress_real_original_runtime_cut_source(original_runtime_part *,unsigned);
int original_runtime_cut_source(original_runtime_part *p,unsigned source)
{ uint64_t t=cpu_ns(); int rc=stress_real_original_runtime_cut_source(p,source); charge(p,t,rc); return rc; }
int stress_real_original_runtime_all_off(original_runtime_part *);
int original_runtime_all_off(original_runtime_part *p)
{ uint64_t t=cpu_ns(); int rc=stress_real_original_runtime_all_off(p); charge(p,t,rc); return rc; }
int stress_real_original_runtime_retire(original_runtime_part *);
int original_runtime_retire(original_runtime_part *p)
{ uint64_t t=cpu_ns(); int rc=stress_real_original_runtime_retire(p); charge(p,t,rc); return rc; }
int stress_real_original_runtime_render(original_runtime_part *,original_runtime_workspace *,float *,float *,uint32_t);
int original_runtime_render(original_runtime_part *p,original_runtime_workspace *w,float *l,float *r,uint32_t n)
{
    uint64_t t=cpu_ns(); int rc=stress_real_original_runtime_render(p,w,l,r,n); charge(p,t,rc);
    unsigned active=0;
    if(p->kind==ORIGINAL_SOUND_CHICAGO) active=p->dsp.chicago.voice.active!=0;
    else for(unsigned i=0;i<ORIGINAL_MIAMI_BACKEND_VOICES;i++) active+=p->dsp.miami.voices[i].active!=0;
    if(measured&&active>max_active[p->kind]) max_active[p->kind]=active;
    return rc;
}
static void packet(unsigned source,unsigned status,unsigned a,unsigned b)
{
    if(drop_one&&source==0&&(status&0xf0)==0x90) { drop_one=0;return; }
    CHECK(mi_w-mi_r<MQ);
    min_q[mi_w++%MQ]=(status>=0xf0?15u:status>>4)|(source<<4)|(status<<8)|(a<<16)|(b<<24);
}
static void stress_inputs(unsigned block)
{
    /* A 48-packet bounded burst per half, split over USB and BLE sources. */
    for(unsigned src=0;src<2;src++) for(unsigned v=0;v<2;v++) {
        unsigned ch=seq.ch[PART_303A+v];
        for(unsigned k=0;k<4;k++) {
            packet(src,0x80|ch,48+5*k,0);
            packet(src,0x90|ch,48+5*k,100);
        }
        packet(src,0xe0|ch,0,60+(block&7));
        packet(src,0xb0|ch,1,(block*3)&127);
    }
    for(unsigned src=0;src<2;src++) for(unsigned i=0;i<4;i++) packet(src,0xb0,74+i,(block+i)&127);
    /* Real SMC input ring, consumer, mixer commands and feedback projection. */
    uint8_t enqueued=smc_inputs[0].enqueued,dispatched=smc_inputs[0].dispatched,dropped=smc_inputs[0].dropped;
    for(unsigned f=0;f<8;f++) {
        uint8_t m[3]={(uint8_t)(0xe0+f),0,(uint8_t)(80+(block+f)%48)};
        smc_ble_midi(m,3);
    }
    smc_poll(); smc_frame();
    CHECK((uint8_t)(smc_inputs[0].enqueued-enqueued)==8);
    CHECK((uint8_t)(smc_inputs[0].dispatched-dispatched)==8);
    CHECK(smc_inputs[0].dropped==dropped);
    CHECK(smc_inputs[0].reason==0);
    engine_set(T_MST,0,MST_DJF,16+(block*5)%96);
    engine_set(T_MST,0,MST_COMP1,80);
    const uint8_t effects[8]={BFX_LP,BFX_HP,BFX_GATE16,BFX_STUT,BFX_SHUF,BFX_REV,BFX_DLY,BFX_LP};
    engine_smc_busfx(0xff,effects);
}
int main(int argc,char **argv)
{
    CHECK(argc==6);
    drop_one=getenv("X0X_STRESS_DROP_ONE")!=NULL;
    unsigned blocks=(unsigned)strtoul(argv[3],NULL,10); CHECK(blocks>=64);
    unsigned factory=(unsigned)strtoul(argv[4],NULL,10),choice=(unsigned)strtoul(argv[5],NULL,10);
    CHECK(factory<30&&choice<7);
    FILE *csv=fopen(argv[1],"w"),*pcm=fopen(argv[2],"wb"); CHECK(csv&&pcm);
    boot(); run_ms(50);
    fixture(0,0); fixture(1,1);
    CHECK(original_sound_init(proj.sound.v[T_303][0],ORIGINAL_SOUND_CHICAGO,0));
    CHECK(original_sound_init(proj.sound.v[T_303][1],ORIGINAL_SOUND_MIAMI,factory));
    CHECK(original_sound_set(proj.sound.v[T_303][0],ORIGINAL_CHICAGO_FX_TYPE,choice));
    engine_apply_sound(&proj.sound);audio();
    CHECK(engine_bass_bank(0)==0&&engine_bass_bank(1)==1);
    pattern_t *p=&proj.pat[0];
    for(unsigned kit=0;kit<2;kit++) {
        p->drum[kit].len=64; p->drum[kit].rate=RATE_32;
        for(unsigned v=0;v<NDRUM;v++) for(unsigned s=0;s<64;s++) sm_set(p->drum[kit].hit[v],s);
    }
    for(unsigned v=0;v<2;v++) {
        p->bass[v].len=64;p->bass[v].rate=RATE_32;
        for(unsigned s=0;s<64;s++) p->bass[v].step[s]=(bstep_t){(uint8_t)(48+s%24),G_NOTE|BS_ACCENT,127};
    }
    p->brk.steps=UINT32_MAX;p->brk.set[BRK_COMPLEX]=127;p->brk.set[BRK_ROLL]=127;
    for(unsigned part=0;part<NPARTS;part++) {
        seq.ppat[part]=0;
        engine_set(T_MIX,part,MX_LEVEL,110);
        engine_set(T_MIX,part,MX_REV,100);engine_set(T_MIX,part,MX_DLY,100);
    }
    engine_set(T_FX,0,FX_RV_LEVEL,100);engine_set(T_FX,0,FX_DL_LEVEL,100);
    engine_set(T_FX,0,FX_DL_FDBK,100);engine_set(T_FX,0,FX_DL_TYPE,1);
    engine_set(T_FX,0,FX_DL_WEAR,100);engine_set(T_FX,0,FX_DIST,6);engine_set(T_FX,0,FX_DRIVE,100);
    seq.bpm=400;seq.clk_src=SEQ_CLK_AUTO;seq_start(&seq);
    proj.set.bt=PROJ_BT_SMC_CENTRAL;smc_ble_link(1);smc_poll();
    ui.part=PART_303A;set_view(V_PART);build_pages();
    uint64_t stages[ENG_PROF_N]={0}; unsigned redraws=0,nonzero=0,audible=0,drums[2]={0};
    fputs("block,cpu_ns,wall_ns,chicago_cpu_ns,miami_cpu_ns,ui_cpu_ns,909_ns,808_ns,brk_ns,fx_ns\n",csv);
    for(unsigned b=0;b<blocks;b++) {
        stress_inputs(b);
        uint32_t before[ENG_PROF_N];for(unsigned k=0;k<ENG_PROF_N;k++)before[k]=eng_prof[k];
        int32_t output[1024];synth_cpu[0]=synth_cpu[1]=0;memset(note_events,0,sizeof note_events);measured=1;
        uint64_t start=cpu_ns();double wall=now_ns();
        /* The native ISR uses exactly these two 256-frame stretches per half. */
        engine_render(output,256);engine_volume(output,256);
        engine_render(output+512,256);engine_volume(output+512,256);
        uint64_t elapsed=cpu_ns()-start;wall=now_ns()-wall;measured=0;
        uint32_t delta[ENG_PROF_N];for(unsigned k=0;k<ENG_PROF_N;k++){delta[k]=eng_prof[k]-before[k];stages[k]+=delta[k];}
        CHECK(mi_r==mi_w);CHECK(!runtime_faults);CHECK(seq.playing);
        for(unsigned kind=0;kind<2;kind++)for(unsigned source=0;source<2;source++)for(unsigned edge=0;edge<2;edge++)
            if(note_events[kind][source][edge]!=4) {
                fprintf(stderr,"external note accounting failed: engine=%u source=%u edge=%u expected=4 got=%u\n",kind,source,edge,note_events[kind][source][edge]);
                exit(3);
            }
        for(unsigned p=0;p<NPARTS;p++)if(eng_peak[p])audible|=1u<<p;
        for(unsigned kit=0;kit<2;kit++)for(unsigned v=0;v<NDRUM;v++)if(eng_hit[kit][v])drums[kit]|=1u<<v;
        for(unsigned k=0;k<1024;k++)nonzero+=output[k]!=0;
        CHECK(fwrite(output,sizeof output[0],1024,pcm)==1024);
        now_ms=(uint32_t)(b*512000.0/44100.0)+100;
        unsigned before_blits=blits;
        start=cpu_ns();enc_acc[EN_ALGO]=1;ui_frame();smc_frame();uint64_t ui_cpu=cpu_ns()-start;
        redraws+=blits!=before_blits;
        fprintf(csv,"%u,%llu,%.0f,%llu,%llu,%llu,%u,%u,%u,%u\n",b,(unsigned long long)elapsed,wall,
            (unsigned long long)synth_cpu[0],(unsigned long long)synth_cpu[1],(unsigned long long)ui_cpu,
            delta[PR_909],delta[PR_808],delta[PR_BRK],delta[PR_FX]);
    }
    printf("coverage: redraws=%u/%u audible=%u drum_masks=%u/%u max_voices=%u/%u\n",redraws,blocks,audible,drums[0],drums[1],max_active[0],max_active[1]);
    CHECK(nonzero&&redraws>blocks/4);CHECK(max_active[0]==1&&max_active[1]==2);
    CHECK(audible==31&&drums[0]==2047&&drums[1]==2047);
    CHECK(stages[PR_909]&&stages[PR_808]&&stages[PR_BRK]&&stages[PR_FX]);
    CHECK(!fclose(csv)&&!fclose(pcm));
    printf("PASS stress coverage: blocks=%u frames=%u Chicago=%u Miami=%u redraws=%u nonzero=%u factory=%u FX=%u; host transport substitutes, no DMA/xrun proof\n",
        blocks,blocks*512,max_active[0],max_active[1],redraws,nonzero,factory,choice);
    return 0;
}
