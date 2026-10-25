/* SPDX-License-Identifier: GPL-3.0-only
 * Direct production bass303 renderer. Host diagnostics only; no firmware edits.
 * argv: output.f32 frames note velocity cutoff reso env wave fenv attack decay sustain release slide sequence accent reserved0 reserved0
 * Sequence 0 isolated; 1 repeated; 2 overlap/slide; 3 overlap/retrigger.
 * Optional --experimental-slide-tau-ms T overrides host instance slew only.
 * Optional --experimental-gate-model GAIN_DB BETA SLOPE_DB_PER_S multiplies the
 * rendered samples by one explicit host gate law. It is off unless named.
 * The law restarts on a non-slide note-on, keeps running through a slide, and
 * holds its last value after the engine note-off. Old-note release is ignored
 * by this explicit monophonic harness. Firmware DSP is not modified.
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "bass303.h"

/* Engine gate edges this harness already sends to bass303. */
static void gate_segment(int frame,int seq,int *on,int *origin,int *frozen){
 if(seq<=1){
  if(frame<22050){*on=1;*origin=0;*frozen=22050;return;}
  if(seq==0||frame<44100){*on=0;*origin=0;*frozen=22050;return;}
  if(frame<66150){*on=1;*origin=44100;*frozen=66150;return;}
  *on=0;*origin=44100;*frozen=66150;return;
 }
 if(seq==2){
  if(frame<33075){*on=1;*origin=0;*frozen=33075;return;}
  *on=0;*origin=0;*frozen=33075;return;
 }
 if(frame<11025){*on=1;*origin=0;*frozen=11025;return;}
 if(frame<33075){*on=1;*origin=11025;*frozen=33075;return;}
 *on=0;*origin=11025;*frozen=33075;
}

static float gate_lin(int frame,int seq,int velocity,float gain_db,float beta,float slope){
 int on,origin,frozen;gate_segment(frame,seq,&on,&origin,&frozen);
 int mark=on?frame:frozen;
 float t=(float)(mark-origin)/44100.0f;
 float db=gain_db+slope*t;
 float g=powf(10.0f,db*(1.0f/20.0f));
 if(beta!=0.0f)g*=powf((float)velocity/100.0f,beta);
 return g;
}

static int parse_opts(int argc,char **argv,bass303_t *b,int *gate,float *gain_db,float *beta,float *slope){
 int i=19,saw_slide=0,saw_gate=0;
 *gate=0;
 while(i<argc){
  if(!strcmp(argv[i],"--experimental-slide-tau-ms")){
   if(saw_slide||i+1>=argc)return 8;
   float tau=strtof(argv[i+1],0);
   if(!isfinite(tau)||tau<1.0f||tau>500.0f)return 9;
   b->slew_c=expf(-1000.0f/(tau*BASS303_SR));b->slew_pw[0]=1;
   for(int k=1;k<=BASS303_CTRL;k++)b->slew_pw[k]=b->slew_pw[k-1]*b->slew_c;
   saw_slide=1;i+=2;continue;
  }
  if(!strcmp(argv[i],"--experimental-gate-model")){
   char *e1,*e2,*e3;float g,be,sl;
   if(saw_gate||i+3>=argc)return 10;
   g=strtof(argv[i+1],&e1);be=strtof(argv[i+2],&e2);sl=strtof(argv[i+3],&e3);
   if(e1==argv[i+1]||*e1||e2==argv[i+2]||*e2||e3==argv[i+3]||*e3)return 10;
   if(!isfinite(g)||!isfinite(be)||!isfinite(sl))return 10;
   if(g<-80.0f||g>24.0f||be<-2.0f||be>4.0f||sl<-40.0f||sl>40.0f)return 11;
   *gate=1;*gain_db=g;*beta=be;*slope=sl;saw_gate=1;i+=4;continue;
  }
  return 8;
 }
 return 0;
}

int main(int argc,char **argv){
 int a[17],seq,accent,gate=0,rc;
 float gain_db=0,beta=0,slope=0;
 if(argc<19)return 2;
 for(int i=0;i<17;i++)a[i]=atoi(argv[i+2]);
 FILE*f=fopen(argv[1],"wb");if(!f)return 3;
 bass303_t b;bass303_init(&b);
 int ids[]={BASS303_CUTOFF,BASS303_RESO,BASS303_ENVMOD,BASS303_WAVE,BASS303_FENV,BASS303_FATTACK,BASS303_DECAY,BASS303_FSUSTAIN,BASS303_FRELEASE,BASS303_SLIDE};
 for(int i=0;i<10;i++)bass303_set(&b,ids[i],a[i+3]);
 bass303_set(&b,BASS303_DRIVE,0);bass303_set(&b,BASS303_DRVTYPE,0);
 bass303_set(&b,BASS303_VOLUME,96);bass303_set(&b,BASS303_TUNE,64);
 rc=parse_opts(argc,argv,&b,&gate,&gain_db,&beta,&slope);
 if(rc){fclose(f);return rc;}
 seq=a[13];accent=a[14]; /* a[15..16] reserved and checked below */
 if(a[15]||a[16]){fclose(f);return 4;}
 if(gate&&(seq<0||seq>3)){fclose(f);return 11;}
 for(int pos=0;pos<a[0];){
  if(pos==0)bass303_note_on_velocity(&b,a[1],0,0,a[2]);
  if(seq>=2&&pos==11025)bass303_note_on_velocity(&b,a[1]+7,accent,seq==2,a[2]);
  if(pos==(seq>=2?33075:22050))bass303_note_off(&b);
  if(seq==1&&pos==44100)bass303_note_on_velocity(&b,a[1],accent,0,a[2]);
  if(seq==1&&pos==66150)bass303_note_off(&b);
  int n=a[0]-pos;if(n>256)n=256;
  int edges[]={11025,16538,22050,33075,44100,66150};for(int i=0;i<6;i++)if(edges[i]>pos&&edges[i]-pos<n)n=edges[i]-pos;
  float out[256];bass303_render(&b,out,n);
  if(gate){for(int i=0;i<n;i++)out[i]*=gate_lin(pos+i,seq,a[2],gain_db,beta,slope);}
  for(int i=0;i<n;i++)if(!isfinite(out[i])){fclose(f);return 5;}
  if(fwrite(out,4,n,f)!=(size_t)n){fclose(f);return 6;}pos+=n;
 }
 return fclose(f)?7:0;
}
