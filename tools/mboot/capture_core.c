/* SPDX-License-Identifier: GPL-3.0-only
 * Original portable implementation of SHA-256 and read-only capture bookkeeping.
 * No platform, transport, filesystem, allocator or flash-write dependency. */
#include "capture_core.h"
/* Freestanding target has no hosted string.h; bounded byte helpers avoid SDK libc. */
static void byte_copy(void *dst, const void *src, size_t n)
{
    uint8_t *d = dst; const uint8_t *s = src;
    while (n--) *d++ = *s++;
}
static void byte_fill(void *dst, int value, size_t n)
{
    uint8_t *d = dst;
    while (n--) *d++ = (uint8_t)value;
}
static int byte_diff(const void *a, const void *b, size_t n)
{
    const uint8_t *x = a, *y = b;
    while (n--) if (*x++ != *y++) return 1;
    return 0;
}

static uint32_t rr(uint32_t x, unsigned n) { return (x >> n) | (x << (32u - n)); }
static uint32_t be32(const uint8_t *p)
{ return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3]; }
static void put32(uint8_t *p, uint32_t x)
{ p[0]=(uint8_t)(x>>24); p[1]=(uint8_t)(x>>16); p[2]=(uint8_t)(x>>8); p[3]=(uint8_t)x; }
static void compress(struct mb_capture_hash *s)
{
    static const uint32_t k[64] = {
        0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
        0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
        0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
        0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
        0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
        0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
        0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
        0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2 };
    uint32_t w[64],a,b,c,d,e,f,g,h,t1,t2; unsigned i;
    for(i=0;i<16;i++) w[i]=be32(s->block+4*i);
    for(i=16;i<64;i++) {
        uint32_t x=w[i-15],y=w[i-2];
        w[i]=w[i-16]+(rr(x,7)^rr(x,18)^(x>>3))+w[i-7]+(rr(y,17)^rr(y,19)^(y>>10));
    }
    a=s->h[0];b=s->h[1];c=s->h[2];d=s->h[3];e=s->h[4];f=s->h[5];g=s->h[6];h=s->h[7];
    for(i=0;i<64;i++) {
        t1=h+(rr(e,6)^rr(e,11)^rr(e,25))+((e&f)^((~e)&g))+k[i]+w[i];
        t2=(rr(a,2)^rr(a,13)^rr(a,22))+((a&b)^(a&c)^(b&c));
        h=g;g=f;f=e;e=d+t1;d=c;c=b;b=a;a=t1+t2;
    }
    s->h[0]+=a;s->h[1]+=b;s->h[2]+=c;s->h[3]+=d;s->h[4]+=e;s->h[5]+=f;s->h[6]+=g;s->h[7]+=h;
}
static void hash_init(struct mb_capture_hash *s)
{
    static const uint32_t iv[8]={0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,
        0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
    byte_fill(s,0,sizeof *s);byte_copy(s->h,iv,sizeof iv);
}
static void hash_add(struct mb_capture_hash *s,const uint8_t *p,size_t n)
{
    s->bytes+=(uint64_t)n;
    while(n) {
        size_t k=64-s->used;if(k>n)k=n;
        byte_copy(s->block+s->used,p,k);s->used+=k;p+=k;n-=k;
        if(s->used==64){compress(s);s->used=0;}
    }
}
static void hash_end(struct mb_capture_hash *s,uint8_t out[32])
{
    uint64_t bits=s->bytes*8u;unsigned i;
    s->block[s->used++]=0x80;
    if(s->used>56){byte_fill(s->block+s->used,0,64-s->used);compress(s);s->used=0;}
    byte_fill(s->block+s->used,0,56-s->used);
    for(i=0;i<8;i++)s->block[63-i]=(uint8_t)(bits>>(8*i));
    compress(s);for(i=0;i<8;i++)put32(out+4*i,s->h[i]);
    byte_fill(s,0,sizeof *s);
}
int mb_capture_sha256(const uint8_t *data,size_t length,uint8_t output[32])
{
    struct mb_capture_hash s;
    if(!output || (!data && length))return MB_CAPTURE_ARGUMENT;
#if SIZE_MAX > (UINT64_MAX / 8u)
    if(length>UINT64_MAX/8u)return MB_CAPTURE_ARGUMENT;
#endif
    hash_init(&s);hash_add(&s,data,length);hash_end(&s,output);return MB_CAPTURE_OK;
}
static int nonzero(const uint8_t p[32])
{ unsigned i;uint8_t x=0;for(i=0;i<32;i++)x|=p[i];return x!=0; }
static int valid_binding(const struct mb_capture_binding *b)
{
    return b && (b->model==1 || b->model==2) && nonzero(b->unit) &&
        nonzero(b->session) && nonzero(b->challenge) && nonzero(b->selection);
}
static int same(const struct mb_capture_binding *a,const struct mb_capture_binding *b)
{
    return b && a->epoch==b->epoch && a->model==b->model &&
        !byte_diff(a->unit,b->unit,32) && !byte_diff(a->session,b->session,32) &&
        !byte_diff(a->challenge,b->challenge,32) && !byte_diff(a->selection,b->selection,32);
}
static int fail(struct mb_capture *c,int rc)
{ if(c){byte_fill(c,0,sizeof *c);c->state=MB_CAPTURE_INVALID;}return rc; }
int mb_capture_platform_supported(void){return 0;}
int mb_capture_start(struct mb_capture *c,const struct mb_capture_binding *b,
                     uint32_t facts,uint64_t now,uint64_t deadline)
{
    if(!c)return MB_CAPTURE_ARGUMENT;
    /* Do not discard another in-progress owner lease. */
    if(c->state!=MB_CAPTURE_EMPTY && c->state!=MB_CAPTURE_CANCELLED &&
       c->state!=MB_CAPTURE_INVALID)return MB_CAPTURE_STATE;
    byte_fill(c,0,sizeof *c);
    if(facts!=MB_CAPTURE_REQUIRED)return fail(c,MB_CAPTURE_UNSUPPORTED);
    if(!valid_binding(b) || deadline<=now)return fail(c,MB_CAPTURE_ARGUMENT);
    byte_copy(&c->binding,b,sizeof *b);c->last_now=now;c->deadline=deadline;c->state=MB_CAPTURE_STREAMING;
    hash_init(&c->full);hash_init(&c->head);return MB_CAPTURE_OK;
}
static int live(struct mb_capture *c,const struct mb_capture_binding *b,uint64_t now)
{
    if(!c)return MB_CAPTURE_ARGUMENT;
    if(c->state!=MB_CAPTURE_STREAMING && c->state!=MB_CAPTURE_COMPLETE_HELD &&
       c->state!=MB_CAPTURE_REVALIDATED_HELD)return fail(c,MB_CAPTURE_STATE);
    if(!same(&c->binding,b) || now<c->last_now || now>=c->deadline)return fail(c,MB_CAPTURE_STALE);
    c->last_now=now;return MB_CAPTURE_OK;
}
int mb_capture_chunk(struct mb_capture *c,const struct mb_capture_binding *b,
                     uint32_t offset,const uint8_t *data,uint32_t n,uint64_t now)
{
    int rc=live(c,b,now);uint32_t hn;
    if(rc)return rc;
    if(c->state!=MB_CAPTURE_STREAMING)return fail(c,MB_CAPTURE_STATE);
    if(!data || !n || n>MB_CAPTURE_CHUNK || offset!=c->next ||
       offset>MB_CAPTURE_SIZE || n>MB_CAPTURE_SIZE-offset)return fail(c,MB_CAPTURE_COVERAGE);
    hash_add(&c->full,data,n);
    if(offset<MB_CAPTURE_HEAD){hn=MB_CAPTURE_HEAD-offset;if(hn>n)hn=n;hash_add(&c->head,data,hn);}
    c->next+=n;return MB_CAPTURE_OK;
}
int mb_capture_finish(struct mb_capture *c,const struct mb_capture_binding *b,
                      const struct mb_capture_digests *target,uint64_t now,
                      struct mb_capture_digests *output)
{
    int rc=live(c,b,now);struct mb_capture_digests got;
    if(rc)return rc;
    if(!target || !output)return fail(c,MB_CAPTURE_ARGUMENT);
    if(c->state!=MB_CAPTURE_STREAMING)return fail(c,MB_CAPTURE_STATE);
    if(c->next!=MB_CAPTURE_SIZE)return fail(c,MB_CAPTURE_COVERAGE);
    hash_end(&c->full,got.full);hash_end(&c->head,got.head);
    if(byte_diff(got.full,target->full,32)||byte_diff(got.head,target->head,32))
        return fail(c,MB_CAPTURE_DIGEST_MISMATCH);
    byte_copy(&c->digests,&got,sizeof got);c->state=MB_CAPTURE_COMPLETE_HELD;byte_copy(output,&got,sizeof got);return MB_CAPTURE_OK;
}
int mb_capture_revalidate(struct mb_capture *c,const struct mb_capture_binding *b,
                          const uint8_t fresh[32],const struct mb_capture_digests *target,uint64_t now)
{
    int rc=live(c,b,now);if(rc)return rc;
    if(c->state!=MB_CAPTURE_COMPLETE_HELD)return fail(c,MB_CAPTURE_STATE);
    if(!fresh || !target)return fail(c,MB_CAPTURE_ARGUMENT);
    if(!nonzero(fresh)||!byte_diff(fresh,c->binding.challenge,32))return fail(c,MB_CAPTURE_REPLAY);
    if(byte_diff(target->full,c->digests.full,32)||byte_diff(target->head,c->digests.head,32))
        return fail(c,MB_CAPTURE_DIGEST_MISMATCH);
    c->state=MB_CAPTURE_REVALIDATED_HELD;return MB_CAPTURE_OK;
}
int mb_capture_evidence_current(struct mb_capture *c,const struct mb_capture_binding *b,uint64_t now)
{
    int rc=live(c,b,now);if(rc)return rc;
    if(c->state!=MB_CAPTURE_REVALIDATED_HELD)return fail(c,MB_CAPTURE_STATE);
    return MB_CAPTURE_OK;
}
void mb_capture_invalidate(struct mb_capture *c){(void)fail(c,MB_CAPTURE_STALE);}
int mb_capture_cancel(struct mb_capture *c,const struct mb_capture_binding *b)
{
    if(!c || !b)return MB_CAPTURE_ARGUMENT;
    if(!same(&c->binding,b))return MB_CAPTURE_FOREIGN_OWNER;
    byte_fill(c,0,sizeof *c);c->state=MB_CAPTURE_CANCELLED;return MB_CAPTURE_OK;
}
