/* SPDX-License-Identifier: GPL-3.0-only */
#include "miami_zero.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"zero oracle line %d\n",__LINE__);exit(1); } } while (0)
int main(void){
 uint32_t words[264],want_words[264];
 uint16_t halves[264],want_halves[264];
 float floats[264],want_floats[264];
 unsigned checks=0;
 for(unsigned offset=0;offset<4;offset++)for(unsigned n=0;n<=256;n++){
  memset(words,0xa5,sizeof words);memcpy(want_words,words,sizeof words);
  memset(want_words+offset,0,n*sizeof *words);
  original_miami_zero_words(words+offset,n);
  CHECK(!memcmp(words,want_words,sizeof words));++checks;
  memset(halves,0xa5,sizeof halves);memcpy(want_halves,halves,sizeof halves);
  memset(want_halves+offset,0,n*sizeof *halves);
  original_miami_zero_halves(halves+offset,n);
  CHECK(!memcmp(halves,want_halves,sizeof halves));++checks;
  memset(floats,0xa5,sizeof floats);memcpy(want_floats,floats,sizeof floats);
  memset(want_floats+offset,0,n*sizeof *floats);
  original_miami_zero_floats(floats+offset,n);
  CHECK(!memcmp(floats,want_floats,sizeof floats));++checks;
 }
 printf("PASS Miami zero buffers: %u byte/canary oracle comparisons\n",checks);
 return 0;
}
