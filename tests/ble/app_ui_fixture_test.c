/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "../../firmware/hal/x0x_app_ui_data.h"
int main(int argc,char **argv)
{
    uint8_t data[XAU_SIZE];xau_descriptor_t d;
    assert(argc==4);
    FILE *f=fopen(argv[1],"rb");assert(f);
    unsigned long offset=strtoul(argv[3],NULL,0);
    assert(!fseek(f,(long)offset,SEEK_SET));assert(fread(&d,1,sizeof d,f)==sizeof d);fclose(f);
    f=fopen(argv[2],"rb");assert(f);assert(fread(data,1,sizeof data,f)==sizeof data);assert(fgetc(f)==EOF);fclose(f);
    assert(!xau_validate(&d,data,sizeof data));
    assert(xau_validate(&d,data,sizeof data-1));
    for(unsigned i=0;i<sizeof d;i++) {
        ((uint8_t *)&d)[i]^=1;assert(xau_validate(&d,data,sizeof data));((uint8_t *)&d)[i]^=1;
    }
    for(unsigned i=0;i<XAU_SIZE;i++) {
        data[i]^=1;assert(xau_validate(&d,data,sizeof data));data[i]^=1;
    }
    puts("actual app UI fixture: descriptor bytes, truncation, all8192 payload corruptions PASS");
}
