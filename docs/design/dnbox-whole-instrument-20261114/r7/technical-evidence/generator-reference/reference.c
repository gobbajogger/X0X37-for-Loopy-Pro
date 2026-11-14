/* Public-source analytical generator fixture exporter. No firmware/audio/SDK. */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <float.h>
/* Exact unmodified translation unit: static RNG helpers remain callable here. */
#include "source/firmware/src/seq/tb3po.c"

static FILE *out;
static unsigned cases,failures;
static uint32_t bits(float x){uint32_t b;memcpy(&b,&x,4);return b;}
static void cfg(const tb3po_cfg_t*g){fprintf(out,"{\"density\":%u,\"accent\":%u,\"slide\":%u,\"oct_range\":%u,\"root\":%u,\"scale\":%u,\"base_oct\":%u,\"mutate_bars\":%u,\"seed_u32\":%u}",g->density,g->accent,g->slide,g->oct_range,g->root,g->scale,g->base_oct,g->mutate_bars,(unsigned)g->seed);}
static void pattern(const bpart_t*b){
 fprintf(out,"{\"len\":%u,\"rate\":%u,\"dir\":%u,\"transpose\":%u,\"gen\":",b->len,b->rate,b->dir,b->transpose);cfg(&b->gen);
 fputs(",\"steps\":[",out);
 for(unsigned i=0;i<NSTEPS;i++)fprintf(out,"%s[%u,%u,%u,%u]",i?",":"",b->step[i].note,b->step[i].flags,b->step[i].velocity,bstep_velocity(b->step+i));
 fputs("]}",out);
}
static unsigned gates(const bpart_t*b){unsigned n=0;for(unsigned i=0;i<b->len;i++)n+=bstep_gate(b->step+i)!=G_REST;return n;}
static void setup(bpart_t*b,unsigned len,uint32_t seed){memset(b,0,sizeof*b);b->len=(uint8_t)len;b->rate=RATE_32;b->dir=DIR_REV;b->transpose=31;tb3po_defaults(&b->gen,seed);}
static void dirty(bpart_t*b){for(unsigned i=0;i<NSTEPS;i++){b->step[i].note=(uint8_t)(36+i);b->step[i].flags=(uint8_t)(G_NOTE|BS_ACCENT|BS_SLIDE|((1+i%15)<<BS_END_SHIFT));b->step[i].velocity=(uint8_t)(1+i*7%127);}}
static void rest(bpart_t*b){for(unsigned i=0;i<NSTEPS;i++){b->step[i].note=49;b->step[i].flags=G_REST;b->step[i].velocity=37;}}
static void startcase(const char*id,const char*op){fprintf(out,"%s{\"id\":\"%s\",\"operation\":\"%s\",",cases?",":"",id,op);cases++;}
static void pair(unsigned scale,unsigned len){
 char id[80];snprintf(id,sizeof id,"scale-%u-%s-length-%u",scale,TB3PO_SCALE_NAMES[scale],len);
 bpart_t b,again;setup(&b,len,UINT32_C(0x12345678));b.gen.scale=(uint8_t)scale;dirty(&b);
 startcase(id,"NEW_THEN_MUTATE");fputs("\"input_initialization\":\"common_dirty64\",\"input\":",out);pattern(&b);
 tb3po_generate(&b);fputs(",\"new_output\":",out);pattern(&b);
 again=b;tb3po_generate(&again);int repeat=!memcmp(again.step,b.step,sizeof b.step);
 if(!repeat||!gates(&b)||b.gen.seed!=UINT32_C(0x12345678))failures++;
 uint32_t r=UINT32_C(0xCAFEBABE);fprintf(out,",\"new_repeat_same_steps\":%s,\"new_local_rng_output\":null,\"new_local_rng_limit\":\"generate keeps its RNG local and does not return or store its final state\",\"mutate_rng_before_u32\":%u",repeat?"true":"false",(unsigned)r);
 tb3po_mutate(&b,&r);fprintf(out,",\"mutate_rng_after_u32\":%u,\"mutate_output\":",(unsigned)r);pattern(&b);fputc('}',out);
}
static void zerodensity(unsigned len){
 char id[64];snprintf(id,sizeof id,"zero-density-new-length-%u",len);
 bpart_t b;setup(&b,len,UINT32_C(0x5A1));b.gen.density=0;dirty(&b);
 startcase(id,"NEW");fputs("\"input\":",out);pattern(&b);tb3po_generate(&b);
 fputs(",\"output\":",out);pattern(&b);
 unsigned n=gates(&b);fprintf(out,",\"nonrest_active_steps\":%u}",n);
 if(n!=1||bstep_gate(b.step)!=G_NOTE||b.step[0].velocity!=0)failures++;
}
static void allrest(unsigned density){
 char id[64];snprintf(id,sizeof id,"all-rest-mutate-density-%u",density);
 bpart_t b;setup(&b,16,UINT32_C(0xBEEF));b.gen.density=(uint8_t)density;rest(&b);
 uint32_t r=UINT32_C(0x12345678);startcase(id,"MUTATE");fputs("\"input\":",out);pattern(&b);fprintf(out,",\"rng_before_u32\":%u",(unsigned)r);
 tb3po_mutate(&b,&r);fprintf(out,",\"rng_after_u32\":%u,\"output\":",(unsigned)r);pattern(&b);
 unsigned n=gates(&b);fprintf(out,",\"nonrest_active_steps\":%u}",n);
 if((density==0&&n)||(density==100&&!n))failures++;
}
static void hiddenreset(void){
 bpart_t b;setup(&b,1,UINT32_C(0xBEEF));dirty(&b);
 startcase("new-resets-hidden64","NEW");fputs("\"input\":",out);pattern(&b);tb3po_generate(&b);
 unsigned clear=0;for(unsigned i=1;i<64;i++)clear+=b.step[i].flags==G_REST&&b.step[i].velocity==0&&b.step[i].note==note_of(&b.gen,0,0);
 fprintf(out,",\"hidden_slots_reset\":%u,\"output\":",clear);pattern(&b);fputc('}',out);if(clear!=63)failures++;
}
static void manualcleanup(void){
 uint32_t chosen=1;
 for(;;chosen++){uint32_t r=chosen;float a=rng_f(&r),b=rng_f(&r);if(a>=.25f&&b>=.25f)break;if(chosen>100000){failures++;return;}}
 bpart_t b;setup(&b,2,UINT32_C(0xBEEF));rest(&b);b.step[0]=(bstep_t){67,(uint8_t)(G_NOTE|BS_ACCENT|BS_SLIDE|(5u<<BS_END_SHIFT)),73};
 uint32_t r=chosen;startcase("manual-accent-slide-into-rest-cleanup","MUTATE");fputs("\"input\":",out);pattern(&b);fprintf(out,",\"rng_before_u32\":%u,\"rng_selection\":\"first positive seed whose first two source rng_f draws skip mutation\"",(unsigned)r);
 tb3po_mutate(&b,&r);fprintf(out,",\"rng_after_u32\":%u,\"output\":",(unsigned)r);pattern(&b);fputc('}',out);
 if(b.step[0].flags!=(G_NOTE|BS_ACCENT|(5u<<BS_END_SHIFT))||b.step[0].note!=67||b.step[0].velocity!=73)failures++;
}
static void anchor(void){
 bpart_t b,probe;setup(&b,1,UINT32_C(0x5A1));b.gen.density=0;rest(&b);b.step[0]=(bstep_t){67,G_NOTE,73};
 uint32_t chosen=1;
 for(;;chosen++){probe=b;uint32_t r=chosen;tb3po_mutate(&probe,&r);if(probe.step[0].velocity==0&&probe.step[0].note==note_of(&b.gen,0,0)&&probe.step[0].flags==G_NOTE)break;if(chosen>100000){failures++;return;}}
 uint32_t r=chosen;startcase("last-note-removal-forces-anchor","MUTATE");fputs("\"input\":",out);pattern(&b);fprintf(out,",\"rng_before_u32\":%u,\"rng_selection\":\"first positive seed that source-mutation removes the only gate and triggers its anchor\"",(unsigned)r);
 tb3po_mutate(&b,&r);fprintf(out,",\"rng_after_u32\":%u,\"output\":",(unsigned)r);pattern(&b);fputc('}',out);
 if(gates(&b)!=1||b.step[0].velocity!=0)failures++;
}
static void zerofallback(void){
 bpart_t b;setup(&b,16,1);b.gen.seed=0;dirty(&b);uint32_t r=0;
 startcase("mutate-zero-rng-and-zero-config-seed","MUTATE");fputs("\"input\":",out);pattern(&b);fputs(",\"rng_before_u32\":0",out);
 tb3po_mutate(&b,&r);fprintf(out,",\"rng_after_u32\":%u,\"output\":",(unsigned)r);pattern(&b);fputc('}',out);
}
static void actualcfg(void){
 bpart_t b;setup(&b,16,UINT32_C(0x5A1));b.gen.base_oct=2;rest(&b);
 startcase("authored-P1-B-config-seed05A1-base2","NEW");fputs("\"input_origin\":\"Explicit public cfg matching previously inspected pattern-init override; no firmware/UI state or time input\",\"input\":",out);pattern(&b);tb3po_generate(&b);fputs(",\"output\":",out);pattern(&b);fputc('}',out);
}
static void entropynew(void){
 bpart_t b;setup(&b,16,UINT32_C(0x5A1));b.gen.base_oct=2;dirty(&b);
 startcase("new-from-explicit-entropy","NEW_SEED_THEN_NEW");fputs("\"input\":",out);pattern(&b);
 uint32_t entropy=UINT32_C(0x12345678);fprintf(out,",\"entropy_u32\":%u",(unsigned)entropy);
 b.gen.seed=tb3po_new_seed(entropy);tb3po_generate(&b);fputs(",\"output\":",out);pattern(&b);fputc('}',out);
}
int main(int argc,char**argv){
 if(argc!=2||sizeof(float)!=4||FLT_RADIX!=2||FLT_MANT_DIG!=24||FLT_EVAL_METHOD!=0)return 2;
 out=fopen(argv[1],"wb");if(!out)return 2;
 fprintf(out,"{\"schema\":\"tb3po-public-c-math-reference-v1\",\"source_commit\":\"3c41ef4bedf9701f549ce2752b73306e2fab1eb8\",\"binary32\":{\"radix\":%d,\"mantissa_bits\":%d,\"eval_method\":%d,\"FLT_ROUNDS\":%d},\"step_columns\":[\"note_midi\",\"flags_u8\",\"velocity_raw\",\"velocity_effective\"],\"flags\":{\"REST\":%u,\"NOTE\":%u,\"TIE\":%u,\"ACCENT\":%u,\"SLIDE\":%u,\"END_SHIFT\":%u},\"common_dirty64\":\"note36+i;flags NOTE|ACCENT|SLIDE|((1+i%%15)<<4);velocity1+(i*7%%127)\",\"pattern_cases\":[",FLT_RADIX,FLT_MANT_DIG,FLT_EVAL_METHOD,FLT_ROUNDS,G_REST,G_NOTE,G_TIE,BS_ACCENT,BS_SLIDE,BS_END_SHIFT);
 const unsigned lens[3]={1,16,64};for(unsigned s=0;s<6;s++)for(unsigned i=0;i<3;i++)pair(s,lens[i]);
 for(unsigned i=0;i<3;i++)zerodensity(lens[i]);
 allrest(0);allrest(100);hiddenreset();manualcleanup();anchor();zerofallback();actualcfg();entropynew();
 fputs("],\"new_seed_vectors\":[",out);
 const uint32_t entropy[]={0,1,UINT32_C(0xBEEF),UINT32_C(0x5A1),UINT32_C(0x12345678),UINT32_C(0xFFFFFFFF),UINT32_C(0x80000000),UINT32_C(0x8BA68577)};
 for(unsigned i=0;i<sizeof entropy/sizeof*entropy;i++){uint32_t mix=entropy[i]*UINT32_C(2654435761)+UINT32_C(0x9E3779B9);uint32_t n=tb3po_new_seed(entropy[i]);fprintf(out,"%s{\"entropy_u32\":%u,\"mixed_wrap32_u32\":%u,\"new_seed_u32\":%u}",i?",":"",(unsigned)entropy[i],(unsigned)mix,(unsigned)n);if(!n)failures++;}
 fputs("],\"defaults_vectors\":[",out);tb3po_cfg_t g;tb3po_defaults(&g,0);cfg(&g);fputc(',',out);tb3po_defaults(&g,UINT32_C(0x5A1));cfg(&g);
 fputs("],\"rng_vectors\":[",out);const uint32_t seeds[]={0,1,UINT32_C(0xBEEF),UINT32_C(0x5A1),UINT32_C(0x12345678),UINT32_C(0xFFFFFFFF)};
 for(unsigned s=0;s<sizeof seeds/sizeof*seeds;s++){
  uint32_t r=seeds[s];fprintf(out,"%s{\"seed_u32\":%u,\"draws\":[",s?",":"",(unsigned)r);
  for(unsigned i=0;i<8;i++){uint32_t before=r,rf=before,raw=rng_u32(&r);float f=rng_f(&rf),p7=f*7.0f,p5=f*5.0f,p3=f*3.0f;if(rf!=r)failures++;
   fprintf(out,"%s{\"before_u32\":%u,\"rng_after_u32\":%u,\"masked24\":%u,\"float_hex\":\"%a\",\"float_bits_hex\":\"%08x\",\"times7_bits_hex\":\"%08x\",\"degree7\":%d,\"times5_bits_hex\":\"%08x\",\"degree5\":%d,\"times3_bits_hex\":\"%08x\",\"oct3\":%d}",i?",":"",(unsigned)before,(unsigned)raw,(unsigned)(raw&0xFFFFFFu),(double)f,(unsigned)bits(f),(unsigned)bits(p7),(int)p7,(unsigned)bits(p5),(int)p5,(unsigned)bits(p3),(int)p3);
  }fputs("]}",out);
 }
 fputs("],\"percentage_float_vectors\":[",out);const unsigned pct[]={0,25,35,40,70,100};
 for(unsigned i=0;i<sizeof pct/sizeof*pct;i++){float f=(float)pct[i]/100.0f;fprintf(out,"%s{\"percent\":%u,\"float_hex\":\"%a\",\"float_bits_hex\":\"%08x\"}",i?",":"",pct[i],(double)f,(unsigned)bits(f));}
 tb3po_defaults(&g,1);g.accent=g.slide=100;uint32_t r=UINT32_C(0x12345678);unsigned flags=roll_kind(&r,&g);
 fprintf(out,"],\"kind_accent100_slide100\":{\"flags_u8\":%u,\"rng_after_u32\":%u},\"pattern_case_count\":%u,\"reference_check_failures\":%u,\"limit\":\"Pure host public-source math, not firmware/emulator/audio/device execution or independent acceptance\"}\n",flags,(unsigned)r,cases,failures);
 fclose(out);printf("TB3PO_PUBLIC_MATH_REFERENCE cases=%u failures=%u\n",cases,failures);return failures?1:0;
}
