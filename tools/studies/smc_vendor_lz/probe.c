/* SPDX-License-Identifier: GPL-3.0-only */
/* Host compressor/tests use the exact existing project codec. Private input/output. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../../firmware/src/app/lz.c"
static uint32_t st_crc32(const void *bytes,uint32_t n)
{
    const uint8_t *p=bytes;uint32_t c=~0u;
    while(n--){c^=*p++;for(unsigned b=0;b<8;b++)c=(c>>1)^(0xEDB88320u & (0u-(c&1)));}
    return ~c;
}
#include "load_probe.h"
static uint8_t raw[65535], packed[70000], decoded[65535];
static uint32_t used;
static int put(void *ctx,const uint8_t *p,uint32_t n)
{(void)ctx;if(n>sizeof packed-used)return -1;memcpy(packed+used,p,n);used+=n;return 0;}
int main(int argc,char **argv)
{
    assert(argc==3);FILE *f=fopen(argv[1],"rb");assert(f);
    size_t n=fread(raw,1,sizeof raw,f);assert(n&&n<sizeof raw&&feof(f));fclose(f);
    lz_sink_t sink={put,0};assert(!lz_compress(raw,(uint32_t)n,&sink));
    xvl_meta m={used,(uint32_t)n,st_crc32(packed,used),st_crc32(raw,(uint32_t)n)};
    assert(!vendor_lz_probe(&m,packed,used,decoded,(uint32_t)n));assert(!memcmp(raw,decoded,n));
    /* Truncated/mutated inputs never pass; full-consumption rejects appended byte
     * even if its compressed CRC is resealed. Bad output CRC prevents admission. */
    for(uint32_t i=0;i<used;i++) {
        assert(vendor_lz_probe(&m,packed,i,decoded,(uint32_t)n));
        packed[i]^=1;assert(vendor_lz_probe(&m,packed,used,decoded,(uint32_t)n));packed[i]^=1;
    }
    xvl_meta bad=m;packed[used]=0;bad.packed_size++;bad.packed_crc=st_crc32(packed,used+1);
    assert(vendor_lz_probe(&bad,packed,used+1,decoded,(uint32_t)n));
    bad=m;bad.raw_crc^=1;assert(vendor_lz_probe(&bad,packed,used,decoded,(uint32_t)n));
    assert(vendor_lz_probe(&m,packed,used,packed,(uint32_t)n));
    {
        uint8_t malformed[3]={0x80,0,0}, guards[5]={0xA5,0,0,0,0x5A};
        xvl_meta z={3,3,st_crc32(malformed,3),0};
        assert(vendor_lz_probe(&z,malformed,3,guards+1,3)); /* backref before output */
        assert(guards[0]==0xA5 && guards[4]==0x5A);
        malformed[0]=2; z.raw_size=1; z.packed_crc=st_crc32(malformed,3);
        assert(vendor_lz_probe(&z,malformed,3,guards+1,1)); /* literal overrun */
        assert(guards[0]==0xA5 && guards[4]==0x5A);
    }
    f=fopen(argv[2],"wb");assert(f);assert(fwrite(packed,1,used,f)==used);fclose(f);
    printf("raw=%u packed=%u saving=%d metadata=16 roundtrip/admission=PASS\n",(unsigned)n,used,(int)n-(int)used);
}
