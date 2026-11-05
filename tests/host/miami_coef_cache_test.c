/* SPDX-License-Identifier: GPL-3.0-only */
/* Differential bit test for caller-owned Miami caches. Static MG tick is
 * included from production to avoid adding a test-only firmware entrypoint. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../../experiments/original_synth/miami_selected_voice.c"

int original_miami_voice_render_add(original_miami_ms20 *,original_voice_gain *,
 original_oscillators *,const original_prepared_oscillators *,
 float,float *,float *,uint32_t);

static uint32_t bits_of(float f){uint32_t n;memcpy(&n,&f,4);return n;}
static float float_of(uint32_t n){float f;memcpy(&f,&n,4);return f;}
#define CHECK(c) do{if(!(c)){fprintf(stderr,"miami cache assertion: %s at %s:%d\n",#c,__FILE__,__LINE__);exit(1);}}while(0)
static void equal(float got,float want,const char *name,unsigned k){
 if(bits_of(got)!=bits_of(want)){
  fprintf(stderr,"%s [%u]: %08x != %08x\n",name,k,bits_of(got),bits_of(want));
  exit(1);
 }
}
static void reference_mg(original_miami_voice_mg *m,float base,float times,float bpm,uint32_t D){
 float numerator=(bpm*.800000011920929f)*(float)D;
 float denominator=(44100.0f*base)*times;
 float delta=numerator/denominator;if(delta>1)delta=1;
 m->phase=m->phase+delta;if(m->phase>=1){m->phase=m->phase-2.0f;m->wrapped=1;}
}
static void check_selected_mg(void){
 static const float bases[]={96,48,36,24,16,12,8,6,4,3};
 static const float tempos[]={20,20,37,64,96,120,120.125f,240,400};
 static const uint32_t durations[]={1,2,32,63,64};
 original_miami_voice_mg got[2]={{0}},want[2]={{0}};
 original_miami_coefficient_cache cache={0};
 for(unsigned i=0;i<2;i++)got[i].phase=want[i].phase=-1.0f;
 for(unsigned k=0;k<50000;k++){
  unsigned i=(k/3)%2;
  float b=bases[(k/37)%10],times=(float)(1+(k/113)%16);
  float bpm=tempos[(k/197)%9];uint32_t D=durations[(k/47)%5];
  reference_mg(&want[i],b,times,bpm,D);
  mg_advance(&got[i],&cache,i,b,times,bpm,D);
  equal(got[i].phase,want[i].phase,"selected MG phase",k);
  CHECK(got[i].wrapped==want[i].wrapped);
  CHECK((cache.mg_valid_mask&(1u<<i))&&cache.base_bits[i]==bits_of(b));
  CHECK(cache.times_bits[i]==bits_of(times)&&cache.bpm_bits==bits_of(bpm));
  CHECK(cache.duration==D);
  equal(cache.denominator[i],(44100.0f*b)*times,"MG denominator",k);
 }
 CHECK(cache.mg_valid_mask);
}
static void check_full_mg(void){
 static const float tempos[]={20,45,90,125.5f,180,400};
 static const uint32_t durations[]={1,2,21,63,64};
 original_mg_tempo_cache cache[2]={{0}};
 float got[2]={-1,-1},want[2]={-1,-1};
 for(unsigned k=0;k<30000;k++){
  unsigned i=(k/4)%2,rate=(k/131)%8,wi=0,wc=0;
  float bpm=tempos[(k/43)%6];uint32_t D=durations[(k/137)%5];
  CHECK(original_mg_tempo(&want[i],rate,bpm,44100.0f,D,&wi));
  CHECK(original_mg_tempo_cached(&cache[i],&got[i],rate,bpm,44100.0f,D,&wc));
  equal(got[i],want[i],"checked MG phase",k);
  CHECK(wi==wc&&cache[i].valid);
 }
}
static void check_glide(void){
 static const float ports[]={0.0f,-0.0f,.01f,.25f,.8f,1.0f};
 static const float mods[]={-.75f,-.2f,-0.0f,0.0f,.1f,.75f};
 static const uint32_t durations[]={1,11,32,63,64};
 float got[4]={0,32,64,127},want[4]={0,32,64,127};
 original_glide_cache cache={0};
 for(unsigned k=0;k<20000;k++){
  float target[4];
  for(unsigned j=0;j<4;j++)target[j]=(float)((k/2+j*37)%128);
  float port=ports[(k/31)%6],mod=mods[(k/47)%6];
  float fs=(k/223)%2?48000.0f:44100.0f;
  uint32_t D=durations[(k/97)%5];
  CHECK(original_glide(want,target,port,mod,fs,D));
  CHECK(original_glide_cached(&cache,got,target,port,mod,fs,D));
  for(unsigned j=0;j<4;j++)equal(got[j],want[j],"glide",k);
  CHECK(cache.duration!=0&&cache.port_bits==bits_of(port)&&cache.mod_bits==bits_of(mod));
  CHECK(cache.fs_bits==bits_of(fs)&&cache.duration==D);
 }
 original_glide_cache saved=cache;
 float bad_target[4]={NAN,0,60,127};
 CHECK(!original_glide_cached(&cache,got,bad_target,.2f,0,44100.0f,64));
 CHECK(memcmp(&saved,&cache,sizeof cache)==0);
 CHECK(!original_glide_cached(&cache,got,want,INFINITY,0,44100.0f,64));
 CHECK(memcmp(&saved,&cache,sizeof cache)==0);
}
static void check_pitch(void){
 original_miami_pitch state;
 original_miami_pitch_controls c={0};
 original_glide_cache cache={0};
 int32_t table[128];
 uint32_t increments[4];
 for(unsigned i=0;i<128;i++)table[i]=(int32_t)(20000+1000*i);
 CHECK(original_miami_pitch_init(&state,60));
 float reference[4]={60,60,60,60};
 for(unsigned k=0;k<1024;k++){
  c.portamento=(k/17)%2?.4f:.6f;c.mod_portamento=(k/23)%2?.15f:-.2f;
  for(unsigned i=0;i<4;i++)state.target[i]=(float)((k/3+i*19)%128);
  CHECK(original_glide(reference,state.target,c.portamento,c.mod_portamento,44100.0f,64));
  CHECK(original_miami_pitch_prepare_cached(&state,&c,table,44100.0f,64,increments,&cache));
  for(unsigned i=0;i<4;i++)equal(state.current[i],reference[i],"Miami pitch",k);
 }
 CHECK(cache.duration!=0);
 CHECK(original_miami_pitch_note(&state,70,60,15,0));
 /* The cache belongs to the part owner, not the pitch; the owner
  * observes cache_witness=0 on the enclosing voice and clears it. */
 CHECK(cache.duration!=0);
}
/* Helper-only witness coverage. Bound owner, decode/adoption, reset and
 * factory-replacement coverage lives in miami_coef_owner_lifecycle_test.c. */
