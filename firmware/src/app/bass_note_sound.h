/* SPDX-License-Identifier: GPL-3.0-only */
/* Stream 7 note sound, version 1. One existing motion-pool record per onset.
 * No pointers, native padding, repurposed step flags or change to project size.
 * Payload: complete product cell32, wire identity12, target mixer5, zero15.
 * Global send processors/master remain project state, as in ordinary playback. */
#pragma once
#include "bass_identity.h"
#define BASS_NOTE_TARGET 0xfdu
#define BASS_NOTE_VERSION 1u
#define BASS_NOTE_CELL 0u
#define BASS_NOTE_TAG 32u
#define BASS_NOTE_MIX 44u
#define BASS_NOTE_USED 49u

static inline int bass_note_find(const lane_t *lanes,int pat,int voice,int step)
{
    for(int k=0;k<NLANE;k++)
        if(lanes[k].used && lanes[k].t==BASS_NOTE_TARGET && lanes[k].pat==pat &&
           lanes[k].part==PART_303A+voice && lanes[k].v==step)return k;
    return -1;
}
static inline void bass_note_clear(lane_t *lanes,int pat,int voice,int step)
{
    int k=bass_note_find(lanes,pat,voice,step);
    if(k>=0)lanes[k].used=0;
}
