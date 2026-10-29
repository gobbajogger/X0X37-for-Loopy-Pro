/* SPDX-License-Identifier: GPL-3.0-only */
#include "chicago_program_output.h"
static int valid(const original_chicago_routing *r){
 if(!r||r->fx[0][0]!=19)return 0;
 for(unsigned slot=0;slot<2;slot++)if(r->fx[slot][0]>29||r->fx[slot][1]>1||r->fx[slot][2]>127||r->fx[slot][3]>127)return 0;
 return 1;
}
int original_chicago_output_init_routing(original_chicago_output *s,const original_chicago_routing *r,original_chicago_fx_parameter set,void *ctx){
 if(!s||!valid(r)||!set)return 0;
 (void)original_chicago_output_init(s,r->fx[0][2],r->fx[0][3],r->fx[0][1]);
 for(unsigned i=0;i<4;i++)set(ctx,i,r->fx[1][i]);
 return 1;
}
int original_chicago_output_apply_routing(original_chicago_output *s,const original_chicago_routing *r,unsigned macro,original_chicago_fx_parameter set,void *ctx){
 if(!s||macro>=22||!valid(r)||(macro>=17&&macro<=19&&!set))return 0;
 if(macro==5)return original_chicago_drive_wrapper_set(&s->fx1,2,r->fx[0][2]);
 if(macro>=17&&macro<=19){unsigned parameter=macro==17?0:macro-16;set(ctx,parameter,r->fx[1][parameter]);}
 return 1;
}
