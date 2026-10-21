/* SPDX-License-Identifier: GPL-3.0-only */
/* Compile BP_BASE_OTA_TEST to the disposable export's existing ota_test.c. */
#include <assert.h>
#define BOARD_SMK37 1
#define T_MODEL 1
#define X0X_SMC_VENDOR_TEST 1
static int admitted;
#define X0X_BUNDLE_UPDATE_ALLOWED() admitted
#define main baseline_ota_test_main
#include BP_BASE_OTA_TEST
#undef main
int main(int argc,char **argv)
{
    assert(argc==3);FILE *f=fopen(argv[1],"rb");assert(f);
    uint8_t *raw=malloc(0x200000);size_t n=fread(raw,1,0x200000,f);fclose(f);assert(n>960);
    logical=malloc(n);for(unsigned i=0;i<20;i++)memcpy(logical+i*47,raw+i*48,47);
    memcpy(logical+940,raw+960,n-960);logical_len=n-20;
    uint8_t saved[0x5000];
    reset_dev();memset(nor+T_AREA,0x6A,sizeof saved);memcpy(saved,nor+T_AREA,sizeof saved);
    assert(ota_session()==-74 && !requests && !committed);
    assert(!memcmp(saved,nor+T_AREA,sizeof saved));
    (void)ota_boot_cleanup();assert(!memcmp(saved,nor+T_AREA,sizeof saved));
    admitted=1;assert(ota_session()==-7 && !committed); /* exact old own loader lacks bundle marker */
    assert(!memcmp(saved,nor+T_AREA,sizeof saved));
    /* Add marker in memory only, retaining the original own marker. An existing
     * uncertain record must reject BEFORE loader body/CRC writing/cleanup. */
    uint32_t body=ota_body_off();memcpy(logical+body+0x48,"X0X-BUNDLE-LDR1",16);
    assert(ota_session()==-75 && !committed);
    assert(!memcmp(saved,nor+T_AREA,sizeof saved));
    /* Positive staging of the actual resolved bundle-capable ota.bin. Modify
     * only in-memory UFW entry framing; no package file or installation. The
     * loader is staged, not executed: whole-bundle payload admission is tested
     * separately by forward/rollback_test. */
    f=fopen(argv[2],"rb");assert(f);uint8_t ota[16384];size_t on=fread(ota,1,sizeof ota,f);assert(feof(f));fclose(f);
    logical=realloc(logical,logical_len+sizeof ota);assert(logical);
    uint8_t h[64];memcpy(h,logical,64);ota_jl_enc(h,64);
    unsigned entries=ota_rd16(h+8),found=0;
    for(unsigned i=0;i<entries;i++) {
        uint8_t e[80];memcpy(e,logical+64+i*80,80);ota_jl_enc(e,80);
        if(ota_rd16(e)!=100)continue;
        uint32_t at=ota_rd32(e+8);assert(at<logical_len);
        memcpy(logical+at,ota,on);logical_len=at+on;
        ota_wr16(e+4,ota_crc16(ota,on,0));ota_wr32(e+12,on);ota_wr32(e+16,(on+31)&~31u);
        ota_jl_enc(e,80);memcpy(logical+64+i*80,e,80);found++;
    }
    assert(found==1);ota_wr32(h+4,logical_len);ota_wr16(h+2,ota_crc16(logical+64,entries*80,0));
    ota_wr16(h,ota_crc16(h+2,62,0));ota_jl_enc(h,64);memcpy(logical,h,64);
    reset_dev();admitted=1;assert(!ota_session()&&committed&&!bad_erase);
    assert(!memcmp(nor+T_AREA+32,ota+32,on-32));
    assert(!memcmp(nor+T_AREA_HI-256,committed_parm,112));
    assert(ota_rd16(committed_parm)==ota_crc16(committed_parm+2,78,0));
    puts("actual ota.c: positive resolved bundle-loader cold recovery staging PASS");
    puts("actual ota.c: early gate, boot cleanup, legacy loader and uncertain record preserve NOR PASS");
    free(raw);free(logical);return 0;
}
