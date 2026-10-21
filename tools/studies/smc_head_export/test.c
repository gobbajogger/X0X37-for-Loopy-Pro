/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "head_export.h"
static uint8_t nor[0x100000],saved[112],original[0x100000];
static unsigned reads,erases,clears,fail_read,fail_finish;
static int rd(uint32_t off,void*p,uint32_t n)
{assert(off<HX_SIZE&&n&&n<=HX_SIZE-off);reads++;if(fail_read)return -1;memcpy(p,nor+off,n);return 0;}
static int finish(void)
{
    if(fail_finish||memcmp(saved,nor+0xBFF00,112))return -1;
    memset(nor+0xBF000,255,4096);erases++;clears++;return 0;
}
int main(void)
{
    hx_state s={0};hx_reply r;uint8_t b[HX_CHUNK];
    for(unsigned i=0;i<sizeof nor;i++)nor[i]=(uint8_t)(i*31+7);
    memcpy(original,nor,sizeof nor);memcpy(saved,nor+0xBFF00,112);
    assert(hx_request(&s,HX_READ,123,0,224,&r,b,rd,finish)<0&&!reads);
    assert(hx_request(&s,HX_INFO,0,0,0,&r,b,rd,finish)<0&&!reads);
    assert(!hx_request(&s,HX_INFO,123,0,0,&r,b,rd,finish));
    assert(r.magic==HX_MAGIC&&r.model==HX_MODEL&&r.offset==HX_SIZE&&r.crc==~hx_crc(~0u,nor,HX_SIZE));
    assert(hx_request(&s,HX_FINISH,123,s.crc,0,&r,b,rd,finish)<0&&!erases);
    assert(hx_request(&s,HX_READ,124,0,224,&r,b,rd,finish)<0);
    assert(hx_request(&s,HX_READ,123,224,224,&r,b,rd,finish)<0);
    assert(hx_request(&s,HX_READ,123,0,225,&r,b,rd,finish)<0);
    for(uint32_t off=0;off<HX_SIZE;) {
        uint32_t n=HX_SIZE-off;if(n>HX_CHUNK)n=HX_CHUNK;
        assert(!hx_request(&s,HX_READ,123,off,n,&r,b,rd,finish));
        assert(r.offset==off&&r.size==n&&!memcmp(b,nor+off,n));
        assert(!hx_request(&s,HX_READ,123,off,n,&r,b,rd,finish)); /* lost reply retry */
        off+=n;
    }
    fail_finish=1;assert(hx_request(&s,HX_FINISH,123,s.crc,0,&r,b,rd,finish)==-3&&!erases&&!clears);
    fail_finish=0;nor[0xBFF00]^=1;assert(hx_request(&s,HX_FINISH,123,s.crc,0,&r,b,rd,finish)==-3&&!erases);
    nor[0xBFF00]^=1;assert(!hx_request(&s,HX_FINISH,123,s.crc,0,&r,b,rd,finish)&&r.status==HX_DONE&&erases==1&&clears==1);
    assert(!hx_request(&s,HX_FINISH,123,s.crc,0,&r,b,rd,finish)&&erases==1); /* idempotent */
    for(unsigned i=0;i<sizeof nor;i++)assert(nor[i]==(i>=0xBF000&&i<0xC0000?255:original[i]));
    s=(hx_state){0};fail_read=1;
    assert(hx_request(&s,HX_INFO,125,0,0,&r,b,rd,finish)==-2&&!s.active&&erases==1);
    fail_read=0;assert(!hx_request(&s,HX_INFO,125,0,0,&r,b,rd,finish));
    assert(hx_request(&s,HX_READ,123,0,224,&r,b,rd,finish)<0); /* restart rejects stale session */
    puts("head export core: fixed range, CRC/model/nonce, partial/retry/restart, owned cleanup and no payload writes PASS");
}
