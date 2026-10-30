/* SPDX-License-Identifier: GPL-3.0-only */
#include "chicago_engine_output.h"
#include "../wobble/miami_fx_conversion.h"
#include <assert.h>
#include <string.h>
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
static int process_core(original_chicago_engine_output *s,original_chicago_output_workspace *w,original_chicago_fx2_workspace *fxw,const original_chicago_routing *r,const uint32_t *il,const uint32_t *ir,uint32_t *ol,uint32_t *orr,uint32_t n,unsigned fz,unsigned known_zero){
 if(!s||!r)return 0;
 if(!n)return 1;
 if(!fxw||n>fxw->capacity||fxw->capacity>4096||!fxw->new_left||!fxw->new_right||!fxw->old_left||!fxw->old_right)return 0;
 int16_t kernel[2],mixed[2];
 int zero_ready=known_zero?
    original_chicago_output_zero_state_ready(&s->output,w,r->master_squared,il,ir,ol,orr,n,fz):
    original_chicago_output_zero_ready(&s->output,w,r->master_squared,il,ir,ol,orr,n,fz);
 if(zero_ready&&
    original_chicago_fx2_wrapper_advance_zero(&s->fx2,n,kernel,mixed)){
  /* Drive is exactly zero. FX2 is a proved constant, possibly the original
   * nonzero EQ residue. Keep used scratch writes and the original master. */
  memset(w->wet_left,0,n*sizeof *w->wet_left);memset(w->wet_right,0,n*sizeof *w->wet_right);
  memset(fxw->old_left,0,n*sizeof *fxw->old_left);memset(fxw->old_right,0,n*sizeof *fxw->old_right);
  if(!kernel[0]&&!kernel[1]){
   memset(w->left,0,n*sizeof *w->left);memset(w->right,0,n*sizeof *w->right);
   memset(fxw->new_left,0,n*sizeof *fxw->new_left);memset(fxw->new_right,0,n*sizeof *fxw->new_right);
   memset(ol,0,n*sizeof *ol);memset(orr,0,n*sizeof *orr);
  }else{
   uint32_t left=miami_fx_output_rne(mixed[0]),right=miami_fx_output_rne(mixed[1]);
   for(uint32_t i=0;i<n;i++){
    w->left[i]=mixed[0];w->right[i]=mixed[1];
    fxw->new_left[i]=kernel[0];fxw->new_right[i]=kernel[1];
    ol[i]=left;orr[i]=right;
   }
  }
  original_master *m=&s->output.master;
  float target=m->desired_mute?0:r->master_squared*4;
  if(!mixed[0]&&!mixed[1]&&m->desired_mute==m->committed_mute&&m->current==target){
   /* zero_ready validated master and volume. With a settled nonnegative
    * gain, positive-zero input stays positive zero after cleanup. Preserve
    * all master writes, including the new zero step and signed-zero current.
    * Keep this admission at the Chicago caller; other owners are unchanged. */
   m->target=target;m->step=0;m->current=target;return 1;
  }
  return original_master_process(m,r->master_squared,ol,orr,ol,orr,n);
 }
 context c={&s->fx2,fxw,1};const original_chicago_fx2 callbacks={&c,process,reset};
 int ok=original_chicago_output_process(&s->output,w,&callbacks,r->master_squared,il,ir,ol,orr,n,fz);
 return ok&&c.status;
}
int original_chicago_engine_output_process(original_chicago_engine_output *s,original_chicago_output_workspace *w,original_chicago_fx2_workspace *fxw,const original_chicago_routing *r,const uint32_t *il,const uint32_t *ir,uint32_t *ol,uint32_t *orr,uint32_t n,unsigned fz){
 return process_core(s,w,fxw,r,il,ir,ol,orr,n,fz,0);
}
int original_chicago_engine_output_process_zero(original_chicago_engine_output *s,original_chicago_output_workspace *w,original_chicago_fx2_workspace *fxw,const original_chicago_routing *r,const uint32_t *il,const uint32_t *ir,uint32_t *ol,uint32_t *orr,uint32_t n,unsigned fz){
 return process_core(s,w,fxw,r,il,ir,ol,orr,n,fz,1);
}
