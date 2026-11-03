/* SPDX-License-Identifier: GPL-3.0-only */
#include "../../firmware/src/app/original_runtime.h"
#include <stdio.h>
int main(void){
 /* Two runtime parts are resident in .pool. This is a host ABI
  * regression sentinel, NOT a substitute for the target linker check. */
 const size_t max_part_bytes = 5368u;
 const size_t actual = sizeof(original_runtime_part);
 if(actual > max_part_bytes){
  fprintf(stderr,"FAIL Miami runtime-part sizeof %zu > %zu\n",actual,max_part_bytes);
  return 1;
 }
 printf("PASS Miami runtime-part sizeof %zu <= %zu\n",actual,max_part_bytes);
 return 0;
}
