/* SPDX-License-Identifier: GPL-3.0-only */
/* Same production engine translation unit, test-only ownership readback. */
#include "../../firmware/src/app/engine.c"
unsigned poly_chord_sequence_keys(int v){unsigned n=0;for(int k=0;k<128;k++)n+=!!original_bass[v].keys[ORIGINAL_INPUT_SEQUENCE][k];return n;}
unsigned poly_chord_sequence_key(int v,int k){return original_bass[v].keys[ORIGINAL_INPUT_SEQUENCE][k];}
