/* SPDX-License-Identifier: GPL-3.0-only */
/* Ordinary insert_process is the unchanged old-path oracle. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../../experiments/original_synth/chicago_fx2_inserts.h"
#include "../../experiments/original_synth/chicago_program.h"
static chi_fx2_insert_arena a,b,before;
static int16_t al[513],ar[513],bl[513],br[513];
static unsigned accepted, refused;
static void block(unsigned type,unsigned n,int note)
{
    memset(al,0,sizeof al); memset(ar,0,sizeof ar);
    if(note) for(unsigned i=0;i<n && i<37;++i) {al[i]=(int16_t)(i*719-12000);ar[i]=(int16_t)(9000-i*311);}
    memcpy(bl,al,sizeof al);memcpy(br,ar,sizeof ar);before=a;
    if(!note && chi_fx2_insert_advance_zero(&a,type,n)) ++accepted;
    else {
        if(!note) {++refused;assert(!memcmp(&a,&before,sizeof a));}
        chi_fx2_insert_process(&a,type,al,ar,n);
    }
    chi_fx2_insert_process(&b,type,bl,br,n);
    assert(!memcmp(al,bl,sizeof al));assert(!memcmp(ar,br,sizeof ar));
    assert(!memcmp(&a,&b,sizeof a));
}
int main(void)
{
    const unsigned types[]={3,12,13,15,16}, spans[]={0,1,2,31,32,127,256,513};
    for(unsigned t=0;t<5;++t) for(unsigned setting=0;setting<4;++setting) {
        unsigned type=types[t];chi_fx2_insert_init(&a,type);
        chi_fx2_insert_set(&a,type,(uint8_t)(setting*42),(uint8_t)(127-setting*41),200+setting*930);b=a;
        unsigned start=accepted;
        for(unsigned k=0;k<100;++k) block(type,spans[k%8],0);
        assert(accepted>start);block(type,513,1);
        for(unsigned k=0;k<800;++k) block(type,spans[k%8],0);
        block(type,513,1); /* restart after the ordinary tail, including tiny floats */
    }
    unsigned factories=0;
    for(unsigned f=0;f<81;++f) {
        original_chicago_program p;original_chicago_routing routing;
        assert(original_chicago_program_init(&p,f));assert(original_chicago_program_routing(&p,&routing));
        unsigned type=routing.fx[1][0];if(!chi_fx2_insert_supports(type)) continue;
        ++factories;chi_fx2_insert_init(&a,type);
        chi_fx2_insert_set(&a,type,routing.fx[1][2],routing.fx[1][3],1200);b=a;
        for(unsigned k=0;k<30;++k) block(type,spans[k%8],0);
        block(type,513,1);
        for(unsigned k=0;k<800;++k) block(type,spans[k%8],0);
        block(type,513,1);
    }
    assert(factories==26);
    /* A nonzero cell anywhere in the history must prevent the shortcut. */
    for(unsigned t=0;t<5;++t) {
        chi_fx2_insert_init(&a,types[t]);chi_fx2_insert_set(&a,types[t],90,90,1200);
        a.ens.buf[0]=1;before=a;
        assert(!chi_fx2_insert_advance_zero(&a,types[t],256));assert(!memcmp(&a,&before,sizeof a));
    }
    printf("Chicago insert identity PASS: %u shortcuts, %u fallbacks; 26 factory routings; all arena/audio bytes equal\n",accepted,refused);
}
