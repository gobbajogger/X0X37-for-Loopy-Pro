/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include "../../firmware/src/app/original_sound.h"
#include "../../firmware/src/app/bass_pattern_preset.h"
int main(void) {
 uint8_t cell[32]; unsigned k,p;
 assert(original_sound_init(cell,3,0));
 assert(original_sound_count(3)==18);
 assert(original_sound_valid(cell));
 assert(bass_pattern_preset_encode(3,1)==0xE1);
 assert(bass_pattern_preset_decode(0xE1,&k,&p)&&k==3&&p==1);
 assert(bass_pattern_preset_decode(0xC1,&k,&p)&&k==2&&p==1);
 assert(bass_pattern_preset_decode(81,&k,&p)&&k==0&&p==81);
 assert(bass_pattern_preset_decode(158,&k,&p)&&k==1&&p==30);
 assert(!bass_pattern_preset_decode(0xFF,&k,&p));
 puts("juno slot identity and legacy recall PASS");
}
