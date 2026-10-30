/* SPDX-License-Identifier: GPL-3.0-only */
#include "state_codec_private.h"
#include <string.h>
static uint32_t get(const unsigned char *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static void put(unsigned char *p,uint32_t u){p[0]=(unsigned char)u;p[1]=(unsigned char)(u>>8);p[2]=(unsigned char)(u>>16);p[3]=(unsigned char)(u>>24);}
static uint32_t checksum(const unsigned char *p,size_t n){uint32_t h=2166136261u;for(size_t i=0;i<n;i++)h=(h^p[i])*16777619u;return h;}
static int schema_valid(const original_state_schema *s){
 if(!s||!s->count||s->count>UINT32_MAX/4u||!s->size)return 0;
 if(!s->fields)return s->size==4u*s->count&&s->size<=8192u;
 for(uint32_t i=0;i<s->count;i++){uint16_t d=s->fields[i];unsigned kind=d&7u,width=kind==ORIGINAL_STATE_B?1u:4u;uint32_t offset=d>>3;if(kind>ORIGINAL_STATE_N||offset>s->size||width>s->size-offset)return 0;}
 return 1;
}
static uint16_t descriptor(const original_state_schema *s,uint32_t i){return s->fields?s->fields[i]:(uint16_t)((i<<5)|(i+1u==s->count?ORIGINAL_STATE_U:ORIGINAL_STATE_N));}
static uint32_t field(const void *object,uint16_t d){const unsigned char *p=(const unsigned char *)object+(d>>3);if((d&7u)==ORIGINAL_STATE_B)return *p;uint32_t u;memcpy(&u,p,4);return u;}
static int valid_word(uint32_t u,unsigned kind){
 if(kind==ORIGINAL_STATE_F)return (u&0x7f800000u)!=0x7f800000u;
 if(kind==ORIGINAL_STATE_B)return u<=255u;
 if(kind==ORIGINAL_STATE_N){uint32_t magnitude=u&0x7fffffffu;return magnitude<=0x3f800000u&&(!(u&0x80000000u)||!magnitude);}
 return 1;
}
int original_state_encode(const original_state_schema *s,const void *object,void *bytes,size_t capacity){
 if(!object||!bytes||!schema_valid(s)||s->count>(UINT32_MAX-16u)/4u||capacity<16u+(size_t)s->count*4u)return 0;
 for(uint32_t i=0;i<s->count;i++){uint16_t d=descriptor(s,i);if(!valid_word(field(object,d),d&7u))return 0;}
 unsigned char *p=bytes;memcpy(p,s->magic,4);p[4]=1;p[5]=s->backend;p[6]=p[7]=0;put(p+8,4u*s->count);
 for(uint32_t i=0;i<s->count;i++)put(p+16+4u*i,field(object,descriptor(s,i)));
 put(p+12,checksum(p+16,4u*s->count));return 1;
}
int original_state_decode(const original_state_schema *s,void *staging,const void *bytes,size_t length){
 if(!staging||!bytes||!schema_valid(s)||s->count>(UINT32_MAX-16u)/4u||length!=16u+(size_t)s->count*4u)return 0;
 const unsigned char *p=bytes;if(memcmp(p,s->magic,4)||p[4]!=1||p[5]!=s->backend||p[6]||p[7]||get(p+8)!=4u*s->count||get(p+12)!=checksum(p+16,4u*s->count))return 0;
 for(uint32_t i=0;i<s->count;i++)if(!valid_word(get(p+16+4u*i),descriptor(s,i)&7u))return 0;
 memset(staging,0,s->size);
 for(uint32_t i=0;i<s->count;i++){
  uint16_t d=descriptor(s,i);uint32_t u=get(p+16+4u*i);unsigned char *dest=(unsigned char *)staging+(d>>3);
  if((d&7u)==ORIGINAL_STATE_B)*dest=(unsigned char)u;else memcpy(dest,&u,4);
 }
 return 1;
}
