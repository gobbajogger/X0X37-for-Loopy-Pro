/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include "original_sound.h"
#include <string.h>

/* Presentation only. These are original macro IDs, never legacy DSP IDs. */
static const int8_t ui_original_chicago_rows[6][4] = {
    {2,3,4,0}, {5,6,1,11}, {7,8,9,10}, {12,20,-1,-1},
    {13,14,15,16}, {17,18,19,21}
};
static const int8_t ui_original_miami_rows[5][4] = {
    {0,1,2,3}, {6,5,7,8}, {11,12,13,9}, {4,16,10,-1}, {14,15,-1,17}
};
static const char *const ui_original_chicago_titles[] = {
    "TONE / OSC", "CHARACTER", "ENVELOPE", "OUTPUT", "LIVE ARP", "FX / DAMPER"
};
static const char *const ui_original_miami_titles[] = {
    "CARRIER", "X-MOD", "WOB/FILTER", "PITCH / FX", "OUTPUT"
};
static const char *ui_original_name(unsigned kind,unsigned id,unsigned fx)
{
    static const char *const chicago[] = {
        "WAVE","GLIDE","CUTOFF","PEAK","ENV AMT","BITE","GNAW",
        "ATTACK","DECAY","SUSTAIN","RELEASE","AMP MODE","LEVEL",
        "ENABLE","DIRECTION","OCTAVES","RATE","TYPE","EDIT 1","EDIT 2","P.BEND","DAMPER"
    };
    static const char *const miami[] = {
        "WAVE","LEVEL","SUB","PITCH","GLIDE","LEVEL","WAVE","TUNE",
        "DEPTH","CUTOFF","CRUSH","SHAPE","DEPTH","RATE","OUTPUT","STEREO","P.BEND","DAMPER"
    };
    /* Actual compact kernel setters. Chorus Edit2 couples feedback and wet;
     * Comp Attack and Decim Mix are active original controls. */
    static const char *const fx_names[7][2] = {
        {"EDIT 1","EDIT 2"},{"RATE","FB/WET"},{"RATE","MOD DEPTH"},
        {"FREQ","MIX"},{"SENS","ATTACK"},{"RATE","MIX"},{"FREQ","GAIN"}
    };
    if(kind==ORIGINAL_SOUND_CHICAGO && id<22)
        return (id==18 || id==19) && fx<7 ? fx_names[fx][id-18] : chicago[id];
    return kind==ORIGINAL_SOUND_MIAMI && id<18 ? miami[id] : "";
}

enum { UI_ORIGINAL_WAVE_W=29, UI_ORIGINAL_WAVE_H=19 };
/* Macro dependencies of every native field read by the display projection.
 * Chicago: gains3/11, cutoff24, peak25, gate107, arp62/61/60, damper108.
 * Miami: shapes1/5/43, cutoff24, peak25 (Crush), damper108.
 * Shape9 and pulse width22 are immutable base fields for Chicago; width22 is
 * also fixed for Miami. Other controls use matrix_value() directly. */
#define UI_ORIGINAL_CHICAGO_PREVIEW ((1u<<0)|(1u<<2)|(1u<<3)|(1u<<11)|(1u<<13)|(1u<<14)|(1u<<15)|(1u<<21))
#define UI_ORIGINAL_MIAMI_PREVIEW ((1u<<0)|(1u<<6)|(1u<<9)|(1u<<10)|(1u<<11)|(1u<<17))
typedef struct {
    uint8_t wave[3][UI_ORIGINAL_WAVE_W], zero[3], shape[3];
    uint8_t kind, arp_on, arp_mode, arp_range, amp_gate, damper, cutoff, peak;
} ui_original_view;
typedef struct {
    uint8_t cell[ORIGINAL_SOUND_BYTES], preview[22], voice, valid;
    uint32_t preview_mask;
    ui_original_view view;
} ui_original_cache;