static void check_invalidation(void){
 original_miami_selected_voice s={0};
 s.cache_witness=1;
 original_miami_selected_voice_invalidate_caches(&s);
 CHECK(!s.cache_witness);
 original_miami_coefficient_cache shared={0};
 shared.mg_valid_mask=3;shared.glide.duration=63;
 CHECK(shared.mg_valid_mask==3&&shared.glide.duration==63);
 /* These values were assigned by this test and are not owner coverage. */
}
static void check_triangle(uint32_t triangle_bits){
 original_osc_controls c={0};
 for(unsigned i=0;i<4;i++){
  c.increment[i]=0x01000000u*(i+1);c.shape[i]=0;c.width[i]=0;c.gain[i]=.75f;
 }
 original_prepared_oscillators prep={0};
 CHECK(original_oscillator_prepare(&prep,&c));
 /* Prepared triangle is usually -0 for small increments. Force +0 explicitly. */
 for(unsigned i=0;i<4;i++)prep.triangle[i]=float_of(triangle_bits);
 original_voice_gain kernel_gain={0},scalar_gain={0};
 for(unsigned i=0;i<4;i++)kernel_gain.oscillator[i].current=kernel_gain.oscillator[i].target=.75f;
 for(unsigned i=0;i<2;i++)kernel_gain.stereo[i].current=kernel_gain.stereo[i].target=.5f;
 scalar_gain=kernel_gain;
 original_miami_ms20 kernel_filter={0},scalar_filter={0};
 CHECK(original_miami_ms20_init(&kernel_filter,0,.125f,.1f,.05f));
 kernel_filter.prepared=1;
 kernel_filter.resonance.current=kernel_filter.resonance.target=.05f;
 kernel_filter.noise_gain.current=kernel_filter.noise_gain.target=.01f;
 scalar_filter=kernel_filter;
 original_oscillators kernel_osc={0},scalar_osc={0};
 for(unsigned i=0;i<4;i++)kernel_osc.phase[i]=(i%2?0xc0000000u:0x40000000u)+c.increment[i];
 scalar_osc=kernel_osc;
 float L[256]={0},R[256]={0};
 CHECK(original_miami_voice_render_add(&kernel_filter,&kernel_gain,&kernel_osc,&prep,1.0f,L,R,256));
 for(unsigned j=0;j<256;j++){
  float mix,filtered,l,r;
  CHECK(original_voice_gain_oscillator(&scalar_gain,&scalar_osc,&prep,0,&mix));
  CHECK(original_miami_ms20_sample(&scalar_filter,mix,1.0f,&filtered));
  CHECK(original_voice_gain_stereo(&scalar_gain,filtered,&l,&r));
  equal(L[j],0.0f+l,"triangle L",j);
  equal(R[j],0.0f+r,"triangle R",j);
 }
 for(unsigned i=0;i<4;i++)CHECK(kernel_osc.phase[i]==scalar_osc.phase[i]);
 for(unsigned i=0;i<5;i++)equal(kernel_filter.history[i],scalar_filter.history[i],"MS20 history",i);
}
int main(void){
 check_selected_mg();
 check_full_mg();
 check_glide();
 check_pitch();
 check_invalidation();
 check_triangle(UINT32_C(0x00000000));
 check_triangle(UINT32_C(0x80000000));
 check_triangle(UINT32_C(0xbe8020c5));
 puts("miami coefficient cache: bitwise differential tests PASS");
 return 0;
}
