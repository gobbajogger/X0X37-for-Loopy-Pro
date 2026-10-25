/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include "dsp/synth_engine.h"
int main(void)
{
    bass303_t voice;
    synth_slot_t slot;
    synth_ops_t unknown = synth303_ops;
    synth_binding_t b = { &unknown, &voice };
    assert(synth_init(&slot,synth303_binding(&voice)));
    unknown.id=0xdeadbeefu;
    assert(!synth_request(&slot,b,0));
    unknown.id=SYNTH_ENGINE_303; unknown.schema=2;
    assert(!synth_request(&slot,b,0));
    unknown.schema=1; unknown.note=0;
    assert(!synth_request(&slot,b,0));
    assert(slot.requested==slot.acknowledged && !synth_boundary(&slot));
    puts("PASS firmware registry rejects unknown engine, unknown schema and incomplete backend without switching");
    return 0;
}
