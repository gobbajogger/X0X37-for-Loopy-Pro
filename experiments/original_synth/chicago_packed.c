/* SPDX-License-Identifier: GPL-3.0-only */
#include "chicago_packed.h"
#include "chicago_packed_private.h"
#include <string.h>
#include <stdint.h>
int original_chicago_packed_lookup(const void *context,uint32_t index,float *a,float *e){
 if(context||!a||!e||a==e)return 0;
 /* The tables are constant: repeat the last decoded pair for the same index. Two
  * Chicago parts may look up at once on the two cores (DnBOX engine.c): the index is
  * cleared before the pair is written and set after it, and a hit is taken only when
  * the index reads the same before and after the pair (else decode again). */
 static volatile uint32_t last_index=UINT32_MAX,last_a,last_e;
 uint32_t ab,eb,i0=last_index;
 ab=last_a;eb=last_e;
 if(i0!=index||last_index!=i0){
  if(!original_lossless_second16_lookup(&original_chicago_packed_a,index,&ab)||!original_lossless_second16_lookup(&original_chicago_packed_e,index,&eb))return 0;
  last_index=UINT32_MAX;last_a=ab;last_e=eb;last_index=index;
 }
 memcpy(a,&ab,4);memcpy(e,&eb,4);return 1;
}
