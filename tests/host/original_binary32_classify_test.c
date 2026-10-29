/* SPDX-License-Identifier: GPL-3.0-only */
#include "binary32_classify.h"
#include "chicago_output.h"
#include "chicago_program.h"
#include <assert.h>
#include <fenv.h>
#include <math.h>
#include <stdio.h>
int baseline_original_chicago_output_suspend(original_chicago_output *,unsigned,unsigned,float);
static float from_bits(uint32_t u){float f;memcpy(&f,&u,4);return f;}
int baseline_original_chicago_output_process(original_chicago_output *,original_chicago_output_workspace *,const original_chicago_fx2 *,float,const uint32_t *,const uint32_t *,uint32_t *,uint32_t *,uint32_t,unsigned);
int baseline_original_master_process(original_master *,float,const uint32_t *,const uint32_t *,uint32_t *,uint32_t *,uint32_t);
int baseline_original_chicago_program_set(original_chicago_program *,unsigned,float);
static unsigned checks,host_snan_flag_differences;
static void flags_equal_or_record(uint32_t bits,int old_ex,int next_ex){
 if(old_ex==next_ex)return;
 if(!((bits&0x7f800000u)==0x7f800000u&&(bits&0x3fffffu)&&!(bits&0x400000u)))fprintf(stderr,"FP mismatch bits=%08x old=%x new=%x check=%u\n",bits,old_ex,next_ex,checks);
 assert((bits&0x7f800000u)==0x7f800000u&&(bits&0x3fffffu)&&!(bits&0x400000u));
 assert(old_ex==FE_INVALID&&next_ex==0);host_snan_flag_differences++;
}
static void check_api(uint32_t bits){
 float x=from_bits(bits);
 for(unsigned slot=0;slot<3;slot++)for(unsigned flag=0;flag<2;flag++){
  original_chicago_output a,b;assert(original_chicago_output_init(&a,100,110,1));b=a;
  feclearexcept(FE_ALL_EXCEPT);int old=baseline_original_chicago_output_suspend(&a,slot,flag,x);int old_ex=fetestexcept(FE_ALL_EXCEPT);
  feclearexcept(FE_ALL_EXCEPT);int next=original_chicago_output_suspend(&b,slot,flag,x);int next_ex=fetestexcept(FE_ALL_EXCEPT);
  assert(old==next&&!memcmp(&a,&b,sizeof a));
  flags_equal_or_record(bits,old_ex,next_ex);
  checks++;
 }
 original_chicago_program pa,pb;assert(original_chicago_program_init(&pa,0));pb=pa;
 feclearexcept(FE_ALL_EXCEPT);int old=baseline_original_chicago_program_set(&pa,0,x),old_ex=fetestexcept(FE_ALL_EXCEPT);
 feclearexcept(FE_ALL_EXCEPT);int next=original_chicago_program_set(&pb,0,x),next_ex=fetestexcept(FE_ALL_EXCEPT);
 assert(old==next&&!memcmp(&pa,&pb,sizeof pa));flags_equal_or_record(bits,old_ex,next_ex);checks++;
 for(unsigned field=0;field<3;field++){
  uint32_t in_l=0x3e800000u,in_r=0xbe800000u,al=0xa5a5a5a5u,ar=al,bl=al,br=ar;
  original_chicago_output a,b;assert(original_chicago_output_init(&a,100,110,1));a.enable_slot[0]=a.enable_slot[1]=0;b=a;
  int16_t sl[1],sr[1],wl[1],wr[1];original_chicago_output_workspace workspace={sl,sr,wl,wr,1};
  if(field==0){
   feclearexcept(FE_ALL_EXCEPT);old=baseline_original_chicago_output_process(&a,&workspace,NULL,x,&in_l,&in_r,&al,&ar,1,0);old_ex=fetestexcept(FE_ALL_EXCEPT);
   feclearexcept(FE_ALL_EXCEPT);next=original_chicago_output_process(&b,&workspace,NULL,x,&in_l,&in_r,&bl,&br,1,0);next_ex=fetestexcept(FE_ALL_EXCEPT);
  }else{
   if(field==2){a.master.current=x;b.master.current=x;}
   float volume=field==2?.5f:x;
   feclearexcept(FE_ALL_EXCEPT);old=baseline_original_master_process(&a.master,volume,&in_l,&in_r,&al,&ar,1);old_ex=fetestexcept(FE_ALL_EXCEPT);
   feclearexcept(FE_ALL_EXCEPT);next=original_master_process(&b.master,volume,&in_l,&in_r,&bl,&br,1);next_ex=fetestexcept(FE_ALL_EXCEPT);
  }
  assert(old==next&&!memcmp(&a,&b,sizeof a)&&al==bl&&ar==br);flags_equal_or_record(bits,old_ex,next_ex);checks++;
 }

}
int main(void){
 /* Every nonfinite sign/payload encoding, including both infinities and all
  * signalling NaNs; finite classification depends on the exponent alone. */
 for(uint32_t payload=0;payload<UINT32_C(0x00800000);payload++){
  assert(!original_binary32_isfinite(from_bits(UINT32_C(0x7f800000)|payload)));
  assert(!original_binary32_isfinite(from_bits(UINT32_C(0xff800000)|payload)));
 }
 const uint32_t mantissa[]={0,1,2,0x3fffff,0x400000,0x7ffffe,0x7fffff};
 for(unsigned sign=0;sign<2;sign++)for(unsigned exponent=0;exponent<256;exponent++)for(unsigned m=0;m<sizeof mantissa/sizeof *mantissa;m++){
  uint32_t u=(sign<<31)|(exponent<<23)|mantissa[m];
  feclearexcept(FE_ALL_EXCEPT);assert(original_binary32_isfinite(from_bits(u))==(exponent!=255));assert(!fetestexcept(FE_ALL_EXCEPT));check_api(u);
 }
 /* Sample-rate lower boundary, ordinary rates, UINT32 conversion/overflow
  * boundaries, signedzero and subnormals are differential API cases. */
 const uint32_t edges[]={0,0x80000000,1,0x80000001,0x007fffff,0x00800000,0x45f9ffff,0x45fa0000,0x45fa0001,0x472c4400,0x473b8000,0x4f7fffff,0x4f800000,0x7f7ffffe,0x7f7fffff};
 for(unsigned i=0;i<sizeof edges/sizeof *edges;i++)check_api(edges[i]);
 printf("PASS16777216 nonfinite encodings; all512sign/exponent classes and mantissaedges; %u R3-vs-repair return/state comparisons; host_sNaN_flag_differences=%u; invalid_nonfinite_FPstatus_unqualified_not_parity; target_FPstatus_not_claimed\n",checks,host_snan_flag_differences);
 return 0;
}
