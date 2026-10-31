/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static unsigned yields;
#define BM_PROGRESS() (++yields)
#include "app/boot_media.h"
static unsigned checks;static uint8_t data[220000],raw[192000],stream[216000];
#define C(x) do{checks++;if(!(x)){printf("FAIL line%d %s\n",__LINE__,#x);return 1;}}while(0)
static void u16(uint8_t*p,unsigned x){p[0]=x;p[1]=x>>8;}
static void u32(uint8_t*p,unsigned x){u16(p,x);u16(p+2,x>>16);}
static unsigned pack(unsigned frames,unsigned ms,unsigned n){memset(data,0,184);data[0]=3;data[1]=data[2]=1;data[3]=16;u16(data+4,80);u16(data+6,80);u16(data+8,frames);u16(data+10,frames*ms);u32(data+12,n);u32(data+16,frames*3200);u32(data+20,~x0x_crc32_upd(~0u,stream,n));u32(data+24,~x0x_crc32_upd(~0u,raw,frames*3200));for(unsigned i=0;i<frames;i++)u16(data+64+2*i,ms);memcpy(data+64+2*frames,stream,n);return 64+2*frames+n;}
static int valid(unsigned n,unsigned cap){bm_reader_t r=bm_memory(data,n);return bm_validate(&r,cap,0);}
/* Token writer controls every compressed edge; CRC/header are always coherent. */
static unsigned sp,flagpos,bit,out;
static void init(void){sp=bit=out=0;memset(raw,0,sizeof raw);}
static void start(void){if(!bit){flagpos=sp++;stream[flagpos]=0;}}
static void lit(uint8_t v){start();stream[flagpos]|=1u<<bit;stream[sp++]=v;raw[out++]=v;bit=(bit+1)&7;}
static void ref(unsigned dist,unsigned len){start();u16(stream+sp,((dist-1)<<4)|(len-3));sp+=2;for(unsigned i=0;i<len;i++){raw[out]=raw[out-dist];out++;}bit=(bit+1)&7;}
static void fill(unsigned n){while(out<n)lit(0);}
int main(void){unsigned n;
 init();lit(0x11);while(out+18<=3200)ref(1,18);fill(3200);n=pack(1,1000,sp);C(valid(n,BM_CUSTOM_MAX)==1);C(yields>=12); /* overlapping distance1 */
 init();fill(1024);ref(1024,18);fill(3200);n=pack(1,100,sp);C(valid(n,BM_COMPILED_MAX)==1);
 init();fill(1025);ref(1025,18);fill(3200);n=pack(1,100,sp);C(valid(n,BM_COMPILED_MAX)==0);
 init();lit(0);ref(1,18);fill(3200);/* first reference now distance2 > produced1 */u16(stream+2,0x001F);n=pack(1,100,sp);C(valid(n,BM_COMPILED_MAX)==0);
 init();fill(3199);ref(1,3);n=pack(1,100,sp);C(valid(n,BM_COMPILED_MAX)==0); /* overshoot */
 init();lit(0);n=pack(1,100,sp-1);C(valid(n,BM_COMPILED_MAX)==0); /* literal underflow */
 init();lit(0);ref(1,18);n=pack(1,100,sp-1);C(valid(n,BM_COMPILED_MAX)==0); /* backref underflow */
 init();lit(0);while(out+18<=3200)ref(1,18);fill(3200);stream[sp++]=0;n=pack(1,100,sp);C(valid(n,BM_COMPILED_MAX)==0);
 init();lit(0);while(out+18<=3200)ref(1,18);fill(3200);C(bit!=0);stream[flagpos]|=0x80;n=pack(1,100,sp);C(valid(n,BM_COMPILED_MAX)==0);
 init();fill(3200);raw[0]=stream[1]=0x11;n=pack(1,100,sp);data[3]=1;C(valid(n,BM_COMPILED_MAX)==0);
 init();lit(0);while(out+18<=192000)ref(1,18);fill(192000);n=pack(60,100,sp);C(valid(n,BM_COMPILED_MAX)==1);u16(data+8,61);C(valid(n,BM_COMPILED_MAX)==0);
 init();lit(0);while(out+18<=3200)ref(1,18);fill(3200);n=pack(1,1000,sp);C(valid(n,n)==1);C(valid(n,n-1)==0);u16(data+64,1001);u16(data+10,1001);C(valid(n,BM_COMPILED_MAX)==0);u16(data+64,99);u16(data+10,99);C(valid(n,BM_COMPILED_MAX)==0);
 { const unsigned cases[4][5]={{3840,6400,622,3,4},{3841,6400,625,4,4},{32768,32000,24039,3,3},{32769,32000,24042,3,3}};
 for(unsigned c=0;c<4;c++){unsigned target=cases[c][0],total=cases[c][1],l=cases[c][4];init();fill(cases[c][2]);ref(1,cases[c][3]);while(out+l<=total)ref(1,l);fill(total);n=pack(total/3200,100,sp);C(n==target);C(valid(n,c<2?BM_CUSTOM_MAX:BM_COMPILED_MAX)==!(c&1));}
 }
 printf("PASS %u actual decoder edge checks; progress=%u\n",checks,yields);return 0;
}
