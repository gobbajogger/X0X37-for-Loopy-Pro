/* SPDX-License-Identifier: GPL-3.0-only */
#define ORIGINAL_FUSED_REFERENCE_IMPL 1
#include "fused_reference.h"
#include "binary32_math.h"
#include <stddef.h>
#include <string.h>
/* Exact fixed-point scale2^-298 covers product of smallest subnormals through
 * product of largest finite floats.18limbs cover all576possible positions. */
#define LIMBS 18
static void deposit(uint32_t x[LIMBS],uint64_t mant,unsigned shift) {
 while(mant){unsigned word=shift/32,bit=shift%32;x[word]|=(uint32_t)mant<<bit;
  if(bit&&word+1<LIMBS)x[word+1]|=(uint32_t)mant>>(32-bit);
  mant>>=32;shift+=32;}
}
static unsigned bit_at(const uint32_t x[LIMBS],unsigned bit){return x[bit/32]>>(bit%32)&1u;}
static void unpack(uint32_t u,uint32_t *mant,int *exponent) {
 unsigned e=(u>>23)&255u;*mant=u&0x7fffffu;
 if(e){*mant|=0x800000u;*exponent=(int)e-150;}else *exponent=-149;
}
int original_fma_finite_bits_reference(uint32_t a,uint32_t b,uint32_t c,uint32_t *out) {
 if(!out||(a&0x7f800000u)==0x7f800000u||(b&0x7f800000u)==0x7f800000u||(c&0x7f800000u)==0x7f800000u)return 0;
 uint32_t x[LIMBS]={0},y[LIMBS]={0},ma,mb,mc;int ea,eb,ec;
 unpack(a,&ma,&ea);unpack(b,&mb,&eb);unpack(c,&mc,&ec);
 unsigned sign=(a^b)>>31,csign=c>>31;
 deposit(x,(uint64_t)ma*mb,(unsigned)(ea+eb+298));deposit(y,mc,(unsigned)(ec+298));
 if(sign==csign){uint64_t carry=0;for(unsigned i=0;i<LIMBS;i++){uint64_t sum=(uint64_t)x[i]+y[i]+carry;x[i]=(uint32_t)sum;carry=sum>>32;}}
 else{
  int cmp=0;for(int i=LIMBS-1;i>=0;i--)if(x[i]!=y[i]){cmp=x[i]>y[i]?1:-1;break;}
  if(!cmp){*out=0;return 1;}
  if(cmp<0){for(unsigned i=0;i<LIMBS;i++){uint32_t t=x[i];x[i]=y[i];y[i]=t;}sign=csign;}
  uint64_t borrow=0;for(unsigned i=0;i<LIMBS;i++){uint64_t sub=(uint64_t)y[i]+borrow;uint64_t old=x[i];x[i]=(uint32_t)(old-sub);borrow=old<sub;}
 }
 int high=LIMBS*32-1;while(high>=0&&!bit_at(x,(unsigned)high))high--;
 if(high<0){*out=sign<<31;return 1;}
 unsigned shift=high>172?(unsigned)high-23:149u; /* minsub unit2^-149 */
 uint32_t q=0;for(unsigned i=0;i<24;i++)q|=bit_at(x,shift+i)<<i;
 unsigned halfway=bit_at(x,shift-1),sticky=0;
 for(unsigned i=0;i<shift-1&&!sticky;i++)sticky=bit_at(x,i);
 if(halfway&&(sticky||(q&1)))q++;
 if(q>=0x1000000u){q>>=1;shift++;}
 if(!q){*out=sign<<31;return 1;}
 if(shift==149&&q<0x800000u){*out=(sign<<31)|q;return 1;}
 int exponent=(int)shift-275;
 if(exponent>127){*out=(sign<<31)|0x7f800000u;return 1;}
 *out=(sign<<31)|((uint32_t)(exponent+127)<<23)|(q-0x800000u);return 1;
}

/* Compact exact alignment domain. Each aligned magnitude is <2^62, so
 * addition is <2^63. Outside that domain retain the complete18-limb oracle. */
