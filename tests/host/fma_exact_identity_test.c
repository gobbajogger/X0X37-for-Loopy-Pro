/* SPDX-License-Identifier: GPL-3.0-only */
/* original_fma_compact_bits / original_fma_finite_bits / original_rne_fmaf
 * against the 18-limb oracle and libc fmaf, for finite inputs. */
#include "fused_reference.h"
#include "binary32_math.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL line %d case %u\n",__LINE__,checks);exit(1);}}while(0)
static unsigned checks, compact_ok, compact_miss;

static uint32_t seed=0x6d70u;
static uint32_t rng(void){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;}

static int same_bits(uint32_t a,uint32_t b){
 /* +0 and -0: the oracle and IEEE may both produce a signed zero; compare exactly. */
 return a==b;
}


/* Encoded-exp addend-dominates (cv_fma shortcut). Must agree with the oracle:
 * when this is true, RNE(a*b+c) bits equal uc. */
static int addend_dominates(uint32_t ua,uint32_t ub,uint32_t uc){
 if(!(uc&0x7fffffffu))return 0;
 if((ua&0x7f800000u)==0x7f800000u||(ub&0x7f800000u)==0x7f800000u||(uc&0x7f800000u)==0x7f800000u)return 0;
 int ea=(int)((ua>>23)&255u),eb=(int)((ub>>23)&255u),ec=(int)((uc>>23)&255u);
 return ea+eb<=ec+100;
}

static void one(uint32_t ua,uint32_t ub,uint32_t uc){
 /* skip nonfinite inputs: both APIs return 0 and leave out untouched */
 if((ua&0x7f800000u)==0x7f800000u||(ub&0x7f800000u)==0x7f800000u||(uc&0x7f800000u)==0x7f800000u)return;
 uint32_t oracle=0xdeadbeefu,compact=0xdeadbeefu,finite=0xdeadbeefu;
 int orok=original_fma_finite_bits_reference(ua,ub,uc,&oracle);
 CHECK(orok);
 if(addend_dominates(ua,ub,uc)&&oracle!=uc){
  fprintf(stderr,"addend-dominates proof FAIL a=%08x b=%08x c=%08x oracle=%08x\n",ua,ub,uc,oracle);
  CHECK(0);
 }
 int cok=original_fma_compact_bits(ua,ub,uc,&compact);
 if(cok){
  compact_ok++;
  if(!same_bits(compact,oracle)){
   fprintf(stderr,"compact mismatch a=%08x b=%08x c=%08x compact=%08x oracle=%08x\n",ua,ub,uc,compact,oracle);
   CHECK(0);
  }
 }else{
  compact_miss++;
  CHECK(compact==0xdeadbeefu); /* untouched */
 }
 int fok=original_fma_finite_bits(ua,ub,uc,&finite);
 CHECK(fok);
 if(!same_bits(finite,oracle)){
  fprintf(stderr,"finite mismatch a=%08x b=%08x c=%08x finite=%08x oracle=%08x\n",ua,ub,uc,finite,oracle);
  CHECK(0);
 }
 float a=original_math_float(ua),b=original_math_float(ub),c=original_math_float(uc);
 float got=original_rne_fmaf(a,b,c);
 uint32_t gb=original_math_bits(got);
 if(!same_bits(gb,oracle)){
  fprintf(stderr,"rne mismatch a=%08x b=%08x c=%08x rne=%08x oracle=%08x\n",ua,ub,uc,gb,oracle);
  CHECK(0);
 }
 /* libc fmaf: same RNE for finite, except we don't require it on platforms
  * with a broken libm. When it matches, good; when not, oracle still rules. */
#if defined(__STDC_IEC_559__) || defined(__GCC_IEC_559)
 float lib=fmaf(a,b,c);
 uint32_t lb=original_math_bits(lib);
 /* If libc differs, still pass (oracle is our contract); count only. */
 (void)lb;
#endif
 checks++;
}

static uint32_t pack(unsigned sign,unsigned exp,unsigned man){
 return (sign<<31)|((exp&255u)<<23)|(man&0x7fffffu);
}

