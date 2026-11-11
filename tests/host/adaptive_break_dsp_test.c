/* SPDX-License-Identifier: GPL-3.0-only */
#define X0X_HOST 1
#define X0X_IMA_TABLES_DEFINE
#include "../../firmware/src/dsp/breaks.c"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
static unsigned checks, fails, events;
#define CHECK(c) do { checks++; if (!(c)) { fails++; fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c); } } while(0)
static uint8_t data[4097], other[4097];
static int last_slice, last_bank;
static void accepted(void *ctx,int sl,int bank,unsigned gen) { (void)ctx;(void)gen;events++;last_slice=sl;last_bank=bank; }
static void render(breaks_t *b,int n) { float y[256];while(n){int k=n>256?256:n;breaks_render(b,y,k);for(int i=0;i<k;i++) CHECK(isfinite(y[i]));n-=k;} }
int main(void)
{
    for(unsigned i=0;i<sizeof data;i++) { data[i]=(uint8_t)(i*13u+17u);other[i]=(uint8_t)(i*23u+5u); }
    const unsigned counts[]={4,8,16,32}, lengths[]={64,65,511,512,513,8193};
    for(unsigned ni=0;ni<4;ni++) for(unsigned li=0;li<6;li++) {
        breaks_t b;breaks_init(&b);b.accepted=accepted;
        brk_loop_t l={data,lengths[li],22050,1,(uint8_t)counts[ni]};
        breaks_set_loop(&b,0,&l);const brk_bank_t *k=breaks_bank(&b,0);
        CHECK(k->valid && k->slices==counts[ni]);
        int32_t pred=0,idx=0;
        for(unsigned pos=0, s=0;pos<l.nsamples;pos++) {
            if(s<32 && pos==k->start[s]) { CHECK(k->at_start[s].pred==pred);CHECK(k->at_start[s].idx==idx);s++; }
            ima_decode(data,pos,&pred,&idx);
        }
        for(unsigned sl=0;sl<counts[ni];sl++) {
            unsigned bi=sl*(32/counts[ni]);
            CHECK(k->start[bi]==(uint64_t)l.nsamples*sl/counts[ni]);
            breaks_slice_on(&b,(int)sl);render(&b,2);
            CHECK(b.v[b.cur].lo==k->start[bi]);CHECK(b.v[b.cur].hi==k->start[bi+32/counts[ni]]);
            breaks_slice_hold(&b,(int)sl,0);render(&b,2);
        }
        events=0;breaks_set(&b,BRK_COMPLEX,0);breaks_set(&b,BRK_ALEN,2);
        for(int q=0;q<8;q++) {breaks_step(&b,q*2,0,64,1); CHECK(b.st_slice==q*(int)counts[ni]/8);render(&b,128);}
        CHECK(events==8);
        for(int sl=0;sl<32;sl++) { breaks_pin(&b,sl);breaks_step(&b,0,0,64,1);CHECK(last_slice==sl%(int)counts[ni]); }
        breaks_set_quiet(&b,1);unsigned before=events;breaks_step(&b,2,0,64,1);render(&b,128);CHECK(events==before);
        breaks_set_quiet(&b,0);breaks_step(&b,4,0,64,0);before=events;render(&b,128);CHECK(events==before);
        breaks_stop(&b);breaks_slice_on(&b,0);before=events;render(&b,128);CHECK(events==before);
        // A-only holds cannot fall through into B; changing count waits for release.
        l.slices=counts[(ni+1)%4];breaks_set_loop(&b,0,&l);CHECK(b.pend_on[0]);CHECK(breaks_count(&b,0)==(int)counts[ni]);
        breaks_slice_hold(&b,0,0);render(&b,128);breaks_flush(&b);CHECK(breaks_count(&b,0)==l.slices);
        // Both snapshot voices keep their exact metadata after count publication.
        breaks_step(&b,0,0,64,1);
        const brk_bank_t *old=b.v[b.cur].snap;unsigned old_count=old->slices;
        l.slices=counts[(ni+2)%4];breaks_set_loop(&b,0,&l);CHECK(old->slices==old_count);
        render(&b,256);breaks_flush(&b);
        // Macro bits are independent of highest slice bit.
        breaks_slice_on(&b,31);breaks_live(&b,8,1);breaks_live(&b,9,1);breaks_live(&b,10,1);render(&b,2);
        CHECK(b.perf.reverse && b.perf.jungle && b.perf.stutter);CHECK(b.slices==(1u<<31));
        breaks_slice_hold(&b,31,0);render(&b,2);CHECK(b.perf.reverse && b.perf.jungle && b.perf.stutter);
    }
    {breaks_t b;breaks_init(&b);b.accepted=accepted;brk_loop_t a={data,8193,22050,1,4},z={other,8193,22050,1,32};
     breaks_set_loop(&b,0,&a);breaks_set_loop(&b,1,&z);breaks_set(&b,BRK_COMPLEX,100);breaks_set(&b,BRK_ROLL,100);
     b.engine_bank=1;b.running=1;b.step_on=1;
     for(int i=0;i<1024;i++){fire_trigger(&b,i&7,0);CHECK(b.st_slice>=0&&b.st_slice<32);}
     breaks_set_loop(&b,1,0);for(int i=0;i<1024;i++){fire_trigger(&b,i&7,0);CHECK(b.st_slice>=0&&b.st_slice<4);CHECK(last_bank==0);}
     breaks_set_loop(&b,0,0);events=0;fire_trigger(&b,0,0);CHECK(events==0);
    }
    printf("adaptive DSP: %u checks, %u failures\n",checks,fails);return fails?1:0;
}
