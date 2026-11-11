/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
/* Engine waveform cue for the original-synth oscillator controls.
 * Restored from ca8299b6 (ui_original.h): a small integer projection of
 * oscillator_motion.c at 128 samples per cycle. It is a raw shape cue, not
 * audio after modulation, filter or effects. It is sampled only when the
 * shown cell or its lock/motion preview changes, never per pixel or frame.
 * Include after ui_original_controls.h. */
enum { UI_ORIGINAL_WAVE_W=29, UI_ORIGINAL_WAVE_H=19 };
/* The preview masks of ca8299b6: a superset of the macros that move the
 * native fields read below (Chicago shapes 1/9, gains 3/11, width 22;
 * Miami shapes 1/5/43, width 22). */
#define UI_ORIGINAL_CHICAGO_WAVE_MASK ((1u<<0)|(1u<<2)|(1u<<3)|(1u<<4)|(1u<<11)|(1u<<13)|(1u<<14)|(1u<<15)|(1u<<16)|(1u<<21))
#define UI_ORIGINAL_MIAMI_WAVE_MASK ((1u<<0)|(1u<<3)|(1u<<6)|(1u<<7)|(1u<<9)|(1u<<10)|(1u<<11)|(1u<<13)|(1u<<17))
typedef struct {
    uint8_t cell[ORIGINAL_SOUND_BYTES], preview[22], voice, valid, kind;
    uint32_t preview_mask;
    uint8_t wave[3][UI_ORIGINAL_WAVE_W], zero[3];
} ui_original_wave_cache;

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
static void ui_original_curve(ui_original_wave_cache *v,unsigned slot,unsigned shape,int32_t width,int mg)
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
static int32_t ui_original_width(unsigned shape,float native_width)
{
    if(native_width<.01f)native_width=.01f;
    if(native_width>.99f)native_width=.99f;
    return shape<2?0:shape==2?32768:(int32_t)((1.0f-native_width)*65536.0f+.5f);
}
static int ui_original_wave_project(ui_original_wave_cache *out,const uint8_t cell[ORIGINAL_SOUND_BYTES],
                                    const uint8_t preview[22],uint32_t mask)
{
    if(!out || !original_sound_valid(cell))return 0;
    unsigned kind=cell[29],n=original_sound_count(kind);
    if(mask>>n)return 0;
    for(unsigned i=0;i<n;i++)if((mask&(1u<<i)) && preview[i]>original_sound_param(kind,i)->max)return 0;
    union { original_chicago_program chicago; original_miami_program miami; } program;
    out->kind=(uint8_t)kind;
    float x;
    if(kind==ORIGINAL_SOUND_CHICAGO){
        original_chicago_program *p=&program.chicago;
        if(!original_sound_restore_chicago(p,cell))return 0;
        for(unsigned i=0;i<n;i++)if(mask&(1u<<i)){
            if(!original_sound_control_value(kind,i,preview[i],&x) || !original_chicago_program_set(p,i,x))return 0;
        }
        float na,nb,ga,gb,w;
        if(!original_chicago_program_native(p,1,&na) || !original_chicago_program_native(p,9,&nb) ||
           !original_chicago_program_native(p,3,&ga) || !original_chicago_program_native(p,11,&gb) ||
           !original_chicago_program_native(p,22,&w))return 0;
        unsigned a=ui_original_quant(na,5),b=ui_original_quant(nb,5);
        if(a>3 || b>3)return 0;
        int gain=(int)(ga*32768.0f+.5f),other=(int)(gb*32768.0f+.5f);
        if(gain<0 || gain>32768 || other<0 || other>32768)return 0;
        int32_t wa=ui_original_width(a,w),wb=ui_original_width(b,w);
        out->zero[0]=9;
        for(unsigned i=0;i<UI_ORIGINAL_WAVE_W;i++){
            int32_t phase=65536-(int32_t)(131072u*i/(UI_ORIGINAL_WAVE_W-1u));
            /* Prehalving keeps each product <= 2^29 and the sum in int32_t. */
            int32_t z=((ui_original_wave(a,phase,wa)/2)*gain+(ui_original_wave(b,phase,wb)/2)*other)/16384;
            out->wave[0][i]=ui_original_plot(z,-16384,16384);
        }
    }else{
        original_miami_program *p=&program.miami;
        if(!original_sound_restore_miami(p,cell))return 0;
        for(unsigned i=0;i<n;i++)if(mask&(1u<<i)){
            if(!original_sound_control_value(kind,i,preview[i],&x) || !original_miami_program_set(p,i,x))return 0;
        }
        float w;
        if(!original_miami_program_native(p,22,&w))return 0;
        const unsigned ids[3]={1,5,43};
        for(unsigned s=0;s<3;s++){
            if(!original_miami_program_native(p,ids[s],&x))return 0;
            unsigned shape=ui_original_quant(x,5);if(shape>3)return 0;
            ui_original_curve(out,s,shape,ui_original_width(shape,w),s==2);
        }
    }
    return 1;
}
static int ui_original_wave_update(ui_original_wave_cache *cache,unsigned voice,
    const uint8_t cell[ORIGINAL_SOUND_BYTES],const uint8_t preview[22],uint32_t mask)
{
    if(cache->valid && cache->voice==voice && cache->preview_mask==mask &&
       !memcmp(cache->cell,cell,ORIGINAL_SOUND_BYTES) && !memcmp(cache->preview,preview,22))return 1;
    cache->valid=0;
    if(voice>1 || !ui_original_wave_project(cache,cell,preview,mask))return 0;
    memcpy(cache->cell,cell,ORIGINAL_SOUND_BYTES);memcpy(cache->preview,preview,22);
    cache->preview_mask=mask;cache->voice=(uint8_t)voice;cache->valid=1;return 1;
}