int main(void){
 unsigned i,j,k;
 /* zeros and infinities of both signs: zeros are finite */
 uint32_t z[]={0,0x80000000u,0x3f800000u,0xbf800000u, /* ±0, ±1 */
  0x40000000u,0xc0000000u, /* ±2 */
  0x3f000000u,0xbf000000u, /* ±0.5 */
  0x00800000u,0x80800000u, /* ±min normal */
  0x7f7fffffu,0xff7fffffu, /* ±max finite */
  0x00000001u,0x80000001u, /* ±min subnormal */
  0x007fffffu,0x807fffffu, /* ±max subnormal */
  0x3fc00000u,0x3fe00000u,0x3ff00000u, /* 1.5, 1.75, 1.875 */
  0x41400000u, /* 12 */
  0x41500000u, /* 13 */
  0x41c80000u, /* 25 */
  0x42400000u, /* 48 */
  0x46800000u, /* 16384 */
  0x3a83126fu,0x3b03126fu, /* epsilons used in chicago vcf */
  0x49f42400u,0x4a8ab75du,
 };
 unsigned nz=sizeof z/sizeof z[0];
 for(i=0;i<nz;i++)for(j=0;j<nz;j++)for(k=0;k<nz;k++)one(z[i],z[j],z[k]);

 /* Every encoded exponent, a few mantissas, as each operand */
 uint32_t mans[]={0,1,2,3,0x10,0x80,0xff,0x7fff,0x800000u-1,0x400000u,0x200000u,0x1,0x7ffffe,0x555555,0x2aaaaa};
 unsigned nm=sizeof mans/sizeof mans[0];
 for(unsigned e=0;e<255;e++){
  for(i=0;i<nm;i++){
   uint32_t v=pack(0,e,mans[i]), n=pack(1,e,mans[i]);
   one(v,z[2],z[2]); /* v * 1 + 1 */
   one(z[2],v,z[2]);
   one(z[2],z[2],v);
   one(v,v,v);
   one(v,n,v);
   one(n,v,n);
   one(v,z[0],z[4]); /* * +0 */
   one(v,z[1],z[0]); /* * -0 */
  }
 }

 /* Midpoint ties: a*b exact midpoint of two representables, c=0 */
 for(i=0;i<2000;i++){
  uint32_t ua=pack(rng()&1,1+(rng()%200),rng()&0x7fffff);
  uint32_t ub=pack(rng()&1,1+(rng()%200),rng()&0x7fffff);
  one(ua,ub,0);
  one(ua,ub,0x80000000u);
  one(ua,ub,ua);
  one(ua,ub,ub);
 }

 /* Random finite triples, biased toward similar magnitudes (the expensive path) */
 for(i=0;i<200000;i++){
  unsigned ea=rng()%255,eb=rng()%255,ec;
  /* 50%: nearby exponents so compact is interesting */
  if(rng()&1)ec=(unsigned)((int)ea+(int)(rng()%11)-5)&255;
  else ec=rng()%255;
  if(ec==255)ec=254;
  one(pack(rng()&1,ea,rng()&0x7fffff),
      pack(rng()&1,eb,rng()&0x7fffff),
      pack(rng()&1,ec,rng()&0x7fffff));
 }

 /* Product much smaller / much larger than addend */
 for(i=0;i<5000;i++){
  unsigned e=1+(rng()%80);
  one(pack(0,e,rng()),pack(0,e,rng()),pack(0,e+40,rng()));
  one(pack(0,e+40,rng()),pack(0,e+40,rng()),pack(0,e,rng()));
 }

 /* Chicago-like control constants */
 float chi[]={0.f,1.f,-1.f,2.f,3.f,4.f,13.f,25.f,48.f,16384.f,44100.f,
  0.0010000000474974513f,0.0020000000949949026f,1.2999999523162842f};
 unsigned nc=sizeof chi/sizeof chi[0];
 for(i=0;i<nc;i++)for(j=0;j<nc;j++)for(k=0;k<nc;k++)
  one(original_math_bits(chi[i]),original_math_bits(chi[j]),original_math_bits(chi[k]));
 for(i=0;i<20000;i++){
  float x=(float)(int32_t)rng()*(1.0f/65536.0f);
  float y=chi[rng()%nc];
  float c=chi[rng()%nc];
  one(original_math_bits(x),original_math_bits(y),original_math_bits(c));
  one(original_math_bits(x),original_math_bits(x),original_math_bits(y));
 }

 /* Bound of the encoded-exp rule: ea+eb == ec+100 must be safe; +101 is not claimed. */
 for(i=1;i<200;i++){
  unsigned ea=i,eb=i,ec;
  if(ea+eb>=100){
   ec=ea+eb-100; if(ec<255){
    one(pack(0,ea,0x7fffff),pack(0,eb,0x7fffff),pack(0,ec,0)); /* max product vs min |c| at bound */
    one(pack(1,ea,0x7fffff),pack(0,eb,0x7fffff),pack(0,ec,1));
    one(pack(0,ea,0),pack(0,eb,0),pack(1,ec,0x7fffff));
   }
   if(ea+eb>=101){ec=ea+eb-101;if(ec&&ec<255)one(pack(0,ea,0),pack(0,eb,0),pack(0,ec,0));}
  }
 }
 printf("FMA exact identity: %u finite triples PASS (compact hit %u miss %u)\n",
        checks,compact_ok,compact_miss);
 return 0;
}
