/* SPDX-License-Identifier: GPL-3.0-only */
#include "chicago_fx2_bank.h"
int baseline_original_chicago_fx2_bank_events(original_chicago_fx2_bank *s,const original_chicago_drive_event *e,uint32_t count,int32_t origin){
 if(!s||(count&&!e))return 0;for(uint32_t i=1;i<count;i++)if(e[i-1].offset>e[i].offset)return 0;
 (void)original_mod_events(&s->ring.queue,e,count);(void)original_mod_events(&s->chorus.queue,e,count);(void)original_mod_events(&s->phaser.queue,e,count);(void)original_mod_events(&s->ensemble.queue,e,count);
 (void)original_chicago_mastercomp_bind_events(&s->mastercomp,e,count);(void)original_chicago_peaking_bind_events(&s->peaking,e,count);(void)original_chicago_decimator_bind_events(&s->decimator,e,count);
 for(unsigned i=0;i<4;i++)s->history_queue[i]=(original_chicago_drive_queue){e,count,0};s->history_event_origin=origin;return 1;
}
