/* SPDX-License-Identifier: GPL-3.0-only */
#include "foundation_adapter.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
int main(void)
{
    float out[256], left[256], right[256];
    wobble_t a, direct, prepared;
    synth_slot_t slot;
    assert(synth_init(&slot, wobble_foundation_binding(&a)));
    assert(a.sample_rate == 44100);
    synth_param_key_t key = {WOBBLE_EXPERIMENT_ENGINE, WOBBLE_EXPERIMENT_SCHEMA, W_STEREO};
    assert(synth_set(&slot,key,89));
    synth_param_key_t bad = synth303_key(W_CUTOFF);
    float cutoff=a.p[W_CUTOFF];
    assert(!synth_set(&slot,bad,0) && a.p[W_CUTOFF]==cutoff);
    bad=key;bad.schema=2;assert(!synth_set(&slot,bad,0));
    bad=key;bad.param=16;assert(!synth_set(&slot,bad,0));
    synth_note(&slot,48,1,1,80);
    direct=a;
    for(int n=1;n<=256;++n) {
        synth_render(&slot,out,n);
        wobble_render(&direct,left,right,(size_t)n);
        for(int i=0;i<n;++i) {
            assert(isfinite(out[i]));
            assert(out[i]==.5f*(left[i]+right[i]));
        }
    }
    synth_off(&slot);
    for(int n=0;n<8;++n)synth_render(&slot,out,256);
    assert(synth_idle(&slot));
    for(int i=0;i<256;++i)assert(out[i]==0);
    wobble_init(&prepared,44100);
    wobble_set(&prepared,W_CRUSH,.3f);
    synth_note(&slot,55,0,0,100);
    uint8_t ticket;
    assert(synth_request(&slot,wobble_foundation_binding(&prepared),&ticket));
    assert(!synth_request(&slot,wobble_foundation_binding(&a),0));
    assert(synth_boundary(&slot));
    assert(slot.acknowledged==ticket && !a.active && !prepared.active);
    assert(prepared.p[W_CRUSH]==.3f);
    synth_note(&slot,48,0,0,80);
    assert(synth_active(&slot));
    synth_transport(&slot,0);
    assert(synth_idle(&slot) && prepared.p[W_CRUSH]==.3f);
    printf("PASS PR212 exact interface:44100Hz, fragments1..256, mono downmix, identity/schema, boundary ACK, panic/settings; state=%zu scratch=%zu\n",sizeof(a),sizeof(right));
    return 0;
}
