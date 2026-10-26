#include "dsp/wobble_backend.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Independent direct-DSP oracle: wrapper state must evolve exactly the same
 * across changing block sizes, channels, note boundaries and bank changes. */
int main(void) {
    wobble_t actual, direct;
    bass303_t old;
    synth_slot_t slot;
    uint8_t ticket;
    float left[258], right[258], oracle_l[256], oracle_r[256];
    unsigned samples=0;
    assert(sizeof(wobble_t)==144);
    assert(synth_init(&slot,synth303_binding(&old)));
    wobble_foundation_init(&actual);
    assert(synth_request(&slot,wobble_foundation_binding(&actual),&ticket));
    assert(synth_boundary(&slot));
    assert(slot.acknowledged==ticket);
    for (unsigned preset=0; preset<6; ++preset) {
        uint8_t bank[16], saved[16];
        for (unsigned j=0;j<16;++j) bank[j]=(uint8_t)((preset*31+j*17)%128);
        assert(wobble_bank_load(&actual,WOBBLE_EXPERIMENT_ENGINE,1,bank,16));
        wobble_bank_save(&actual,saved);
        assert(!memcmp(bank,saved,16));
        synth_note(&slot,36+(int)preset*7,1,1,35+(int)preset*17);
        direct=actual;
        for (int n=1;n<=256;++n) {
            left[0]=right[0]=left[n+1]=right[n+1]=123.0f;
            assert(wobble_backend_stereo(&slot,left+1,right+1,n));
            wobble_render(&direct,oracle_l,oracle_r,(size_t)n);
            assert(!memcmp(left+1,oracle_l,(size_t)n*sizeof(float)));
            assert(!memcmp(right+1,oracle_r,(size_t)n*sizeof(float)));
            assert(left[0]==123 && right[0]==123 && left[n+1]==123 && right[n+1]==123);
            samples+=(unsigned)n;
        }
        direct=actual;
        for (int n=256;n>=1;--n) {
            left[0]=left[n+1]=123.0f;
            synth_render(&slot,left+1,n);
            wobble_render(&direct,oracle_l,oracle_r,(size_t)n);
            for(int j=0;j<n;++j) assert(left[j+1]==.5f*(oracle_l[j]+oracle_r[j]));
            assert(left[0]==123 && left[n+1]==123);
            samples+=(unsigned)n;
        }
        synth_transport(&slot,0);
        assert(synth_idle(&slot));
        wobble_bank_save(&actual,saved); assert(!memcmp(bank,saved,16));
        assert(wobble_backend_stereo(&slot,left,right,256));
        for(int j=0;j<256;++j) assert(left[j]==0 && right[j]==0);
    }
    assert(synth_request(&slot,synth303_binding(&old),0));
    assert(synth_boundary(&slot));
    assert(!wobble_backend_stereo(&slot,left,right,256));
    printf("PASS independent direct stereo + mono oracle: %u samples, 6 banks, fragments 1..256, canaries, transport, back-to303\n",samples);
}
