/* SPDX-License-Identifier: GPL-3.0-only */
#include "session_wire.h"
static void wcopy(void *dst,const void *src,size_t n)
{ uint8_t *d=dst;const uint8_t *s=src;while(n--)*d++=*s++; }
static int wequal(const uint8_t *a,const uint8_t *b,size_t n)
{ unsigned x=0;while(n--)x|=*a++^*b++;return x==0; }
static void w64(uint8_t *p,uint64_t n)
{ unsigned i;for(i=0;i<8;i++)p[i]=(uint8_t)(n>>(8*i)); }
static size_t envelope(uint8_t *r,const struct mb_capture_binding *b,uint64_t now)
{
    wcopy(r,b->unit,32);wcopy(r+32,b->session,32);wcopy(r+64,b->challenge,32);
    wcopy(r+96,b->selection,32);w64(r+128,b->epoch);mb_wire_put32(r+136,b->model);
    w64(r+140,now);return 148;
}
static int release_owned(struct mb_wire_session *s)
{
    if(!s->active)return MB_W_OK;
    s->quarantined=1;
    mb_capture_invalidate(&s->core);
    if(s->provider->release(s->provider->user,&s->binding))return MB_W_RELEASE_PENDING;
    /* Keep the last owner's binding/provider as a close-only tombstone. A lost
     * CLOSE response or automatic error cleanup must remain reconcilable. */
    s->active=0;s->quarantined=0;return MB_W_OK;
}
static int check_current(struct mb_wire_session *s,uint64_t *now)
{
    struct mb_capture_binding b={{0},{0},{0},{0},0,0};
    if(s->quarantined||s->provider->current(s->provider->user,&b,now))return MB_W_STALE;
    return mb_capture_lease_current(&s->core,&b,*now)?MB_W_STALE:MB_W_OK;
}
int mb_wire_tick(struct mb_wire_session *s)
{
    uint64_t now;
    if(!s||!s->active)return MB_W_OK;
    if(check_current(s,&now))return release_owned(s)==MB_W_OK?MB_W_STALE:MB_W_RELEASE_PENDING;
    return MB_W_OK;
}
size_t mb_wire_dispatch(struct mb_wire_session *s,const struct mb_wire_provider *p,
                       const uint8_t *q,size_t n,uint8_t *r,size_t cap)
{
    uint32_t seq,off,len,facts=0;uint64_t now=0,deadline=0;size_t size;
    struct mb_capture_binding b={{0},{0},{0},{0},0,0};struct mb_capture_digests target,got;
    uint8_t data[MB_CAPTURE_CHUNK];int rc;
    if(!p)return mb_wire_disabled(q,n,r,cap);
    if(!q||!r||n<6||cap<MB_WIRE_RESPONSE_MAX)return 0;
    wcopy(r,q,6);r[6]=MB_W_BAD_PACKET;
    if(q[0]!=1||q[1]>MB_W_CLOSE||n>MB_WIRE_REQUEST_MAX)return 7;
    if(!p->acquire||!p->current||!p->read||!p->release){r[6]=MB_W_UNSUPPORTED;return 7;}
    seq=mb_wire_u32(q+2);
    if(q[1]==MB_W_CAPS){size=mb_wire_disabled(q,n,r,cap);if(n==6)mb_wire_put32(r+7,MB_CAPTURE_REQUIRED);return size;}
    if(!s){r[6]=MB_W_UNSUPPORTED;return 7;}
    if(q[1]==MB_W_OPEN){
        if(n!=70||seq!=0)return 7;
        if(s->active){r[6]=MB_W_BUSY;return 7;}
        if(p->acquire(p->user,q+6,q+38,&b,&facts,&now,&deadline)){r[6]=MB_W_FAILED;return 7;}
        s->provider=p;s->active=1;s->quarantined=0;wcopy(&s->binding,&b,sizeof b);
        /* Bind the returned lease to exactly this host request before exposure. */
        if(!wequal(b.challenge,q+6,32)||!wequal(b.selection,q+38,32)||
           mb_capture_start(&s->core,&b,facts,now,deadline))goto failed;
        s->sequence=1;s->next=0;s->finished=0;r[6]=MB_W_OK;size=7+envelope(r+7,&b,now);
        mb_wire_put32(r+size,facts);w64(r+size+4,deadline);return size+12;
    }
    if(n<38)return 7;
    if(!s->active){
        r[6]=(q[1]==MB_W_CLOSE&&n==38&&s->provider==p&&
              wequal(q+6,s->binding.session,32))?MB_W_OK:MB_W_FOREIGN;
        return 7; /* acknowledgment only: no callback, acquisition or read */
    }
    if(s->provider!=p||!wequal(q+6,s->binding.session,32)){r[6]=MB_W_FOREIGN;return 7;}
    if(q[1]==MB_W_CLOSE){
        if(n!=38)return 7;
        /* CLOSE is owner-bound and idempotent at provider, even after a lost
         * response/expiry; no sequence requirement to strand its held lease. */
        r[6]=(uint8_t)release_owned(s);return 7;
    }
    if(seq!=s->sequence||seq==UINT32_MAX)goto stale;
    if(check_current(s,&now))goto stale;
    if(q[1]==MB_W_READ){
        if(n!=44)goto failed;
        off=mb_wire_u32(q+38);len=(uint32_t)q[42]|(uint32_t)q[43]<<8;
        /* Check complete bounds and ordered coverage BEFORE provider read. */
        if(s->finished||!len||len>MB_CAPTURE_CHUNK||
           off!=s->next||off>MB_CAPTURE_SIZE||len>MB_CAPTURE_SIZE-off)goto failed;
        if(p->read(p->user,off,data,len)||check_current(s,&now)||
           mb_capture_chunk(&s->core,&s->binding,off,data,len,now))goto stale;
        size=7+envelope(r+7,&s->binding,now);mb_wire_put32(r+size,off);
        wcopy(r+size+4,data,len);size+=4+len;s->next+=len;
    }else if(q[1]==MB_W_FINISH){
        if(n!=102)goto failed;
        wcopy(&target,q+38,64);
        if(mb_capture_finish(&s->core,&s->binding,&target,now,&got))goto failed;
        size=7+envelope(r+7,&s->binding,now);wcopy(r+size,&got,64);size+=64;s->finished=1;
    }else if(q[1]==MB_W_REVALIDATE){
        if(n!=134)goto failed;
        wcopy(&target,q+70,64);
        if(mb_capture_revalidate(&s->core,&s->binding,q+38,&target,now))goto failed;
        size=7+envelope(r+7,&s->binding,now);wcopy(r+size,q+38,96);size+=96;
    }else if(q[1]==MB_W_CURRENT){
        if(n!=38||mb_capture_evidence_current(&s->core,&s->binding,now))goto failed;
        size=7+envelope(r+7,&s->binding,now);
    }else goto failed;
    s->sequence++;r[6]=MB_W_OK;return size;
stale: rc=MB_W_STALE;goto cleanup;
failed: rc=MB_W_FAILED;
cleanup:
    if(release_owned(s)!=MB_W_OK)rc=MB_W_RELEASE_PENDING;
    r[6]=(uint8_t)rc;return 7;
}
