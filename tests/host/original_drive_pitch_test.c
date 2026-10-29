/* SPDX-License-Identifier: GPL-3.0-only */
#include "chicago_drive.h"
#include "chicago_mp4.h"
#include "chicago_packed.h"
#include "miami_pitch.h"
#include "motion_tables_private.h"
#include "chicago_packed_private.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
static unsigned bits(float x){unsigned u;memcpy(&u,&x,4);return u;}
static float fl(unsigned u){float x;memcpy(&x,&u,4);return x;}
static FILE *file(const char *d,const char *n){char p[2048];assert(snprintf(p,sizeof p,"%s/%s",d,n)>0);FILE *f=fopen(p,"r");assert(f);return f;}
static void drive(const char *dir){
 FILE *f=file(dir,"drive.txt");char tag;original_chicago_drive s;unsigned count=0;
 while(fscanf(f," %c",&tag)==1){
  if(tag=='I')original_chicago_drive_init(&s);
  else if(tag=='R')original_chicago_drive_reset(&s);
  else if(tag=='P'){unsigned p,v;assert(fscanf(f,"%u %u",&p,&v)==2);assert(original_chicago_drive_set(&s,p,v));}
  else if(tag=='A'){unsigned a;assert(fscanf(f,"%u",&a)==1);assert(original_chicago_drive_activity(&s,a));}
  else{
   assert(tag=='S');int l,r,ol,orr,expected[7];assert(fscanf(f,"%d %d %d %d %d %d %d %d %d %d %d",&l,&r,&ol,&orr,&expected[0],&expected[1],&expected[2],&expected[3],&expected[4],&expected[5],&expected[6])==11);
   int16_t left=(int16_t)l,right=(int16_t)r,lo,ro;assert(original_chicago_drive_process(&s,&left,&right,&lo,&ro,1));assert(lo==ol&&ro==orr);
   int32_t state[]={s.drive,s.gain,s.threshold,s.previous_driven[0],s.previous_driven[1],s.previous_nonlinear[0],s.previous_nonlinear[1]};assert(!memcmp(state,expected,sizeof state));count++;
  }
 }
 fclose(f);assert(count==81920);
 original_chicago_drive before=s;assert(original_chicago_drive_process(&s,NULL,NULL,NULL,NULL,0)&&!memcmp(&s,&before,sizeof s));assert(!original_chicago_drive_set(&s,2,128)&&!memcmp(&s,&before,sizeof s));assert(!original_chicago_drive_set(&s,1,0)&&!memcmp(&s,&before,sizeof s));
 int16_t l[1024],r[1024],a[1024],b[1024],c[1024],d[1024];for(unsigned i=0;i<1024;i++){l[i]=(int16_t)(i*31u);r[i]=(int16_t)(32767-i*47);}
 original_chicago_drive x=s,y=s,z=s;assert(original_chicago_drive_process(&x,l,r,a,b,1024));for(unsigned i=0;i<1024;i++)assert(original_chicago_drive_process(&y,l+i,r+i,c+i,d+i,1));assert(!memcmp(a,c,sizeof a)&&!memcmp(b,d,sizeof b)&&!memcmp(&x,&y,sizeof x));
 /* Swap initial histories as well before symmetric replay. */
 z=s;int32_t tmp=z.previous_driven[0];z.previous_driven[0]=z.previous_driven[1];z.previous_driven[1]=tmp;tmp=z.previous_nonlinear[0];z.previous_nonlinear[0]=z.previous_nonlinear[1];z.previous_nonlinear[1]=tmp;
 assert(original_chicago_drive_process(&z,r,l,c,d,1024));assert(!memcmp(a,d,sizeof a)&&!memcmp(b,c,sizeof b));
 y=s;memcpy(c,l,sizeof l);memcpy(d,r,sizeof r);assert(original_chicago_drive_process(&y,c,d,c,d,1024)&&!memcmp(c,a,sizeof c)&&!memcmp(d,b,sizeof d));
 printf("PASS Drive %u integer oracle frames, full target pairs, activity, reset, empty, block partition, stereo and in-place; state %zu bytes\n",count,sizeof s);
}
static void pitch(const char *dir){
 FILE *f=file(dir,"pitch.txt");unsigned u,n=0;original_miami_pitch s;original_miami_pitch_controls c;
 while(fscanf(f,"%x",&u)==1){
  memset(&c,0,sizeof c);assert(original_miami_pitch_init(&s,0));s.current[0]=fl(u);for(unsigned i=1;i<4;i++){assert(fscanf(f,"%x",&u)==1);s.current[i]=fl(u);}
  assert(fscanf(f,"%x",&u)==1);for(unsigned i=0;i<4;i++)s.target[i]=fl(u);
  assert(fscanf(f,"%x",&u)==1);c.unison_coordinate=fl(u);assert(fscanf(f,"%x",&u)==1);c.bend=fl(u);assert(fscanf(f,"%x",&u)==1);c.mod_slave_pitch=fl(u);assert(fscanf(f,"%x",&u)==1);c.portamento=fl(u);
  unsigned D;assert(fscanf(f,"%d %d %u",&c.semitone[0],&c.semitone[2],&D)==3);c.semitone[1]=-12;c.bend_range=12;c.unison_detune=.017999999225139618f;c.fine_tune[2]=.003000020980834961f;c.fine_tune[3]=-.003000020980834961f;c.modulation_on=1;
  unsigned expected_current[4],expected_increment[4];for(unsigned i=0;i<4;i++)assert(fscanf(f,"%x",&expected_current[i])==1);for(unsigned i=0;i<4;i++)assert(fscanf(f,"%u",&expected_increment[i])==1);
  unsigned result[4];assert(original_miami_pitch_prepare(&s,&c,original_pitch44100,44100,D,result));for(unsigned i=0;i<4;i++)assert(bits(s.current[i])==expected_current[i]&&result[i]==expected_increment[i]);n++;
 }
 fclose(f);assert(n==8192);assert(original_miami_pitch_init(&s,127));for(unsigned i=0;i<4;i++)assert(s.oscillator.phase[i]==0&&s.current[i]==127);
 s.oscillator.phase[0]=0x12345678;s.oscillator.phase[3]=0xffffffff;assert(original_miami_pitch_note(&s,48,127,15,1));assert(s.current[0]==127&&s.target[0]==48&&s.oscillator.phase[0]==0x12345678&&s.oscillator.phase[3]==0xffffffff);
 assert(original_miami_pitch_note(&s,60,40,2,0)&&s.current[1]==127&&s.target[1]==60&&s.target[0]==48);
 original_miami_pitch before=s;assert(!original_miami_pitch_note(&s,128,0,15,1)&&!memcmp(&s,&before,sizeof s));unsigned result[4]={1,2,3,4},saved[4];memcpy(saved,result,sizeof saved);c.portamento=NAN;assert(!original_miami_pitch_prepare(&s,&c,original_pitch44100,44100,64,result)&&!memcmp(result,saved,sizeof saved)&&!memcmp(&s,&before,sizeof s));
 printf("PASS %u exact four-pitch cases, constructor zero phases, full/partial note phase preservation, MIDI127 and invalid transaction; state %zu bytes\n",n,sizeof s);
}
static void packed_filter(void){
 original_mp4_tables t={original_chicago_a44100,original_chicago_e44100};original_chicago_mp4 raw,packed;
 assert(original_chicago_mp4_init(&raw,t.a[0],t.e[0]));packed=raw;
 for(unsigned j=0;j<=16384;j++){
  float a,b,cutoff=(float)j/16384,R=(float)(j%129)/128;unsigned kick=j%128==0;
  assert(original_chicago_mp4_prepare(&raw,&t,cutoff,0,0,0,0,48,0,R,0,64,kick,&a));
  assert(original_chicago_mp4_prepare_reader(&packed,original_chicago_packed_lookup,NULL,cutoff,0,0,0,0,48,0,R,0,64,kick,&b));
  assert(bits(a)==bits(b)&&!memcmp(&raw,&packed,sizeof raw));
  float x=(float)((int)(j%257)-128)/256;assert(original_chicago_mp4_sample(&raw,x,&a)&&original_chicago_mp4_sample(&packed,x,&b));assert(bits(a)==bits(b)&&!memcmp(&raw,&packed,sizeof raw));
 }
 puts("PASS lossless provider integrated into MP4: all 16385 indices, identical preparation and sample states");
}
static void packed(void){
 for(unsigned i=0;i<16385;i++){unsigned a,e;assert(original_lossless_second16_lookup(&original_chicago_packed_a,i,&a));assert(original_lossless_second16_lookup(&original_chicago_packed_e,i,&e));assert(a==bits(original_chicago_a44100[i])&&e==bits(original_chicago_e44100[i]));}
 unsigned out=123;assert(!original_lossless_second16_lookup(&original_chicago_packed_a,16385,&out)&&out==123);
 original_lossless_table16 bad=original_chicago_packed_a;bad.bytes=1;assert(!original_lossless_second16_lookup(&bad,0,&out)&&out==123);bad=original_chicago_packed_a;bad.blocks--;assert(!original_lossless_second16_lookup(&bad,0,&out)&&out==123);
 uint8_t bytes[]={0,0,0,0,33};uint32_t offsets[]={0};original_lossless_table legacy_bad={bytes,offsets,5,1,1};assert(!original_lossless_lookup(&legacy_bad,0,&out)&&out==123);
 bytes[4]=0;legacy_bad.count=32;assert(original_lossless_lookup(&legacy_bad,31,&out)&&out==0);
 volatile uint32_t sum=0;clock_t start=clock();for(unsigned repeat=0;repeat<8;repeat++)for(unsigned i=0;i<16385;i++){unsigned a,e;assert(original_lossless_second16_lookup(&original_chicago_packed_a,i,&a)&&original_lossless_second16_lookup(&original_chicago_packed_e,i,&e));sum+=a^e;}
 size_t bytes_total=sizeof original_chicago_packed_a_data+sizeof original_chicago_packed_e_data+sizeof original_chicago_packed_a_offset+sizeof original_chicago_packed_e_offset;
 printf("PASS all 32770 exact binary32 coefficients, lookup bounds; storage %zu versus131080 bytes; host262160 lookups %.6f seconds, checksum%u. Optional provider proved; no active firmware.\n",bytes_total,(double)(clock()-start)/CLOCKS_PER_SEC,(unsigned)sum);
}
int main(int argc,char **argv){assert(argc==2);drive(argv[1]);pitch(argv[1]);packed();packed_filter();return 0;}
