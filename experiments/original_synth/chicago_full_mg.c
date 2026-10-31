/* SPDX-License-Identifier: GPL-3.0-only */
#include "chicago_full_mg.h"
#include <string.h>
int original_chicago_full_mg_resolve(const original_chicago_full_mg_input *in,original_chicago_full_mg_output *out){
 if(!in||!out||in->voice_id>=128||in->head_id>=128||in->shape[0]!=0||in->shape[1]!=0||in->key_sync[0]>1||in->key_sync[1]>1)return 0;
 original_chicago_full_mg_output n;memset(&n,0,sizeof n);memcpy(n.voice,in->current,sizeof n.voice);memcpy(n.global,in->global,sizeof n.global);n.reevaluate_voice_mask=3;
 if(in->voice_id==in->head_id){
  n.seed_core_counter=1;
  for(unsigned i=0;i<2;i++){
   if(in->key_sync[i]){
    n.reset_global_mask|=(uint8_t)(1u<<i);
    /* Native direct initializer phase−1 and exact triangle evaluator output.
     * Wrapped1 is consumed/cleared by global evaluator; voice wrap gets1. */
    n.global[i].phase=UINT32_C(0xbf800000);n.global[i].sample=0;n.global[i].wrapped=0;n.voice[i].wrapped=1;
   }
   n.voice[i].phase=n.global[i].phase;
  }
 }else{
  for(unsigned i=0;i<2;i++){n.voice[i].phase=in->head[i].phase;n.voice[i].hold=in->head[i].hold;n.voice[i].noise=in->head[i].noise;}
 }
 *out=n;return 1;
}
