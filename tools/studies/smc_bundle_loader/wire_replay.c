/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual ota.c read-response and ldr_core.c replay, PRIVATE fixture bytes.
 * Header framing exists only in memory. No UFW/FWSC writer or hardware access. */
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
#define SOURCE_SIZE (0x100000u+96u)
#define SOURCE_BASE 0x400u
#define LOADER_BASE 0x110000u
static _Alignas(4) uint8_t source[SOURCE_SIZE],loader[16384],head[1024],nor[0x100000],before[0x100000];
static uint32_t policy,commit_off;
static uint32_t loader_size,rx_len,rx_full,now_ms,requests,erases,cleared,ab_reads;
static unsigned bad_finish,bad_addr,bad_len,swap_source,overflows;
static uint8_t rx[1024];
static int serve(uint32_t off,uint8_t *p,uint32_t n);
static uint32_t pack7(const uint8_t*p,uint32_t n,uint8_t*out)
{uint32_t acc=0,nb=0,o=0;while(n--){acc|=(uint32_t)*p++<<nb;nb+=8;while(nb>=7){out[o++]=acc&127;acc>>=7;nb-=7;}}if(nb)out[o++]=acc&127;return o;}
static uint32_t unpack7(const uint8_t*p,uint32_t n,uint8_t*out)
{uint32_t acc=0,nb=0,o=0;while(n--){acc|=(uint32_t)*p++<<nb;nb+=7;if(nb>=8){out[o++]=acc;acc>>=8;nb-=8;}}return o;}
static int ota_wire_send(const uint8_t*p,uint32_t n)
{
    uint8_t u[64]={0},m[600]={0};uint32_t d=unpack7(p+1,n-2,u),sum=0;
    assert(d==15&&u[2]==0x30);uint32_t addr=u[7]|(uint32_t)u[8]<<8|(uint32_t)u[9]<<16|(uint32_t)u[10]<<24;
    uint32_t len=u[11]|(uint32_t)u[12]<<8|(uint32_t)u[13]<<16;assert(len<=512);requests++;
    if(bad_len&&len)len--;
    if(serve(addr,m+14,len))return 0; /* host refuses unbound/overflow read */
    uint32_t echo=bad_addr?addr^4u:addr;
    memcpy(m,"\0\x59\x30",3);m[3]=(uint8_t)(len+8);m[4]=(len+8)>>8;
    memcpy(m+7,&echo,4);m[11]=len;m[12]=len>>8;
    for(unsigned i=6;i<14+len;i++)sum+=m[i];
    m[14+len]=(uint8_t)~sum;
    rx_len=pack7(m,15+len,rx);rx_full=1;return 0;
}
static int ota_frame_get(const uint8_t **p,uint32_t*n){if(!rx_full)return 0;*p=rx;*n=rx_len;return 1;}
static void ota_frame_done(void){rx_full=0;}
static uint32_t ota_now_ms(void){return now_ms;}
static void ota_idle(void){now_ms++;}
static int ota_erase(uint32_t o){(void)o;return -1;}
static int ota_prog(uint32_t o,const void*p,uint32_t n){(void)o;(void)p;(void)n;return -1;}
static int ota_fread(uint32_t o,void*p,uint32_t n){if(o>sizeof nor||n>sizeof nor-o)return -1;memcpy(p,nor+o,n);return 0;}
static void ota_show(uint32_t a,int32_t b){(void)a;(void)b;}
static void ota_commit(const uint8_t*p){(void)p;}
#include BP_OTA_SOURCE
static int ldr_fread(uint32_t o,void*p,uint32_t n){return ota_fread(o,p,n);}
static int writable(uint32_t o,uint32_t n)
{
    const uint32_t lo[]={0x4000,0xAB000,0xAD000,0xFE000,0xBF000},hi[]={0x9D000,0xAD000,0xAF000,0xFF000,0xC0000};
    for(unsigned i=0;i<5;i++)if(o>=lo[i]&&o<hi[i]&&n<=hi[i]-o)return 1;
    return 0;
}
static int ldr_erase(uint32_t o){assert(!(o&4095)&&writable(o,4096));erases++;memset(nor+o,255,4096);return 0;}
static int ldr_prog(uint32_t o,const void*p,uint32_t n){assert(writable(o,n)&&n<=256&&!(o&255));const uint8_t*b=p;for(unsigned i=0;i<n;i++)nor[o+i]&=b[i];return 0;}
static void ldr_record_clear(void){cleared++;}
static void ldr_progress(uint32_t a,uint32_t b){(void)a;(void)b;}
#include BP_LDR_SOURCE
static int serve(uint32_t off,uint8_t*p,uint32_t n)
{
    if(off==0xF0000000u){if(n!=8)return -1;memcpy(p,bad_finish?"failure":"success",8);return 0;}
    const uint8_t*b=0;
    if(off<sizeof head&&n<=sizeof head-off)b=head+off;
    else if(off>=SOURCE_BASE&&off-SOURCE_BASE<SOURCE_SIZE&&n<=SOURCE_SIZE-(off-SOURCE_BASE))b=source+off-SOURCE_BASE;
    else if(off>=LOADER_BASE&&off-LOADER_BASE<sizeof loader&&n<=sizeof loader-(off-LOADER_BASE)&&off-LOADER_BASE<loader_size&&n<=loader_size-(off-LOADER_BASE))b=loader+off-LOADER_BASE;
    if(!b){overflows++;return -1;}memcpy(p,b,n);
    if(off==SOURCE_BASE+0xAB000 && ++ab_reads==2 && swap_source)p[0]^=1;
    return 0;
}
static void framing(uint32_t source_base)
{
    memset(head,0,sizeof head);ota_wr16(head+8,2);ota_wr32(head+4,LOADER_BASE+loader_size);
    uint8_t *e=head+64;ota_wr16(e,0);ota_wr32(e+8,source_base);ota_wr32(e+12,SOURCE_SIZE);ota_jl_enc(e,80);
    e+=80;ota_wr16(e,100);ota_wr32(e+8,LOADER_BASE);ota_wr32(e+12,loader_size);ota_jl_enc(e,80);
    ota_wr16(head+2,ota_crc16(head+64,160,0));ota_wr16(head,ota_crc16(head+2,62,0));ota_jl_enc(head,64);
}
static void reset(void)
{
    memset(nor,0xA5,sizeof nor);memcpy(nor,source,0x4000);
    memset(nor+OTA_AREA,255,OTA_AREA_LEN);memcpy(nor+OTA_AREA+32,loader+32,loader_size-32);
    uint8_t*h=nor+OTA_AREA;memset(h,0,32);ota_wr16(h+2,65535);ota_wr32(h+4,OTA_AREA);ota_wr32(h+8,loader_size);
    h[12]=0x41;h[13]=255;ota_wr16(h+14,65535);memcpy(h+16,"LOADER.BIN",10);ota_wr16(h,ota_crc16(h+2,30,0));
    uint8_t*r=nor+OTA_RES;memset(r,0,112);ota_wr16(r+2,0x5A0D);ota_wr16(r+4,0x5A01);ota_wr16(r+6,0x5441);
    memcpy(r+8,OTA_PARM_ID,sizeof OTA_PARM_ID-1);ota_wr32(r+72,OTA_AREA);ota_wr16(r,ota_crc16(r+2,78,0));
    memcpy(before,nor,sizeof nor);now_ms=requests=erases=cleared=ab_reads=rx_full=0;
    bad_finish=bad_addr=bad_len=swap_source=overflows=0;framing(SOURCE_BASE);
}
static void load(const char*path,uint8_t*p,size_t cap,size_t exact,size_t*n)
{FILE*f=fopen(path,"rb");assert(f);*n=fread(p,1,cap,f);assert(fgetc(f)==EOF&&(!exact||*n==exact));fclose(f);}
static void installed_baseline(void)
{
    uint32_t key,c=~0u;uint8_t b[512];assert(!ldr_chip_key(&key));
    for(uint32_t k=0;k<0x99000u;k+=sizeof b) {
        memcpy(b,nor+0x4000u+k,sizeof b);
        if(k<BP_RB_CRYPT_SIZE) {
            uint32_t n=BP_RB_CRYPT_SIZE-k;if(n>sizeof b)n=sizeof b;
            ldr_sfc(b,n,k,key);
        }
        c=bp_crc(c,b,sizeof b);
    }
    assert(~c==BP_RB_MAIN_CRC); /* exact canonical approved main, independently from NOR */
    memcpy(b,nor+0x8B000u,128);ldr_sfc(b,128,0x87000u,key);
    assert(ota_rd32(b)==0x43423058u && ota_rd16(b+4)==2 && ota_rd16(b+6)==68);
    for(uint32_t k=0xAB000;k<0xAF000;k++)assert(nor[k]==255);
    for(uint32_t k=0xFE000;k<0xFF000;k++)assert(nor[k]==255);
}
/* Reseal the transaction CRCs after a source mutation: the approved baseline
 * pin must still reject an internally consistent alternative main. */
