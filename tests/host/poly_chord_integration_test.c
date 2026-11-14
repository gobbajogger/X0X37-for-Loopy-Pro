/* SPDX-License-Identifier: GPL-3.0-only */
/* Combined 0.0.26: actual sequencer -> slot runtime, not a mocked DSP sink. */
#define SMC_UI_MAIN retained_fixture_main
#include "smc_ui_test.c"
#include "app/original_runtime.h"
static void blocks(unsigned n){int32_t audio[512];while(n--)engine_render(audio,256);}
unsigned poly_chord_sequence_keys(int voice);
unsigned poly_chord_sequence_key(int voice,int key);
#define sequence_keys poly_chord_sequence_keys
static void refused_drone_selection(int voice)
{
 static project_t saved;
 struct {
  int bank,status,preset,pending,requested_bank,requested_preset,gate;
  uint32_t epoch;
  unsigned keys;
 } before[NBASS];
 uint8_t notes[]={60,64,67};
 memset(proj.arr.lane,0,sizeof proj.arr.lane);
 CHECK(held_chord_set(proj.arr.lane,0,voice,0,notes,3));
 proj.pat[0].rsv[voice]=(uint8_t)bass_pattern_preset_encode(ORIGINAL_SOUND_DRONE,1);
 memcpy(&saved,&proj,sizeof saved);
 uint32_t serial=engine_bass_choice_serial();
 for(int v=0;v<NBASS;v++){
  before[v].bank=engine_bass_bank(v);before[v].status=engine_bass_status(v);
  before[v].preset=engine_bass_preset_index(v);before[v].pending=engine_bass_preset_pending(v);
  before[v].requested_bank=engine_bass_requested_bank(v);
  before[v].requested_preset=engine_bass_requested_preset(v);
  before[v].gate=engine_bass_preset_gate(v);before[v].epoch=engine_bass_pattern_epoch(v);
  before[v].keys=sequence_keys(v);
 }
 CHECK(engine_bass_select_preset(voice,ORIGINAL_SOUND_DRONE,0,&proj.sound)==-2);
 CHECK(!memcmp(&saved,&proj,sizeof saved)); /* Includes sound cells, tags and chord lanes. */
 CHECK(engine_bass_choice_serial()==serial);
 for(int v=0;v<NBASS;v++){
  CHECK(engine_bass_bank(v)==before[v].bank);CHECK(engine_bass_status(v)==before[v].status);
  CHECK(engine_bass_preset_index(v)==before[v].preset);
  CHECK(engine_bass_preset_pending(v)==before[v].pending);
  CHECK(engine_bass_requested_bank(v)==before[v].requested_bank);
  CHECK(engine_bass_requested_preset(v)==before[v].requested_preset);
  CHECK(engine_bass_preset_gate(v)==before[v].gate);
  CHECK(engine_bass_pattern_epoch(v)==before[v].epoch);
  CHECK(sequence_keys(v)==before[v].keys);
 }
}
int main(void){
 CHECK(original_runtime_kind_available(ORIGINAL_SOUND_DRONE)==X0X_DRONE_RUNTIME_ENABLED);
 CHECK(original_runtime_kind_available(ORIGINAL_SOUND_JUNO));
 int fixture_kind=original_runtime_kind_available(ORIGINAL_SOUND_DRONE)?ORIGINAL_SOUND_DRONE:ORIGINAL_SOUND_JUNO;
 for(int kind=ORIGINAL_SOUND_DRONE;kind<=ORIGINAL_SOUND_JUNO;kind++)for(int v=0;v<NBASS;v++){
  setup(V_PART,PART_303A+v);
  proj.set.bt |= PROJ_BT_CLK_IN; /* SMC fixture overrides this required settings bit. */
  int available=original_runtime_kind_available(kind);
  CHECK(engine_bass_bank_available(v,kind)==available);
  if(!available){
   CHECK(kind==ORIGINAL_SOUND_DRONE);
   refused_drone_selection(v);
  }else{
  CHECK(engine_bass_select_preset(v,kind,0,&proj.sound)==0);
  for(int i=0;i<1024&&engine_bass_preset_pending(v);i++)blocks(1);
  CHECK(engine_bass_bank(v)==kind);
  uint8_t notes[]={60,64,67};
  memset(proj.arr.lane,0,sizeof proj.arr.lane);
  for(int step=0;step<16;step++)proj.pat[0].bass[v].step[step]=(bstep_t){60,G_REST,100};
  proj.pat[0].bass[v].step[0]=(bstep_t){60,G_NOTE,100};
  proj.pat[0].bass[v].step[1]=(bstep_t){60,G_TIE|0xf0,100};
  CHECK(held_chord_set(proj.arr.lane,0,v,0,notes,3));
  if(kind==fixture_kind&&v==0){
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
 }
 printf("Combined poly chord sequencer/runtime: %u checks PASS\n",checks);return 0;
}
