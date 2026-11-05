/* SPDX-License-Identifier: GPL-3.0-only */
#include "chicago_fx2_compact.h"
#include <string.h>

#define COMPACT_READY UINT32_C(0x43464331)

int original_chicago_fx2_compact_supports(unsigned type)
{
    return type == 0 || type == 4 || type == 5 || type == 10 ||
        type == 18 || type == 22 || type == 24;
}

static int live(const original_chicago_fx2_compact *s)
{
    return s && s->ready == COMPACT_READY;
}

static void queue_to_selected(original_chicago_fx2_compact *s)
{
    const original_mod_queue *q = &s->queue;
    original_chicago_dyn_queue d = {q->events,q->count,q->cursor};
    switch (s->selected_type) {
    case 4: s->state.chorus.queue = *q; break;
    case 5: s->state.phaser.queue = *q; break;
    case 10: s->state.ring.queue = *q; break;
    case 18: s->state.mastercomp.queue = d; break;
    case 22: s->state.decimator.queue = d; break;
    case 24: s->state.peaking.queue = d; break;
    default: break;
    }
}

static void queue_from_selected(original_chicago_fx2_compact *s)
{
    switch (s->selected_type) {
    case 4: s->queue.cursor = s->state.chorus.queue.cursor; break;
    case 5: s->queue.cursor = s->state.phaser.queue.cursor; break;
    case 10: s->queue.cursor = s->state.ring.queue.cursor; break;
    case 18: s->queue.cursor = s->state.mastercomp.queue.cursor; break;
    case 22: s->queue.cursor = s->state.decimator.queue.cursor; break;
    case 24: s->queue.cursor = s->state.peaking.queue.cursor; break;
    default: break;
    }
}

static void parameter(void *p,unsigned id,unsigned value)
{
    original_chicago_fx2_compact *s = p;
    switch (s->selected_type) {
    case 0: (void)original_chicago_bypass_parameter(id,(int32_t)value); break;
    case 4: (void)original_chicago_chorus_set(&s->state.chorus,id,value); break;
    case 5: (void)original_chicago_phaser_set(&s->state.phaser,id,value); break;
    case 10: (void)original_chicago_ring_set(&s->state.ring,id,value); break;
    case 18: (void)original_chicago_mastercomp_set(&s->state.mastercomp,id-2,(int32_t)value); break;
    case 22: (void)original_chicago_decimator_set(&s->state.decimator,id-2,(int32_t)value); break;
    case 24: (void)original_chicago_peaking_set(&s->state.peaking,id-2,value); break;
    default: break;
    }
}

static void reset(void *p)
{
    original_chicago_fx2_compact *s = p;
    switch (s->selected_type) {
    case 4: original_chicago_chorus_reset(&s->state.chorus); break;
    case 5: original_chicago_phaser_reset(&s->state.phaser); break;
    case 10: original_chicago_ring_reset(&s->state.ring); break;
    case 18: original_chicago_mastercomp_reset(&s->state.mastercomp); break;
    case 22: original_chicago_decimator_reset(&s->state.decimator); break;
    case 24: original_chicago_peaking_reset(&s->state.peaking); break;
    default: break;
    }
}

static void process(void *p,int16_t *left,int16_t *right,uint32_t n)
{
    original_chicago_fx2_compact *s = p;
    switch (s->selected_type) {
    case 0: (void)original_chicago_bypass_process(left,right,n); break;
    case 4: (void)original_chicago_chorus_process(&s->state.chorus,left,right,n); break;
    case 5: (void)original_chicago_phaser_process(&s->state.phaser,left,right,n); break;
    case 10: (void)original_chicago_ring_process(&s->state.ring,left,right,n); break;
    case 18: (void)original_chicago_mastercomp_process(&s->state.mastercomp,left,right,n); break;
    case 22: (void)original_chicago_decimator_process(&s->state.decimator,left,right,n); break;
    case 24: (void)original_chicago_peaking_process(&s->state.peaking,left,right,n); break;
    default: break;
    }
    queue_from_selected(s);
}

static void begin(void *p) { (void)p; }
static void tempo(void *p,int32_t value) { (void)p; (void)value; }

