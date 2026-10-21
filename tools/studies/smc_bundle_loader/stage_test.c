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
    assert(argc==2);FILE *f=fopen(argv[1],"rb");assert(f);
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
    puts("actual ota.c: early gate, boot cleanup, legacy loader and uncertain record preserve NOR PASS");
    free(raw);free(logical);return 0;
}
