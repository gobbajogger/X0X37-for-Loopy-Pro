/* SPDX-License-Identifier: GPL-3.0-only */
/* Exact-state and audio fingerprint comparison; sample pointers are process-local. */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include SOURCE
static uint64_t hash(const void *vp, size_t n, uint64_t h) {
    const unsigned char *p=vp;
    while(n--) { h ^= *p++; h *= UINT64_C(1099511628211); }
    return h;
}
int main(void) {
    drum909_t d, copy;
    drum909_init(&d); copy=d;
    for(int i=0;i<4;i++) copy.smp[i].adpcm=0;
    printf("init %016llx\n", (unsigned long long)hash(&copy,sizeof copy,UINT64_C(14695981039346656037)));
    const float vel[]={0.0f,0.25f,0.5f,0.75f,1.0f,1.5f};
    for(int v=0;v<DR_NUM;v++) for(unsigned vi=0;vi<sizeof vel/sizeof vel[0];vi++) for(int profile=0;profile<4;profile++) {
        drum909_init(&d);
        if(profile) for(int p=0;p<drum909_nparams(v);p++) drum909_set(&d,v,p,profile==1?0:profile==2?127:64);
        drum909_trigger(&d,v,vel[vi]);
        uint64_t h=UINT64_C(14695981039346656037);
        for(int off=0;off<131072;off+=128) {
            float l[128]={0},r[128]={0},rev[128]={0},dly[128]={0};
            if(off==32768) drum909_trigger(&d,v,vel[vi]);
            drum909_render_st(&d,l,r,rev,dly,128);
            h=hash(l,sizeof l,h);h=hash(r,sizeof r,h);h=hash(rev,sizeof rev,h);h=hash(dly,sizeof dly,h);
        }
        printf("%d %u %d %016llx\n",v,vi,profile,(unsigned long long)h);
    }
    return 0;
}
