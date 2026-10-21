/* SPDX-License-Identifier: GPL-3.0-only */
#define main forward_test_unused
#include "test.c"
#undef main
#define OTA_AREA 0xBB000u
#define OTA_AREA_LEN 0x5000u
#define OTA_RES 0xBFF00u
#define OTA_PARM_ID "ota-" SMK_MODEL "_016"
static unsigned cleared;
static void ldr_record_clear(void){cleared++;}
static uint32_t ota_rd16(const uint8_t*p){return p[0]|(uint32_t)p[1]<<8;}
static void ota_wr16(uint8_t*p,uint32_t x){p[0]=x;p[1]=x>>8;}
static void ota_wr32(uint8_t*p,uint32_t x){ota_wr16(p,x);ota_wr16(p+2,x>>16);}
static uint32_t ota_crc16(const uint8_t*p,uint32_t n,uint32_t c)
{while(n--){c^=(uint32_t)*p++<<8;for(unsigned i=0;i<8;i++)c=(c<<1)^(c&0x8000?0x1021:0);}return c&65535;}
#include "record_owner.h"
#include "update_gate.h"
static void record_fixture(void)
{
    fixture();cleared=0;
    uint8_t *r=flash+OTA_RES,*a=src+0xC4000,*h=flash+OTA_AREA;
    memset(r,0,112);ota_wr16(r+2,0x5A0D);ota_wr16(r+4,0x5A01);ota_wr16(r+6,0x5441);
    memcpy(r+8,OTA_PARM_ID,sizeof OTA_PARM_ID-1);ota_wr32(r+72,OTA_AREA);ota_wr16(r,ota_crc16(r+2,78,0));
    memset(a,0,544);ota_wr32(a+4,32);ota_wr32(a+8,512);ota_wr16(a,ota_crc16(a+2,30,0));
    ota_wr32(a+36,10000);ota_wr32(a+40,0x01C0A800);ota_wr16(a+32,ota_crc16(a+34,30,0));
    memcpy(a+0x48,"X0X-BUNDLE-LDR1",16);
    memset(h,0,32);ota_wr32(h+4,OTA_AREA);ota_wr32(h+8,544);h[12]=0x41;h[13]=255;ota_wr16(h+14,65535);memcpy(h+16,"LOADER.BIN",10);ota_wr16(h,ota_crc16(h+2,30,0));
    memcpy(h+32,a+32,512);
}
int main(void)
{
    bp_update_gate g={0};assert(!bp_update_allowed(&g));bp_update_recovery(&g);assert(bp_update_allowed(&g));bp_update_attempt(&g);assert(!bp_update_allowed(&g));bp_update_recovery(&g);assert(!bp_update_allowed(&g));
    record_fixture();assert(!bp_record_admit(0xC4000,544,hdr+384,sec,cur));
    flash[OTA_RES+72]^=1;ota_wr16(flash+OTA_RES,ota_crc16(flash+OTA_RES+2,78,0));
    assert(bp_record_admit(0xC4000,544,hdr+384,sec,cur)==-72);
    record_fixture();flash[OTA_AREA+100]^=1;assert(bp_record_admit(0xC4000,544,hdr+384,sec,cur)==-72);
    record_fixture();src[0xC4000+0x48]=0;assert(bp_record_admit(0xC4000,544,hdr+384,sec,cur)==-72);
    record_fixture();assert(!bp_record_admit(0xC4000,544,hdr+384,sec,cur));flash[OTA_RES+8]^=1;assert(bp_record_finish(hdr+384,sec)==-72&&!cleared);
    record_fixture();assert(!bp_record_admit(0xC4000,544,hdr+384,sec,cur));record_mode=1;
    assert(!bp_record_finish(hdr+384,sec)&&cleared==1);
    for(unsigned point=1;point<=2;point++) {
        record_fixture();assert(!bp_record_admit(0xC4000,544,hdr+384,sec,cur));
        record_mode=1;inject=1;cut_at=point;
        if(!setjmp(power_loss)){(void)bp_record_finish(hdr+384,sec);assert(0);}
        inject=0;assert(!cleared);
        /* Before erase: owned record resumes loader. After erase: cold SPL
         * may boot the already verified final app; RAM record was not cleared.
         * These are record states, not a simulation of vendor SPL execution. */
        if(point==1)assert(!memcmp(flash+OTA_RES,hdr+384,112));
        else for(unsigned i=0;i<112;i++)assert(flash[OTA_RES+i]==255);
    }
    record_fixture();assert(!bp_record_admit(0xC4000,544,hdr+384,sec,cur));
    flash_reads=0;fail_flash_at=1;assert(bp_record_finish(hdr+384,sec)==-71&&!cleared);
    assert(!memcmp(flash+OTA_RES,hdr+384,112));
    record_fixture();assert(!bp_record_admit(0xC4000,544,hdr+384,sec,cur));
    record_mode=1;flash_reads=0;fail_flash_at=2;
    assert(bp_record_finish(hdr+384,sec)==-71&&!cleared);
    for(unsigned i=0;i<112;i++)assert(flash[OTA_RES+i]==255); /* uncertain committed */
    /* Partial final-sector erase entering the record itself: final payload is
     * already committed; the uncertain record is NOT accepted as owned. The
     * staged loader body below BF000 remains intact. No SPL behavior asserted. */
    record_fixture();assert(!bp_record_admit(0xC4000,544,hdr+384,sec,cur));
    record_mode=inject=torn=1;cut_at=1;erase_prefix=0xF01;
    if(!setjmp(power_loss)){(void)bp_record_finish(hdr+384,sec);assert(0);}
    inject=torn=0;erase_prefix=0;assert(!cleared);
    assert(memcmp(flash+OTA_RES,hdr+384,112));
    assert(!memcmp(flash+OTA_AREA+32,src+0xC4000+32,512));
    assert(bp_record_admit(0xC4000,544,hdr+384,sec,cur)==-72);
    puts("record ownership, foreign valid-CRC record, source-loader mismatch, legacy marker, lifecycle gate PASS");
}
