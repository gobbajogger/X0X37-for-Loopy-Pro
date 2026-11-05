/* Deterministic whole-BREAK-renderer host trace: compare original checkpoint
 * renderer and repaired bounded reverse seek renderer, using exact verified
 * public Git breaks.c/h blobs. Not a target / MIDI ISR / device test. */
#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include "firmware/src/dsp/breaks.h"

static uint32_t xs(uint32_t *s) {uint32_t x=*s;x^=x<<13;x^=x>>17;x^=x<<5;return *s=x;}
static int32_t errors;
static uint64_t frames, nonzero, zeroafter, clipped;
static uint64_t h=UINT64_C(14695981039346656037);
static void consume(const float *buf, unsigned n, FILE *out, int tail) {
  for (unsigned i=0;i<n;i++) {
    float v=buf[i];
    if (!isfinite(v) || fabsf(v)>32769.0f) {if(errors++<3)fprintf(stderr,"BAD_SAMPLE frame=%llu value=%f\n",(unsigned long long)frames,v);}
    if (v!=0.0f) nonzero++;
    if (tail && v==0.0f) zeroafter++;
    if (fabsf(v)>32767.0f) clipped++;
    uint32_t bits;memcpy(&bits,&v,4);
    for(int j=0;j<4;j++){h^=(uint8_t)(bits>>(8*j));h*=UINT64_C(1099511628211);}
    if (out && fwrite(&bits,4,1,out)!=1) {perror("write");exit(3);}
    frames++;
  }
}
static void rendern(breaks_t *b,int chunks,FILE *out,int tail){float samples[256];for(int i=0;i<chunks;i++){breaks_render(b,samples,256);consume(samples,256,out,tail);}}
static int one(uint32_t n, int mode, FILE *out){
 uint32_t n2=n-3u>=64u?n-3u:n+3u;
 size_t sz=(n+1u)/2u, sz2=(n2+1u)/2u;
 uint8_t *adpcm=malloc(sz),*adpcm2=malloc(sz2);
 breaks_t *b=calloc(1,sizeof(*b));
 if(!adpcm||!adpcm2||!b){fprintf(stderr,"ALLOC_FAIL n=%u\n",n);exit(3);}
 uint32_t seed=0x517b0d3fU+n;
 for(size_t i=0;i<sz;i++) adpcm[i]=(uint8_t)xs(&seed);
 for(size_t i=0;i<sz2;i++) adpcm2[i]=(uint8_t)xs(&seed);
 brk_loop_t a={.adpcm=adpcm,.nsamples=n,.rate=44100,.bars=1};
 brk_loop_t alt={.adpcm=adpcm2,.nsamples=n2,.rate=22050,.bars=2};
 breaks_init(b);breaks_set_loop(b,0,&a);breaks_set_loop(b,1,&alt);
 if(!breaks_bank(b,0)->valid||!breaks_bank(b,1)->valid||breaks_bank(b,0)->n!=n) {fprintf(stderr,"BAD_BANK n=%u\n",n);return 1;}
 breaks_set(b,BRK_COMPLEX,0);
 breaks_set(b,BRK_ROLL,0);
 breaks_set(b,BRK_BCHANCE,0);
 breaks_set(b,BRK_LEVEL,111);
 for(int step=0;step<64;step++){
   if(step==4)breaks_live(b,8,1);
   if(step==12)breaks_live(b,8,0);
   if(step==16)breaks_live(b,0,1);
   if(step==20)breaks_live(b,0,0); /* physical key-release proxy, not MIDI NoteOff transport */
   if(step==24){breaks_live(b,8,1);breaks_live(b,2,1);}
   if(step==26)breaks_live(b,2,0);
   if(step==32){breaks_live(b,8,0);breaks_live(b,10,1);}
   if(step==40)breaks_live(b,10,0);
   if(step==48){breaks_set_loop(b,0,&a);breaks_live(b,8,1);} /* bank generation invalidate */
   if(step==50)breaks_live(b,8,0);
   if(step==52){breaks_set_quiet(b,1);}
   if(step==54)breaks_set_quiet(b,0);
   breaks_step(b,step%16,step/16,5512.5f,(step%7)!=5);
   rendern(b,22,out,0);
 }
 breaks_stop(b);
 breaks_live(b,8,0);
 breaks_live(b,0,0);
 breaks_live(b,2,0);
 breaks_live(b,10,0);
 rendern(b,4,out,1);
 breaks_set_quiet(b,1);
 rendern(b,2,out,1);
 printf("n=%u mode=%d frames=%llu hash=%016llx nonzero=%llu zeros_tail=%llu clipped=%llu err=%d\n",n,mode,(unsigned long long)frames,(unsigned long long)h,(unsigned long long)nonzero,(unsigned long long)zeroafter,(unsigned long long)clipped,errors);
 free(b);free(adpcm2);free(adpcm);return errors?1:0;
}
int main(int argc,char**argv){
 if(argc!=3 || strcmp(argv[1],"--trace")) {fputs("usage: --trace <filename>\n",stderr);return 2;}
 FILE *fp=fopen(argv[2],"wb");if(!fp){perror(argv[2]);return 3;}
 const uint32_t sizes[]={64,65,511,512,513,1023,1024,1025,122880,176400,705600,1411200,2116800};
 int ret=0;for(unsigned i=0;i<sizeof(sizes)/sizeof(sizes[0]);i++)ret|=one(sizes[i],i,fp);
 if(fclose(fp))return 3;
 if(nonzero==0) {fprintf(stderr,"never got nonzero audio\n");return 1;}
 return ret;
}
