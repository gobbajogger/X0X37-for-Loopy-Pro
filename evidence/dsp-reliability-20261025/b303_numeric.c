#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include "bass303.h"
static float out[256];
int main(void) {
 int lite,drv,wave,note,mask,k,i,fail=0; float global=0;
 for(lite=0;lite<2;lite++) for(drv=0;drv<3;drv++) for(wave=0;wave<2;wave++) for(note=0;note<128;note+=note==0?24:note==120?7:24) for(mask=0;mask<16;mask++) {
 bass303_t b; float peak=0; bass303_init(&b); bass303_set_lite(&b,lite);
 bass303_set(&b,BASS303_WAVE,wave); bass303_set(&b,BASS303_DRVTYPE,drv); bass303_set(&b,BASS303_DRIVE,127); bass303_set(&b,BASS303_VOLUME,127);
 bass303_set(&b,BASS303_CUTOFF,(mask&1)?127:0); bass303_set(&b,BASS303_RESO,(mask&2)?127:0); bass303_set(&b,BASS303_ENVMOD,(mask&4)?127:0); bass303_set(&b,BASS303_ACCENT,(mask&8)?127:0);
 bass303_note_on_velocity(&b,note,1,0,127);
 for(k=0;k<200;k++) { if(k==100)bass303_note_off(&b); bass303_render(&b,out,256); for(i=0;i<256;i++){ if(!isfinite(out[i])){printf("NONFINITE lite=%d drv=%d wave=%d note=%d mask=%d sample=%d\n",lite,drv,wave,note,mask,k*256+i);fail++;goto next;} if(fabsf(out[i])>peak)peak=fabsf(out[i]); }}
 if(peak>global)global=peak;
 next: ;
 }
 printf("fail=%d global_peak=%g\n",fail,global); return fail!=0;
}
