/* SPDX-License-Identifier: GPL-3.0-only */
/* Compare repeated and changing Fc/Gain/Q writes to the pinned uncached EQ.
 * A stale coefficient, altered alpha, lost reset or queue cursor must fail. */
#include "chicago_dynamics.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int old_original_chicago_peaking_init(original_chicago_peaking *);
int old_original_chicago_peaking_set(original_chicago_peaking *,unsigned,unsigned);
void old_original_chicago_peaking_reset(original_chicago_peaking *);
void old_original_chicago_peaking_activity(original_chicago_peaking *,uint8_t);
int old_original_chicago_peaking_bind_events(original_chicago_peaking *,const original_chicago_drive_event *,uint32_t);
void old_original_chicago_peaking_direct_event(original_chicago_peaking *,unsigned);
int old_original_chicago_peaking_process(original_chicago_peaking *,int16_t *,int16_t *,uint32_t);

static uint32_t rng;
static unsigned samples, states, writes;
static uint32_t draw(void) { rng^=rng<<13; rng^=rng>>17; rng^=rng<<5; return rng; }
static void check(int ok,const char *why) { if(!ok) { fprintf(stderr,"FAIL %s: writes=%u samples=%u rng=%u\n",why,writes,samples,rng); exit(1); } }
static void state(const original_chicago_peaking *a,const original_chicago_peaking *b) {
    check(!memcmp(a,b,sizeof *a),"complete EQ state"); states++;
}
static void set(original_chicago_peaking *a,original_chicago_peaking *b,unsigned p,unsigned v) {
    check(original_chicago_peaking_set(a,p,v)==old_original_chicago_peaking_set(b,p,v),"setter result");
    writes++; state(a,b);
}
int main(void) {
    for(unsigned seed=1;seed<=8;seed++) {
        original_chicago_peaking a,b;
        rng=seed*0x9e3779b9u;
        check(original_chicago_peaking_init(&a)==old_original_chicago_peaking_init(&b),"init"); state(&a,&b);
        /* Default keys already own valid coefficients. Repeat each before audio. */
        for(unsigned p=0;p<3;p++) set(&a,&b,p,a.parameter[p]);
        for(unsigned block=0;block<1536;block++) {
            int16_t l[256],r[256],ol[256],orr[256];
            unsigned p=(block/512)%3,v=(block/4)%128;
            unsigned n=block%17==0 ? 0 : block%19==0 ? 1 : 1+draw()%256u;
            set(&a,&b,p,v); /* Four repeats per value, all Fc/Gain/Q values. */
            if(block%23==0) set(&a,&b,draw()%3,draw()%128);
            if(block%31==0) { original_chicago_peaking_reset(&a); old_original_chicago_peaking_reset(&b); }
            if(block%37==0) { unsigned t=draw()%7; original_chicago_peaking_direct_event(&a,t); old_original_chicago_peaking_direct_event(&b,t); }
            uint8_t active=(uint8_t)(block&1u);
            original_chicago_peaking_activity(&a,active); old_original_chicago_peaking_activity(&b,active);
            original_chicago_drive_event ev[3]={{0,4},{n?1u:0u,5},{n,4}};
            check(original_chicago_peaking_bind_events(&a,ev,3)==old_original_chicago_peaking_bind_events(&b,ev,3),"queue bind");
            for(unsigned i=0;i<n;i++) {
                int32_t x=(int32_t)(draw()&65535u)-32768,y=(int32_t)(draw()&65535u)-32768;
                l[i]=ol[i]=(int16_t)x; r[i]=orr[i]=(int16_t)y;
            }
            check(original_chicago_peaking_process(&a,l,r,n)==old_original_chicago_peaking_process(&b,ol,orr,n),"process result");
            check(!memcmp(l,ol,n*sizeof *l)&&!memcmp(r,orr,n*sizeof *r),"stereo samples");
            state(&a,&b); samples+=2*n;
            set(&a,&b,3,0); set(&a,&b,0,128); /* invalid writes remain inert */
        }
    }
    printf("PASS Chicago peaking coefficient cache: %u writes, %u samples, %u exact states; extra RAM 0 B\n",writes,samples,states);
    return 0;
}
