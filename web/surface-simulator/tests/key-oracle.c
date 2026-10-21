#include <stdio.h>
#include "firmware/src/app/smk_clip_keys.h"
int main(void) { unsigned note,scale,root,snap,inv; int type; while(scanf("%u %u %u %u %d %u",&note,&scale,&root,&snap,&type,&inv)==6) {uint8_t out[7];unsigned n=ck_notes(note,ck_mask(scale,root),snap,type,inv,out);for(unsigned i=0;i<n;i++)printf("%s%u",i?",":"",out[i]);puts("");} }
