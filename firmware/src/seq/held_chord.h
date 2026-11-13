/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include "motion.h"
/* Sparse chord notes live in the existing project lane pool. The primary note,
 * velocity and gate remain in bstep_t. No pattern/song layout changes. */
#define HELD_CHORD_TARGET 0xfcu
#define HELD_CHORD_NOTES 6
static inline int held_chord_lane_valid(const lane_t *l)
{
    if(l->used!=1 || l->t!=HELD_CHORD_TARGET || l->pat>=NPAT ||
       l->v>=NBASS || l->part!=NKIT+l->v || l->i>=HELD_CHORD_NOTES-1 || l->rsv) return 0;
    for(int s=0;s<NSTEPS;s++) if(l->val[s]!=MOT_NONE && l->val[s]>127)return 0;
    return 1;
}
static inline int held_chord_find(const lane_t *L,int pat,int b,int i)
{
    for(int k=0;k<NLANE;k++)if(L[k].used && L[k].t==HELD_CHORD_TARGET &&
        L[k].pat==pat && L[k].part==NKIT+b && L[k].v==b && L[k].i==i)return k;
    return -1;
}
static inline int held_chord_get(const lane_t *L,int pat,int b,int step,int root,uint8_t *out)
{
    if((unsigned)pat>=NPAT || (unsigned)b>=NBASS || (unsigned)step>=NSTEPS || (unsigned)root>127)return 0;
    out[0]=(uint8_t)root;int n=1;
    for(int i=0;i<HELD_CHORD_NOTES-1;i++) {
        int k=held_chord_find(L,pat,b,i);
        if(k<0 || L[k].val[step]==MOT_NONE)break;
        int note=L[k].val[step];
        if(note>127 || note<=out[n-1])break; /* corrupt/duplicate cells never create extra keys */
        out[n++]=(uint8_t)note;
    }
    return n;
}
/* Negative count checks capacity only. Success publication is audio-owned;
 * callers must have admitted the sound lock before the positive write. */
static inline int held_chord_write(lane_t *L,int pat,int b,int step,const uint8_t *notes,int count,int reserve)
{
    int check=count<0,n=check?-count:count,k[HELD_CHORD_NOTES-1],need=0,free=0;
    if((unsigned)pat>=NPAT || (unsigned)b>=NBASS || (unsigned)step>=NSTEPS || n>HELD_CHORD_NOTES)return 0;
    for(int i=0;i<n;i++)if(!notes || notes[i]>127 || (i && notes[i]<=notes[i-1]))return 0;
    for(int i=0;i<HELD_CHORD_NOTES-1;i++) {
        k[i]=held_chord_find(L,pat,b,i);
        if(i<n-1 && k[i]<0)need++;
    }
    for(int q=0;q<NLANE;q++)if(!L[q].used)free++;
    if(need+reserve>free)return 0;
    if(check)return 1;
    for(int i=0;i<HELD_CHORD_NOTES-1;i++) {
        if(k[i]<0 && i<n-1) {
            for(int q=0;q<NLANE;q++)if(!L[q].used){k[i]=q;break;}
            lane_t *l=&L[k[i]];l->gen++;l->pat=(uint8_t)pat;l->part=(uint8_t)(NKIT+b);
            l->t=HELD_CHORD_TARGET;l->v=(uint8_t)b;l->i=(uint8_t)i;l->rsv=0;
            for(int s=0;s<NSTEPS;s++)l->val[s]=MOT_NONE;
            __asm__ volatile("" ::: "memory");l->used=1;
        }
        if(k[i]>=0) {
            lane_t *l=&L[k[i]];l->val[step]=i<n-1?notes[i+1]:MOT_NONE;
            if(i>=n-1) {
                int any=0;for(int st=0;st<NSTEPS;st++)any|=l->val[st]!=MOT_NONE;
                if(!any)l->used=0;
            }
        }
    }
    return 1;
}

static inline int held_chord_set(lane_t *L,int pat,int b,int step,const uint8_t *notes,int count)
{ return held_chord_write(L,pat,b,step,notes,count,0); }
