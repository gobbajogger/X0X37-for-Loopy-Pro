/* SPDX-License-Identifier: GPL-3.0-only */
#include "chicago_key_admission.h"
int original_chicago_canonical_keys_valid(const chicago_arp_control *s){
 if(!s||s->committed_enable>1||s->effective_latch>1||s->current_note<0||s->current_note>255)return 0;
 uint32_t p=0,l=0;
 for(unsigned i=0;i<128;i++){
  if(s->physical[i]>127||s->latched[i]>127)return 0;
  p+=(s->physical[i]!=0);l+=(s->latched[i]!=0);
 }
 return p==s->physical_count&&l==s->latched_count;
}
