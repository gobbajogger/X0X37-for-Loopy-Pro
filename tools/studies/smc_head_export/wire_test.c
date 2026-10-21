/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#define BOARD_SMK37 1
#define SMK_MODEL "SMK-37 Elite"
#define FELUCCA_ID "ota-SMK-37 Elite_900"
#define FELUCCA_OTA_DRYRUN 0
#define FELUCCA_LOADER 1
static uint8_t source[0x120000],nor[0x100000],before[0x100000],rx[1024],reply[512];
static uint32_t slen,rxlen,ready,now,erases,clears,replylen;
static int failfinish;
static uint32_t pack(const uint8_t*p,uint32_t n,uint8_t*out){uint32_t a=0,b=0,o=0;while(n--){a|=(uint32_t)*p++<<b;b+=8;while(b>=7){out[o++]=a&127;a>>=7;b-=7;}}if(b)out[o++]=a&127;return o;}
static uint32_t unpack(const uint8_t*p,uint32_t n,uint8_t*out){uint32_t a=0,b=0,o=0;while(n--){a|=(uint32_t)*p++<<b;b+=7;if(b>=8){out[o++]=a;a>>=8;b-=8;}}return o;}
static int ota_wire_send(const uint8_t*p,uint32_t n){
 uint8_t u[512],m[600]={0};uint32_t d=unpack(p+1,n-2,u);assert(d>=7);
 if(u[2]==0x40){memcpy(reply,u+6,d-7);replylen=d-7;return 0;}
 assert(u[2]==0x30&&d==15);uint32_t off=u[7]|(uint32_t)u[8]<<8|(uint32_t)u[9]<<16|(uint32_t)u[10]<<24,k=u[11]|(uint32_t)u[12]<<8;
 assert(k<=512);memcpy(m,"\0\x59\x30",3);m[3]=k+8;m[4]=(k+8)>>8;memcpy(m+7,&off,4);m[11]=k;m[12]=k>>8;
 if(off==0xE0000000){assert(k==8);memcpy(m+14,"success",8);}else{assert(off<slen&&k<=slen-off);memcpy(m+14,source+off,k);}
 unsigned sum=0;for(unsigned i=6;i<14+k;i++)sum+=m[i];m[14+k]=~sum;rxlen=pack(m,15+k,rx);ready=1;return 0;
}
static int ota_frame_get(const uint8_t**p,uint32_t*n){if(!ready)return 0;*p=rx;*n=rxlen;return 1;}
static void ota_frame_done(void){ready=0;}
static uint32_t ota_now_ms(void){return now;}
static void ota_idle(void){now++;}
static int ota_erase(uint32_t o){assert(o>=0xBB000&&o<0xC0000);memset(nor+o,255,4096);return 0;}
static int ota_prog(uint32_t o,const void*p,uint32_t n){assert(o>=0xBB000&&o+n<=0xC0000);memcpy(nor+o,p,n);return 0;}
static int ota_fread(uint32_t o,void*p,uint32_t n){assert(o+n<=sizeof nor);memcpy(p,nor+o,n);return 0;}
static void ota_show(uint32_t a,int32_t b){(void)a;(void)b;}
static void ota_commit(const uint8_t*p){(void)p;}
#include HX_OTA_SOURCE
static int ldr_fread(uint32_t o,void*p,uint32_t n){return ota_fread(o,p,n);}
static int ldr_erase(uint32_t o){assert(o==0xBF000);erases++;if(failfinish){memset(nor+o,255,2048);return -1;}memset(nor+o,255,4096);return 0;}
static void ldr_record_clear(void){clears++;}
static void ldr_progress(uint32_t a,uint32_t b){(void)a;(void)b;}
static uint32_t ldr_ms(void){return now;}
#include HX_CORE_SOURCE
static void req(unsigned cmd,unsigned nonce,unsigned off,unsigned n){uint8_t b[23]={0,0x59,0x40,16,0,0};ota_wr32(b+6,cmd);ota_wr32(b+10,nonce);ota_wr32(b+14,off);ota_wr32(b+18,n);unsigned sum=0;for(unsigned i=6;i<22;i++)sum+=b[i];b[22]=~sum;rxlen=pack(b,23,rx);ready=1;replylen=0;hx_service();}
int main(int argc,char**argv){assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f);slen=fread(source,1,sizeof source,f);fclose(f);assert(slen>0x10000);
 for(unsigned i=0;i<sizeof nor;i++)nor[i]=(uint8_t)(i*31+7);
 assert(!ota_stage()); /* exact old app staging primitives, no main writes */
 uint8_t rec[112]={0};ota_wr16(rec+2,0x5A0D);ota_wr16(rec+4,0x5A01);ota_wr16(rec+6,0x5441);memcpy(rec+8,OTA_PARM_ID,sizeof OTA_PARM_ID-1);ota_wr32(rec+72,OTA_AREA);ota_wr16(rec,ota_crc16(rec+2,78,0));memcpy(nor+OTA_RES,rec,112);
 memcpy(before,nor,sizeof nor);req(0,123,0,0);assert(replylen==28&&ota_rd32(reply+24)==5);
 req(HX_INFO,123,0,0);assert(!replylen);
 uint32_t fo,fs,oo,os;uint8_t h[1024];assert(!ota_ufw(h,&fo,&fs,&oo,&os));
 source[oo+1000]^=1;assert(hx_admit()==-72&&!hx_admitted&&!erases);source[oo+1000]^=1;
 assert(!hx_admit());
 req(HX_INFO,123,0,0);assert(replylen==28);uint32_t crc=ota_rd32(reply+12);
 for(unsigned off=0;off<HX_SIZE;off+=HX_CHUNK){unsigned n=HX_SIZE-off;if(n>HX_CHUNK)n=HX_CHUNK;req(HX_READ,123,off,n);assert(replylen==28+n&&!memcmp(reply+28,nor+off,n));}
 nor[OTA_RES+20]^=1;req(HX_FINISH,123,crc,0);assert(!replylen&&!erases&&!clears);nor[OTA_RES+20]^=1;
 failfinish=1;req(HX_FINISH,123,crc,0);assert(!replylen&&!clears);failfinish=0;
 req(HX_FINISH,123,crc,0);assert(replylen==28&&clears==1);
 for(unsigned i=0;i<sizeof nor;i++)assert(nor[i]==(i>=0xBF000&&i<0xC0000?255:before[i]));
 puts("actual ota_stage + exporter wire: capability/admission, full head, foreign record, torn erase retry, protected NOR PASS");
}
