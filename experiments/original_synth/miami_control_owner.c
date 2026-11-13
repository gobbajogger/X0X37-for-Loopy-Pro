/* SPDX-License-Identifier: GPL-3.0-only */
#include "miami_control_owner.h"
int original_miami_control_owner_bind(original_miami_control_owner *s,original_miami_program *p,original_miami_voice_controls *c,original_miami_output *out,original_chicago_scheduler *scheduler,original_chicago_global_mg_input *input){
 if(!s||!p||!c||!out||!scheduler||!input||p->revision!=c->revision||input->waveform1!=c->mg_shape[0]||input->waveform2!=c->mg_shape[1])return 0;
 *s=(original_miami_control_owner){p,c,out,scheduler,input,1};return 1;
}
int original_miami_control_owner_edit(original_miami_control_owner *s,unsigned id,float value){
 if(!s||!s->status)return 0;
 original_miami_program p=*s->program;original_miami_voice_controls c;
 if(!original_miami_program_set(&p,id,value)||!original_miami_voice_compile(&c,&p))return 0;
 *s->program=p;*s->controls=c;s->global_input->waveform1=c.mg_shape[0];s->global_input->waveform2=c.mg_shape[1];
 if(!original_miami_output_apply(s->output,s->program,id)){s->status=0;return 0;}
 if(id==17&&!original_chicago_scheduler_damper(s->scheduler,c.damper?127u:0u)){s->status=0;return 0;}
 return 1;
}
int original_miami_control_owner_block_prepare(original_miami_control_owner *s,chicago_arp_clock *clock1){
 if(!s||!s->status||!clock1)return 0;
 chicago_arp_clock_prepare(clock1,44100,s->controls->mg_base[0],s->controls->mg_times[0]);
 chicago_arp_clock_prepare(s->scheduler->clock,44100,s->controls->mg_base[1],s->controls->mg_times[1]);
 return original_chicago_scheduler_prepare(s->scheduler);
}
