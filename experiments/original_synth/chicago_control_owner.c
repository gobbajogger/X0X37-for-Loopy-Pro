/* SPDX-License-Identifier: GPL-3.0-only */
#include "chicago_control_owner.h"
int original_chicago_control_owner_bind(original_chicago_control_owner *s,original_chicago_program *p,original_chicago_voice_controls *v,original_chicago_engine_output *o,original_chicago_scheduler *scheduler,original_chicago_global_mg_input *global){
 if(!s||!p||!v||!o||!scheduler||!scheduler->allocator||!scheduler->control_input||!global)return 0;
 *s=(original_chicago_control_owner){p,v,o,scheduler,global,1};return 1;
}
int original_chicago_control_owner_edit(original_chicago_control_owner *s,unsigned macro,float normalized){
 if(!s||!s->status)return 0;
 original_chicago_program next=*s->program;original_chicago_voice_controls compiled;
 if(!original_chicago_program_set(&next,macro,normalized)||!original_chicago_voice_compile(&compiled,&next))return 0;
 const original_chicago_routing *r=&compiled.routing;
 if(!original_chicago_fx2_wrapper_supports(&s->output->fx2,r->fx[1][0]))return 0;
 /* Publish before immediate native108 callbacks: they can inspect the newly
  * converted view and existing ordered note ownership. No audio/event owner
  * interleaves this serial edit or sees half the multi-target public mapping. */
 *s->program=next;*s->voice=compiled;
 chicago_arp_control_input *control=s->scheduler->control_input;
 control->enable=r->arp_enable;control->latch=r->arp_latch;
 s->scheduler->allocator->arp_mode=r->arp_mode;s->scheduler->allocator->arp_range=r->arp_range;
 original_chicago_global_mg_input *g=s->global_input;
 g->rate1=compiled.mg_frequency[0];g->rate2=compiled.mg_frequency[1];
 g->waveform1=compiled.mg_shape[0];g->waveform2=compiled.mg_shape[1];g->tempo_sync1=compiled.mg_tempo[0];g->tempo_sync2=compiled.mg_tempo[1];
 if(!original_chicago_engine_output_apply(s->output,r,macro)){s->status=0;return 0;}
 if(macro==21&&!original_chicago_scheduler_damper(s->scheduler,r->damper)){s->status=0;return 0;}
 return 1;
}
