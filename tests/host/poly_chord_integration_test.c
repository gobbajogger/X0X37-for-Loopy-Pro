/* SPDX-License-Identifier: GPL-3.0-only */
/* Combined 0.0.26: actual sequencer -> slot runtime, not a mocked DSP sink. */
#define SMC_UI_MAIN retained_fixture_main
#include "smc_ui_test.c"
static void blocks(unsigned n){int32_t audio[512];while(n--)engine_render(audio,256);}
unsigned poly_chord_sequence_keys(int voice);
unsigned poly_chord_sequence_key(int voice,int key);
#define sequence_keys poly_chord_sequence_keys
int main(void){
 for(int kind=ORIGINAL_SOUND_DRONE;kind<=ORIGINAL_SOUND_JUNO;kind++)for(int v=0;v<NBASS;v++){
  setup(V_PART,PART_303A+v);
  proj.set.bt |= PROJ_BT_CLK_IN; /* SMC fixture overrides this required settings bit. */
  CHECK(engine_bass_select_preset(v,kind,0,&proj.sound)==0);
  for(int i=0;i<1024&&engine_bass_preset_pending(v);i++)blocks(1);
  CHECK(engine_bass_bank(v)==kind);
  uint8_t notes[]={60,64,67};
  memset(proj.arr.lane,0,sizeof proj.arr.lane);
  for(int step=0;step<16;step++)proj.pat[0].bass[v].step[step]=(bstep_t){60,G_REST,100};
  proj.pat[0].bass[v].step[0]=(bstep_t){60,G_NOTE,100};
  proj.pat[0].bass[v].step[1]=(bstep_t){60,G_TIE|0xf0,100};
  CHECK(held_chord_set(proj.arr.lane,0,v,0,notes,3));
  if(kind==ORIGINAL_SOUND_DRONE&&v==0){
   project_capture_stage();CHECK(project_validate_stage()==0);
   uint8_t header[PROJECT_FILE_HDR];project_file_header(header);
   CHECK(header[20]==15);CHECK(project_file_validate(header)==0);
   const char *fixture=getenv("POLY_CHORD_PROJECT_FIXTURE");
   if(fixture){FILE *f=fopen(fixture,"wb");CHECK(f!=NULL);CHECK(fwrite(header,1,sizeof header,f)==sizeof header);CHECK(fwrite(&project_stage,1,sizeof project_stage,f)==sizeof project_stage);CHECK(fclose(f)==0);}
  }
  seq.ppat[TRK_BASS0+v]=0;seq.mute=0;seq_start(&seq);blocks(1);
  CHECK(sequence_keys(v)==3);
  CHECK(poly_chord_sequence_key(v,60));
  CHECK(poly_chord_sequence_key(v,64));
  CHECK(poly_chord_sequence_key(v,67));
  seq_stop(&seq);blocks(1);CHECK(sequence_keys(v)==0);
  seq_start(&seq);blocks(1);CHECK(sequence_keys(v)==3);
  seq.mute|=1u<<(MUTE_BASS0+v);blocks(1);CHECK(sequence_keys(v)==0);
  seq_stop(&seq);blocks(1);seq.mute=0;
  seq_start(&seq);blocks(1);CHECK(sequence_keys(v)==3);
  engine_request_notes_off();blocks(1);CHECK(sequence_keys(v)==0);
 }
 printf("Combined poly chord sequencer/runtime: %u checks PASS\n",checks);return 0;
}
