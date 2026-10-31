/* SPDX-License-Identifier: GPL-3.0-only */
#include "chicago_packed.h"
#include "chicago_packed_private.h"
#include <string.h>
#include <stdint.h>
int original_chicago_packed_lookup(const void *context,uint32_t index,float *a,float *e){
 if(context||!a||!e||a==e)return 0;
 /* The tables are constant: repeat the last decoded pair for the same index. */
 static uint32_t last_index=UINT32_MAX,last_a,last_e;
 uint32_t ab,eb;
 if(index==last_index){ab=last_a;eb=last_e;}
 else{
  if(!original_lossless_second16_lookup(&original_chicago_packed_a,index,&ab)||!original_lossless_second16_lookup(&original_chicago_packed_e,index,&eb))return 0;
  last_index=index;last_a=ab;last_e=eb;
 }
 memcpy(a,&ab,4);memcpy(e,&eb,4);return 1;
}
