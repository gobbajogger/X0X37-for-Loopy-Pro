/* SPDX-License-Identifier: GPL-3.0-only */
#include "miami_startup.h"
#include "binary32_classify.h"
#include <string.h>
int original_miami_startup_reset(original_miami_startup_state *s,const original_miami_startup_owner *owner,uint32_t *scratch,float fs){
 if(!s||!owner||!scratch||!owner->reset_both_wrappers||!owner->reset_core||!owner->restore||!owner->render||!original_binary32_isfinite(fs)||fs<8000||fs>2147482880.0f)return 0;
 float target=fs*.100000001490116119384765625f;
 *s=(original_miami_startup_state){(uint32_t)target,0,0,ORIGINAL_MIAMI_STARTUP_WRAPPERS,0};
 if(!owner->reset_both_wrappers(owner->context))return 0;
 s->stage=ORIGINAL_MIAMI_STARTUP_CORE;if(!owner->reset_core(owner->context))return 0;
 s->stage=ORIGINAL_MIAMI_STARTUP_RESTORE;if(!owner->restore(owner->context,0))return 0;
 s->stage=ORIGINAL_MIAMI_STARTUP_WARMUP;
 while(s->rendered<s->target){
  memset(scratch,0,4096);
  if(!owner->render(owner->context,scratch,scratch+256,scratch+512,scratch+768,256))return 0;
  s->rendered+=256;s->blocks++;
 }
 s->stage=ORIGINAL_MIAMI_STARTUP_COMPLETE;s->status=1;return 1;
}
