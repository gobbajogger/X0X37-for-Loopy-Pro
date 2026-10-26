/* SPDX-License-Identifier: GPL-3.0-only */
/* Offline QA renderer; no firmware or reference-host execution. */
#include "wobble.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc,char **argv) {
    if(argc != 6 && argc != 7)return 2;
    unsigned rate=(unsigned)strtoul(argv[1],0,10), note=(unsigned)strtoul(argv[2],0,10);
    unsigned notes=(unsigned)strtoul(argv[4],0,10);
    if(rate<8000 || rate>192000 || note>127 || notes<1 || notes>2)return 2;
    wobble_t s; wobble_init(&s,(float)rate); wobble_set(&s,W_CUTOFF,1);
    if(!strcmp(argv[3],"mod_pulse")) {
        wobble_set(&s,W_CARRIER_LEVEL,0); wobble_set(&s,W_XMOD_LEVEL,.7f);
        wobble_set(&s,W_XMOD_WAVE,1);
    } else if(!strcmp(argv[3],"mix_depth") || !strcmp(argv[3],"mix_extreme")) {
        wobble_set(&s,W_XMOD_LEVEL,.7f);
        wobble_set(&s,W_XMOD_DEPTH,!strcmp(argv[3],"mix_extreme") ? 1 : .15f);
    } else if(strcmp(argv[3],"carrier_open"))return 2;
    FILE *f=fopen(argv[5],"wb"); if(!f)return 1;
    for(unsigned k=0;k<notes;k++) {
        unsigned current_note=(k && argc==7) ? (unsigned)strtoul(argv[6],0,10) : note;
        if(current_note>127)return 2;
        wobble_note_on(&s,(uint8_t)current_note,80);
        for(unsigned i=0;i<rate;i++) {
            float l,r;
            if(i==rate/2)wobble_note_off(&s,(uint8_t)current_note);
            wobble_render(&s,&l,&r,1);
            float pair[2]={l,r};
            if(fwrite(pair,sizeof(float),2,f)!=2)return 1;
        }
    }
    return fclose(f)!=0;
}
