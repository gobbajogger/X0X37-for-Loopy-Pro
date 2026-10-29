/* SPDX-License-Identifier: GPL-3.0-only */
#include "chicago_engine_output.h"
#include <assert.h>
static void parameter(void *p,unsigned id,unsigned value){int ok=original_chicago_fx2_wrapper_set(p,id,value);assert(ok);(void)ok;}
static int routing_valid(const original_chicago_routing *r){
 return r&&r->fx[0][0]==19&&r->fx[0][1]<=1&&r->fx[1][1]<=1&&r->fx[0][2]<=127&&r->fx[0][3]<=127&&r->fx[1][2]<=127&&r->fx[1][3]<=127;
}
int original_chicago_engine_output_init(original_chicago_engine_output *s,const original_chicago_routing *r,const original_chicago_fx2_owner *owner){
 if(!s||!routing_valid(r)||!owner||!owner->find||!owner->reset_all||!owner->find(owner->context,r->fx[1][0]))return 0;
 if(!original_chicago_fx2_wrapper_init(&s->fx2,owner))return 0;
 return original_chicago_output_init_routing(&s->output,r,parameter,&s->fx2);
}
int original_chicago_engine_output_init_cold(original_chicago_engine_output *s,const original_chicago_routing *r,const original_chicago_fx2_cold_owner *owner){
 if(!s||!routing_valid(r)||!owner||!owner->supports||!owner->select_cold||!owner->discard||!owner->supports(owner->context,r->fx[1][0]))return 0;
 if(!original_chicago_fx2_wrapper_init_cold(&s->fx2,owner))return 0;
 return original_chicago_output_init_routing(&s->output,r,parameter,&s->fx2);
}
int original_chicago_engine_output_apply(original_chicago_engine_output *s,const original_chicago_routing *r,unsigned macro){
 if(!s||macro>=22||!routing_valid(r)||!original_chicago_fx2_wrapper_supports(&s->fx2,r->fx[1][0]))return 0;
 return original_chicago_output_apply_routing(&s->output,r,macro,parameter,&s->fx2);
}
typedef struct {original_chicago_fx2_wrapper *wrapper;original_chicago_fx2_workspace *workspace;int status;} context;
static void process(void *p,int16_t *l,int16_t *r,uint32_t n){context *c=p;c->status=original_chicago_fx2_wrapper_process(c->wrapper,c->workspace,l,r,n);}
static void reset(void *p){context *c=p;original_chicago_fx2_wrapper_reset(c->wrapper);}
int original_chicago_engine_output_process(original_chicago_engine_output *s,original_chicago_output_workspace *w,original_chicago_fx2_workspace *fxw,const original_chicago_routing *r,const uint32_t *il,const uint32_t *ir,uint32_t *ol,uint32_t *orr,uint32_t n,unsigned fz){
 if(!s||!r)return 0;if(!n)return 1;
 if(!fxw||n>fxw->capacity||fxw->capacity>4096||!fxw->new_left||!fxw->new_right||!fxw->old_left||!fxw->old_right)return 0;
 context c={&s->fx2,fxw,1};const original_chicago_fx2 callbacks={&c,process,reset};
 int ok=original_chicago_output_process(&s->output,w,&callbacks,r->master_squared,il,ir,ol,orr,n,fz);
 return ok&&c.status;
}
