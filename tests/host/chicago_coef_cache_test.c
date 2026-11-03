/* SPDX-License-Identifier: GPL-3.0-only */
/* A second compile of chicago_voice.c uses the exact pre-cache coefficient
 * expressions (CHICAGO_COEF_CACHE_REFERENCE); compare actual audio and state
 * on every forced helper tick, including parameter changes during notes. */
#include "chicago_voice.h"
#include "chicago_voice_snapshot.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int uncached_original_chicago_voice_init(original_chicago_voice *,const original_chicago_voice_tables *,const original_chicago_voice_seed *);
int uncached_original_chicago_voice_full_note(original_chicago_voice *,const original_chicago_voice_controls *,const original_chicago_voice_tables *,const original_chicago_voice_note *,float);
int uncached_original_chicago_voice_control(original_chicago_voice *,const original_chicago_voice_controls *,const original_chicago_voice_tables *,float,unsigned,unsigned *);
int uncached_original_chicago_voice_render_frame(original_chicago_voice *,const original_chicago_voice_controls *,float *,float *);
int uncached_original_chicago_voice_note_off(original_chicago_voice *,const original_chicago_voice_controls *);

static uint32_t bits(float f){uint32_t u;memcpy(&u,&f,4);return u;}
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL line %u, step %u: %s\n",(unsigned)__LINE__,step,#x);exit(1);}}while(0)
static int lookup(const void *ctx,uint32_t index,float *a,float *e){
 (void)ctx;*a=(float)(1+(index&7))/16.0f;*e=(float)(int)(index&7)/32.0f;return 1;
}
static void state_equal(const original_chicago_voice *a,const original_chicago_voice *b,unsigned step){
 CHECK(a->active==b->active&&a->vca.state==b->vca.state&&a->vcf.state==b->vcf.state);
 CHECK(bits(a->vca.level)==bits(b->vca.level)&&bits(a->vcf.level)==bits(b->vcf.level));
 CHECK(bits(a->mod_cutoff)==bits(b->mod_cutoff)&&bits(a->mod_gain)==bits(b->mod_gain));
 for(unsigned i=0;i<2;i++){
  CHECK(bits(a->mg[i].phase)==bits(b->mg[i].phase));
  CHECK(bits(a->mg[i].bipolar)==bits(b->mg[i].bipolar));
  CHECK(bits(a->mg[i].unipolar)==bits(b->mg[i].unipolar));
 }
 for(unsigned i=0;i<4;i++)CHECK(bits(a->pitch.current[i])==bits(b->pitch.current[i]));
 CHECK(a->wave_preparations==b->wave_preparations);
}
static void note_start(original_chicago_voice *a,original_chicago_voice *b,
  const original_chicago_voice_controls *c,const original_chicago_voice_tables *t,
  unsigned key,unsigned step,float bpm){
 original_chicago_voice_note n;memset(&n,0,sizeof n);
 n.raw_note=(int32_t)key;n.velocity=107;n.unison_flag=1;
 n.tuned_pitch=(float)key;n.last_pitch=(float)key-3.0f;
 for(unsigned i=0;i<2;i++)n.mg[i].phase=-1.0f;
 CHECK(original_chicago_voice_full_note(a,c,t,&n,bpm));
 CHECK(uncached_original_chicago_voice_full_note(b,c,t,&n,bpm));
 state_equal(a,b,step);
}
int main(void){
 unsigned step=0,stages[4]={0},modes[2]={0},nonzero=0;
 int32_t pitches[128];for(unsigned i=0;i<128;i++)pitches[i]=(int32_t)(i+1)*1000000;
 original_chicago_voice_tables table;
 CHECK(original_chicago_voice_tables_bind(&table,lookup,NULL,pitches));
 original_chicago_voice_seed seed;memset(&seed,0,sizeof seed);
 seed.last_pitch=60.0f;for(unsigned i=0;i<2;i++)seed.mg[i].phase=-1.0f;
 original_chicago_program program;original_chicago_voice_controls controls;
 CHECK(original_chicago_program_init(&program,0));
 CHECK(original_chicago_voice_compile(&controls,&program));
 original_chicago_voice a,b;
 CHECK(original_chicago_voice_init(&a,&table,&seed));
 CHECK(uncached_original_chicago_voice_init(&b,&table,&seed));
 const float v[]={0.0f,0.0001f,0.02f,0.11f,0.4f,0.75f,1.0f};
 note_start(&a,&b,&controls,&table,60,step,120.0f);
 for(step=1;step<=2200;step++){
  /* Recompile/adopt a new program during testing; both instances cold-reset. */
  if(step==1100){
   CHECK(original_chicago_program_init(&program,7));
   CHECK(original_chicago_voice_compile(&controls,&program));
   CHECK(original_chicago_voice_init(&a,&table,&seed));
   CHECK(uncached_original_chicago_voice_init(&b,&table,&seed));
   note_start(&a,&b,&controls,&table,65,step,123.5f);
  }
  /* Controls are snapshots in the real owner. Deliberately replace their
   * fields between helper ticks to sweep bitwise cache invalidation. */
  controls.vcf[0]=v[(step/7)%7];
  controls.vcf[1]=v[(step/11)%7];
  controls.vcf[2]=.34f;
  controls.vcf[3]=v[(step/13)%7];
  controls.vca[0]=v[(step/19)%7];
  controls.vca[1]=v[(step/23)%7];
  controls.vca[2]=.42f;
  controls.vca[3]=v[(step/29)%7];
  controls.pitch.portamento=v[(step/5)%7];
  controls.mg_tempo[0]=(uint8_t)((step/31)%2);
  controls.mg_tempo[1]=(uint8_t)(((step+19)/29)%2);
  for(unsigned i=0;i<2;i++){
   controls.mg_base[i]=v[(step/(i?17:37))%7]*32.0f+3.0f;
   controls.mg_times[i]=v[(step/(i?43:41))%7]*8.0f+1.0f;
   controls.mg_frequency[i]=v[(step/(i?47:53))%7];
   modes[controls.mg_tempo[i]?1:0]++;
  }
  /* Both signs of zero are distinct keys even if +0 normalizes the maths. */
  if(step%71==0){controls.vcf[0]=-0.0f;controls.vcf[3]=-0.0f;
                   controls.pitch.portamento=-0.0f;controls.mg_frequency[0]=-0.0f;}
  float bpm=(step/17)%3==0?20.0f:(step/17)%3==1?400.0f:123.456f;
  if(step%113==0&&a.active){
   CHECK(original_chicago_voice_note_off(&a,&controls));
   CHECK(uncached_original_chicago_voice_note_off(&b,&controls));
  }
  if(!a.active&&a.vca.state==6)note_start(&a,&b,&controls,&table,48+(step%32),step,bpm);
  if(a.vcf.state<=3)stages[a.vcf.state==3?3:a.vcf.state]++;
  /* Original control_tick produces D=63 or 64 for these counters. */
  a.counter=b.counter=step%2?43410u:44099u;
  unsigned ac=0,bc=0;
  CHECK(original_chicago_voice_control(&a,&controls,&table,bpm,0,&ac));
  CHECK(uncached_original_chicago_voice_control(&b,&controls,&table,bpm,0,&bc));
  CHECK(ac==bc);
  state_equal(&a,&b,step);
  float la=0,ra=0,lb=0,rb=0;
  CHECK(original_chicago_voice_render_frame(&a,&controls,&la,&ra));
  CHECK(uncached_original_chicago_voice_render_frame(&b,&controls,&lb,&rb));
  CHECK(bits(la)==bits(lb)&&bits(ra)==bits(rb));
  nonzero|=((bits(la)|bits(ra))&0x7fffffffu)!=0;
  CHECK(a.coefficients.glide.valid);
  CHECK(a.coefficients.mg[0].delta_valid&&a.coefficients.mg[1].delta_valid);
 }
 /* Snapshot wire format excludes caches; decoding must clear their validity.
  * Verify the first restored tick produces the same raw audio/state. */
 original_chicago_voice_snapshot shot,restored;
 unsigned char wire[ORIGINAL_CHICAGO_VOICE_SNAPSHOT_BYTES];
 unsigned char roundtrip[ORIGINAL_CHICAGO_VOICE_SNAPSHOT_BYTES];
 memset(&shot,0,sizeof shot);shot.voice=a;shot.program_revision=controls.revision;
 CHECK(original_chicago_voice_snapshot_encode(&shot,wire,sizeof wire));
 memset(&restored,0xa5,sizeof restored);
 CHECK(original_chicago_voice_snapshot_decode(&restored,wire,sizeof wire));
 CHECK(!restored.voice.coefficients.vcf.attack_valid);
 CHECK(!restored.voice.coefficients.vcf.decay_valid);
 CHECK(!restored.voice.coefficients.vcf.release_valid);
 CHECK(!restored.voice.coefficients.vcf.release_ratio_valid);
 CHECK(!restored.voice.coefficients.mg[0].tempo_valid);
 CHECK(!restored.voice.coefficients.mg[0].free_valid);
 CHECK(!restored.voice.coefficients.mg[0].delta_valid);
 CHECK(!restored.voice.coefficients.mg[1].delta_valid);
 CHECK(!restored.voice.coefficients.glide.valid);
 CHECK(original_chicago_voice_snapshot_encode(&restored,roundtrip,sizeof roundtrip));
 CHECK(!memcmp(wire,roundtrip,sizeof wire));
 state_equal(&a,&restored.voice,step);
 if(a.active){
  unsigned ac=0,bc=0;float la=0,ra=0,lb=0,rb=0;
  a.counter=restored.voice.counter=43410u;
  CHECK(original_chicago_voice_control(&a,&controls,&table,120.0f,0,&ac));
  CHECK(original_chicago_voice_control(&restored.voice,&controls,&table,120.0f,0,&bc));
  CHECK(ac==bc);state_equal(&a,&restored.voice,step);
  CHECK(original_chicago_voice_render_frame(&a,&controls,&la,&ra));
  CHECK(original_chicago_voice_render_frame(&restored.voice,&controls,&lb,&rb));
  CHECK(bits(la)==bits(lb)&&bits(ra)==bits(rb));
 }
 CHECK(stages[0]&&stages[1]&&stages[3]&&modes[0]&&modes[1]&&nonzero);
 printf("PASS chicago-coef-cache: %u ticks; attack=%u decay=%u release=%u; tempo=%u free=%u; bit-identical PCM/state\n",
        step-1,stages[0],stages[1],stages[3],modes[1],modes[0]);
 return 0;
}