static unsigned width64(uint64_t x){
 unsigned n=0;if(x>>32){x>>=32;n+=32;}if(x>>16){x>>=16;n+=16;}
 if(x>>8){x>>=8;n+=8;}if(x>>4){x>>=4;n+=4;}if(x>>2){x>>=2;n+=2;}
 if(x>>1){x>>=1;n++;}return n+(x!=0);
}
static unsigned trailing64(uint64_t x){
 unsigned n=0;if(!(uint32_t)x){x>>=32;n+=32;}if(!(x&65535u)){x>>=16;n+=16;}
 if(!(x&255u)){x>>=8;n+=8;}if(!(x&15u)){x>>=4;n+=4;}if(!(x&3u)){x>>=2;n+=2;}
 if(!(x&1u))n++;
 return n;
}
int original_fma_compact_bits(uint32_t a,uint32_t b,uint32_t c,uint32_t *out){
 if(!out||(a&0x7f800000u)==0x7f800000u||(b&0x7f800000u)==0x7f800000u||(c&0x7f800000u)==0x7f800000u)return 0;
 uint32_t ma,mb,mc;int ea,eb,ec;unpack(a,&ma,&ea);unpack(b,&mb,&eb);unpack(c,&mc,&ec);
 uint64_t product=(uint64_t)ma*mb,addend=mc;unsigned sign=(a^b)>>31,csign=c>>31;
 if(!product&&!addend){*out=(sign==csign?sign:0u)<<31;return 1;}
 int ep=ea+eb;
 /* Less than one quarter of the addend's encoded ulp cannot cross either
  * RNE midpoint, including the narrower interval below a binade boundary.
  * Strict exponent bound, no tie shortcut; valid for subnormal addends too. */
 if(addend&&(!product||ep+(int)width64(product)<=ec-2)){*out=c;return 1;}
 /* Remove only exact powers of two, avoiding unnecessary alignment width
  * when table differences or control constants have short significands. */
 if(product){unsigned z=trailing64(product);product>>=z;ep+=(int)z;}
 if(addend){unsigned z=trailing64(addend);addend>>=z;ec+=(int)z;}
 int base=product?(addend?(ep<ec?ep:ec):ep):ec;
 unsigned sp=product?(unsigned)(ep-base):0,sc=addend?(unsigned)(ec-base):0;
 if(sp>62||sc>62||width64(product)+sp>62||width64(addend)+sc>62)return 0;
 product<<=sp;addend<<=sc;uint64_t magnitude;
 if(sign==csign)magnitude=product+addend;
 else if(product>=addend)magnitude=product-addend;
 else{magnitude=addend-product;sign=csign;}
 if(!magnitude){*out=0;return 1;}
 int high=(int)width64(magnitude)-1,shift=high-23;
 if(base+shift< -149)shift=-149-base;
 uint64_t q;
 if(shift<=0)q=magnitude<<(unsigned)(-shift);
 else if(shift>64)q=0;
 else if(shift==64){uint64_t half=UINT64_C(1)<<63;q=magnitude>half?1:0;}
 else{
  q=magnitude>>(unsigned)shift;uint64_t mask=(UINT64_C(1)<<(unsigned)shift)-1,rem=magnitude&mask,half=UINT64_C(1)<<(unsigned)(shift-1);
  if(rem>half||(rem==half&&(q&1)))q++;
 }
 int unit=base+shift;
 if(q>=0x1000000u){q>>=1;unit++;}
 if(!q){*out=sign<<31;return 1;}
 if(unit== -149&&q<0x800000u){*out=(sign<<31)|(uint32_t)q;return 1;}
 int exponent=unit+150;
 if(exponent>=255){*out=(sign<<31)|0x7f800000u;return 1;}
 *out=(sign<<31)|((uint32_t)exponent<<23)|((uint32_t)q-0x800000u);return 1;
}
int original_fma_finite_bits(uint32_t a,uint32_t b,uint32_t c,uint32_t *out){
 if(original_fma_compact_bits(a,b,c,out))return 1;
 return original_fma_finite_bits_reference(a,b,c,out);
}

float original_rne_fmaf(float a,float b,float c) {
 uint32_t ua=original_math_bits(a),ub=original_math_bits(b),uc=original_math_bits(c),out=0x7fc00000u;
 (void)original_fma_finite_bits(ua,ub,uc,&out);return original_math_float(out);
}
