/* SPDX-License-Identifier: GPL-3.0-only */
/* Host model of the exact sizing helper, not proof of SPL/USB/DMA ownership. */
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#define SMK_MODEL "SMK-37 Elite"
static uint8_t src[0x100000+96],flash[0x100000],before[0x100000];
static uint8_t hdr[1024] __attribute__((aligned(4))),sec[4096],cur[4096];
static unsigned erases,finish_bad,fail_read,ab_reads,mutate_source;
static int ota_read(uint32_t off,uint8_t *p,uint32_t n)
{
    if(off==0xF0000000u){if(finish_bad)return -1;memcpy(p,"success",8);return 0;}
    if(off==0xAB000 && ++ab_reads==2 && mutate_source)src[off]^=1;
    if(fail_read||off>sizeof src||n>sizeof src-off)return -1;
    memcpy(p,src+off,n);return 0;
}
static int ldr_fread(uint32_t off,uint8_t *p,uint32_t n){assert(off+n<=sizeof flash);memcpy(p,flash+off,n);return 0;}
static int allowed(uint32_t off){return (off>=0x4000&&off<0x9D000)||(off>=0xAB000&&off<0xAF000)||(off>=0xFE000&&off<0xFF000);}
static int ldr_erase(uint32_t off){assert(allowed(off)&&!(off&4095));memset(flash+off,255,4096);erases++;return 0;}
static int ldr_prog(uint32_t off,const uint8_t *p,uint32_t n){assert(allowed(off)&&!(off&255)&&n<=256);for(unsigned i=0;i<n;i++)flash[off+i]&=p[i];return 0;}
static void ldr_progress(uint32_t a,uint32_t b){(void)a;(void)b;}
static int ota_memeq(const uint8_t*a,const uint8_t*b,uint32_t n){return !memcmp(a,b,n);}
static uint32_t ota_rd32(const uint8_t*p){return p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static void ldr_sfc(uint8_t *p,uint32_t n,uint32_t base,uint32_t key)
{
    for(uint32_t i=0;i<n;i+=32){uint32_t k=key^((base+i)>>2);for(uint32_t j=0;j<32&&i+j<n;j++){p[i+j]^=(uint8_t)k;k=((k<<1)^(k&0x8000?0x1021:0))&65535;}}
}
#include "bp_manifest.h"
#include "bundle_probe.h"
static void reseal(void)
{
    bp_header *h=(bp_header *)(void *)(src+0x100000);uint32_t c=~0u;
    for(unsigned i=0;i<4;i++){h->p[i].crc=~bp_crc(~0u,src+h->p[i].off,h->p[i].size);c=bp_crc(c,src+h->p[i].off,h->p[i].size);}
    h->bundle_crc=~c;h->header_crc=0;h->header_crc=~bp_crc(~0u,(uint8_t*)h,sizeof *h);
}
static void fixture(void)
{
    memset(src,0x55,sizeof src);memset(flash,0xA5,sizeof flash);memcpy(before,flash,sizeof flash);
    erases=finish_bad=fail_read=ab_reads=mutate_source=0;
    bp_header *h=(bp_header *)(void *)(src+0x100000);memset(h,0,sizeof *h);
    h->magic=BP_MAGIC;h->version=1;h->model=~bp_crc(~0u,(const uint8_t*)SMK_MODEL,sizeof SMK_MODEL-1);h->policy=1;h->commit_off=BP_COMMIT_OFF;h->commit_size=128;h->appdesc_off=0x5000;h->image_id=7;
    const uint32_t off[]={0x4000,0xAB000,0xAD000,0xFE000},sz[]={0x99000,8192,8192,4096};
    for(unsigned i=0;i<4;i++){h->p[i].off=off[i];h->p[i].size=sz[i];}
    uint32_t desc[]={0x55493058,1,0xAB000,8192,~bp_crc(~0u,src+0xAB000,8192)};memcpy(src+0x5000,desc,sizeof desc);
    xb4_manifest_t *v=(xb4_manifest_t *)(void *)(src+BP_COMMIT_OFF);memset(v,0,sizeof *v);
    v->magic=XB4_MAGIC;v->version=XB4_VERSION;v->header_size=128;v->abi=4;v->model=h->model;v->contract=XB4_CONTRACT;v->flags=XB4_CENTRAL_TEST;v->image_id=7;
    v->init=XB4_XIP(BP_COMMIT_OFF)+128;v->poll=v->init+4;
    v->ram_lo=XB4_LOW_RAM_LO;v->ram_hi=v->ram_lo+4096;v->isr_stack_lo=v->ram_lo+256;v->isr_stack_hi=v->ram_lo+1024;v->data_ram_lo=v->data_ram_hi=XB4_DATA_RAM_LO;
    for(unsigned i=0;i<3;i++){v->segment[i].off=xb4_segment_off(i);v->segment[i].size=1024;v->segment[i].exec_size=i?0:512;unsigned skip=i?0:128;v->segment[i].crc32=~bp_crc(~0u,src+v->segment[i].off+skip,1024-skip);}
    v->manifest_crc32=xb4_manifest_crc(v);ldr_sfc(src+0x4000,0x99000,0,123);reseal();
}
int main(void)
{
    fixture();assert(!bp_install(0,sizeof src,123,hdr,sec,cur));assert(erases);
    for(unsigned i=0;i<sizeof flash;i++)assert(flash[i]==(allowed(i)?src[i]:before[i]));
    fixture();fail_read=1;assert(bp_install(0,sizeof src,123,hdr,sec,cur)&&!erases);
    fixture();src[0x100000]^=1;assert(bp_install(0,sizeof src,123,hdr,sec,cur)&&!erases);
    fixture();src[0xAB000]^=1;assert(bp_install(0,sizeof src,123,hdr,sec,cur)&&!erases);
    fixture();finish_bad=1;assert(bp_install(0,sizeof src,123,hdr,sec,cur));assert(!memcmp(flash+0xBFF00,before+0xBFF00,112));
    fixture();mutate_source=1;assert(bp_install(0,sizeof src,123,hdr,sec,cur));for(unsigned i=0;i<128;i++)assert(flash[BP_COMMIT_OFF+i]==255);assert(!memcmp(flash+0xBFF00,before+0xBFF00,112));
    puts("bundle sizing helper: forward/scope/CRC/source-change/finish failure PASS");
}
