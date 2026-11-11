/* SPDX-License-Identifier: GPL-3.0-only */
#include "capture_core.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static struct mb_capture_binding b;
static struct mb_capture_digests expected;
static uint8_t bytes[MB_CAPTURE_SIZE]; /* host fixture only; core never allocates this */
static unsigned groups;
static void start(struct mb_capture *c)
{ mb_capture_invalidate(c); assert(mb_capture_start(c,&b,MB_CAPTURE_REQUIRED,1,100)==0); }
static void stream(struct mb_capture *c,unsigned stride)
{
    uint32_t off=0;
    while(off<MB_CAPTURE_SIZE){uint32_t n=MB_CAPTURE_SIZE-off;
        if(n>stride)n=stride;
        assert(mb_capture_chunk(c,&b,off,bytes+off,n,2)==0);off+=n;}
}
static void complete(struct mb_capture *c)
{
    struct mb_capture_digests out;
    start(c);stream(c,224);assert(mb_capture_finish(c,&b,&expected,3,&out)==0);
    assert(!memcmp(&out,&expected,sizeof out));
}
static void validated(struct mb_capture *c)
{ uint8_t nonce[32]={9};complete(c);assert(mb_capture_revalidate(c,&b,nonce,&expected,4)==0); }
static void sha_vectors(void)
{
    static const char *msgs[]={"","abc","abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"};
    static const char *hex[]={
        "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
        "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
        "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1"};
    unsigned i,j;uint8_t out[32];char str[65];
    for(i=0;i<3;i++){assert(!mb_capture_sha256((const uint8_t *)msgs[i],strlen(msgs[i]),out));
        for(j=0;j<32;j++)sprintf(str+2*j,"%02x",out[j]);
        assert(!strcmp(str,hex[i]));}
    memset(bytes,'a',1000000);assert(!mb_capture_sha256(bytes,1000000,out));
    for(j=0;j<32;j++)sprintf(str+2*j,"%02x",out[j]);
    assert(!strcmp(str,"cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0"));
    groups++;
}
int main(void)
{
    struct mb_capture c={0};struct mb_capture_binding other;
    struct mb_capture_digests out,bad,sentinel;uint8_t fresh[32]={9};unsigned i,j;
    sha_vectors();
    memset(&b,0,sizeof b);b.unit[0]=1;b.session[0]=2;b.challenge[0]=3;b.selection[0]=4;b.model=2;b.epoch=7;
    for(i=0;i<MB_CAPTURE_SIZE;i++)bytes[i]=(uint8_t)i;
    assert(!mb_capture_sha256(bytes,sizeof bytes,expected.full));
    assert(!mb_capture_sha256(bytes,MB_CAPTURE_HEAD,expected.head));
    assert(mb_capture_platform_supported()==0);
    for(i=0;i<MB_CAPTURE_REQUIRED;i++)assert(mb_capture_start(&c,&b,i,1,100)==MB_CAPTURE_UNSUPPORTED);
    assert(mb_capture_start(&c,&b,31,1,100)==MB_CAPTURE_UNSUPPORTED);groups++;
    for(i=0;i<6;i++){other=b;
        if(i==0)other.model=0;
        if(i==1)other.model=3;
        if(i==2)memset(other.unit,0,32);
        if(i==3)memset(other.session,0,32);
        if(i==4)memset(other.challenge,0,32);
        if(i==5)memset(other.selection,0,32);
        assert(mb_capture_start(&c,&other,15,1,100)==MB_CAPTURE_ARGUMENT);}
    assert(mb_capture_start(&c,&b,15,100,100)==MB_CAPTURE_ARGUMENT);groups++;
    /* Both models; exact coverage with head-boundary crossing and SHA padding splits. */
    for(j=1;j<=2;j++){b.model=j;for(i=1;i<=224;i=i==1?63:i==63?64:i==64?223:225){
        start(&c);stream(&c,i);assert(!mb_capture_finish(&c,&b,&expected,3,&out));}
        complete(&c);assert(c.state==MB_CAPTURE_COMPLETE_HELD);
        assert(!mb_capture_revalidate(&c,&b,fresh,&expected,4));
        assert(!mb_capture_evidence_current(&c,&b,5));}
    groups++;
    start(&c);other=b;other.unit[0]++;
    assert(mb_capture_start(&c,&other,15,1,100)==MB_CAPTURE_STATE);
    assert(c.state==MB_CAPTURE_STREAMING && !memcmp(c.binding.unit,b.unit,32));groups++;
    /* output remains byte-identical on all refused finish operations. */
    memset(&sentinel,0xa5,sizeof sentinel);
    for(i=0;i<4;i++){start(&c);out=sentinel;
        if(i){stream(&c,224);bad=expected;
        if(i==1)bad.full[0]^=1;
        if(i==2)bad.head[31]^=1;}
        assert(mb_capture_finish(&c,&b,i?&bad:&expected,3,i==3?NULL:&out)!=0);
        assert(!memcmp(&out,&sentinel,sizeof out));assert(c.state==MB_CAPTURE_INVALID);}
    groups++;
    for(i=0;i<6;i++){start(&c);
        assert(mb_capture_chunk(&c,&b,i==0?1:i==1?UINT32_MAX:0,
            i==2?NULL:bytes,i==3?0:i==4?225:i==5?UINT32_MAX:1,2)!=0);
        assert(c.state==MB_CAPTURE_INVALID);}
    start(&c);assert(!mb_capture_chunk(&c,&b,0,bytes,1,2));
    assert(mb_capture_chunk(&c,&b,0,bytes,1,2)==MB_CAPTURE_COVERAGE);
    start(&c);stream(&c,224);assert(mb_capture_chunk(&c,&b,MB_CAPTURE_SIZE,bytes,1,2)==MB_CAPTURE_COVERAGE);groups++;
    for(i=0;i<6;i++){other=b;
        if(i==0)other.unit[0]++;
        if(i==1)other.session[0]++;
        if(i==2)other.challenge[0]++;
        if(i==3)other.selection[0]++;
        if(i==4)other.model=1;
        if(i==5)other.epoch++;
        start(&c);assert(mb_capture_chunk(&c,&other,0,bytes,1,2)==MB_CAPTURE_STALE);
        validated(&c);assert(mb_capture_evidence_current(&c,&other,5)==MB_CAPTURE_STALE);}
    groups++;
    for(i=0;i<4;i++){start(&c);
        if(i==0)assert(mb_capture_chunk(&c,&b,0,bytes,1,100)==MB_CAPTURE_STALE);
        if(i==1)assert(mb_capture_finish(&c,&b,&expected,100,&out)==MB_CAPTURE_STALE);
        if(i==2){complete(&c);assert(mb_capture_revalidate(&c,&b,fresh,&expected,100)==MB_CAPTURE_STALE);}
        if(i==3){validated(&c);assert(mb_capture_evidence_current(&c,&b,100)==MB_CAPTURE_STALE);}}
    start(&c);assert(mb_capture_chunk(&c,&b,0,bytes,1,0)==MB_CAPTURE_STALE);
    validated(&c);assert(mb_capture_evidence_current(&c,&b,3)==MB_CAPTURE_STALE);groups++;
    complete(&c);assert(mb_capture_revalidate(&c,&b,b.challenge,&expected,4)==MB_CAPTURE_REPLAY);
    memset(fresh,0,32);complete(&c);assert(mb_capture_revalidate(&c,&b,fresh,&expected,4)==MB_CAPTURE_REPLAY);fresh[0]=9;
    complete(&c);bad=expected;bad.head[0]^=1;
    assert(mb_capture_revalidate(&c,&b,fresh,&bad,4)==MB_CAPTURE_DIGEST_MISMATCH);groups++;
    validated(&c);mb_capture_invalidate(&c);assert(mb_capture_evidence_current(&c,&b,5)==MB_CAPTURE_STATE);
    assert(c.state==MB_CAPTURE_INVALID);groups++;
    complete(&c);other=b;other.unit[0]++;
    assert(mb_capture_cancel(&c,&other)==MB_CAPTURE_FOREIGN_OWNER);assert(c.state==MB_CAPTURE_COMPLETE_HELD);
    assert(!mb_capture_cancel(&c,&b));assert(c.state==MB_CAPTURE_CANCELLED);
    assert(mb_capture_revalidate(&c,&b,fresh,&expected,4)==MB_CAPTURE_STATE);groups++;
    complete(&c);assert(mb_capture_chunk(&c,&b,MB_CAPTURE_SIZE,bytes,1,4)==MB_CAPTURE_STATE);
    complete(&c);assert(mb_capture_finish(&c,&b,&expected,4,&out)==MB_CAPTURE_STATE);
    start(&c);assert(mb_capture_evidence_current(&c,&b,2)==MB_CAPTURE_STATE);
    validated(&c);assert(mb_capture_revalidate(&c,&b,fresh,&expected,5)==MB_CAPTURE_STATE);groups++;
    assert(mb_capture_start(NULL,&b,15,1,100)==MB_CAPTURE_ARGUMENT);
    assert(mb_capture_chunk(NULL,&b,0,bytes,1,2)==MB_CAPTURE_ARGUMENT);
    assert(mb_capture_sha256(NULL,1,out.full)==MB_CAPTURE_ARGUMENT);
    assert(mb_capture_sha256(bytes,1,NULL)==MB_CAPTURE_ARGUMENT);
    mb_capture_invalidate(NULL);groups++;
    printf("PASS: %u core groups; context=%zu bytes; hash=%zu bytes; platform=disabled\n",groups,sizeof c,sizeof c.full);
    return 0;
}
