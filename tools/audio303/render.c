/* SPDX-License-Identifier: GPL-3.0-only
 * Direct production bass303 renderer. Host diagnostics only; no firmware edits.
 * argv: output.f32 frames note velocity cutoff reso env wave fenv attack decay sustain release slide sequence accent reserved0 reserved0
 * Sequence 0 isolated; 1 repeated; 2 overlap/slide; 3 overlap/retrigger.
 * Optional --experimental-slide-tau-ms T overrides host instance slew only.
 * Old-note release is ignored by this explicit monophonic harness.
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "bass303.h"
int main(int argc,char **argv){
 if(argc!=19 && argc!=21)return 2;
 int a[17];for(int i=0;i<17;i++)a[i]=atoi(argv[i+2]);
 FILE*f=fopen(argv[1],"wb");if(!f)return 3;
 bass303_t b;bass303_init(&b);
 int ids[]={BASS303_CUTOFF,BASS303_RESO,BASS303_ENVMOD,BASS303_WAVE,BASS303_FENV,BASS303_FATTACK,BASS303_DECAY,BASS303_FSUSTAIN,BASS303_FRELEASE,BASS303_SLIDE};
 for(int i=0;i<10;i++)bass303_set(&b,ids[i],a[i+3]);
 bass303_set(&b,BASS303_DRIVE,0);bass303_set(&b,BASS303_DRVTYPE,0);
 bass303_set(&b,BASS303_VOLUME,96);bass303_set(&b,BASS303_TUNE,64);
 if(argc==21){
  if(strcmp(argv[19],"--experimental-slide-tau-ms"))return 8;
  float tau=strtof(argv[20],0);if(!isfinite(tau)||tau<1||tau>500)return 9;
  b.slew_c=expf(-1000.0f/(tau*BASS303_SR));b.slew_pw[0]=1;
  for(int i=1;i<=BASS303_CTRL;i++)b.slew_pw[i]=b.slew_pw[i-1]*b.slew_c;
 }
 int seq=a[13],accent=a[14]; /* a[15..16] reserved and checked below */
 if(a[15]||a[16])return 4;
 for(int pos=0;pos<a[0];){
  if(pos==0)bass303_note_on_velocity(&b,a[1],0,0,a[2]);
  if(seq>=2&&pos==11025)bass303_note_on_velocity(&b,a[1]+7,accent,seq==2,a[2]);
  if(pos==(seq>=2?33075:22050))bass303_note_off(&b);
  if(seq==1&&pos==44100)bass303_note_on_velocity(&b,a[1],accent,0,a[2]);
  if(seq==1&&pos==66150)bass303_note_off(&b);
  int n=a[0]-pos;if(n>256)n=256;
  int edges[]={11025,16538,22050,33075,44100,66150};for(int i=0;i<6;i++)if(edges[i]>pos&&edges[i]-pos<n)n=edges[i]-pos;
  float out[256];bass303_render(&b,out,n);for(int i=0;i<n;i++)if(!isfinite(out[i]))return 5;
  if(fwrite(out,4,n,f)!=(size_t)n)return 6;pos+=n;
 }
 return fclose(f)?7:0;
}
