/* SPDX-License-Identifier: GPL-3.0-only */
/* Native helper + actual store hooks. Transaction below is a model, NOT updater code. */
#define BOARD_SMK37 1
#define X0X_SMC_VENDOR_TEST 1
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/hal/x0x_app_ui_data.h"
#include "../firmware/hal/fm1_flash_map.h"
#include "../firmware/src/app/plat.h"
static uint8_t flash[0x100000], old[0x100000];
static int st_read(uint32_t o, void *p, uint32_t n) { memcpy(p,flash+o,n); return 0; }
static int st_erase(uint32_t o) { assert(FL_STORE_OK(o,4096)); memset(flash+o,255,4096); return 0; }
static int st_prog(uint32_t o,const void *p,uint32_t n) { assert(FL_STORE_OK(o,n)); memcpy(flash+o,p,n); return 0; }
#include "../firmware/src/storage.c"
/* Host protocol model: each step requires verified predecessor while app/core1
 * are parked in loader. A new boot alone does not authorize an active rewrite. */
static unsigned phase;
static int advance(unsigned step, int loader_quiet, int verified)
{
    if (!loader_quiet || !verified || step != phase+1 || step>4) return 0;
    phase=step; return 1;
}
int main(void)
{
    xau_descriptor_t d={XAU_MAGIC,XAU_VERSION,XAU_OFF,XAU_SIZE,0}, t;
    uint32_t i;
    memset(flash,0x5a,sizeof flash); d.crc=st_crc32(flash+XAU_OFF,XAU_SIZE);
    assert(xau_validate(&d,flash+XAU_OFF,XAU_SIZE)==0);
    assert(xau_validate(&d,flash+XAU_OFF,XAU_SIZE-1));
    assert(xau_validate(&d,0,XAU_SIZE));
    for(i=0;i<sizeof d;i++) {t=d;((uint8_t*)&t)[i]^=1;assert(xau_validate(&t,flash+XAU_OFF,XAU_SIZE));}
    for(i=0;i<XAU_SIZE;i++) {flash[XAU_OFF+i]^=1;assert(xau_validate(&d,flash+XAU_OFF,XAU_SIZE));flash[XAU_OFF+i]^=1;}
    /* Both erased and foreign owners preserve external UI/vendor data. */
    for(unsigned unknown=0;unknown<2;unknown++) {
        memset(flash,0x5a,sizeof flash); if(unknown)memset(flash+0xAF000,255,4096);
        memcpy(old,flash,sizeof flash); st_owner_check();
        for(i=0;i<sizeof flash;i++)
            if(!((i>=0x9D000&&i<0xAB000)||(i>=0xAF000&&i<0xB0000))) assert(flash[i]==old[i]);
    }
    for(i=0xAB000;i<0xAF000;i++) { assert(!FL_STORE_OK(i,1)); assert(!FL_LDR_RANGE_OK(i,1)); }
    assert(!FL_STORE_OK(0xAAFFF,2)); assert(FL_STORE_OK(0xAF000,4096));
    assert(FL_STORE_OK(0xB0000,0xB000)); /* sample slot unchanged */
    assert(st_save(OBJ_SET,"x",1)==0); {char c;assert(st_load(OBJ_SET,&c,1)==1&&c=='x');}
    /* Stage verified data -> compatible smaller app -> controller -> manifest.
     * Interruptions before each next step leave its prerequisite incomplete. */
    for(unsigned step=1;step<=4;step++) {
        assert(!advance(step,0,1)); assert(!advance(step,1,0));
        assert(!advance(step+1,1,1)); assert(advance(step,1,1));
    }
    assert(!advance(1,0,1)); /* active profile overwrite still forbidden */
    puts("app UI: CRC/mismatch/store preservation/update-order model PASS");
}
