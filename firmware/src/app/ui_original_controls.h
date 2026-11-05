/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include "original_sound.h"
#ifdef X0X_HOST
#include <string.h>
#endif /* Native unity build supplies the freestanding memory routines. */

/* Control metadata only. The released UI still owns all drawing and cadence. */
enum { UI_ORIGINAL_CHICAGO_PAGES = 7, UI_ORIGINAL_MIAMI_PAGES = 5 };
static const int8_t ui_original_chicago_rows[UI_ORIGINAL_CHICAGO_PAGES][4] = {
    {0,1,-1,-1}, {2,3,4,-1}, {5,6,-1,-1}, {7,8,9,10},
    {11,12,-1,-1}, {13,15,14,16}, {17,18,19,-1}
};
static const int8_t ui_original_miami_rows[UI_ORIGINAL_MIAMI_PAGES][4] = {
    {0,1,2,3}, {6,5,7,8}, {9,10,12,-1}, {11,13,-1,-1}, {4,15,14,-1}
};
static const char *const ui_original_chicago_titles[] = {
    "OSCILLATOR", "FILTER 1", "FILTER 2", "ENVELOPE", "AMPLIFIER", "ARPEGGIATOR", "EFFECT"
};
static const char *const ui_original_miami_titles[] = {
    "CARRIER", "XMOD", "FILTER", "WOBBLE", "OUTPUT"
};
static const char *ui_original_name(unsigned kind, unsigned id)
{
    static const char *const chicago[] = {
        "WAVE","GLIDE","CUTOFF","PEAK","ENV","BITE","GNAW",
        "ATTACK","DECAY","SUSTAIN","RELEASE","ENV/GATE","LEVEL",
        "ON","MODE","RANGE","SPEED","TYPE","EDIT 1","EDIT 2"
    };
    static const char *const miami[] = {
        "WAVE","LEVEL","SUB","PITCH","GLIDE","LEVEL","WAVE","TUNE",
        "DEPTH","CUTOFF","CRUSH","SHAPE","WOBBLE","RATE","OUTPUT","STEREO"
    };
    if (kind == ORIGINAL_SOUND_CHICAGO && id < 20) return chicago[id];
    return kind == ORIGINAL_SOUND_MIAMI && id < 16 ? miami[id] : "";
}

/* Storage remains q127; an encoder detent moves one original selector state. */
static unsigned ui_original_states(unsigned kind, unsigned id)
{
    if (kind == ORIGINAL_SOUND_CHICAGO) {
        if (id == 11 || id == 13) return 2;
        if (id == 14 || id == 15) return 3;
        if (id == 16) return 10;
    } else if (kind == ORIGINAL_SOUND_MIAMI) {
        if (id == 0) return 3;
        if (id == 3) return 25;
        if (id == 6 || id == 11) return 4;
        if (id == 13) return 8;
    }
    return 0;
}
static unsigned ui_original_quant(float x, unsigned count)
{
    unsigned q = (unsigned)(x * (float)(count - 1) + .5f);
    return q < count ? q : count - 1;
}

/* Presentation-only native 29-point waveform projection. This is a cached
 * shape cue, never a sampled audio waveform or a replacement DSP. */
enum { UI_ORIGINAL_WAVE_W=29, UI_ORIGINAL_WAVE_H=19 };
typedef struct {
    uint8_t wave[3][UI_ORIGINAL_WAVE_W], zero[3];
    uint8_t kind, arp_on, arp_mode, arp_range, arp_speed, amp_gate;
    uint8_t shape[3], wobble_rate;
    int8_t carrier_pitch, env_percent;
    int16_t xmod_tune;
} ui_original_control_view;

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
static void ui_original_curve(ui_original_control_view *v,unsigned slot,unsigned shape,int32_t width,int mg)
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


typedef struct {
    uint8_t cell[ORIGINAL_SOUND_BYTES], preview[22], voice, valid;
    uint32_t preview_mask;
    ui_original_control_view view;
} ui_original_control_cache;

/* Exactly the macro dependencies of native fields read below. A lock/motion
 * preview is explicit even if its byte equals a rounded factory display byte. */
