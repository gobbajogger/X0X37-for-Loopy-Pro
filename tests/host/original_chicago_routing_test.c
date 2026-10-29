/* SPDX-License-Identifier: GPL-3.0-only */
#include "chicago_program_output.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
typedef struct {unsigned p[4],sets,calls,frames,resets;} capture;
static void parameter(void *ctx,unsigned p,unsigned value){capture *c=ctx;assert(p<4);c->p[p]=value;c->sets++;}
/* Contract capture only: this is explicitly NOT a substitute Chicago FX2.
 * Processor parity is exercised by the independent output oracle, with its
 * separately labelled synthetic processor. Here the seam checks exact public
 * control writes into Drive and caller FX2 ownership plus native96/master. */
static void process(void *ctx,int16_t *l,int16_t *r,uint32_t n){capture *c=ctx;c->calls++;c->frames+=n;(void)l;(void)r;}
static void reset(void *ctx){((capture *)ctx)->resets++;}
static uint32_t bits(float f){uint32_t u;memcpy(&u,&f,4);return u;}
int main(void){
 unsigned blocks=0;
 for(unsigned factory=0;factory<81;factory++){
  original_chicago_program p;original_chicago_routing r;original_chicago_output s;capture c={{0},0,0,0,0};
  assert(original_chicago_program_init(&p,factory));assert(original_chicago_program_routing(&p,&r));
  assert(original_chicago_output_init_routing(&s,&r,parameter,&c));assert(c.sets==4);
  for(unsigned i=0;i<4;i++)assert(c.p[i]==r.fx[1][i]);
  int16_t temp[4][64];uint32_t in[2][64],out[2][64];
  original_chicago_output_workspace w={temp[0],temp[1],temp[2],temp[3],64};original_chicago_fx2 fx={&c,process,reset};
  for(unsigned i=0;i<64;i++){in[0][i]=bits((float)((int)i-32)/64);in[1][i]=bits((float)(31-(int)i)/64);}
  assert(original_chicago_output_process(&s,&w,&fx,r.master_squared,in[0],in[1],out[0],out[1],1,0));blocks++;
  assert(s.fx1.drive.target_gain==24900&&s.fx1.drive.target_drive==(r.fx[0][2]==127?32767:(int)r.fx[0][2]*258));
  assert(s.fx1.activity_counter==1&&s.fx1.drive.activity==1);
  for(unsigned id=0;id<22;id++){
   original_chicago_program held=p;float x=(float)((id*37+factory*17)%128)/127;
   assert(original_chicago_program_set(&p,id,x));assert(original_chicago_program_routing(&p,&r));
   unsigned sets=c.sets;int32_t activity=s.fx1.activity_counter;
   assert(original_chicago_output_apply_routing(&s,&r,id,parameter,&c));
   assert(s.fx1.activity_counter==activity);assert(!memcmp(held.macro,p.macro,id*sizeof(float)));
   assert(c.sets==sets+(unsigned)(id>=17&&id<=19));
   for(unsigned i=0;i<4;i++)assert(c.p[i]==r.fx[1][i]);
   assert(s.fx1.drive.target_drive==(r.fx[0][2]==127?32767:(int)r.fx[0][2]*258));
   assert(original_chicago_output_process(&s,&w,&fx,r.master_squared,in[0],in[1],out[0],out[1],3,0));blocks++;
   assert(bits(s.master.target)==bits(r.master_squared*4));assert(bits(s.master.current)==bits(r.master_squared*4));
   assert(s.fx1.drive.activity==(s.fx1.activity_counter>=0));
  }
  original_chicago_output saved=s;capture before=c;
  assert(original_chicago_output_process(&s,NULL,NULL,NAN,NULL,NULL,NULL,NULL,0,99));
  assert(!memcmp(&saved,&s,sizeof s)&&!memcmp(&before,&c,sizeof c));
  assert(!original_chicago_output_apply_routing(&s,&r,22,parameter,&c));
  assert(!original_chicago_output_apply_routing(&s,&r,17,NULL,&c));
  assert(!memcmp(&saved,&s,sizeof s)&&!memcmp(&before,&c,sizeof c));
 }
 printf("PASS81 original programs/all22 edits joined through canonical Drive/FX2parameter seam/native96master; %u blocks; FX2capture is not an original processor\n",blocks);
 return 0;
}
