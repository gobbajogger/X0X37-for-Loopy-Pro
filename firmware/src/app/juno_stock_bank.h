/* SPDX-License-Identifier: GPL-3.0-only */
/* DnBOX native Jun60 factory sound data. Values are normalized q0..127,
 * not imported Roland/plug-in patches. Index is part of schema-2 storage:
 * never reorder or alter the first four entries. Keep additions append-only. */
#pragma once
#include <stdint.h>
static const uint8_t original_sound_juno_factories[][16] = {
    /* 0..3: existing shipping identities, byte-for-byte unchanged. */
    {42,28,70,18,45,8,40,92,40,0,15,8,64,25,45,100},   /* JUNO POLY  */
    {90,18,42,24,35,65,55,95,75,0,12,15,55,40,55,100},  /* JUNO PAD   */
    {15,50,58,36,80,0,35,30,20,5,20,0,64,25,18,110},   /* JUNO PLUCK */
    {70,10,105,12,20,0,20,110,15,12,25,5,80,60,50,96}, /* JUNO BRASS */
    /* 4..7: originals composed for the actual six-voice Juno DSP. */
    {94,85,42,9,20,0,24,104,21,0,6,0,64,20,0,90},       /* SUB BASS   */
    {45,14,59,18,26,4,54,68,44,5,15,6,63,29,30,95},     /* WARM KEYS  */
    {113,17,90,13,58,1,28,111,26,0,24,9,40,25,12,82},   /* PULSE LEAD */
    {32,20,67,15,35,43,65,99,70,2,19,20,71,37,49,88},   /* SLOW STR   */
};
static const char *const original_sound_juno_names[] = {
    "JUNO POLY", "JUNO PAD", "JUNO PLUCK", "JUNO BRASS",
    "SUB BASS", "WARM KEYS", "PULSE LEAD", "SLOW STR"
};
