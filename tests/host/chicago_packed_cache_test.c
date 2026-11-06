/* SPDX-License-Identifier: GPL-3.0-only */
#include "chicago_packed.h"
#include "chicago_packed_private.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x); exit(1); } } while(0)
static uint32_t bits(float f){uint32_t u;memcpy(&u,&f,4);return u;}
static void rejected(uint32_t index){
 float a=1.25f,e=-2.5f;
 CHECK(!original_chicago_packed_lookup(NULL,index,&a,&e));
 CHECK(bits(a)==bits(1.25f)&&bits(e)==bits(-2.5f));
}
int main(void){
 /* Must be the first lookup: UINT32_MAX was also the cold cache sentinel. */
 rejected(UINT32_MAX);
 uint32_t count=original_chicago_packed_a.count;
 CHECK(count==original_chicago_packed_e.count);
 for(uint32_t i=0;i<count;i++){
  uint32_t ab,eb;float a,e;
  CHECK(original_lossless_second16_lookup(&original_chicago_packed_a,i,&ab));
  CHECK(original_lossless_second16_lookup(&original_chicago_packed_e,i,&eb));
  for(unsigned repeat=0;repeat<2;repeat++){
   CHECK(original_chicago_packed_lookup(NULL,i,&a,&e));
   CHECK(bits(a)==ab&&bits(e)==eb);
  }
 }
 rejected(count);rejected(count+1);rejected(UINT32_MAX);
 float a=1.25f,e=-2.5f;
 CHECK(!original_chicago_packed_lookup(&a,0,&a,&e));
 CHECK(!original_chicago_packed_lookup(NULL,0,NULL,&e));
 CHECK(!original_chicago_packed_lookup(NULL,0,&a,NULL));
 CHECK(!original_chicago_packed_lookup(NULL,0,&a,&a));
 CHECK(bits(a)==bits(1.25f)&&bits(e)==bits(-2.5f));
 printf("PASS Chicago packed cache: %u checks, %u entries, cold/warm invalid indices preserve outputs\n",checks,count);
 return 0;
}
