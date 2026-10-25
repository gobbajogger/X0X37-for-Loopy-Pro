/* SPDX-License-Identifier: GPL-3.0-only */
/* Independent regression: adapter must reproduce the original303 byte-for-byte. */
#define X0X_SYNTH_HOST_TEST 1
#include "dsp/synth_engine.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct { int held, param, panics, transport, lite; } mock_t;
static void mi(void *p) { memset(p,0,sizeof(mock_t)); }
static void mr(void *p,float *o,int n) { while(n--) o[n]=(float)((mock_t*)p)->param; }
static void mn(void *p,int n,int a,int s,int v) { (void)n;(void)a;(void)s;(void)v; ((mock_t*)p)->held=1; }
static void mo(void *p) { ((mock_t*)p)->held=0; }
static void mp(void *p) { ((mock_t*)p)->held=0; ++((mock_t*)p)->panics; }
static void ms(void *p,int i,int v) { (void)i; ((mock_t*)p)->param=v; }
static void mt(void *p,int run) { ((mock_t*)p)->transport=run; }
static int ma(const void *p) { return ((const mock_t*)p)->held; }
static int mz(const void *p) { return !((const mock_t*)p)->held; }
static void ml(void *p,int lite) { ((mock_t*)p)->lite=lite; }
static const synth_ops_t mock_ops={0x7ffffffeu,2,1,mi,mr,mn,mo,mp,ms,mt,ma,mz,ml};

int main(void)
{
    bass303_t raw, adapted;
    synth_slot_t slot;
    float a[256],b[256];
    int nblock[]={1,7,16,63,128,256};
    bass303_init(&raw);
    assert(synth_init(&slot,synth303_binding(&adapted)));
    for(int p=0;p<BASS303_NPARAMS;p++)
        for(int v=0;v<128;v++) {
            bass303_set(&raw,p,v);
            assert(synth_set(&slot,synth303_key((unsigned)p),v));
            int note=(p*13+v)%128, accent=v&1, slide=(v>>1)&1;
            bass303_note_on_velocity(&raw,note,accent,slide,v);
            synth_note(&slot,note,accent,slide,v);
            for(unsigned k=0;k<sizeof nblock/sizeof nblock[0];k++) {
                int n=nblock[k];
                bass303_render(&raw,a,n); synth_render(&slot,b,n);
                assert(!memcmp(a,b,(size_t)n*sizeof(float)));
                assert(raw.gate==synth_active(&slot));
                assert(raw.idle==synth_idle(&slot));
            }
            bass303_note_off(&raw); synth_off(&slot);
            bass303_set_lite(&raw,v&1); synth_lite(&slot,v&1);
            bass303_render(&raw,a,256); synth_render(&slot,b,256);
            assert(!memcmp(a,b,sizeof a));
        }
    assert(!synth_set(&slot,(synth_param_key_t){SYNTH_ENGINE_303,2,0},100));
    assert(!synth_set(&slot,synth303_key(65535),100));
    mock_t mock;
    synth_binding_t alt={&mock_ops,&mock};
    mi(&mock); mock.param=42;
    synth_note(&slot,42,1,0,100);
    uint8_t ticket;
    assert(synth_request(&slot,alt,&ticket));
    assert(!synth_request(&slot,alt,0));
    assert(slot.active.ops->id==SYNTH_ENGINE_303);
    assert(slot.acknowledged!=ticket);
    assert(synth_boundary(&slot));
    assert(slot.acknowledged==ticket);
    assert(!adapted.gate && !mock.held && mock.param==42);
    assert(!synth_set(&slot,synth303_key(0),99));
    assert(mock.param==42);
    assert(synth_set(&slot,(synth_param_key_t){mock_ops.id,2,0},17));
    synth_transport(&slot,1); synth_lite(&slot,1);
    assert(mock.transport==1 && mock.lite==1);
    synth_note(&slot,42,0,0,99); assert(mock.held);
    synth_transport(&slot,0); assert(!mock.held && mock.transport==0);
    for(int i=0;i<600;i++) {
        assert(synth_request(&slot,(i&1)?alt:synth303_binding(&adapted),&ticket));
        assert(synth_boundary(&slot));
        assert(slot.acknowledged==ticket);
    }
    assert(mock.param==17);
    puts("PASS: original303/adapter exhaustive pot sweep, note/slide/velocity, variable blocks, lite transitions, identity rejection, boundary acknowledgement, cleanup, retained settings, ticket wrap");
    return 0;
}
