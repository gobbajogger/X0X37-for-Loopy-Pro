/* SPDX-License-Identifier: GPL-3.0-only */
#include "miami_output.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static uint32_t bits(float f){uint32_t u;memcpy(&u,&f,4);return u;}
static int32_t dl[257],dr[257],rl[257],rr[257];
static int16_t sl[257],sr[257],rsl[257],rsr[257];
static original_miami_fx_workspace work={dl,dr,sl,sr,257},refwork={rl,rr,rsl,rsr,257};
static void apply_ref(original_miami_fx_chain *s,const original_miami_program *p){
 float a,b;assert(original_miami_program_native(p,110,&a)&&original_miami_program_native(p,111,&b));
 assert(miami_decimator_adapter_set(&s->decimator,1,(int32_t)fmaf(a,127,.5f)));
 assert(miami_decimator_adapter_set(&s->decimator,2,(int32_t)fmaf(b,127,.5f)));
}
/* The unchanged independent FX chain always uses cleanup/scaled/fcvtzs.
 * Exercise zero inputs while effects retain samples and wrappers/master ramp;
 * compare final int32 work values, PCM and complete state, not silence alone. */
static void zero_input_paths(void){
 static const uint32_t neighbors[]={1u,0x80000001u,0x007fffffu,0x807fffffu,
  0x00800000u,0x80800000u,0x3f000000u,0xbf000000u,0x7f800000u,0xff800000u,
  0x7fc12345u,0xffc12345u};
 uint32_t il[257],ir[257],ol[257],orr[257],gl[257],gr[257];
 int32_t dry_l[257],dry_r[257];
 uint64_t hash=UINT64_C(1469598103934665603);
 unsigned frames=0,zero_channels=0,tail_nonzero=0,reset_count=0,neighbor_seen=0;
 for(unsigned fz=0;fz<2;fz++)for(unsigned factory=0;factory<30;factory++){
  original_miami_program p;assert(original_miami_program_init(&p,factory));
  original_miami_output s={0};original_miami_output_init(&s);assert(original_miami_output_apply(&s,&p,10));
  original_miami_fx_chain fx={0};original_miami_fx_init(&fx);apply_ref(&fx,&p);
  original_master master={0};original_master_init(&master);
  for(unsigned block=0;block<40;block++){
   unsigned n=block%10==0?257:1+(block*37)%257,kind=block%8;
   for(unsigned i=0;i<n;i++){
    uint32_t l=bits((float)((int)((i*29+block*11)%1024)-512)/511);
    uint32_t r=bits((float)((int)((i*43+block*7)%1024)-512)/533);
    if(kind==1)l=r=0;
    else if(kind==2)l=r=0x80000000u;
    else if(kind==3)l=(i&1)?0x80000000u:0;
    else if(kind==4)r=(i&1)?0x80000000u:0;
    else if(kind==5){
     unsigned j=(i/2)%12;
     if(i&1){l=neighbors[j];r=0x80000000u;}
     else {j=(j+3)%12;l=0;r=neighbors[j];}
     neighbor_seen|=1u<<j;
    }
    il[i]=l;ir[i]=r;zero_channels+=(l&0x7fffffffu)==0;zero_channels+=(r&0x7fffffffu)==0;
   }
   if(block%7==0){assert(original_miami_program_set(&p,10,(float)(block%8)/7));assert(original_miami_output_apply(&s,&p,10));apply_ref(&fx,&p);}
   if(block%5==0)assert(original_miami_program_set(&p,14,(float)(block%4)/3));
   if(block%11==0){original_master_mute(&s.master,(uint8_t)(block&1));original_master_mute(&master,(uint8_t)(block&1));}
   if(block%13==0){original_fixed_fx_internal_enable(&s.fx.fx1,(uint8_t)(block&1));original_fixed_fx_internal_enable(&fx.fx1,(uint8_t)(block&1));}
   uint32_t events[4]={0,0,n/2,n-1};unsigned ne=block%9==0?4:0,event=0;
   if(kind==1&&!ne){
    /* A legal held-sample state makes zero input audibly non-silent. This
     * specifically rejects an erroneous whole-output/FX idle shortcut. */
    assert(original_miami_program_set(&p,14,1));
    original_master_mute(&s.master,0);original_master_mute(&master,0);
    original_fixed_fx_internal_enable(&s.fx.fx1,1);original_fixed_fx_internal_enable(&fx.fx1,1);
    assert(miami_decimator_adapter_set(&s.fx.decimator,2,127));assert(miami_decimator_adapter_set(&fx.decimator,2,127));
    assert(miami_decimator_seed(&s.fx.decimator.kernel,0,12345,-6789,1));
    assert(miami_decimator_seed(&fx.decimator.kernel,0,12345,-6789,1));
   }
   float volume;assert(original_miami_program_native(&p,96,&volume));
   assert(original_miami_output_process(&s,&p,&work,il,ir,ol,orr,n,fz,events,ne));
   for(unsigned i=0;i<n;i++){
    while(event<ne&&events[event]==i){assert(miami_decimator_adapter_reset(&fx.decimator));event++;reset_count++;}
    assert(original_miami_fx_process(&fx,&refwork,il+i,ir+i,gl+i,gr+i,1,fz));
    dry_l[i]=rl[0];dry_r[i]=rr[0];
   }
   assert(original_master_process(&master,volume*volume,gl,gr,gl,gr,n));
   assert(!memcmp(ol,gl,n*4)&&!memcmp(orr,gr,n*4));
   assert(!memcmp(dl,dry_l,n*sizeof *dl)&&!memcmp(dr,dry_r,n*sizeof *dr));
   assert(!memcmp(&s.fx,&fx,sizeof fx)&&!memcmp(&s.master,&master,sizeof master));
   for(unsigned i=0;i<n;i++){
    hash^=ol[i];hash*=UINT64_C(1099511628211);hash^=orr[i];hash*=UINT64_C(1099511628211);
    if((il[i]&0x7fffffffu)==0&&(ir[i]&0x7fffffffu)==0)tail_nonzero+=(ol[i]&0x7fffffffu)!=0||(orr[i]&0x7fffffffu)!=0;
   }
   frames+=n;
  }
  original_miami_output before=s;uint32_t sentinel=0x12345678u;ol[0]=orr[0]=sentinel;
  assert(!original_miami_output_process(&s,&p,&work,il,ir,ol,orr,1,2,NULL,0));
  assert(!memcmp(&s,&before,sizeof s)&&ol[0]==sentinel&&orr[0]==sentinel);
 }
 assert(zero_channels>0&&tail_nonzero>0&&reset_count>0&&neighbor_seen==0xfffu);
 printf("PASS zero/mixed/neighbor input paths: %u stereo frames, %u zero channels, %u nonzero zero-input tail frames, %u resets, fz0/1, PCM %016llx\n",frames,zero_channels,tail_nonzero,reset_count,(unsigned long long)hash);
}
int main(void){
 uint32_t inl[257],inr[257],ol[257],orr[257],goldl[257],goldr[257];
 unsigned frames=0;
 for(unsigned factory=0;factory<30;factory++) {
  original_miami_program p;assert(original_miami_program_init(&p,factory));
  original_miami_output s={0};original_miami_output_init(&s);assert(original_miami_output_apply(&s,&p,10));
  original_miami_fx_chain fx={0};original_miami_fx_init(&fx);apply_ref(&fx,&p);
  original_master master={0};original_master_init(&master);
  for(unsigned block=0;block<48;block++) {
   unsigned n=1+(block*37)%257;
   for(unsigned i=0;i<n;i++){inl[i]=bits((float)((int)((i*29+block*11)%1024)-512)/511);inr[i]=bits((float)((int)((i*43+block*7)%1024)-512)/533);}
   if(block%7==0){assert(original_miami_program_set(&p,10,(float)(block%8)/7));assert(original_miami_output_apply(&s,&p,10));apply_ref(&fx,&p);}
   if(block%11==0)assert(original_miami_program_set(&p,14,(float)(block%4)/3));
   if(block%13==0){original_master_mute(&s.master,(uint8_t)(block&1));original_master_mute(&master,(uint8_t)(block&1));}
   float volume;assert(original_miami_program_native(&p,96,&volume));
   assert(original_miami_output_process(&s,&p,&work,inl,inr,ol,orr,n,0,NULL,0));
   assert(original_miami_fx_process(&fx,&refwork,inl,inr,goldl,goldr,n,0));
   assert(original_master_process(&master,volume*volume,goldl,goldr,goldl,goldr,n));
   assert(!memcmp(ol,goldl,n*4)&&!memcmp(orr,goldr,n*4));
   assert(!memcmp(&s.fx,&fx,sizeof fx)&&!memcmp(&s.master,&master,sizeof master));frames+=n;
  }
  original_miami_output before=s;
  assert(original_miami_output_process(&s,&p,&work,NULL,NULL,NULL,NULL,0,0,NULL,0));assert(!memcmp(&s,&before,sizeof s));
  assert(!original_miami_output_process(&s,&p,&work,inl,inr,ol,orr,2049,0,NULL,0)&&!memcmp(&s,&before,sizeof s));
  uint32_t bad[]={64,63};assert(!original_miami_output_process(&s,&p,&work,inl,inr,ol,orr,257,0,bad,2)&&!memcmp(&s,&before,sizeof s));
 }
 /* 16th Cutz's original Crush mapping writes frequency19, wet22. */
 original_miami_program p;assert(original_miami_program_init(&p,0));original_miami_output s={0};original_miami_output_init(&s);assert(original_miami_output_apply(&s,&p,10));
 assert(s.fx.decimator.frequency_integer==19&&s.fx.decimator.wet_integer==22);
 /* Explicit reset events affect the effect, not wrapper gain or master.
  * With no setter in the invocation, scalar reference calls have the same
  * held/phase trajectory while exercising independent reset ordering. */
 original_miami_fx_chain fx={0};original_miami_fx_init(&fx);apply_ref(&fx,&p);
 original_master master={0};original_master_init(&master);
 const uint32_t events[]={0,0,63,64,128,256};
 for(unsigned i=0;i<257;i++){inl[i]=bits((float)((int)i-128)/128);inr[i]=bits((float)(127-(int)i)/129);}
 assert(original_miami_output_process(&s,&p,&work,inl,inr,ol,orr,257,0,events,6));
 unsigned event=0;float volume;assert(original_miami_program_native(&p,96,&volume));
 for(unsigned i=0;i<257;i++){
  while(event<6&&events[event]==i){assert(miami_decimator_adapter_reset(&fx.decimator));event++;}
  assert(original_miami_fx_process(&fx,&refwork,inl+i,inr+i,goldl+i,goldr+i,1,0));
  assert(original_master_process(&master,volume*volume,goldl+i,goldr+i,goldl+i,goldr+i,1));
 }
 assert(!memcmp(ol,goldl,sizeof ol)&&!memcmp(orr,goldr,sizeof orr));
 assert(!memcmp(&s.fx,&fx,sizeof fx)&&!memcmp(&s.master,&master,sizeof master));
 zero_input_paths();
 printf("PASS joined all30 program Crush/master output profiles, %u stereo frames and whole state exact; selected projection, not full instrument\n",frames);return 0;
}
