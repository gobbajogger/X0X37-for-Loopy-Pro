/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <string.h>
#include "../../firmware/src/seq/held_chord.h"
static int called;
static int base(void *c,int t,int v,int i){(void)c;(void)t;(void)v;(void)i;called++;return 0;}
static void apply(void*c,int t,int v,int i,int value){(void)c;(void)t;(void)v;(void)i;(void)value;called++;}
int main(void){
 lane_t lanes[NLANE]={0};uint8_t n[]={0,60,64,67,71,127},out[6];
 assert(held_chord_set(lanes,0,0,4,n,-6));assert(motion_count(lanes)==0);
 assert(held_chord_set(lanes,0,0,4,n,6));assert(motion_count(lanes)==5);
 assert(held_chord_get(lanes,0,0,4,0,out)==6);assert(!memcmp(out,n,6));
 assert(held_chord_get(lanes,1,0,4,0,out)==1);
 for(int k=0;k<NLANE;k++)if(!lanes[k].used)lanes[k].used=1;
 lane_t saved[NLANE];memcpy(saved,lanes,sizeof lanes);
 assert(!held_chord_set(lanes,1,0,0,n,6));assert(!memcmp(saved,lanes,sizeof lanes));
 assert(held_chord_set(lanes,0,0,4,n,1));assert(held_chord_get(lanes,0,0,4,0,out)==1);
 assert(!held_chord_set(lanes,32,0,0,n,6));
 memset(lanes,0,sizeof lanes);for(int k=1;k<NLANE;k++)lanes[k].used=1;
 assert(held_chord_set(lanes,0,0,0,n,-2));
 assert(!held_chord_write(lanes,0,0,0,n,-2,1)); /* last lane also needed by NOTE LOCK */
 memset(lanes,0,sizeof lanes);assert(held_chord_set(lanes,0,0,0,n,2));
 motion_t m;motion_init(&m,lanes);m.base=base;m.apply=apply;
 motion_step(&m,NKIT,0,0,1,0);motion_tick(&m,64);motion_release(&m);motion_clear(&m,0);assert(!called);
 return 0;
}