/* Small integer projection of oscillator_motion.c at D=0x02000000 (128 samples
 * per cycle). At that increment the triangle correction is zero and the saw
 * edge coefficient rounds to .5 exactly. Q16 phase/width -> Q14 amplitude.
 * It is a raw shape cue, not audio after modulation/filter/effects. Sampling
 * occurs only when the displayed cell/preview changes, never per pixel/frame. */
static int32_t ui_original_saw(int32_t p)
{
    int32_t edge=p*64;
    if(edge>65536)edge=65536;
    if(edge< -65536)edge= -65536;
    return p-edge;
}
static int32_t ui_original_wave(unsigned shape,int32_t p,int32_t width)
{
    if(shape==0)return (65536-2*(p<0?-p:p))/4;
    int32_t z=ui_original_saw(p);
    if(shape>=2){int32_t q=p+2*width;if(q>65536)q-=131072;z-=ui_original_saw(q);}
    return z/4;
}
static int32_t ui_original_mg(unsigned shape,int32_t p)
{
    if(shape==0)return (65536-2*(p<0?-p:p))/4;
    if(shape==1)return -p/4;
    if(shape==2)return p/4;
    return p>=0?-16384:16384;
}
static uint8_t ui_original_plot(int32_t z,int32_t lo,int32_t hi)
{
    int32_t y=1+((hi-z)*16+(hi-lo)/2)/(hi-lo);
    return (uint8_t)(y<0?0:y>=UI_ORIGINAL_WAVE_H?UI_ORIGINAL_WAVE_H-1:y);
}
static void ui_original_curve(ui_original_view *v,unsigned slot,unsigned shape,int32_t width,int mg)
{
    int32_t lo=-16384,hi=16384;
    if(!mg && shape>=2){lo=-width/2;hi=32768-width/2;}
    v->zero[slot]=ui_original_plot(0,lo,hi);
    for(unsigned i=0;i<UI_ORIGINAL_WAVE_W;i++){
        int32_t phase=65536-(int32_t)(131072u*i/(UI_ORIGINAL_WAVE_W-1u));
        int32_t z=mg?ui_original_mg(shape,-phase):ui_original_wave(shape,phase,width);
        v->wave[slot][i]=ui_original_plot(z,lo,hi);
    }
}
static unsigned ui_original_quant(float x,unsigned count)
{ unsigned q=(unsigned)(x*(float)(count-1)+.5f);return q<count?q:count-1; }
static float ui_original_chicago_native(const original_chicago_program *p,unsigned id)
{ float x=0;(void)original_chicago_program_native(p,id,&x);return x; }
static float ui_original_miami_native(const original_miami_program *p,unsigned id)
{ float x=0;(void)original_miami_program_native(p,id,&x);return x; }
static int32_t ui_original_width(unsigned shape,float native_width)
{
    if(native_width<.01f)native_width=.01f;
    if(native_width>.99f)native_width=.99f;
    return shape<2?0:shape==2?32768:(int32_t)((1.0f-native_width)*65536.0f+.5f);
}

/* Factory floats remain exact. Override only explicit current motion/lock
 * previews, including an override numerically equal to the rounded cell byte.
 * Local program projection owns no DSP state and invokes no voice compiler. */
