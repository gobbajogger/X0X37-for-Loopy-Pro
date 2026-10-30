/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "app/boot_media.h"
static unsigned checks;
#define CHECK(x) do {checks++;if(!(x)){fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);return 1;}}while(0)
typedef struct {uint8_t *p;uint32_t size,fail_at;int bad;} fault_t;
static int fault_read(void *ctx,uint32_t off,uint8_t *out,uint32_t n){fault_t*f=ctx;if(off>f->size||n>f->size-off){f->bad=1;return 1;}if(off<=f->fail_at&&n>f->fail_at-off)return 1;memcpy(out,f->p+off,n);return 0;}
int main(int argc,char **argv)
{
    CHECK(argc==2);FILE *f=fopen(argv[1],"rb");CHECK(f);uint8_t *p=malloc(BM_COMPILED_MAX),*copy=malloc(BM_COMPILED_MAX);CHECK(p&&copy);
    size_t n=fread(p,1,BM_COMPILED_MAX,f);CHECK(!ferror(f));fclose(f);CHECK(n==29718);memcpy(copy,p,n);
    bm_reader_t r=bm_memory(p,(uint32_t)n);bm_info_t m;CHECK(bm_validate(&r,BM_COMPILED_MAX,&m)==1);CHECK(m.frames==27&&m.duration==2743);CHECK(bm_validate(&r,BM_CUSTOM_MAX,0)==0);
    bm_decoder_t d;bm_decoder_begin(&d,r,&m);uint8_t v;for(unsigned i=0;i<86400;i++)CHECK(bm_next(&d,&v));CHECK(bm_finish(&d,&m));CHECK(!bm_next(&d,&v));
    unsigned fields[]={0,1,2,3,4,6,8,10,12,16,20,24,28,48,64,118};
    for(unsigned i=0;i<sizeof fields/sizeof fields[0];i++){memcpy(p,copy,n);p[fields[i]]^=0x80;CHECK(bm_validate(&r,BM_COMPILED_MAX,0)==0);}
    memcpy(p,copy,n);
    for(unsigned i=0;i<128;i++){r.size=i;CHECK(bm_validate(&r,BM_COMPILED_MAX,0)==0);}r.size=n;
    for(unsigned i=0;i<200;i++){fault_t x={p,(uint32_t)n,i,0};bm_reader_t q={fault_read,&x,(uint32_t)n};CHECK(bm_validate(&q,BM_COMPILED_MAX,0)==-1);CHECK(!x.bad);}
    /* Callback bounds survive declared/backing mismatch: no decoder reads past header's declared extent. */
    fault_t x={p,(uint32_t)n,UINT32_MAX,0};bm_reader_t q={fault_read,&x,(uint32_t)n-1};CHECK(bm_validate(&q,BM_COMPILED_MAX,0)==0);CHECK(!x.bad);
    free(p);free(copy);printf("boot media: %u checks PASS\n",checks);return 0;
}
