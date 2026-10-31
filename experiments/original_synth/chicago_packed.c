/* SPDX-License-Identifier: GPL-3.0-only */
#include "chicago_packed.h"
#include "chicago_packed_private.h"
#include <string.h>
int original_chicago_packed_lookup(const void *context,uint32_t index,float *a,float *e){
 if(context||!a||!e||a==e)return 0;
 uint32_t ab,eb;
 if(!original_lossless_second16_lookup(&original_chicago_packed_a,index,&ab)||!original_lossless_second16_lookup(&original_chicago_packed_e,index,&eb))return 0;
 memcpy(a,&ab,4);memcpy(e,&eb,4);return 1;
}
