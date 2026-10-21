/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../../firmware/src/ble/smc_host_ingress.h"

typedef struct { uint8_t n, b[6]; } message_t;
static message_t queue[64];
static unsigned written, limit = 64;
static int smc_queue_copy(void *ctx, const uint8_t *p, uint8_t n)
{
    (void)ctx; assert(n <= 6);
    if (written >= limit) return 0;
    queue[written].n = n; memcpy(queue[written].b, p, n); ++written;
    return 1;
}
static void expect(unsigned at, const uint8_t *p, unsigned n)
{ assert(at < written && queue[at].n == n && !memcmp(queue[at].b, p, n)); }
static void reset(smc_host_ingress_t *s)
{ memset(s, 0, sizeof *s); written = 0; limit = 64; smc_hi_link(s, 1); }
#define FEED(s, p) smc_hi_notification((s), (p), sizeof(p), smc_queue_copy, NULL)
int main(void)
{
    smc_host_ingress_t s;
    reset(&s);
    uint8_t packet[] = {0x80,0x81,0xB0,16,1,17,2,0x82,0x90,24,127};
    const uint8_t cc1[]={0xB0,16,1},cc2[]={0xB0,17,2},note[]={0x90,24,127};
    assert(FEED(&s,packet)==SMC_HI_OK && written==3);
    memset(packet,0xDD,sizeof packet); /* no vendor/mailbox pointer survives */
    expect(0,cc1,3);expect(1,cc2,3);expect(2,note,3);
    const uint8_t running[]={0x80,25,3};
    assert(FEED(&s,running)==SMC_HI_OK && written==4);
    const uint8_t next[]={0x90,25,3};expect(3,next,3);
    /* Program/pressure two-byte messages and timestamped realtime retain status. */
    const uint8_t two[]={0x80,0x81,0xC0,42,43,0x82,0xF8,44};
    assert(FEED(&s,two)==SMC_HI_OK && written==8);
    const uint8_t program[]={0xC0,42},program2[]={0xC0,43},clock[]={0xF8},program3[]={0xC0,44};
    expect(4,program,2);expect(5,program2,2);expect(6,clock,1);expect(7,program3,2);
    const uint8_t interleaved[]={0x80,0x81,0xB0,16,0xF8,1};
    assert(FEED(&s,interleaved)==SMC_HI_OK && written==10);
    expect(8,clock,1);expect(9,cc1,3);
    /* Fragmented six-byte MMC: in-SysEx timestamp and realtime are not data. */
    const uint8_t first[]={0x80,0x81,0xF0,0x7F};
    const uint8_t tail[]={0x80,0x82,0x7F,6,0xF8,33,0xF7,0xF7};
    const uint8_t mmc[]={0xF0,0x7F,0x7F,6,33,0xF7};
    assert(FEED(&s,first)==SMC_HI_OK && s.decoder.in_sysex && written==10);
    assert(FEED(&s,tail)==SMC_HI_OK && !s.decoder.in_sysex && written==12);
    expect(10,clock,1);expect(11,mmc,6);
    /* Timestamp-low127 before more SysEx data uses baseline disambiguation. */
    const uint8_t sx127[]={0x80,0x81,0xF0,0x7F,0xF7,0x7F,6,33,0xF7};
    assert(FEED(&s,sx127)==SMC_HI_OK);expect(12,mmc,6);
    /* Packet preflight rejects malformed tail before any callback/state side effect. */
    const uint8_t badtail[]={0x80,0x81,0xB0,16,1,17};
    unsigned before=written;
    assert(FEED(&s,badtail)==SMC_HI_TRUNCATED && written==before && !s.decoder.running);
    const uint8_t highdata[]={0x80,0x81,0xB0,16,0x90};
    assert(FEED(&s,highdata)==SMC_HI_DATA && written==before);
    const uint8_t badheader[]={0xC0,0x81,0xB0,16,1};
    assert(FEED(&s,badheader)==SMC_HI_HEADER);
    const uint8_t shortdata[]={0x80,0x81,0xB0,16};
    assert(FEED(&s,shortdata)==SMC_HI_TRUNCATED);
    const uint8_t timestamp_only[]={0x80,0x81,0xB0,16,1,0x82};
    assert(FEED(&s,timestamp_only)==SMC_HI_TRUNCATED && written==before);
    const uint8_t no_status[]={0x80,0x81,16,1};
    assert(FEED(&s,no_status)==SMC_HI_STATUS);
    const uint8_t unsupported[]={0x80,0x81,0xF1,0};
    assert(FEED(&s,unsupported)==SMC_HI_STATUS);
    const uint8_t too_long_sx[]={0x80,0x81,0xF0,1,2,3,4,5,6,7,8,9,0xF7};
    assert(FEED(&s,too_long_sx)==SMC_HI_SYSEX && !s.decoder.in_sysex);
    uint8_t big[129]={0x80,0x81,0xB0};
    assert(FEED(&s,big)==SMC_HI_SIZE);
    assert(smc_hi_notification(&s,NULL,5,smc_queue_copy,NULL)==SMC_HI_SIZE);
    for(unsigned n=0;n<3;n++) assert(smc_hi_notification(&s,big,n,smc_queue_copy,NULL)==SMC_HI_SIZE);
    assert(written==before);
    /* Both connect and disconnect boundaries clear stale partial/running state. */
    assert(FEED(&s,first)==SMC_HI_OK && s.decoder.in_sysex);
    uint32_t rejected=s.rejected;
    smc_hi_link(&s,0);assert(!s.decoder.in_sysex && !s.decoder.running);
    assert(FEED(&s,interleaved)==SMC_HI_LINK);
    smc_hi_link(&s,1);assert(s.rejected==rejected+1);
    assert(FEED(&s,tail)==SMC_HI_STATUS && written==before);
    assert(FEED(&s,interleaved)==SMC_HI_OK && written==before+2);
    smc_hi_link(&s,1);assert(FEED(&s,running)==SMC_HI_STATUS);
    /* Queue refusal increments drops, does not leak borrowed pointers or change
     * APC/Loopy ingress; later good packets still parse. */
    reset(&s); limit=1;
    const uint8_t pair[]={0x80,0x81,0xB0,16,1,17,2};
    assert(FEED(&s,pair)==SMC_HI_OK && s.messages==1 && s.dropped==1 && written==1);
    expect(0,cc1,3);limit=64;
    assert(FEED(&s,interleaved)==SMC_HI_OK && s.messages==3 && s.dropped==1);
    reset(&s);
    uint8_t maximum[128]={0x80,0x81,0xC0};
    for(unsigned i=3;i<sizeof maximum;i++) maximum[i]=(uint8_t)(i&0x7F);
    assert(FEED(&s,maximum)==SMC_HI_OK && s.messages==64 && s.dropped==61);
    /* Bounded deterministic malformed-input sweep under sanitizers. */
    uint32_t random=0x12345678u;
    for(unsigned trial=0;trial<10000;trial++) {
        unsigned n=trial%129u;
        for(unsigned i=0;i<n;i++) { random=random*1664525u+1013904223u;maximum[i]=(uint8_t)(random>>24); }
        if(!(trial%17u)) smc_hi_link(&s,trial&1u);
        (void)smc_hi_notification(&s,maximum,n,smc_queue_copy,NULL);
    }
    printf("PASS vendor copied-payload ingress: state %zu B; parsing/reconnect/reject/copy/drop checks\n",sizeof s);
    return 0;
}
