/* SPDX-License-Identifier: GPL-3.0-only */
/* Read-only head export core. Not a staging tool or hardware admission.
 * Callbacks may read only 0..0x3fff, except owned-record admission/finish hooks.
 * Finish must verify the exact saved staged-loader/record identity, erase only
 * its record sector, verify it and clear the RAM record last. No payload writer.
 * Wire words are little-endian, encoded by caller's existing MIDI packing. */
#pragma once
#include <stdint.h>
#define HX_MAGIC 0x31455848u /* HXE1 */
#define HX_MODEL 0x894C0FA4u
#define HX_SIZE 16384u
#define HX_CHUNK 224u
#define HX_INFO 1u
#define HX_READ 2u
#define HX_FINISH 3u
#define HX_DONE 4u
/* Each response contains this header, then exactly size bytes. */
typedef struct { uint32_t magic,model,nonce,crc,offset,size,status; } hx_reply;
typedef struct { uint32_t nonce,crc,next,last,last_size,active,done; } hx_state;
_Static_assert(sizeof(hx_reply)==28,"head-export response header");
typedef int (*hx_read_fn)(uint32_t,void*,uint32_t);
typedef int (*hx_finish_fn)(void);
static uint32_t hx_crc(uint32_t c,const uint8_t*p,uint32_t n)
{while(n--){c^=*p++;for(unsigned i=0;i<8;i++)c=(c>>1)^(0xEDB88320u&(0u-(c&1)));}return c;}
/* One request: cmd, nonce, offset, size. Caller first validates exact framing.
 * No flash cleanup on failed reads, malformed requests or incomplete transfer.
 * Successful FINISH is idempotent until reset. The host must persist and verify
 * the complete local file before requesting it. A lost reply is retryable. */
static int hx_request(hx_state*s,uint32_t cmd,uint32_t nonce,uint32_t off,uint32_t n,
                      hx_reply*r,uint8_t data[HX_CHUNK],hx_read_fn read,hx_finish_fn finish)
{
    *r=(hx_reply){HX_MAGIC,HX_MODEL,nonce,s->crc,off,0,0};
    if(!nonce)return -1;
    if(cmd==HX_INFO) {
        uint32_t c=~0u;
        if(off||n||s->done)return -1;
        s->active=0;
        for(uint32_t p=0;p<HX_SIZE;p+=HX_CHUNK) {
            uint32_t k=HX_SIZE-p;if(k>HX_CHUNK)k=HX_CHUNK;
            if(read(p,data,k))return -2;
            c=hx_crc(c,data,k);
        }
        *s=(hx_state){nonce,~c,0,0,0,1,0};r->crc=s->crc;r->offset=HX_SIZE;
        return 0;
    }
    if(!s->active||nonce!=s->nonce)return -1;
    r->crc=s->crc;
    if(cmd==HX_READ) {
        if(s->done||off>=HX_SIZE||!n||n>HX_CHUNK||n>HX_SIZE-off)return -1;
        if(off!=s->next && !(off==s->last&&n==s->last_size))return -1;
        if(read(off,data,n))return -2;
        if(off==s->next){s->last=off;s->last_size=n;s->next+=n;}
        r->size=n;return 0;
    }
    if(cmd==HX_FINISH) {
        if(n||off!=s->crc||s->next!=HX_SIZE)return -1;
        if(!s->done && finish())return -3;
        s->done=1;r->status=HX_DONE;return 0;
    }
    return -1;
}
