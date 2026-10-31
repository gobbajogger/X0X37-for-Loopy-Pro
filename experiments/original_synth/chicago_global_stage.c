/* SPDX-License-Identifier: GPL-3.0-only */
#include "chicago_global_stage.h"
int original_chicago_selected_global_bind(original_chicago_selected_global_stage *s,original_chicago_global_mg *state,const original_chicago_global_mg_input *input,chicago_arp_clock *c1,chicago_arp_clock *c2,const original_chicago_allocator *a){
 if(!s||!state||!input||!c1||!c2||!a||input->sample_rate_integer!=44100||input->sample_rate!=44100||input->tempo_sync2!=1)return 0;
 *s=(original_chicago_selected_global_stage){state,input,c1,c2,a,0,0,1};return 1;
}
void original_chicago_selected_global_seed(original_chicago_selected_global_stage *s,unsigned id){
 if(!s||!s->status)return;
 if(s->allocator->voice_active.head>=0&&(unsigned)s->allocator->voice_active.head==id){s->state->counter=s->input->sample_rate_integer;s->full_head_seeds++;}
}
static void unqualified_step(void *p){((original_chicago_selected_global_stage *)p)->status=0;}
int original_chicago_selected_global_exit(original_chicago_selected_global_stage *s){
 if(!s||!s->status)return 0;
 if(s->input->sample_rate_integer!=44100||s->input->sample_rate!=44100||s->input->tempo_sync2!=1){s->status=0;return 0;}
 (void)original_chicago_global_mg_after_voices(s->state,s->input,s->clock1,s->clock2,unqualified_step,s);s->helper_entries++;return s->status;
}
