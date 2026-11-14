/* Tests the porter's cached-list boundary against actual AudioKit sp_port.
 * No private preset values, renderer stubs or production firmware changes. */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "soundpipe.h"
#include "parameter_flags.h"
#include "cache_candidate.h"
static uint32_t rng=0x58305837u;
static uint32_t next(void){rng=rng*1664525u+1013904223u;return rng;}
static uint32_t bits(float x){uint32_t u;memcpy(&u,&x,4);return u;}
static int same(const sp_port*a,const sp_port*b){return bits(a->yt1)==bits(b->yt1)&&bits(a->htime)==bits(b->htime)&&bits(a->prvhtim)==bits(b->prvhtim)&&bits(a->c1)==bits(b->c1)&&bits(a->c2)==bits(b->c2)&&bits(a->onedsr)==bits(b->onedsr);}
int main(void){
 const int rates[]={44100,48000,96000};unsigned indexes[PARAM_COUNT], n=build_indexes(indexes);unsigned expected=0;for(unsigned i=0;i<PARAM_COUNT;i++)expected+=smooth_flags[i];
 if(n!=expected){fprintf(stderr,"FAIL selected parameter count: got %u expected %u\n",n,expected);return 1;}
 for(unsigned i=0;i<n;i++){if(indexes[i]>=PARAM_COUNT||!smooth_flags[indexes[i]])return 2;for(unsigned j=0;j<i;j++)if(indexes[j]==indexes[i]){fprintf(stderr,"FAIL duplicated parameter\n");return 1;}}
 unsigned long outputs=0,states=0,reset_checks=0,updates=0;
 for(unsigned sr=0;sr<3;sr++){
  sp_data sp={0};sp.sr=rates[sr];sp_port ref[PARAM_COUNT]={{0}},opt[PARAM_COUNT]={{0}};float a[PARAM_COUNT]={0},b[PARAM_COUNT]={0},target[PARAM_COUNT]={0};
  for(unsigned i=0;i<PARAM_COUNT;i++){sp_port_init(&sp,&ref[i],.0053125f);sp_port_init(&sp,&opt[i],.0053125f);if(smooth_flags[i])prepare_cache(&opt[i]);}
  for(unsigned frame=0;frame<8192;frame++){
   if(frame%7==0){for(unsigned i=0;i<PARAM_COUNT;i++){target[i]=((int)(next()&65535u)-32768)*.03125f;if(!smooth_flags[i])a[i]=b[i]=target[i];}}
   if(frame%64==0){float h=(1+(next()%990000u))*.000001f;for(unsigned i=0;i<PARAM_COUNT;i++)if(smooth_flags[i]){ref[i].htime=opt[i].htime=h;prepare_cache(&opt[i]);}updates++;}
   if(frame%257==0){for(unsigned i=0;i<PARAM_COUNT;i++)if(smooth_flags[i]){float seed=((int)(next()&255u)-128)*.25f;sp_port_reset(&sp,&ref[i],&seed);sp_port_reset(&sp,&opt[i],&seed);reset_checks++;}}
   for(unsigned i=0;i<PARAM_COUNT;i++)if(smooth_flags[i])sp_port_compute(&sp,&ref[i],&target[i],&a[i]);
   for(unsigned j=0;j<n;j++){unsigned i=indexes[j];b[i]=cached_tick(&opt[i],target[i]);}
   for(unsigned i=0;i<PARAM_COUNT;i++){
    if(bits(a[i])!=bits(b[i])){fprintf(stderr,"FAIL output sr=%d frame=%u parameter=%u\n",rates[sr],frame,i);return 1;}outputs++;
    if(smooth_flags[i]){if(!same(&ref[i],&opt[i])){fprintf(stderr,"FAIL state sr=%d frame=%u parameter=%u\n",rates[sr],frame,i);return 1;}states++;}
   }
  }
 }
 printf("CACHE_LIST_ORACLE_PASS outputs=%lu states=%lu resets=%lu half_time_updates=%lu parameters=150 smoothed=%u samplerates=44100,48000,96000\n",outputs,states,reset_checks,updates,n);return 0;
}