static void reseal_source(void)
{
    bp_header *h=(bp_header *)(void *)(source+0x100000u);uint32_t all=~0u;
    for(unsigned i=0;i<4;i++) {
        h->p[i].crc=~bp_crc(~0u,source+h->p[i].off,h->p[i].size);
        all=bp_crc(all,source+h->p[i].off,h->p[i].size);
    }
    h->bundle_crc=~all;h->header_crc=0;h->header_crc=~bp_crc(~0u,(const uint8_t*)h,sizeof *h);
}
int main(int argc,char**argv)
{
    assert(argc==3);size_t n;load(argv[1],source,sizeof source,sizeof source,&n);load(argv[2],loader,sizeof loader,0,&n);loader_size=n;
    policy=ota_rd32(source+0x100000u+12);commit_off=ota_rd32(source+0x100000u+16);
    assert((policy==1&&commit_off==0x84000u)||(policy==2&&commit_off==0x8B000u));
    reset();int rc=ldr_session();printf("actual byte/wire policy%u rc=%d requests=%u erases=%u\n",policy,rc,requests,erases);assert(!rc&&cleared==1);
    if(policy==2)installed_baseline();
    for(unsigned i=0;i<sizeof nor;i++) {
        if(i>=0xBF000&&i<0xC0000)assert(nor[i]==255);
        else assert(nor[i]==(writable(i,1)?source[i]:before[i]));
    }
    reset();nor[OTA_AREA+100]^=1;assert(ldr_session()==-72&&!erases&&!cleared);
    reset();nor[OTA_RES+8]^=1;ota_wr16(nor+OTA_RES,ota_crc16(nor+OTA_RES+2,78,0));
    assert(ldr_session()==-72&&!erases&&!cleared); /* foreign but valid-CRC record */
    reset();bad_finish=1;assert(ldr_session()==-68&&!cleared);assert(!memcmp(nor+OTA_RES,before+OTA_RES,112));
    reset();bad_addr=1;assert(ldr_session()&&!erases&&!cleared);
    reset();bad_len=1;assert(ldr_session()&&!erases&&!cleared);
    reset();framing(0xFFFFFFF0u);assert(ldr_session()&&!erases&&!cleared);
    { static uint8_t h[1024] __attribute__((aligned(4))),a[4096],b[4096];
      assert(serve(0xFFFFFFF0u,a,512)&&overflows);
      assert(bp_install(0xFFFFFFF0u,SOURCE_SIZE,0x1234,h,a,b)==-60&&!erases); }
    reset();swap_source=1;assert(ldr_session()&&!cleared);assert(!memcmp(nor+OTA_RES,before+OTA_RES,112));
    for(unsigned i=0;i<128;i++)assert(nor[commit_off+i]==255);
    if(policy==2) {
        uint8_t saved[96];memcpy(saved,source+0x100000u,sizeof saved);
        source[0x5000]^=1;reseal_source();reset();assert(ldr_session()==-76&&!erases&&!cleared);
        source[0x5000]^=1;memcpy(source+0x100000u,saved,sizeof saved);
        puts("logical rollback: installed canonical approved main/68-byte ABI2, exact FF spares, resealed alternative-main rejection PASS");
    }
    puts("actual app/UI/vendor protocol replay: source swap, overflow, address/length/finish mismatch, protected outside ranges PASS");
    return 0;
}