static int advance_zero(void *p,uint32_t n,int16_t constant[2])
{
    original_chicago_fx2_compact *s=p;
    if(!live(s)||!s->selected||!constant)return 0;
    int16_t value[2]={0,0};
    int ok=0;
    switch(s->selected_type){
    case 0: ok=1; break;
    case 4: ok=original_chicago_chorus_advance_zero(&s->state.chorus,n); break;
    case 5: ok=original_chicago_phaser_advance_zero(&s->state.phaser,n); break;
    case 10: ok=original_chicago_ring_advance_zero(&s->state.ring,n); break;
    case 18: ok=original_chicago_mastercomp_advance_zero(&s->state.mastercomp,n); break;
    case 22: ok=original_chicago_decimator_advance_zero(&s->state.decimator,n); break;
    case 24: ok=original_chicago_peaking_advance_zero(&s->state.peaking,n,value); break;
    default: break;
    }
    if(ok){queue_from_selected(s);constant[0]=value[0];constant[1]=value[1];}
    return ok;
}

static void retire(void *p)
{
    original_chicago_fx2_compact *s = p;
    s->selected = 0;
}

static void activity(void *p,unsigned value)
{
    original_chicago_fx2_compact *s = p;
    switch (s->selected_type) {
    case 4: s->state.chorus.activity = (uint8_t)value; break;
    case 5: s->state.phaser.activity = (uint8_t)value; break;
    case 10: s->state.ring.activity = (uint8_t)value; break;
    case 18: original_chicago_mastercomp_activity(&s->state.mastercomp,(uint8_t)value); break;
    case 24: original_chicago_peaking_activity(&s->state.peaking,(uint8_t)value); break;
    default: break;
    }
}

static int supports(void *p,unsigned type)
{
    return live(p) && original_chicago_fx2_compact_supports(type);
}

static const original_chicago_fx2_kernel *select_cold(void *p,unsigned type)
{
    original_chicago_fx2_compact *s = p;
    if (!supports(s,type)) return NULL;
    if (s->selected) s->kernel.retire(s->kernel.context);
    /* Constructor state matters: Master reset retains its highpass history.
     * A reset of reused union bytes alone is not a valid new object. */
    memset(&s->state,0,sizeof s->state);
    switch (type) {
    case 4: (void)original_chicago_chorus_init(&s->state.chorus,s->history,s->history+1024); break;
    case 5: original_chicago_phaser_init(&s->state.phaser); break;
    case 10: original_chicago_ring_init(&s->state.ring); break;
    case 18: (void)original_chicago_mastercomp_init(&s->state.mastercomp); break;
    case 22: (void)original_chicago_decimator_init(&s->state.decimator); break;
    case 24: (void)original_chicago_peaking_init(&s->state.peaking); break;
    default: break;
    }
    s->selected_type = (uint8_t)type;
    s->selected = 1;
    queue_to_selected(s);
    return &s->kernel;
}

static void discard(void *p)
{
    original_chicago_fx2_compact *s = p;
    if (!live(s)) return;
    if (s->selected) s->kernel.retire(s->kernel.context);
    memset(&s->state,0,sizeof s->state);
    memset(&s->queue,0,sizeof s->queue);
    s->selected_type = 0;
}

int original_chicago_fx2_compact_init(original_chicago_fx2_compact *s)
{
    if (!s) return 0;
    memset(s,0,sizeof *s);
    s->kernel = (original_chicago_fx2_kernel){s,process,parameter,reset,begin,retire,tempo,activity};
    s->ready = COMPACT_READY;
    return 1;
}

original_chicago_fx2_cold_owner original_chicago_fx2_compact_owner(original_chicago_fx2_compact *s)
{
    original_chicago_fx2_cold_owner owner = {s,supports,select_cold,discard,advance_zero};
    return owner;
}

int original_chicago_fx2_compact_events(original_chicago_fx2_compact *s,
    const original_chicago_drive_event *events,uint32_t n)
{
    if (!live(s) || (n && !events)) return 0;
    for (uint32_t i = 1; i < n; ++i)
        if (events[i-1].offset > events[i].offset) return 0;
    s->queue = (original_mod_queue){events,n,0};
    if (s->selected) queue_to_selected(s);
    return 1;
}

void original_chicago_fx2_compact_direct_event(original_chicago_fx2_compact *s,unsigned type)
{
    if (!live(s) || !s->selected) return;
    switch (s->selected_type) {
    case 4: original_chicago_chorus_direct_event(&s->state.chorus,type); break;
    case 5: original_chicago_phaser_direct_event(&s->state.phaser,type); break;
    case 10: original_chicago_ring_direct_event(&s->state.ring,type); break;
    case 18: original_chicago_mastercomp_direct_event(&s->state.mastercomp,type); break;
    case 22: original_chicago_decimator_direct_event(&s->state.decimator,type); break;
    case 24: original_chicago_peaking_direct_event(&s->state.peaking,type); break;
    default: break;
    }
}