#define UI_ORIGINAL_CHICAGO_TEXT_MASK ((1u<<0)|(1u<<2)|(1u<<3)|(1u<<4)|(1u<<11)|(1u<<13)|(1u<<14)|(1u<<15)|(1u<<16)|(1u<<21))
#define UI_ORIGINAL_MIAMI_TEXT_MASK ((1u<<0)|(1u<<3)|(1u<<6)|(1u<<7)|(1u<<9)|(1u<<10)|(1u<<11)|(1u<<13)|(1u<<17))
static int ui_original_controls_project(ui_original_control_view *out,
    const uint8_t cell[ORIGINAL_SOUND_BYTES], const uint8_t preview[22], uint32_t mask)
{
    if (!out || !original_sound_valid(cell)) return 0;
    unsigned kind = cell[29], n = original_sound_count(kind);
    if (mask >> n) return 0;
    for (unsigned i = 0; i < n; i++)
        if ((mask & (1u << i)) && preview[i] > original_sound_param(kind, i)->max) return 0;
    union { original_chicago_program chicago; original_miami_program miami; } program;
    memset(out, 0, sizeof *out);
    out->kind = (uint8_t)kind;
    if (kind == ORIGINAL_SOUND_CHICAGO) {
        original_chicago_program *p = &program.chicago;
        if (!original_sound_restore_chicago(p, cell)) return 0;
        for (unsigned i = 0; i < n; i++) if (mask & (1u << i)) {
            float x;
            if (!original_sound_control_value(kind, i, preview[i], &x) ||
                !original_chicago_program_set(p, i, x)) return 0;
        }
        float x;
#define CN(field, id, count) do { if (!original_chicago_program_native(p,id,&x)) return 0; out->field=(uint8_t)ui_original_quant(x,count); } while (0)
        CN(arp_on,62,2); CN(arp_mode,61,4); CN(arp_range,60,6);
        out->arp_range++;
        CN(amp_gate,107,2);
        out->arp_speed = (uint8_t)ui_original_quant(p->macro[16],10);
        if (!original_chicago_program_native(p,26,&x)) return 0;
        out->env_percent = (int8_t)((int)ui_original_quant(x,201)-100);
        float fa,fb,fw,fg,fo;
        if (!original_chicago_program_native(p,1,&fa) ||
            !original_chicago_program_native(p,9,&fb) ||
            !original_chicago_program_native(p,22,&fw) ||
            !original_chicago_program_native(p,3,&fg) ||
            !original_chicago_program_native(p,11,&fo)) return 0;
        unsigned a=ui_original_quant(fa,5),b=ui_original_quant(fb,5);
        if(a>3 || b>3)return 0;
        out->shape[0]=(uint8_t)a;
        int gain=(int)(fg*32768.0f+.5f),other=(int)(fo*32768.0f+.5f);
        if(gain<0 || gain>32768 || other<0 || other>32768)return 0;
        int32_t wa=ui_original_width(a,fw),wb=ui_original_width(b,fw);
        out->zero[0]=9;
        for(unsigned i=0;i<UI_ORIGINAL_WAVE_W;i++){
            int32_t phase=65536-(int32_t)(131072u*i/(UI_ORIGINAL_WAVE_W-1u));
            int32_t z=((ui_original_wave(a,phase,wa)/2)*gain+
                       (ui_original_wave(b,phase,wb)/2)*other)/16384;
            out->wave[0][i]=ui_original_plot(z,-16384,16384);
        }
#undef CN
    } else {
        original_miami_program *p = &program.miami;
        if (!original_sound_restore_miami(p, cell)) return 0;
        for (unsigned i = 0; i < n; i++) if (mask & (1u << i)) {
            float x;
            if (!original_sound_control_value(kind, i, preview[i], &x) ||
                !original_miami_program_set(p, i, x)) return 0;
        }
        float x;
        if (!original_miami_program_native(p,2,&x)) return 0;
        out->carrier_pitch = (int8_t)((int)ui_original_quant(x,49)-24);
        if (!original_miami_program_native(p,80,&x)) return 0;
        out->xmod_tune = (int16_t)((int)ui_original_quant(x,2001)-1000);
        out->wobble_rate = (uint8_t)ui_original_quant(p->macro[13],8);
        const unsigned ids[3] = {1,5,43};
        for (unsigned i = 0; i < 3; i++) {
            if (!original_miami_program_native(p,ids[i],&x)) return 0;
            unsigned shape = ui_original_quant(x,5);
            if (shape > 3) return 0;
            out->shape[i] = (uint8_t)shape;
            float fw;
            if (!original_miami_program_native(p,22,&fw)) return 0;
            ui_original_curve(out,i,shape,ui_original_width(shape,fw),i==2);
        }
    }
    return 1;
}

/* Called only for a text readout; unchanged controls reuse the tiny result. */
static int ui_original_controls_update(ui_original_control_cache *cache, unsigned voice,
    const uint8_t cell[ORIGINAL_SOUND_BYTES], const uint8_t preview[22], uint32_t mask)
{
    if (cache->valid && cache->voice == voice && cache->preview_mask == mask &&
        !memcmp(cache->cell,cell,ORIGINAL_SOUND_BYTES)) {
        unsigned i;
        for (i = 0; i < 22; i++)
            if ((mask & (1u << i)) && cache->preview[i] != preview[i]) break;
        if (i == 22) return 1;
    }
    cache->valid = 0;
    if (!ui_original_controls_project(&cache->view,cell,preview,mask)) return 0;
    memcpy(cache->cell,cell,ORIGINAL_SOUND_BYTES);
    memcpy(cache->preview,preview,22);
    cache->voice = (uint8_t)voice;
    cache->preview_mask = mask;
    cache->valid = 1;
    return 1;
}
