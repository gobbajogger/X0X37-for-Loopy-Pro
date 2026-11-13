/* SPDX-License-Identifier: GPL-3.0-only */
#include "../experiments/original_synth/chicago_packed_private.h"
int chicago_raw_bits(uint32_t i,uint32_t *a,uint32_t *e){
 return original_lossless_second16_lookup(&original_chicago_packed_a,i,a)&&original_lossless_second16_lookup(&original_chicago_packed_e,i,e);
}
