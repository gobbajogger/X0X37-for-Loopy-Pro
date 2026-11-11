/* SPDX-License-Identifier: GPL-3.0-only */
/* Compare incremental controls with complete compilation, including failures. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "chicago_voice.h"
#include "miami_selected_voice.h"
#include "app/original_runtime.h"
#include "chicago_fx2_compact.h"
static unsigned checks,widths;
#define CHECK(x) do {++checks;if(!(x)){fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);exit(1);}}while(0)
int test_real_original_pw_width(unsigned,float,float,float,float,float,float *);
int original_pw_width(unsigned shape,float intensity,float source,float width,float mi,float mw,float *out)
{++widths;return test_real_original_pw_width(shape,intensity,source,width,mi,mw,out);}
#ifdef TEST_BASELINE
#define original_chicago_voice_recompile(c,p,id) original_chicago_voice_compile(c,p)
#define original_miami_voice_recompile(c,p,id) original_miami_voice_compile(c,p)
#else
int original_chicago_voice_recompile(original_chicago_voice_controls *,const original_chicago_program *,unsigned);
int original_miami_voice_recompile(original_miami_voice_controls *,const original_miami_program *,unsigned);
#endif
#define EXERCISE(kind,factories,macros,targets,bend,cutoff) \
static void kind##_test(void){ \
 for(unsigned factory=0;factory<factories;factory++){ \
  original_##kind##_program p;original_##kind##_voice_controls actual,expected; \
  CHECK(original_##kind##_program_init(&p,factory));CHECK(original_##kind##_voice_compile(&actual,&p)); \
  for(unsigned step=0;step<384;step++){ \
   unsigned id=step%3==0?bend:step%3==1?cutoff:(step/3)%macros;float value=(float)((step*29)%129)/128; \
   CHECK(original_##kind##_program_set(&p,id,value));CHECK(original_##kind##_voice_compile(&expected,&p));widths=0; \
   CHECK(original_##kind##_voice_recompile(&actual,&p,id));CHECK(!memcmp(&actual,&expected,sizeof actual)); \
   CHECK(widths==((id==bend||id==cutoff)?0:4)); \
  } \
  /* A revision mismatch or unready cache must rebuild, including wraparound. */ \
  actual.revision^=17;CHECK(original_##kind##_program_set(&p,bend,.5f));widths=0; \
  CHECK(original_##kind##_voice_recompile(&actual,&p,bend));CHECK(widths==4); \
  actual.ready=0;CHECK(original_##kind##_program_set(&p,cutoff,.25f));widths=0; \
  CHECK(original_##kind##_voice_recompile(&actual,&p,cutoff));CHECK(widths==4); \
  p.revision=actual.revision=UINT32_MAX;CHECK(original_##kind##_program_set(&p,bend,.5f)); \
  CHECK(original_##kind##_voice_compile(&expected,&p));widths=0; \
  CHECK(original_##kind##_voice_recompile(&actual,&p,bend));CHECK(!memcmp(&actual,&expected,sizeof actual));CHECK(widths==0); \
  /* Invalid unrelated cells and out-of-profile shapes must still reject atomically. */ \
  for(unsigned lane=0;lane<macros+targets;lane++)for(unsigned v=0;v<4;v++){ \
   original_##kind##_program bad=p;float value=v==0?NAN:v==1?-1:v==2?2:1; \
   if(lane<macros)bad.macro[lane]=value;else bad.target[lane-macros]=value; \
   if(original_##kind##_voice_compile(&expected,&bad))continue; \
   bad.revision=actual.revision+1;original_##kind##_voice_controls save=actual; \
   CHECK(!original_##kind##_voice_recompile(&actual,&bad,bend));CHECK(!memcmp(&actual,&save,sizeof actual)); \
  } \
  CHECK(!original_##kind##_voice_recompile(NULL,&p,bend));CHECK(!original_##kind##_voice_recompile(&actual,NULL,bend)); \
 } \
 /* Every 14-bit bend input through the existing mapper, with repeated values. */ \
 original_##kind##_program p;original_##kind##_voice_controls a,e; \
 CHECK(original_##kind##_program_init(&p,0));CHECK(original_##kind##_voice_compile(&a,&p)); \
 for(unsigned n=0;n<16384;n++){float v=n<8192?(float)n/16384:.5f+(float)(n-8192)/16382; \
  CHECK(original_##kind##_program_set(&p,bend,v));CHECK(original_##kind##_voice_compile(&e,&p)); \
  CHECK(original_##kind##_voice_recompile(&a,&p,bend));CHECK(!memcmp(&a,&e,sizeof a)); \
 } \
}
EXERCISE(chicago,81,22,29,20,2)
EXERCISE(miami,30,18,24,16,12)
static void production_owner_test(void){
 static original_runtime_part part;
 static original_chicago_tuning tuning;
 static original_chicago_fx2_compact fx;
 CHECK(original_chicago_tuning_bind(&tuning,0,0,NULL,0x6d70));
 CHECK(original_chicago_fx2_compact_init(&fx));
 original_chicago_fx2_cold_owner owner=original_chicago_fx2_compact_owner(&fx);
 for(unsigned kind=0;kind<2;kind++){
  uint8_t cell[32];CHECK(original_sound_init(cell,kind,0));
  CHECK(original_runtime_init(&part,cell,120,&tuning,&owner)==ORIGINAL_RUNTIME_OK);
  for(unsigned n=0;n<96;n++){
   unsigned id=n%3==0?(kind?16:20):n%3==1?(kind?12:2):0;
   float value=(float)((n/2)%9)/8;
   if(kind){
    original_miami_program p=part.dsp.miami.program;original_miami_voice_controls controls;
    CHECK(original_miami_program_set(&p,id,value));CHECK(original_miami_voice_compile(&controls,&p));widths=0;
    CHECK(original_runtime_param(&part,id,value)==ORIGINAL_RUNTIME_OK);
    CHECK(widths==(id==0?4:0));CHECK(!memcmp(&part.dsp.miami.program,&p,sizeof p));
    CHECK(!memcmp(&part.dsp.miami.controls,&controls,sizeof controls));
   }else{
    original_chicago_program p=part.dsp.chicago.program;original_chicago_voice_controls controls;
    CHECK(original_chicago_program_set(&p,id,value));CHECK(original_chicago_voice_compile(&controls,&p));widths=0;
    CHECK(original_runtime_param(&part,id,value)==ORIGINAL_RUNTIME_OK);
    CHECK(widths==(id==0?4:0));CHECK(!memcmp(&part.dsp.chicago.program,&p,sizeof p));
    CHECK(!memcmp(&part.dsp.chicago.controls,&controls,sizeof controls));
   }
  }
  CHECK(original_runtime_retire(&part)==ORIGINAL_RUNTIME_OK);
 }
}
int main(void){chicago_test();miami_test();production_owner_test();printf("PASS control recompilation: %u checks; exact snapshots, validation and bounded width work\n",checks);return 0;}
