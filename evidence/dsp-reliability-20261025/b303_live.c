#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include "bass303.h"
static uint32_t r=719;static unsigned rng(void){r=r*1664525u+1013904223u;return r;}
int main(void){bass303_t b;float x[256],peak=0;int k,i; bass303_init(&b);bass303_note_on(&b,45,1,0);for(k=0;k<200000;k++){if(k%31==0){int p=(rng()>>16)%BASS303_NPARAMS;int v=(rng()>>16)%128;bass303_set(&b,p,v);}if(k%173==0)bass303_note_on_velocity(&b,(rng()>>16)%128,(rng()>>16)&1,(rng()>>16)&1,1+(rng()>>16)%127);if(k%251==0)bass303_note_off(&b);if(k%191==0)bass303_set_lite(&b,(rng()>>16)&1);int n=1+(rng()>>16)%256;bass303_render(&b,x,n);for(i=0;i<n;i++){if(!isfinite(x[i])){printf("nonfinite block=%d sample=%d rng=%u\n",k,i,r);return 1;}if(fabsf(x[i])>peak)peak=fabsf(x[i]);}}printf("live blocks=200000 finite=PASS peak=%g\n",peak);return 0;}
