/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define X0X_SYNTH_HOST_TEST 1
#include "dsp/synth_engine.h"

typedef struct { int gate, value, panics, running, lite; } probe_t;
static void init(void *p) { memset(p, 0, sizeof(probe_t)); }
static void render(void *p, float *o, int n) { while(n--) *o++ = (float)((probe_t*)p)->value; }
static void note(void *p,int n,int a,int sl,int v) { (void)n;(void)a;(void)sl;(void)v; ((probe_t*)p)->gate=1; }
static void off(void *p) { ((probe_t*)p)->gate=0; }
static void panic(void *p) { off(p); ((probe_t*)p)->panics++; }
static void set(void *p,int i,int v) { (void)i; ((probe_t*)p)->value=v; }
static void transport(void *p,int run) { ((probe_t*)p)->running=run; }
static int active(const void *p) { return ((const probe_t*)p)->gate; }
static int idle(const void *p) { return !active(p); }
static void lite(void *p,int v) { ((probe_t*)p)->lite=v; }
static const synth_ops_t probe_ops={0x7ffffffe,7,1,init,render,note,off,panic,set,transport,active,idle,lite};
int main(void)
{
    bass303_t direct[2], adapted[2]; synth_slot_t slots[2];
    float a[256], b[256]; unsigned long samples=0;
    for(int part=0;part<2;part++) {
        bass303_init(&direct[part]);
        assert(synth_init(&slots[part],synth303_binding(&adapted[part])));
        /* Every pot at every legal value, two independent voices, both modes,
         * changing block lengths, release tails, accent/slide/velocity. */
        for(int param=0;param<BASS303_NPARAMS;param++)
            for(int value=0;value<128;value++) {
                int n=1+(value*37+param*13)%256;
                bass303_set(&direct[part],param,value);
                assert(synth_set(&slots[part],synth303_key((unsigned)param),value));
                if(value%11==0) {
                    int note_no=24+(value+param+part)%60;
                    bass303_note_on_velocity(&direct[part],note_no,value%2,param%2,1+value%127);
                    synth_note(&slots[part],note_no,value%2,param%2,1+value%127);
                }
                if(value%17==0) { bass303_note_off(&direct[part]); synth_off(&slots[part]); }
                bass303_set_lite(&direct[part],value%2); synth_lite(&slots[part],value%2);
                bass303_render(&direct[part],a,n); synth_render(&slots[part],b,n);
                assert(memcmp(a,b,(size_t)n*sizeof(float))==0); samples+=(unsigned)n;
                assert(memcmp(&direct[part],&adapted[part],sizeof(bass303_t))==0);
            }
    }
    probe_t probe; init(&probe); probe.value=73;
    synth_binding_t alternate={&probe_ops,&probe};
    uint8_t ticket;
    synth_note(&slots[0],60,1,0,100);
    assert(synth_request(&slots[0],alternate,&ticket));
    assert(slots[0].acknowledged!=ticket && slots[0].active.ops->id==SYNTH_ENGINE_303);
    assert(!synth_request(&slots[0],alternate,0));
    assert(synth_boundary(&slots[0]));
    assert(slots[0].acknowledged==ticket && !adapted[0].gate && !probe.gate);
    assert(!synth_set(&slots[0],synth303_key(0),12) && probe.value==73);
    synth_param_key_t key={probe_ops.id,probe_ops.schema,0};
    assert(synth_set(&slots[0],key,42));
    key.schema++; assert(!synth_set(&slots[0],key,8));
    key.schema--; key.param=1; assert(!synth_set(&slots[0],key,8));
    synth_note(&slots[0],62,0,0,90); synth_transport(&slots[0],0);
    assert(!probe.gate && !probe.running && probe.value==42);
    synth_transport(&slots[0],1);
    assert(synth_request(&slots[0],synth303_binding(&adapted[0]),&ticket));
    assert(synth_boundary(&slots[0]) && !probe.gate);
    assert(adapted[0].pot[0]==direct[0].pot[0]);
    for(int i=0;i<600;i++) { /* includes ticket rollover */
        assert(synth_request(&slots[0],alternate,&ticket));
        assert(synth_boundary(&slots[0]) && slots[0].acknowledged==ticket);
        assert(!synth_boundary(&slots[0]));
    }
    printf("PASS synth engine: %lu bit-exact303 samples; switching, note cleanup, identity, retained settings, rollover; slot=%zu ops=%zu\n",samples,sizeof(synth_slot_t),sizeof(synth_ops_t));
    return 0;
}
