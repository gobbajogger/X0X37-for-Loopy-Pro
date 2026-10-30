/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual object/LZ/project code. Synthetic project only; no hardware. */
#define BREAK_PROJECT_FIXTURE 1
#define main project_stage_fixture_main
#include "project_stage_test.c"
#undef main
static void fresh_counts(void) { project_incompatible=0; fixture("adaptive BREAK persistence"); }
static void legacy_header(uint8_t *header,unsigned mode,unsigned stream) {
 project_stage=proj;
 for(unsigned p=0;p<NPAT;p++) {
  project_stage.pat[p].brk.slice_mode=(uint8_t)mode;
  project_stage.pat[p].brk.rsv=(uint8_t)(0x80+p);
  for(unsigned i=0;i<16;i++)project_stage.pat[p].brk.slice[i]=(uint8_t)(i%9);
 }
 project_file_header(header);put32(header+20,stream);
}
int main(void) {
 uint8_t h[32];
 fresh_counts();
#if X0X_ORIGINAL_SYNTH
 CHECK(PROJ_STREAM == 13); CHECK(PROJ_SND_REV == 13);
 CHECK(stream_known(13));
 for(unsigned stream=1;stream<13;stream++) CHECK(!stream_known(stream));
#else
 CHECK(stream_known(6)); CHECK(stream_known(8));
 CHECK(stream_known(7)==!!X0X_BASS_NOTE_SOUND);
 CHECK(stream_known(9)==!!X0X_BASS_NOTE_SOUND);
 CHECK(!stream_known(4) && !stream_known(5) && !stream_known(10));
#endif
 for(unsigned code=0;code<16;code++) {
  proj.pat[0].brk.slice_mode=(uint8_t)code;proj.pat[0].brk.rsv=0xA5;
  for(unsigned s=0;s<16;s++)proj.pat[0].brk.slice[s]=(uint8_t)(s+17);
  REQUIRE(project_save()==0);snapshot();
  CHECK(project_load_stage()==0);check_live_unchanged();
  CHECK(project_stage.pat[0].brk.slice_mode==code);
  CHECK(project_stage.pat[0].brk.rsv==0xA5);
  CHECK(!memcmp(project_stage.pat[0].brk.slice,proj.pat[0].brk.slice,16));
  project_file_header(h);CHECK(get32(h+20)==PROJ_STREAM);CHECK(project_file_validate(h)==0);
 }
 fresh_counts();
#if !X0X_ORIGINAL_SYNTH
 for(unsigned stream=6;stream<=(X0X_BASS_NOTE_SOUND ? 7u : 6u);stream++)
 for(unsigned mode=0;mode<=1;mode++) {
  legacy_header(h,mode,stream);project_t original=project_stage;
  CHECK(project_file_validate(h)==0);
  project_file_header(h); /* editor VALIDATE canonicalizes before COMMIT */
  CHECK(project_file_validate(h)==0);
  for(unsigned p=0;p<NPAT;p++) {
   CHECK(project_stage.pat[p].brk.slice_mode==0);
   CHECK(project_stage.pat[p].brk.rsv==original.pat[p].brk.rsv);
   CHECK(!memcmp(project_stage.pat[p].brk.slice,original.pat[p].brk.slice,16));
  }
  legacy_header(h,mode,stream);h[12]^=1;original=project_stage;
  CHECK(project_file_validate(h)==-4);CHECK(!memcmp(&original,&project_stage,sizeof original));
 }
 legacy_header(h,1,6);project_stage.pat[0].brk.slice[0]=9;
 put32(h+12,crc32_of((uint8_t *)&project_stage,sizeof project_stage));
 CHECK(project_file_validate(h)==-9);CHECK(project_stage.pat[0].brk.slice_mode==1);
 legacy_header(h,2,6);CHECK(project_file_validate(h)==-9);
 project_stage=proj;project_stage.pat[0].brk.slice_mode=16;project_file_header(h);
 CHECK(project_file_validate(h)==-9);
 project_stage=proj;project_stage.pat[0].brk.slice[0]=33;project_file_header(h);
 CHECK(project_file_validate(h)==-9);
 /* A legacy compressed stream validates CRC before converting its mode byte. */
 fresh_counts();proj.pat[0].brk.slice_mode=1;proj.pat[0].brk.rsv=0xB7;
 REQUIRE(project_save()==0);
 for(int k=0;k<NSTREAM;k++) {
  int n=plat_store_load(STREAM_OBJ[k],io_buf,sizeof io_buf);REQUIRE(n>=4);
  put32(io_buf,6);REQUIRE(plat_store_save(STREAM_OBJ[k],io_buf,(unsigned)n)==0);
 }
 snapshot();CHECK(project_load_stage()==0);check_live_unchanged();
 CHECK(project_stage.pat[0].brk.slice_mode==0);CHECK(project_stage.pat[0].brk.rsv==0xB7);
#endif
 project_stage=proj;project_stage.pat[0].brk.slice_mode=16;project_file_header(h);
 CHECK(project_file_validate(h)==-9);
 project_stage=proj;project_stage.pat[0].brk.slice[0]=33;project_file_header(h);
 CHECK(project_file_validate(h)==-9);
 check_protected_flash();
 printf("adaptive_break_storage: %u checks, %u failures\n",checks,failures);
 return failures!=0;
}
