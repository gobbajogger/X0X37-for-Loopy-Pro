/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../../firmware/src/seq/motion.c"
#include "../../firmware/src/app/undo.c"
static lane_t lanes[NLANE], before[NLANE], shadow[NLANE];
static unsigned applied;
static void apply(void *ctx,int t,int v,int i,int value)
{ (void)ctx;(void)t;(void)v;(void)i;(void)value;applied++; }
static int base(void *ctx,int t,int v,int i)
{ (void)ctx;(void)t;(void)v;(void)i;return 17; }
int main(void)
{
    uint8_t values[16], expected[16];
    unsigned j;
    int k, lock, copy;
    motion_t runtime;
    undo_t undo;
    uint32_t lo,hi;
    for(j=0;j<16;j++) values[j]=(uint8_t)(j*7u);
    memcpy(expected,values,16);
    assert(sizeof(lane_t)==72 && sizeof lanes==11520);
    lock=motion_alloc(lanes,0,2,2,0,3);
    lanes[lock].val[4]=77;
    k=motion_snapshot_capture(lanes,0,2,4,2,values);
    assert(k>=0 && motion_count(lanes)==2);
    values[0]=99;
    assert(!memcmp(motion_snapshot_read(lanes,0,2,4),expected,16));
    assert(motion_snapshot_engine(lanes,0,2,4)==2);
    assert(!strcmp(motion_snapshot_engine_label(2),"WBL1"));
    assert(!strcmp(motion_snapshot_engine_label(1),"303"));
    assert(!motion_snapshot_engine_label(0)[0] && !motion_snapshot_engine_label(3)[0]);
    /* Main replacement has a complete old bank until one selector publication. */
    assert(lanes[k].val[63]==0);
    assert(motion_snapshot_edit(lanes,0,2,4,5,101)==k);
    assert(lanes[k].val[63]==16 && lanes[k].val[5]==expected[5]);
    expected[5]=101;
    assert(!memcmp(motion_snapshot_read(lanes,0,2,4),expected,16));
    assert(motion_snapshot_engine(lanes,0,2,4)==2);
    /* Part-copy's existing all-val-byte path preserves both bank and identity. */
    copy=motion_alloc(lanes,1,2,lanes[k].t,lanes[k].v,lanes[k].i);
    memcpy(lanes[copy].val,lanes[k].val,NSTEPS);
    assert(!memcmp(motion_snapshot_read(lanes,1,2,4),expected,16));
    assert(motion_snapshot_engine(lanes,1,2,4)==2);
    assert(motion_snapshot_edit(lanes,0,2,4,5,11)==k);
    assert(motion_snapshot_read(lanes,1,2,4)[5]==101);
    /* Snapshot slots are opaque to ordinary motion even when stale runtime ramps exist. */
    motion_init(&runtime,lanes);runtime.base=base;runtime.apply=apply;
    runtime.rt[k].ramp=1;runtime.rt[k].gen=lanes[k].gen;
    motion_step(&runtime,2,0,4,5,16);motion_tick(&runtime,32);motion_release(&runtime);
    assert(runtime.rt[k].applied==MOT_NONE);
    assert(applied>0); /* separate knob lane still works */
    /* Snapshot removal leaves all independent locks byte-identical. */
    memcpy(before,lanes,sizeof lanes);
    motion_snapshot_remove(lanes,0,2,4);
    assert(!lanes[k].used && !memcmp(&lanes[lock],&before[lock],sizeof(lane_t)));
    /* Every malformed input and full-pool insertion rejects before mutation. */
    memcpy(before,lanes,sizeof lanes);
    assert(motion_snapshot_capture(lanes,-1,2,4,2,values)<0);
    assert(motion_snapshot_capture(lanes,0,1,4,2,values)<0);
    assert(motion_snapshot_capture(lanes,0,2,64,2,values)<0);
    assert(motion_snapshot_capture(lanes,0,2,4,3,values)<0);
    values[7]=128;
    assert(motion_snapshot_capture(lanes,0,2,4,2,values)<0);
    assert(!memcmp(before,lanes,sizeof lanes));values[7]=49;
    for(j=0;j<NLANE;j++) if(!lanes[j].used) assert(motion_alloc(lanes,2,2,2,0,1)>=0);
    memcpy(before,lanes,sizeof lanes);
    assert(motion_snapshot_capture(lanes,0,2,4,2,values)<0);
    assert(!memcmp(before,lanes,sizeof lanes));
    /* Existing snapshots can be replaced/edited without needing a free slot. */
    assert(motion_snapshot_edit(lanes,1,2,4,15,127)==copy);
    assert(motion_snapshot_capture(lanes,1,2,4,1,values)==copy);
    assert(motion_snapshot_engine(lanes,1,2,4)==1);
    /* Real byte-diff undo is complete across bank value/selector changes. */
    undo_init(&undo,lanes,shadow,sizeof lanes);
    memcpy(before,lanes,sizeof lanes);
    assert(motion_snapshot_edit(lanes,1,2,4,0,71)==copy);
    assert(undo_commit(&undo)==1);
    assert(undo_undo(&undo,0,&lo,&hi)==1);
    assert(!memcmp(lanes,before,sizeof lanes));
    assert(undo_redo(&undo,0,&lo,&hi)==1);
    assert(motion_snapshot_read(lanes,1,2,4)[0]==71);
    memcpy(before,lanes,sizeof lanes);
    assert(motion_snapshot_edit(lanes,1,2,4,16,1)<0);
    assert(motion_snapshot_edit(lanes,1,2,4,0,128)<0);
    assert(!memcmp(before,lanes,sizeof lanes));
    puts("snapshot storage: immutable16, identity, bank publication, independent locks, copy bytes, undo/redo, full/invalid rejection PASS");
    return 0;
}
