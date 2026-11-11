/* SPDX-License-Identifier: GPL-3.0-only */
#include "lossless_table.h"
int original_lossless_lookup(const original_lossless_table *t,uint32_t index,uint32_t *out){
 if(!t||!out||!t->data||!t->offset||!t->count||index>=t->count||t->blocks!=(t->count/32u+(t->count%32u!=0)))return 0;
 uint32_t block=index/32u,entry=index%32u;size_t pos=t->offset[block];
 if(pos>t->bytes||t->bytes-pos<5)return 0;
 const uint8_t *p=t->data+pos;unsigned width=p[4];if(width>32)return 0;
 uint32_t remaining=t->count-block*32u,entries=remaining>32?32:remaining;
 size_t payload=((size_t)(entries-1)*width+7u)/8u;if(t->bytes-pos-5<payload)return 0;
 if(block+1<t->blocks&&t->offset[block+1]!=pos+5+payload)return 0;
 if(block+1==t->blocks&&pos+5+payload!=t->bytes)return 0;
 uint32_t value=(uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
 size_t bit=0;p+=5;
 for(unsigned n=0;n<entry;n++){
  uint32_t z=0;unsigned used=0;
  while(used<width){unsigned shift=(unsigned)(bit%8u),take=8-shift;if(take>width-used)take=width-used;
   z|=((uint32_t)(p[bit/8u]>>shift)&((1u<<take)-1u))<<used;used+=take;bit+=take;
  }
  value+=(z>>1)^(0u-(z&1u));
 }
 *out=value;return 1;
}

static int second_decode(const uint8_t *p,size_t bytes,uint32_t entries,uint32_t entry,uint32_t *out){
 if(bytes<9)return 0;
 unsigned width=p[8];if(width>32)return 0;
 size_t residuals=entries>1?entries-2:0,payload=(residuals*width+7u)/8u;
 if(bytes!=9+payload)return 0;
 uint32_t value=(uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
 uint32_t delta=(uint32_t)p[4]|(uint32_t)p[5]<<8|(uint32_t)p[6]<<16|(uint32_t)p[7]<<24;
 if(entry)value+=delta;
 size_t bit=0;p+=9;
 /* At widths up to 24, refilling one byte while fewer than width bits
  * remain never exceeds 31 live bits. Consume each payload byte once, with
  * no speculative read past the validated payload and no 64-bit shifts.
  * Wider generic blocks retain the original fragment decoder below. */
 if(width<=24){
  uint32_t reservoir=0,mask=(1u<<width)-1u;unsigned available=0;
  for(unsigned n=1;n<entry;n++){
   while(available<width){reservoir|=(uint32_t)*p++<<available;available+=8;}
   uint32_t z=reservoir&mask;reservoir>>=width;available-=width;
   delta+=(z>>1)^(0u-(z&1u));value+=delta;
  }
  *out=value;return 1;
 }
 for(unsigned n=1;n<entry;n++){
  uint32_t z=0;unsigned used=0;
  while(used<width){unsigned shift=(unsigned)(bit%8u),take=8-shift;if(take>width-used)take=width-used;
   z|=((uint32_t)(p[bit/8u]>>shift)&((1u<<take)-1u))<<used;used+=take;bit+=take;
  }
  delta+=(z>>1)^(0u-(z&1u));value+=delta;
 }
 *out=value;return 1;
}

int original_lossless_second_lookup(const original_lossless_table *t,uint32_t index,uint32_t *out){
 if(!t||!out||!t->data||!t->offset||!t->count||index>=t->count||t->blocks!=(t->count/32u+(t->count%32u!=0))||t->offset[0]!=0)return 0;
 uint32_t block=index/32u,remaining=t->count-block*32u;
 size_t pos=t->offset[block],end=block+1<t->blocks?t->offset[block+1]:t->bytes;
 if(pos>end||end>t->bytes)return 0;
 return second_decode(t->data+pos,end-pos,remaining>32?32:remaining,index%32u,out);
}

int original_lossless_second16_lookup(const original_lossless_table16 *t,uint32_t index,uint32_t *out){
 if(!t||!out||!t->data||!t->offset||!t->count||index>=t->count||t->bytes>UINT16_MAX||t->blocks!=(t->count/32u+(t->count%32u!=0))||t->offset[0]!=0)return 0;
 uint32_t block=index/32u,remaining=t->count-block*32u;
 size_t pos=t->offset[block],end=block+1<t->blocks?t->offset[block+1]:t->bytes;
 if(pos>end||end>t->bytes)return 0;
 return second_decode(t->data+pos,end-pos,remaining>32?32:remaining,index%32u,out);
}
