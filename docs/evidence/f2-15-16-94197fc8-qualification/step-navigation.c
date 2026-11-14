/* SPDX-License-Identifier: GPL-3.0-only */
/* Whole production UI/engine. Reuses hardware/clip substitutes of smc_ui_test.c. */
#define SMC_UI_MAIN unused_smc_main
#include "/workspace/f2-15-16-qualification/gobbajogger-X0X37-for-Loopy-Pro-94197fc/tests/host/smc_ui_test.c"
static unsigned bad, assertions;
#define EXPECT(x) do {assertions++;if(!(x)){bad++;printf("  FAIL %s\n",#x);}}while(0)
static void render_blocks(void){int32_t out[512];for(int i=0;i<4;i++)engine_render(out,256);}
static void qual_frame(void){now_ms++;input();render_blocks();}
static void tap(int b){held_btn=1u<<b;qual_frame();held_btn=0;qual_frame();}
int main(void) {
 const char *paths[]={"release","page then release","list then release","HOME cancel then release","part change then release","blocked SEQ then release","list choose page then release","pattern change then release","HELP then release"};
 for(int bank=0;bank<2;bank++)for(int voice=0;voice<2;voice++)for(int path=0;path<9;path++) {
  int before_bad=bad,part=PART_303A+voice,step=2,id=bank==0?2:13,knob=-1,page=-1;
  setup(V_SEQ,part);
  EXPECT(!engine_bass_select_preset(voice,bank,0,&proj.sound));render_blocks();build_pages();
  set_view(V_PART);build_pages();
  for(int p=0;p<pg.n;p++)for(int k=0;k<4;k++){pref_t r=pg.r[p][k];if(r.kind==R_ENG&&r.a==T_303&&r.b==voice&&r.c==id){page=p;knob=k;}}
  EXPECT(page>=0&&knob>=0);if(page<0||knob<0)return 2;
  *page_of(V_PART)=(uint8_t)page;
  pref_t ref=pg.r[page][knob];pref_set(ref,64);render_blocks();
  uint32_t base=engine_original_value_bits(voice,id),other=engine_original_value_bits(1-voice,id);
  set_view(V_SEQ);
  bpart_t *bp=&proj.pat[seq.ppat[part]].bass[voice];bp->len=NSTEPS;bp->step[step].flags=G_NOTE;bp->step[step].note=48;
  held_keys=1u<<WHITE_KEY[step];qual_frame();EXPECT(ui.held_step==step);
  tap(B_EDIT);EXPECT(ui.view==V_PART);
  accel_off=1;enc_acc[EN_K1+knob]=5;qual_frame();accel_off=0;
  int lane=motion_find(proj.arr.lane,seq.ppat[part],part,T_303,voice,id);
  EXPECT(lane>=0);EXPECT(engine_original_value_bits(voice,id)!=base);
  static lane_t kept[NLANE];memcpy(kept,proj.arr.lane,sizeof kept);
  switch(path) {
   case 1:tap(B_EDIT);break;
   case 2:tap(B_SEL);EXPECT(ui.overlay==O_LIST);break;
   case 3:tap(B_HOME);break;
   case 4:enc_acc[EN_ALGO]=voice?1:-1;qual_frame();EXPECT(ui.part!=part);break;
   case 5:tap(B_SEQ);EXPECT(ui.view==V_PART);break;
   case 6:tap(B_SEL);enc_acc[EN_SELECT]=1;qual_frame();tap(B_SEL);break;
   case 7:enc_acc[EN_PRESET]=1;qual_frame();break;
   case 8:ui.help=1;qual_frame();break; /* overlay setup only; no help button in host */
  }
  held_keys=0;qual_frame();qual_frame();
  printf("bank=%d voice=%d path=%s base=%08x actual=%08x held=%d keymask=%x\n",bank,voice,paths[path],base,engine_original_value_bits(voice,id),ui.held_step,engine_bass_keys_held());
  EXPECT(engine_original_value_bits(voice,id)==base);
  EXPECT(engine_original_value_bits(1-voice,id)==other);
  EXPECT(ui.held_step<0);EXPECT(!(engine_bass_keys_held()&(1u<<voice)));
  EXPECT(!memcmp(kept,proj.arr.lane,sizeof kept));
  printf("CASE %s\n",bad==before_bad?"PASS":"FAIL");
 }
 for(int part=PART_909;part<=PART_303B;part++) {
  setup(V_SEQ,part);int first=WHITE_KEY[2],second=WHITE_KEY[3];
  held_keys=1u<<first;qual_frame();held_keys|=1u<<second;qual_frame();
  EXPECT(part<NKIT?ui.hold_step==3:ui.held_step==3);
  held_keys&=~(1u<<first);qual_frame();
  EXPECT(part<NKIT?ui.hold_step==3:ui.held_step==3);
  if(part>=PART_303A)EXPECT(engine_bass_keys_held()&(1u<<(part-PART_303A)));
  now_ms+=STEP_TAP_MS;held_keys=0;qual_frame();
  EXPECT(part<NKIT?ui.hold_step<0:ui.held_step<0);
  EXPECT(!(engine_bass_keys_held()&3));
  printf("PASS older-step release preserves successor; final release clears part=%d\n",part);
 }
 printf("step-navigation %u assertions %u failures\n",assertions,bad);return bad?1:0;
}
