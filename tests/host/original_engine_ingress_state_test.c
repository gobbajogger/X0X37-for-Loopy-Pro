/* SPDX-License-Identifier: GPL-3.0-only */
#include "program_envelope.h"
#include "miami_voice_snapshot.h"
#include "chicago_voice_snapshot.h"
#include "miami_startup.h"
#include "binary32_math.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static unsigned program_cases,snapshot_cases,startup_cases;
static void rehash(unsigned char *p,size_t n){uint32_t h=2166136261u;for(size_t i=16;i<n;i++)h=(h^p[i])*16777619u;for(unsigned j=0;j<4;j++)p[12+j]=(unsigned char)(h>>(8*j));}
static void put(unsigned char *p,uint32_t x){for(unsigned i=0;i<4;i++)p[i]=(unsigned char)(x>>(8*i));}
static void programs(void){
 unsigned char bytes[224],before[224];
 for(unsigned kind=0;kind<2;kind++)for(unsigned factory=0;factory<(kind?81u:30u);factory++){
  original_miami_program m,mout;original_chicago_program c,cout;assert(kind?original_chicago_program_init(&c,factory):original_miami_program_init(&m,factory));
  for(unsigned parameter=0;parameter<(kind?22u:18u);parameter++)for(unsigned endpoint=0;endpoint<3;endpoint++){
   float x=endpoint==0?-0.0f:endpoint==1?.123456791f:1.0f;
   if(kind){assert(original_chicago_program_set(&c,parameter,x));c.revision=UINT32_MAX;assert(original_chicago_program_encode(&c,bytes,sizeof bytes)&&original_chicago_program_decode(&cout,bytes,224)&&!memcmp(&c,&cout,sizeof c));}
   else{assert(original_miami_program_set(&m,parameter,x));m.revision=UINT32_MAX;assert(original_miami_program_encode(&m,bytes,sizeof bytes)&&original_miami_program_decode(&mout,bytes,188)&&!memcmp(&m,&mout,sizeof m));}
   program_cases++;
  }
 }
 original_miami_program m,out;assert(original_miami_program_init(&m,0));out=m;assert(original_miami_program_encode(&m,bytes,sizeof bytes));memcpy(before,bytes,188);
 for(unsigned i=0;i<188;i++){bytes[i]^=1;assert(!original_miami_program_decode(&out,bytes,188)&&!memcmp(&out,&m,sizeof m));memcpy(bytes,before,188);}
 for(unsigned n=0;n<188;n++)assert(!original_miami_program_decode(&out,bytes,n)&&!memcmp(&out,&m,sizeof m));
 put(bytes+16,0x7fc00000);rehash(bytes,188);assert(!original_miami_program_decode(&out,bytes,188)&&!memcmp(&out,&m,sizeof m));
 memcpy(bytes,before,188);put(bytes+16,0xbf800000);rehash(bytes,188);assert(!original_miami_program_decode(&out,bytes,188));
 memset(bytes,0x55,sizeof bytes);memcpy(before,bytes,sizeof bytes);m.target[3]=original_math_float(0x7f800000);
 assert(!original_miami_program_encode(&m,bytes,sizeof bytes)&&!memcmp(before,bytes,sizeof bytes));
}
static void snapshot(void){
 original_miami_voice_snapshot s,out,before;memset(&s,0xa5,sizeof s);s.voice.pitch.current[0]=-0.0f;s.pending.note=-1;s.voice.mg[0].noise=0xdead6d70;s.program_revision=UINT32_MAX;s.voice_id=127;
 unsigned char bytes[ORIGINAL_MIAMI_VOICE_SNAPSHOT_BYTES],copy[ORIGINAL_MIAMI_VOICE_SNAPSHOT_BYTES],reencoded[ORIGINAL_MIAMI_VOICE_SNAPSHOT_BYTES];
 assert(original_miami_voice_snapshot_encode(&s,bytes,sizeof bytes)&&original_miami_voice_snapshot_decode(&out,bytes,sizeof bytes));assert(original_miami_voice_snapshot_encode(&out,reencoded,sizeof reencoded)&&!memcmp(bytes,reencoded,sizeof bytes));
 assert(original_math_bits(out.voice.pitch.current[0])==0x80000000u&&out.pending.note==-1&&out.voice.mg[0].noise==0xdead6d70u&&out.program_revision==UINT32_MAX&&out.voice_id==127);before=out;memcpy(copy,bytes,sizeof bytes);
 for(unsigned i=0;i<sizeof bytes;i++){bytes[i]^=1;assert(!original_miami_voice_snapshot_decode(&out,bytes,sizeof bytes)&&!memcmp(&before,&out,sizeof out));memcpy(bytes,copy,sizeof bytes);snapshot_cases++;}
 for(unsigned n=0;n<sizeof bytes;n++){assert(!original_miami_voice_snapshot_decode(&out,bytes,n)&&!memcmp(&before,&out,sizeof out));snapshot_cases++;}
 /* Fixed v1 schema: word4=float pitch.current0, word47=byte filter.prepared.
  * Recompute checksum so these exercise type validation, not integrity alone. */
 put(bytes+16+4*4,0x7fa00001);rehash(bytes,sizeof bytes);assert(!original_miami_voice_snapshot_decode(&out,bytes,sizeof bytes)&&!memcmp(&before,&out,sizeof out));
 memcpy(bytes,copy,sizeof bytes);put(bytes+16+4*47,256);rehash(bytes,sizeof bytes);assert(!original_miami_voice_snapshot_decode(&out,bytes,sizeof bytes)&&!memcmp(&before,&out,sizeof out));
 s.voice.filter.history[0]=original_math_float(0x7f800000);memcpy(bytes,copy,sizeof bytes);assert(!original_miami_voice_snapshot_encode(&s,bytes,sizeof bytes)&&!memcmp(bytes,copy,sizeof bytes));
 original_chicago_voice_snapshot chicago,restored,unchanged;memset(&chicago,0xa5,sizeof chicago);chicago.voice.raw_note=187;chicago.voice.raw_velocity=0;chicago.voice.pitch.target[0]=187;chicago.pending.oscillator_note[2]=187;chicago.pending.mask=5;chicago.voice.mg[0].noise=0x12346d70;
 unsigned char chi_bytes[ORIGINAL_CHICAGO_VOICE_SNAPSHOT_BYTES],chi_copy[ORIGINAL_CHICAGO_VOICE_SNAPSHOT_BYTES];
 assert(original_chicago_voice_snapshot_encode(&chicago,chi_bytes,sizeof chi_bytes)&&original_chicago_voice_snapshot_decode(&restored,chi_bytes,sizeof chi_bytes));assert(restored.voice.raw_note==187&&restored.voice.pitch.target[0]==187&&restored.pending.oscillator_note[2]==187&&restored.pending.mask==5&&restored.voice.mg[0].noise==0x12346d70u);unchanged=restored;memcpy(chi_copy,chi_bytes,sizeof chi_bytes);
 for(unsigned i=0;i<sizeof chi_bytes;i++){chi_bytes[i]^=1;assert(!original_chicago_voice_snapshot_decode(&restored,chi_bytes,sizeof chi_bytes)&&!memcmp(&unchanged,&restored,sizeof restored));memcpy(chi_bytes,chi_copy,sizeof chi_bytes);snapshot_cases++;}
 chi_bytes[5]=1;assert(!original_chicago_voice_snapshot_decode(&restored,chi_bytes,sizeof chi_bytes)&&!memcmp(&unchanged,&restored,sizeof restored));
 union {original_chicago_voice_snapshot value;unsigned char data[ORIGINAL_CHICAGO_VOICE_SNAPSHOT_BYTES];} overlap;memcpy(overlap.data,chi_copy,sizeof chi_copy);assert(original_chicago_voice_snapshot_decode(&overlap.value,overlap.data,sizeof overlap.data)&&overlap.value.voice.raw_note==187&&overlap.value.pending.mask==5);
}
typedef struct {unsigned call,fail_stage,render_calls,fail_render;uint32_t *scratch;} observed;
static int wrappers(void *p){observed *s=p;assert(s->call++==0);return s->fail_stage!=1;}
static int core(void *p){observed *s=p;assert(s->call++==1);return s->fail_stage!=2;}
static int restore(void *p,unsigned w1){observed *s=p;assert(s->call++==2&&w1==0);return s->fail_stage!=3;}
static int render(void *p,const uint32_t *il,const uint32_t *ir,uint32_t *ol,uint32_t *orr,uint32_t n){
 observed *s=p;assert(s->call==3&&n==256&&il==s->scratch&&ir==il+256&&ol==il+512&&orr==il+768);
 for(unsigned i=0;i<1024;i++)assert(!s->scratch[i]);s->render_calls++;if(s->fail_render==s->render_calls)return 0;
 for(unsigned i=0;i<256;i++){ol[i]=0x3f800000;orr[i]=0xbf800000;}return 1;
}
static void startup(void){
 uint32_t memory[1026];memory[0]=0x55aa1234;memory[1025]=0xaabb4321;
 for(unsigned k=0;k<5;k++){
  float fs[]={8000,44100,48000,96000,192000};unsigned blocks[]={4,18,19,38,75};
  observed o={0,0,0,0,memory+1};original_miami_startup_owner owner={&o,wrappers,core,restore,render};original_miami_startup_state s;
  assert(original_miami_startup_reset(&s,&owner,memory+1,fs[k])&&s.status&&s.stage==5&&s.blocks==blocks[k]&&s.rendered==256*blocks[k]&&o.render_calls==blocks[k]);assert(memory[0]==0x55aa1234&&memory[1025]==0xaabb4321);startup_cases++;
 }
 for(unsigned fail=1;fail<=4;fail++){
  observed o={0,fail<4?fail:0,0,fail==4?4:0,memory+1};original_miami_startup_owner owner={&o,wrappers,core,restore,render};original_miami_startup_state s;
  assert(!original_miami_startup_reset(&s,&owner,memory+1,44100)&&!s.status&&s.stage==fail);
  assert(fail!=4||(s.blocks==3&&s.rendered==768&&o.render_calls==4));startup_cases++;
 }
 observed o={0,0,0,0,memory+1};original_miami_startup_owner owner={&o,wrappers,core,restore,render};original_miami_startup_state s={1,2,3,4,5},before=s;
 assert(!original_miami_startup_reset(&s,&owner,memory+1,original_math_float(0x7fa00001))&&!o.call&&!memcmp(&s,&before,sizeof s));
 assert(!original_miami_startup_reset(&s,&owner,memory+1,7999)&&!o.call&&!memcmp(&s,&before,sizeof s));
}
int main(void){programs();snapshot();startup();printf("PASS %u program envelope roundtrips/all111factory edits and legacy/malformed/domain rejection;%u full130/135-field voice snapshot integrity/length negatives with signed-zero/full32noise/generated187/pending/revision/overlap witnesses;%u exact reset/restore/warmup schedules+failure-progress cases;engine-owned proposed formats,not storage adoption\n",program_cases,snapshot_cases,startup_cases);return 0;}
