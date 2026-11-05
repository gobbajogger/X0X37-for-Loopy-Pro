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

/* These extra cases hold each production cache key and duration for consecutive
 * calls. Cache witnesses below are test controls, not firmware defect claims. */
static void warm_tick(original_chicago_voice *a,original_chicago_voice *b,
 const original_chicago_voice_controls *c,const original_chicago_voice_tables *t,
 float bpm,unsigned duration,unsigned step){
 unsigned ac=0,bc=0;
 a->counter=b->counter=duration==64?43410u:44099u;
 CHECK(original_chicago_voice_control(a,c,t,bpm,0,&ac));
 CHECK(uncached_original_chicago_voice_control(b,c,t,bpm,0,&bc));
 CHECK(ac==bc&&a->last_duration==duration&&b->last_duration==duration);
 state_equal(a,b,step);
 float la=0,ra=0,lb=0,rb=0;
 CHECK(original_chicago_voice_render_frame(a,c,&la,&ra));
 CHECK(uncached_original_chicago_voice_render_frame(b,c,&lb,&rb));
 CHECK(bits(la)==bits(lb)&&bits(ra)==bits(rb));
 CHECK(a->coefficients.glide.valid);
 CHECK(a->coefficients.glide.portamento_bits==bits(c->pitch.portamento));
 CHECK(a->coefficients.glide.duration==duration);
 for(unsigned i=0;i<2;i++){
  const original_chicago_voice_mg_coefficients *m=&a->coefficients.mg[i];
  CHECK(m->delta_valid&&m->duration==duration&&m->mode==c->mg_tempo[i]);
  if(c->mg_tempo[i]){
   CHECK(m->tempo_valid&&m->base_bits==bits(c->mg_base[i]));
   CHECK(m->times_bits==bits(c->mg_times[i])&&m->bpm_bits==bits(bpm));
  }else CHECK(m->free_valid&&m->frequency_bits==bits(c->mg_frequency[i]));
 }
 if(a->vcf.state==0)CHECK(a->coefficients.vcf.attack_valid&&a->coefficients.vcf.attack_bits==bits(c->vcf[0]));
 if(a->vcf.state==1)CHECK(a->coefficients.vcf.decay_valid&&a->coefficients.vcf.decay_bits==bits(c->vcf[1]));
 if(a->vcf.state==3){
  CHECK(a->coefficients.vcf.release_valid&&a->coefficients.vcf.release_bits==bits(c->vcf[3]));
  CHECK(a->coefficients.vcf.release_ratio_valid&&a->coefficients.vcf.release_duration==duration);
 }
}
static void check_warm_branches(const original_chicago_voice_controls *initial,
 const original_chicago_voice_tables *t,const original_chicago_voice_seed *seed){
 unsigned step=3000,warm=0,invalidations=0,controls=0;
 const unsigned stage[]={0,1,3};
 for(unsigned k=0;k<3;k++)for(unsigned mode=0;mode<2;mode++){
  original_chicago_voice_controls c=*initial;
  c.vcf[0]=c.vcf[1]=c.vcf[3]=.7f;c.vcf[2]=.3f;
  c.pitch.portamento=.7f;
  for(unsigned i=0;i<2;i++){
   c.mg_tempo[i]=(uint8_t)mode;c.mg_base[i]=i?6.0f:3.0f;
   c.mg_times[i]=i?2.0f:1.0f;c.mg_frequency[i]=i?.45f:.25f;
  }
  original_chicago_voice a,b;
  CHECK(original_chicago_voice_init(&a,t,seed));
  CHECK(uncached_original_chicago_voice_init(&b,t,seed));
  note_start(&a,&b,&c,t,60,step,120.0f);
  a.vcf.state=b.vcf.state=stage[k];a.vcf.level=b.vcf.level=stage[k]==0?.1f:.8f;
  a.vca.state=b.vca.state=1;a.vca.level=b.vca.level=.8f;
  /* Explicit cold tick, then two warm calls for each duration. */
  memset(&a.coefficients,0,sizeof a.coefficients);
  for(unsigned d=64;d>=63;d--){
   warm_tick(&a,&b,&c,t,120.0f,d,++step);
   for(unsigned n=0;n<2;n++){
    original_chicago_voice_coefficients before=a.coefficients;
    warm_tick(&a,&b,&c,t,120.0f,d,++step);
    CHECK(a.vcf.state==stage[k]);
    CHECK(!memcmp(&before,&a.coefficients,sizeof before));warm++;
   }
  }
  /* Every trial starts from the same warm input and changes exactly one key.
   * Each changed key is followed by another same-key production call. */
  for(unsigned change=0;change<12;change++){
   original_chicago_voice x=a,y=b;original_chicago_voice_controls edited=c;
   float bpm=120.0f;unsigned d=63;
   if(change==0)edited.vcf[stage[k]==0?0:stage[k]==1?1:3]=.6f;
   else if(change==1)edited.pitch.portamento=.6f;
   else if(change==2)d=64;
   else if(change==3){if(!mode)continue;bpm=180.0f;}
   else {
    unsigned i=(change-4)/4,field=(change-4)%4;
    if(field==0)edited.mg_tempo[i]=(uint8_t)!mode;
    if(field==1){if(!mode)continue;edited.mg_base[i]+=3.0f;}
    if(field==2){if(!mode)continue;edited.mg_times[i]+=1.0f;}
    if(field==3){if(mode)continue;edited.mg_frequency[i]+=.1f;}
   }
   warm_tick(&x,&y,&edited,t,bpm,d,++step);invalidations++;
   original_chicago_voice_coefficients before=x.coefficients;
   warm_tick(&x,&y,&edited,t,bpm,d,++step);
   CHECK(!memcmp(&before,&x.coefficients,sizeof before));warm++;
  }
  /* Finite bad-cache witnesses prove that the warm branch consumes cached
   * values. The uncached reference starts from the same unmodified voice.
   * Deliberate divergence is the expected test-control result. */
  for(unsigned witness=0;witness<4;witness++){
   original_chicago_voice x=a,y=a;
   if(witness==0){
    if(stage[k]==0)x.coefficients.vcf.attack_den*=2.0f;
    if(stage[k]==1)x.coefficients.vcf.decay_den*=2.0f;
    if(stage[k]==3)x.coefficients.vcf.release_ratio*=.5f;
   }else if(witness<3)x.coefficients.mg[witness-1].delta*=.5f;
   else x.coefficients.glide.alpha*=.5f;
   unsigned ac=0,bc=0;x.counter=y.counter=44099u;
   CHECK(original_chicago_voice_control(&x,&c,t,120.0f,0,&ac));
   CHECK(uncached_original_chicago_voice_control(&y,&c,t,120.0f,0,&bc));
   CHECK(ac==bc);
   if(witness==0)CHECK(bits(x.vcf.level)!=bits(y.vcf.level));
   else if(witness<3)CHECK(bits(x.mg[witness-1].phase)!=bits(y.mg[witness-1].phase));
   else CHECK(bits(x.pitch.current[0])!=bits(y.pitch.current[0]));
   controls++;
  }
  /* A changed numerator key must reuse the still-warm denominator/frequency
   * layer. Poison only that layer, retaining legitimate keys and validity. */
  for(unsigned i=0;i<2;i++){
   original_chicago_voice x=a,y=a;unsigned ac=0,bc=0;
   float bpm=mode?180.0f:120.0f;unsigned d=mode?63:64;
   if(mode)x.coefficients.mg[i].tempo_den*=2.0f;
   else x.coefficients.mg[i].free_f*=.5f;
   x.counter=y.counter=d==64?43410u:44099u;
   CHECK(original_chicago_voice_control(&x,&c,t,bpm,0,&ac));
   CHECK(uncached_original_chicago_voice_control(&y,&c,t,bpm,0,&bc));
   CHECK(ac==bc&&bits(x.mg[i].phase)!=bits(y.mg[i].phase));controls++;
  }
  if(stage[k]==3){
   original_chicago_voice x=a,y=a;unsigned ac=0,bc=0;
   x.coefficients.vcf.release_den*=2.0f;x.counter=y.counter=43410u;
   CHECK(original_chicago_voice_control(&x,&c,t,120.0f,0,&ac));
   CHECK(uncached_original_chicago_voice_control(&y,&c,t,120.0f,0,&bc));
   CHECK(ac==bc&&bits(x.vcf.level)!=bits(y.vcf.level));controls++;
  }
 }
 CHECK(warm==75&&invalidations==51&&controls==38);
 printf("PASS chicago-warm-cache: warm=%u one-key=%u bad-cache-test-controls=%u; attack/decay/release, two MGs tempo/free, glide, D=63/64; bit-identical PCM/state\n",warm,invalidations,controls);
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
 check_warm_branches(&controls,&table,&seed);
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
