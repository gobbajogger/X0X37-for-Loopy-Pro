/* SPDX-License-Identifier: GPL-3.0-only */
/* Time prepare + events + both synth renders. Host timing is not AC79 timing. */
#include "app/original_runtime.h"
#include "chicago_fx2_compact.h"
#include "chicago_tuning.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static original_runtime_part part[2];
static original_runtime_workspace work;
static original_chicago_tuning tuning;
static original_chicago_fx2_compact fx;
static original_chicago_fx2_cold_owner owner;
static unsigned width_calls;
static int check_precompute;
#ifdef X0X_PEAK_COUNT_WIDTH
int __real_original_pw_width(unsigned,float,float,float,float,float,float *);
int __wrap_original_pw_width(unsigned s,float i,float v,float p,float m,float w,float *o)
{ ++width_calls; return __real_original_pw_width(s,i,v,p,m,w,o); }
#endif
static double ns(clockid_t clock)
{ struct timespec t; clock_gettime(clock,&t); return (double)t.tv_sec*1e9+t.tv_nsec; }
static unsigned updates(unsigned i)
{
    if (part[i].kind==ORIGINAL_SOUND_CHICAGO) return part[i].dsp.chicago.voice.control_updates;
    unsigned n=0;
    for(unsigned v=0;v<ORIGINAL_MIAMI_BACKEND_VOICES;v++) n+=part[i].dsp.miami.voices[v].control_updates;
    return n;
}
static int run(FILE *csv,FILE *pcm,const char *name,int kind,unsigned notes,unsigned factory,unsigned choice)
{
    unsigned count=kind==2?2:1;
    double cold_cpu=ns(CLOCK_THREAD_CPUTIME_ID);
    for(unsigned i=0;i<count;i++) {
        unsigned k=kind==2?(i==0?ORIGINAL_SOUND_MIAMI:ORIGINAL_SOUND_CHICAGO):(unsigned)kind;
        uint8_t cell[32];
        memset(&part[i],0,sizeof part[i]);
        if(!original_sound_init(cell,k,k==ORIGINAL_SOUND_MIAMI?factory:0)) return 1;
        if(k==ORIGINAL_SOUND_CHICAGO && choice && !original_sound_set(cell,ORIGINAL_CHICAGO_FX_TYPE,choice)) return 1;
        if(original_runtime_init(&part[i],cell,120,&tuning,&owner)!=1) return 1;
    }
    cold_cpu=ns(CLOCK_THREAD_CPUTIME_ID)-cold_cpu;
    width_calls=0;
    static float l[256],r[256],saved[2][512];
    const unsigned keys[4]={48,55,60,64};
    for(unsigned b=0;b<128;b++) {
        unsigned u[2]={updates(0),count==2?updates(1):0};
        double wall=ns(CLOCK_MONOTONIC),cpu=ns(CLOCK_THREAD_CPUTIME_ID);
        for(unsigned i=0;i<count;i++) if(original_runtime_prepare(&part[i],120)!=1) return 1;
        if(strstr(name,"edits") && b>0 && b<=108) {
            /* All 18 macro endpoints at 0, .5 and 1, including live width edits.
             * Compilation is allowed here; it must leave no width work for ticks. */
            unsigned calls=width_calls;
            for(unsigned i=0;i<count;i++) if(part[i].kind==ORIGINAL_SOUND_MIAMI &&
                original_runtime_param(&part[i],(b-1)/6,((b-1)%6)/2*.5f)!=1) return 1;
            width_calls=calls;
        }
        if(b%32==0 || b%32==16) {
            for(unsigned i=0;i<count;i++) for(unsigned n=0;n<notes;n++)
                if(original_runtime_key(&part[i],ORIGINAL_INPUT_SEQUENCE,keys[n],b%32==0?100:0)!=1) return 1;
        }
        double elapsed=ns(CLOCK_THREAD_CPUTIME_ID)-cpu;
        for(unsigned i=0;i<count;i++) {
            cpu=ns(CLOCK_THREAD_CPUTIME_ID);
            if(original_runtime_render(&part[i],&work,l,r,256)!=1) return 1;
            elapsed+=ns(CLOCK_THREAD_CPUTIME_ID)-cpu;
            for(unsigned j=0;j<256;j++) { saved[i][2*j]=l[j];saved[i][2*j+1]=r[j]; }
        }
        wall=ns(CLOCK_MONOTONIC)-wall;
        if(pcm) for(unsigned i=0;i<count;i++)
            if(fwrite(saved[i],sizeof(float),512,pcm)!=512) return 1;
        fprintf(csv,"%s,%u,%u,%.0f,%.0f,%u,%u,%.0f\n",name,notes,b,elapsed,wall,updates(0)-u[0],count==2?updates(1)-u[1]:0,b==0?cold_cpu:0);
    }
#ifdef X0X_PEAK_COUNT_WIDTH
    printf("%s notes=%u pw_width_calls_after_init=%u\n",name,notes,width_calls);
#endif
    if(check_precompute && width_calls) {
        fprintf(stderr,"FAIL: pulse width recomputed during note/control work\n");
        return 1;
    }
    for(unsigned i=0;i<count;i++) if(original_runtime_retire(&part[i])!=1) return 1;
    return 0;
}
int main(int argc,char **argv)
{
    if(argc!=3 && argc!=4) return 2;
    check_precompute=argc==4 && !strcmp(argv[3],"--check-precompute");
#ifndef X0X_PEAK_COUNT_WIDTH
    if(check_precompute) return 2;
#endif
    FILE *csv=fopen(argv[1],"w"),*pcm=fopen(argv[2],"wb");
    if(!csv||!pcm||!original_chicago_tuning_bind(&tuning,0,0,NULL,0x6d70)||!original_chicago_fx2_compact_init(&fx)) return 1;
    owner=original_chicago_fx2_compact_owner(&fx);
    fputs("scenario,notes,block,cpu_ns,wall_ns,control0,control1,cold_init_cpu_ns\n",csv);
    int rc=0;
    for(unsigned n=1;n<=4;n+=3) {
        rc|=run(csv,pcm,"chicago",0,n,0,0);
        rc|=run(csv,pcm,"miami",1,n,0,0);
        rc|=run(csv,pcm,"both",2,n,0,0);
        rc|=run(csv,pcm,"both-miami29-chorus",2,n,29,1);
        rc|=run(csv,pcm,"both-miami18-peaking",2,n,18,6);
        rc|=run(csv,pcm,"miami-edits",1,n,0,0);
        for(unsigned p=0;p<ORIGINAL_MIAMI_BACKEND_FACTORIES;p++) {
            char name[40];snprintf(name,sizeof name,"miami-p%02u",p);
            rc|=run(csv,pcm,name,1,n,p,0);
        }
    }
    if(fclose(csv)||fclose(pcm)) rc=1;
    return rc;
}
