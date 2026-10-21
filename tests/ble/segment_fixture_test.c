/* SPDX-License-Identifier: MPL-2.0 */
/* Native validation of private generated fixtures; no target writes. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../firmware/hal/x0x_ble_segment.h"
static unsigned char *part[3];
static unsigned length[3], read_calls;
static const uint8_t *reader(void *ctx, uint32_t off, uint32_t n)
{
    (void)ctx; ++read_calls;
    for (unsigned i=0;i<3;i++) {
        uint32_t base=xb4_segment_off(i);
        if (off>=base && off-base<=length[i] && n<=length[i]-(off-base))
            return part[i]+off-base;
    }
    return NULL;
}
int main(int argc,char **argv)
{
    assert(argc==4);
    for(unsigned i=0;i<3;i++) {
        FILE *f=fopen(argv[i+1],"rb"); assert(f);
        assert(!fseek(f,0,SEEK_END)); long n=ftell(f); assert(n>0 && n<=xb4_segment_capacity(i));
        assert(!fseek(f,0,SEEK_SET)); length[i]=(unsigned)n; part[i]=malloc(length[i]); assert(part[i]);
        assert(fread(part[i],1,length[i],f)==length[i]); fclose(f);
    }
    xb4_manifest_t h; memcpy(&h,part[0],sizeof h);
    uint32_t model=xb4_crc32((const uint8_t *)"SMK-37 Elite",12);
    assert(xb4_validate(&h,sizeof h,model,reader,NULL)==XB4_OK);
    assert(xb4_validate(&h,sizeof h,model^1,reader,NULL)!=XB4_OK);
    xb4_manifest_t legacy = h; legacy.contract = 0x534D4302u;
    legacy.manifest_crc32 = xb4_manifest_crc(&legacy); read_calls = 0;
    assert(xb4_validate(&legacy,sizeof legacy,model,reader,NULL)==XB4_E_HEADER && !read_calls);
    legacy = h; legacy.abi = 5; legacy.manifest_crc32 = xb4_manifest_crc(&legacy);
    assert(xb4_validate(&legacy,sizeof legacy,model,reader,NULL)==XB4_E_HEADER && !read_calls);
    for(unsigned i=0;i<sizeof h;i++) {
        ((unsigned char *)&h)[i]^=1;
        assert(xb4_validate(&h,sizeof h,model,reader,NULL)!=XB4_OK);
        ((unsigned char *)&h)[i]^=1;
    }
    for(unsigned i=0;i<3;i++) {
        length[i]--;
        assert(xb4_validate(&h,sizeof h,model,reader,NULL)!=XB4_OK);
        length[i]++;
        unsigned off=i?0:sizeof h;
        for(unsigned k=0;k<3;k++) {
            unsigned at=k==0?off:k==1?length[i]/2:length[i]-1;
            part[i][at]^=1;
            assert(xb4_validate(&h,sizeof h,model,reader,NULL)!=XB4_OK);
            part[i][at]^=1;
        }
    }
    for(unsigned i=0;i<3;i++) free(part[i]);
    puts("actual ELF fixture: manifest/model, every header byte, segment truncation and payload corruption PASS");
    return 0;
}
