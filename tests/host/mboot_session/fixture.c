/* SPDX-License-Identifier: GPL-3.0-only
 * Explicit synthetic provider, never linked into firmware. */
#include <string.h>
#include "session_wire.h"
static struct mb_wire_session s;
static struct mb_capture_binding binding;
static unsigned mode, reads, releases, acquires, held;
static uint64_t now;
static int acquire(void *u,const uint8_t *challenge,const uint8_t *selection,
                   struct mb_capture_binding *b,uint32_t *facts,uint64_t *time,uint64_t *deadline)
{
    (void)u;acquires++;memset(&binding,0,sizeof binding);
    memset(binding.unit,1,32);memset(binding.session,(int)(acquires+1),32);
    memcpy(binding.challenge,challenge,32);memcpy(binding.selection,selection,32);
    binding.epoch=7;binding.model=2;*b=binding;
    *facts=mode==1?0:15;*time=++now;*deadline=1000000;held=1;return 0;
}
static int current(void *u,struct mb_capture_binding *b,uint64_t *time)
{
    (void)u;*b=binding;*time=++now;
    if(mode==2)b->unit[0]^=1;
    if(mode==3)*time=1000000;
    if(mode==7&&reads)b->epoch++;
    return 0;
}
static int read_bytes(void *u,uint32_t off,uint8_t *p,uint32_t n)
{ unsigned i;(void)u;reads++;if(mode==4)return -1;for(i=0;i<n;i++)p[i]=(uint8_t)((off+i)%251);return 0; }
static int release(void *u,const struct mb_capture_binding *b)
{ (void)u;(void)b;releases++;if(mode==5)return -1;held=0;return 0; }
static const struct mb_wire_provider provider={0,acquire,current,read_bytes,release};
void fixture_reset(void){memset(&s,0,sizeof s);mode=reads=releases=acquires=held=0;now=0;}
void fixture_mode(unsigned n){mode=n;}
unsigned fixture_count(unsigned n){return n==0?reads:n==1?releases:n==2?acquires:held;}
size_t fixture_exchange(const uint8_t *q,size_t n,uint8_t *r,size_t cap)
{return mb_wire_dispatch(&s,mode==6?0:&provider,q,n,r,cap);}
int fixture_tick(void){return mb_wire_tick(&s);}
#ifdef FIXTURE_MAIN
#include <assert.h>
#include <stdio.h>
static uint8_t q[134],r[383],image[MB_CAPTURE_SIZE];
static void open_session(void)
{ memset(q,0,sizeof q);q[0]=1;q[1]=MB_W_OPEN;memset(q+6,3,32);memset(q+38,4,32);
  assert(fixture_exchange(q,70,r,sizeof r)==167);assert(r[6]==0); }
static void owned(unsigned op,unsigned seq)
{memset(q,0,sizeof q);q[0]=1;q[1]=(uint8_t)op;mb_wire_put32(q+2,seq);memset(q+6,2,32);}
int main(void)
{
    unsigned i,seq=1,off,n;uint8_t full[32],head[32];
    fixture_reset();fixture_mode(6);q[0]=1;q[1]=MB_W_OPEN;
    assert(fixture_exchange(q,70,r,sizeof r)==7&&r[6]==MB_W_UNSUPPORTED&&acquires==0);
    fixture_reset();open_session();owned(MB_W_READ,1);mb_wire_put32(q+38,1);q[42]=1;
    assert(fixture_exchange(q,44,r,sizeof r)==7&&r[6]!=0&&reads==0&&!held);
    fixture_reset();open_session();owned(MB_W_READ,1);q[6]^=1;q[42]=1;
    assert(fixture_exchange(q,44,r,sizeof r)==7&&r[6]==MB_W_FOREIGN&&held&&reads==0);
    fixture_mode(3);assert(fixture_tick()==MB_W_STALE&&!held);
    fixture_reset();open_session();fixture_mode(5);owned(MB_W_CLOSE,1);
    assert(fixture_exchange(q,38,r,sizeof r)==7&&r[6]==MB_W_RELEASE_PENDING&&held);
    fixture_mode(0);assert(fixture_exchange(q,38,r,sizeof r)==7&&r[6]==0&&!held);
    fixture_reset();open_session();fixture_mode(7);owned(MB_W_READ,1);q[42]=1;
    assert(fixture_exchange(q,44,r,sizeof r)==7&&r[6]==MB_W_STALE&&reads==1&&!held);
    fixture_reset();open_session();
    for(i=0;i<sizeof image;i++)image[i]=(uint8_t)(i%251);
    assert(!mb_capture_sha256(image,sizeof image,full));assert(!mb_capture_sha256(image,MB_CAPTURE_HEAD,head));
    for(off=0;off<MB_CAPTURE_SIZE;off+=n){n=MB_CAPTURE_SIZE-off;if(n>224)n=224;
        owned(MB_W_READ,seq++);mb_wire_put32(q+38,off);q[42]=(uint8_t)n;
        assert(fixture_exchange(q,44,r,sizeof r)==159+n&&r[6]==0);
        assert(!memcmp(r+159,image+off,n));}
    owned(MB_W_FINISH,seq++);memcpy(q+38,full,32);memcpy(q+70,head,32);
    assert(fixture_exchange(q,102,r,sizeof r)==219&&r[6]==0);
    owned(MB_W_REVALIDATE,seq++);memset(q+38,8,32);memcpy(q+70,full,32);memcpy(q+102,head,32);
    assert(fixture_exchange(q,134,r,sizeof r)==251&&r[6]==0);
    owned(MB_W_CURRENT,seq++);assert(fixture_exchange(q,38,r,sizeof r)==155&&r[6]==0);
    owned(MB_W_CLOSE,seq);assert(fixture_exchange(q,38,r,sizeof r)==7&&r[6]==0&&!held);
    puts("session C: disabled, bounds-before-read, foreign-owner, timeout, release quarantine, post-read stale, full stream PASS");return 0;
}
#endif
