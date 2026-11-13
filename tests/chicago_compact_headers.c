/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <string.h>
#define X0X_CHICAGO_COMPACT_HEADERS 1
#include "../experiments/original_synth/chicago_packed.c"
extern int chicago_raw_bits(uint32_t index,uint32_t *a,uint32_t *e);
int main(void){
 for(uint32_t i=0;i<16385;i++){
  uint32_t ra,re,ca,ce; float a,e;
  assert(chicago_raw_bits(i,&ra,&re));
  assert(chicago_compact_lookup(&original_chicago_packed_a,i,original_chicago_packed_a_prefix,&ca));
  assert(chicago_compact_lookup(&original_chicago_packed_e,i,original_chicago_packed_e_prefix,&ce));
  assert(ra==ca && re==ce);
  assert(original_chicago_packed_lookup(0,i,&a,&e));
  assert(memcmp(&a,&ra,4)==0 && memcmp(&e,&re,4)==0);
  assert(original_chicago_packed_lookup(0,i,&a,&e)); /* cached */
 }
 uint32_t v; float a,e;
 assert(!original_chicago_packed_lookup(0,16385,&a,&e));
 assert(!original_chicago_packed_lookup(0,UINT32_MAX,&a,&e));
 assert(!original_chicago_packed_lookup(&v,0,&a,&e));
 assert(!original_chicago_packed_lookup(0,0,0,&e));
 assert(!original_chicago_packed_lookup(0,0,&a,0));
 assert(!original_chicago_packed_lookup(0,0,&a,&a));
 original_lossless_table16 t=original_chicago_packed_a;
 #define BAD() assert(!chicago_compact_lookup(&t,0,original_chicago_packed_a_prefix,&v))
 assert(!chicago_compact_lookup(0,0,0,&v));
 assert(!chicago_compact_lookup(&t,0,0,0));
 t.data=0;BAD();t=original_chicago_packed_a;
 t.offset=0;BAD();t=original_chicago_packed_a;
 t.count=0;BAD();t=original_chicago_packed_a;
 t.blocks--;BAD();t=original_chicago_packed_a;
 t.bytes=65536;BAD();t=original_chicago_packed_a;
 uint16_t offsets[513];memcpy(offsets,t.offset,sizeof offsets);t.offset=offsets;
 offsets[0]=1;BAD();offsets[0]=0;
 offsets[1]=6;BAD();offsets[1]=65535;BAD();
 uint8_t block[8]={0};
 assert(!chicago_compact_decode(block,6,1,0,0,&v));
 assert(!chicago_compact_decode(block,8,1,0,0,&v));
 return 0;
}
