#define SMC_UI_MAIN unused_smc_main
#include "/workspace/f2-15-16-qualification/gobbajogger-X0X37-for-Loopy-Pro-94197fc/tests/host/smc_ui_test.c"
#include "physical-input.inc"
static unsigned bad,assertions;
#define EXPECT(x) do {assertions++;if(!(x)){bad++;printf("  FAIL %s\n",#x);}}while(0)
static void qual_audio_block(void){int32_t out[512];engine_render(out,256);}
static void scan(void){now_ms++;ui_input_only();qual_audio_block();ui_input_only();qual_audio_block();}
static void physical(unsigned p,unsigned v){smk__key_event(p,v);held_keys=fm1_in.notes;for(unsigned k=0;k<NKEYS;k++)key_velocity[k]=fm1_in.key_vel[k+5];}
static void edge(unsigned p,unsigned v){physical(p,v);scan();}
static void tap(int b){held_btn=1u<<b;scan();held_btn=0;scan();}
static void fresh(int bank,int voice){
 for(unsigned p=0;p<37;p++)physical(p,0);
 setup(V_PART,PART_303A+voice);
 EXPECT(!engine_bass_select_preset(voice,bank,0,&proj.sound));
 for(int i=0;i<8;i++)qual_audio_block();
 ui.freeplay[voice]=1;build_pages();scan();scan();
 EXPECT(live_owner==voice);
}
int main(void){
 const char *paths[]={"list open","list close HOME","part change","global menu","FX navigation","SMC popup","EDIT page","SEQ navigation","octave","pattern"};
 for(int bank=0;bank<2;bank++)for(int voice=0;voice<2;voice++)for(int path=0;path<10;path++){
  int before=bad;fresh(bank,voice);edge(17,100);
  EXPECT(engine_live_key_probe(voice,2));EXPECT(!engine_live_key_probe(1-voice,2));
  switch(path){
   case 0:tap(B_SEL);EXPECT(ui.overlay==O_LIST);break;
   case 1:tap(B_SEL);tap(B_HOME);EXPECT(ui.overlay==O_NONE&&ui.view==V_PART);break;
   case 2:enc_acc[EN_ALGO]=voice?1:-1;scan();break;
   case 3:tap(B_GLO);EXPECT(ui.view==V_GLO);break;
   case 4:tap(B_FX);EXPECT(ui.view==V_FX);break;
   case 5:fader(0,0);EXPECT(smc_ui_active());scan();break;
   case 6:tap(B_EDIT);break;
   case 7:tap(B_SEQ);EXPECT(ui.view==V_SEQ);break;
   case 8:tap(B_OCTUP);break;
   case 9:enc_acc[EN_PRESET]=1;scan();break;
  }
  printf("bank=%d voice=%d path=%s held-gate=%u source-token=%u other-gate=%u\n",bank,voice,paths[path],engine_live_key_probe(voice,2),engine_live_key_probe(voice,0),engine_live_key_probe(1-voice,2));
  EXPECT(engine_live_key_probe(voice,2)); /* required ownership through actual release */
  EXPECT(!engine_live_key_probe(1-voice,2));
  edge(17,0);scan();
  EXPECT(!engine_live_key_probe(voice,2));EXPECT(!engine_live_key_probe(1-voice,2));
  EXPECT(!(engine_bass_keys_held()&3));printf("CASE %s\n",bad==before?"PASS":"FAIL");
 }
 /* Source switch, successor press and delayed/duplicate old release. */
 for(int bank=0;bank<2;bank++){
  fresh(bank,0);edge(17,100);enc_acc[EN_ALGO]=-1;scan();EXPECT(ui.part==PART_303B);
  edge(18,100);EXPECT(engine_live_key_probe(1,2));
  edge(17,0);EXPECT(engine_live_key_probe(1,2));edge(17,0);EXPECT(engine_live_key_probe(1,2));
  edge(18,0);EXPECT(!engine_live_key_probe(1,2));EXPECT(!(engine_bass_keys_held()&3));
  printf("PASS delayed and duplicate source release keeps successor bank=%d\n",bank);
  fresh(bank,0);edge(17,100);edge(18,100);
  physical(17,0);physical(18,0);scan();EXPECT(!engine_live_key_probe(0,2));EXPECT(!(engine_bass_keys_held()&3));
  printf("PASS simultaneous older/latest releases leave no held note bank=%d\n",bank);
 }
 printf("live-navigation %u assertions %u failures\n",assertions,bad);return bad?1:0;
}