static int ui_original_project(ui_original_view *out,const uint8_t cell[ORIGINAL_SOUND_BYTES],
                               const uint8_t preview[22],uint32_t mask)
{
    if(!out || !original_sound_valid(cell))return 0;
    unsigned kind=cell[29],n=original_sound_count(kind);
    if(mask>>n)return 0;
    for(unsigned i=0;i<n;i++)if((mask&(1u<<i)) && preview[i]>original_sound_param(kind,i)->max)return 0;
    union { original_chicago_program chicago; original_miami_program miami; } program;
    memset(out,0,sizeof *out);out->kind=(uint8_t)kind;
    if(kind==ORIGINAL_SOUND_CHICAGO){
        original_chicago_program *p=&program.chicago;
        if(!original_sound_restore_chicago(p,cell))return 0;
        for(unsigned i=0;i<n;i++)if(mask&(1u<<i)){
            float x;if(!original_sound_control_value(kind,i,preview[i],&x) || !original_chicago_program_set(p,i,x))return 0;
        }
#define CN(id) ui_original_chicago_native(p,id)
        out->arp_on=(uint8_t)ui_original_quant(CN(62),2);
        out->arp_mode=(uint8_t)ui_original_quant(CN(61),4);
        out->arp_range=(uint8_t)(1+ui_original_quant(CN(60),6));
        out->amp_gate=(uint8_t)ui_original_quant(CN(107),2);
        out->damper=(uint8_t)(ui_original_quant(CN(108),128)>=64);
        out->cutoff=(uint8_t)ui_original_quant(CN(24),128);out->peak=(uint8_t)ui_original_quant(CN(25),128);
        unsigned a=ui_original_quant(CN(1),5),b=ui_original_quant(CN(9),5);
        if(a>3 || b>3)return 0;
        out->shape[0]=(uint8_t)a;
        int gain=(int)(CN(3)*32768.0f+.5f),other=(int)(CN(11)*32768.0f+.5f);
        if(gain<0 || gain>32768 || other<0 || other>32768)return 0;
        /* gain may be zero at the pure second-oscillator endpoint. Mark the
         * explicit two-source path separately from a standalone pulse cue. */
        int32_t wa=ui_original_width(a,CN(22)),wb=ui_original_width(b,CN(22));
        out->zero[0]=9;
        for(unsigned i=0;i<UI_ORIGINAL_WAVE_W;i++){
            int32_t phase=65536-(int32_t)(131072u*i/(UI_ORIGINAL_WAVE_W-1u));
            /* |Q14 wave|<=32768 even for a narrow pulse. Prehalving makes
             * each product <=2^29 and their sum <=2^30, within int32_t.
             * The lost sub-Q14 precision is below the checked pixel bound. */
            int32_t z=((ui_original_wave(a,phase,wa)/2)*gain+(ui_original_wave(b,phase,wb)/2)*other)/16384;
            out->wave[0][i]=ui_original_plot(z,-16384,16384);
        }
#undef CN
    }else{
        original_miami_program *p=&program.miami;
        if(!original_sound_restore_miami(p,cell))return 0;
        for(unsigned i=0;i<n;i++)if(mask&(1u<<i))if(!original_miami_program_set(p,i,(float)preview[i]/127.0f))return 0;
#define MN(id) ui_original_miami_native(p,id)
        out->damper=(uint8_t)(ui_original_quant(MN(108),128)>=64);
        out->cutoff=(uint8_t)ui_original_quant(MN(24),128);out->peak=(uint8_t)ui_original_quant(MN(25),128);
        const unsigned ids[3]={1,5,43};
        for(unsigned s=0;s<3;s++){
            unsigned shape=ui_original_quant(MN(ids[s]),5);if(shape>3)return 0;
            out->shape[s]=(uint8_t)shape;
            ui_original_curve(out,s,shape,ui_original_width(shape,MN(22)),s==2);
        }
#undef MN
    }
    return 1;
}
static int ui_original_update(ui_original_cache *cache,unsigned voice,
                              const uint8_t cell[ORIGINAL_SOUND_BYTES],const uint8_t preview[22],uint32_t mask)
{
    if(cache->valid && cache->voice==voice && cache->preview_mask==mask &&
       !memcmp(cache->cell,cell,ORIGINAL_SOUND_BYTES) && !memcmp(cache->preview,preview,22))return 0;
    cache->valid=0;
    if(voice>1 || !ui_original_project(&cache->view,cell,preview,mask))return -1;
    memcpy(cache->cell,cell,ORIGINAL_SOUND_BYTES);memcpy(cache->preview,preview,22);
    cache->preview_mask=mask;cache->voice=(uint8_t)voice;cache->valid=1;return 1;
}
