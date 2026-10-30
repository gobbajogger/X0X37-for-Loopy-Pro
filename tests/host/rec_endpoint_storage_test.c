/* SPDX-License-Identifier: GPL-3.0-only */
/* Real NOR/object/LZ staging fixture; only synthetic bytes. */
#define main project_stage_fixture_main
#include "project_stage_test.c"
#undef main

static void fresh(const char *name) {
 project_incompatible=0; /* fixture() replaces the complete simulated device image. */
 fixture(name);
}
static void mark_stream(unsigned format) {
 for(int k=0;k<NSTREAM;k++) {
  int n=plat_store_load(STREAM_OBJ[k],io_buf,sizeof io_buf);
  REQUIRE(n>=4);put32(io_buf,format);REQUIRE(plat_store_save(STREAM_OBJ[k],io_buf,(unsigned)n)==0);
 }
 memset(stored_known,0,sizeof stored_known);
}
static void rejected_preserves_storage(void) {
 memcpy(before_flash,nor,sizeof nor);unsigned e=erases,p=programs,s=saves;
 CHECK(project_load()==-9);CHECK(project_storage_unsupported());
 project_defaults(); /* normal boot fallback may not authorize erasing unknown data */
 CHECK(project_save()==-9);
 CHECK(erases==e && programs==p && saves==s);
 CHECK(!memcmp(nor,before_flash,sizeof nor));check_protected_flash();
 project_reset_stage();CHECK(project_validate_stage()==0);project_commit_stage();
 CHECK(project_save()==0);CHECK(!project_storage_unsupported()); /* explicit validated reset/adoption is the recovery boundary */
}
static void multisegment_fixture(void) {
 musical_fixture();
 for(unsigned k=0;k<8;k++) {
  for(unsigned b=0;b<NKIT;b++) for(unsigned v=0;v<NDRUM;v++) for(unsigned i=0;i<NSW;i++)
   proj.pat[k].drum[b].hit[v][i]=next_random();
  for(unsigned b=0;b<NBASS;b++) for(unsigned i=0;i<NSTEPS;i++)
   proj.pat[k].bass[b].step[i]=(bstep_t){next_random()%128,G_NOTE,next_random()%128};
 }
 REQUIRE(project_save()==0);
 REQUIRE(packed_len>(sizeof io_buf)-STREAM_HDR);
}
static void mixed_segment_storage(void) {
 for(unsigned format=4;format<=9;format++) {
  if(stream_known(format)) continue;
  fresh("used unsupported continuation is preserved");multisegment_fixture();
  int n=plat_store_load(OBJ_PAT1,io_buf,sizeof io_buf);REQUIRE(n>4);
  put32(io_buf,format);REQUIRE(plat_store_save(OBJ_PAT1,io_buf,(unsigned)n)==0);
  snapshot();CHECK(project_load_stage()==-9);check_live_unchanged();
  CHECK(project_storage_unsupported());rejected_preserves_storage();
 }
 fresh("unknown continuation is protected against autosave");multisegment_fixture();
 int n=plat_store_load(OBJ_PAT1,io_buf,sizeof io_buf);REQUIRE(n>4);
 put32(io_buf,0);REQUIRE(plat_store_save(OBJ_PAT1,io_buf,(unsigned)n)==0);
 rejected_preserves_storage(); /* revision 13 rejects every unknown stream */
 fresh("unused stale unsupported tails are ignored");REQUIRE(project_save()==0);
 REQUIRE(packed_len<=(sizeof io_buf)-STREAM_HDR);
 for(unsigned k=1;k<NSTREAM;k++) {
  n=plat_store_load(STREAM_OBJ[k],io_buf,sizeof io_buf);REQUIRE(n>=4);
  put32(io_buf,5);REQUIRE(plat_store_save(STREAM_OBJ[k],io_buf,(unsigned)n)==0);
 }
 snapshot();CHECK(project_load_stage()==0);check_live_unchanged();
 CHECK(project_load()==0);CHECK(!project_storage_unsupported());
 CHECK(project_save()==0);
}
int main(void) {
 mixed_segment_storage();
 fresh("stream6 endpoint roundtrip");
 proj.pat[0].bass[0].step[0]=(bstep_t){60,G_NOTE|(5u<<BS_END_SHIFT),90};
 REQUIRE(project_save()==0);snapshot();
 CHECK(project_load_stage()==0);check_live_unchanged();
 CHECK(project_stage.pat[0].bass[0].step[0].flags==(G_NOTE|(5u<<BS_END_SHIFT)));
 CHECK(project_stage.pat[0].bass[0].step[0].velocity==90);
 int n=plat_store_load(OBJ_PAT0,io_buf,sizeof io_buf);CHECK(n>=4 && get32(io_buf)==PROJ_STREAM);
 mark_stream(3);rejected_preserves_storage();
 for(unsigned format=4;format<=5;format++) {
  fresh("frozen feature format rejected without rewrite");REQUIRE(project_save()==0);
  mark_stream(format);rejected_preserves_storage();
 }
 fresh("legacy3 is rejected even without endpoint bytes");
 proj.pat[0].bass[0].step[0]=(bstep_t){62,G_NOTE|BS_ACCENT|BS_SLIDE,75};
 REQUIRE(project_save()==0);mark_stream(3);
 rejected_preserves_storage(); /* no legacy data migration, no silent rewrite */
 fresh("snapshot target is not reinterpreted as endpoint-only motion");REQUIRE(project_save()==0);
 proj.arr.lane[0].used=1;proj.arr.lane[0].t=0xFE;
 CHECK(project_save()==-9); /* reject even before settings/sound writes */
 REQUIRE(measure_project(&proj,&packed_len)==0);REQUIRE(save_stream()==0); /* synthesize unsupported CRC-valid input */
 rejected_preserves_storage();
 printf("rec_endpoint_storage: %u checks %u failures; no unsupported rewrite\n",checks,failures);
 return failures!=0;
}
