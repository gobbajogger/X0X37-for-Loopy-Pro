/* SPDX-License-Identifier: GPL-3.0-only */
/* Private host renderer only. Writes headerless native float32 LR interleaved. */
#include "wobble.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int configure(wobble_t *s,const char *mode)
{
    wobble_set(s,W_CUTOFF,1);
    if(!strcmp(mode,"carrier_filter"))wobble_set(s,W_CUTOFF,.6f);
    else if(!strcmp(mode,"square_open"))wobble_set(s,W_CARRIER_WAVE,1);
    else if(!strcmp(mode,"sub_open")) {
        wobble_set(s,W_CARRIER_LEVEL,0); wobble_set(s,W_SUB_LEVEL,.7f);
    } else if(!strcmp(mode,"mod_triangle") || !strcmp(mode,"mod_pulse")) {
        wobble_set(s,W_CARRIER_LEVEL,0); wobble_set(s,W_XMOD_LEVEL,.7f);
        if(!strcmp(mode,"mod_pulse"))wobble_set(s,W_XMOD_WAVE,1);
    } else if(!strcmp(mode,"mix_depth")) {
        wobble_set(s,W_XMOD_LEVEL,.7f); wobble_set(s,W_XMOD_DEPTH,.15f);
    } else if(!strcmp(mode,"creative") || !strcmp(mode,"sweep")) {
        wobble_set(s,W_CARRIER_WAVE,1); wobble_set(s,W_SUB_LEVEL,.3f);
        wobble_set(s,W_XMOD_LEVEL,.35f); wobble_set(s,W_XMOD_DEPTH,.22f);
        wobble_set(s,W_CUTOFF,.75f); wobble_set(s,W_CRUSH,.18f);
        wobble_set(s,W_WOBBLE_DEPTH,.7f); wobble_set(s,W_WOBBLE_RATE,.65f);
        wobble_set(s,W_STEREO,.7f);
    } else if(strcmp(mode,"carrier_open"))return 0;
    return 1;
}
int main(int argc,char **argv)
{
    if(argc<3 || argc>4) { fprintf(stderr,"usage: render MODE OUTPUT.f32 [NOTES=1]\n");return 2; }
    unsigned notes=argc==4 ? (unsigned)strtoul(argv[3],0,10) : 1;
    if(notes<1 || notes>16)return 2;
    wobble_t s; wobble_init(&s,48000);
    if(!configure(&s,argv[1]))return 2;
    FILE *f=fopen(argv[2],"wb"); if(!f)return 1;
    float l[256],r[256],lr[512];
    const int creative=!strcmp(argv[1],"creative") || !strcmp(argv[1],"sweep");
    const int sweep=!strcmp(argv[1],"sweep");
    static const uint8_t riff[8]={36,36,43,39,36,48,46,43};
    for(unsigned k=0;k<notes;++k) {
        uint8_t note=creative ? riff[k%8] : 48;
        wobble_note_on(&s,note,80);
        for(unsigned p=0;p<48000;) {
            if(p==24000)wobble_note_off(&s,note);
            unsigned end=p<24000 ? 24000 : 48000;
            unsigned n=end-p<256 ? end-p : 256;
            if(sweep) {
                wobble_set(&s,W_CUTOFF,(float)p/48000);
                wobble_set(&s,W_XMOD_DEPTH,(float)(k%8)/7);
            }
            wobble_render(&s,l,r,n);
            for(unsigned i=0;i<n;++i) {lr[i*2]=l[i];lr[i*2+1]=r[i];}
            if(fwrite(lr,sizeof(float)*2,n,f)!=n) {fclose(f);return 1;} p+=n;
        }
    }
    if(fclose(f))return 1;
    fprintf(stderr,"rendered %u notes %u frames, 48000Hz float32 stereo; state=%zu bytes\n",
            notes,notes*48000,sizeof s);
    return 0;
}
