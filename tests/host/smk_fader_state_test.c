/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual production keyboard scan + UI store/queue/motion; engine recorder forwards writes. */
#define engine_set keyboard_record_set
#define SMC_UI_MAIN unused_smc_ui_main
#include "smc_ui_test.c"
#undef engine_set
extern void engine_set(int,int,int,int);
extern float keyboard_test_mix_level(int);
static unsigned writes;
static int wt,wv,wi,wn;
void keyboard_record_set(int t,int v,int i,int n) { writes++;wt=t;wv=v;wi=i;wn=n;engine_set(t,v,i,n); }
enum { SMK_BTN_FADER_BANK=1 };
static struct { uint32_t frames,buttons; uint8_t fader[4]; } fm1_in;
static uint8_t smk_fbank;
#include "smk_faders.inc"
static void render(void) { int32_t out[512]; engine_render(out,256); }
static void reset_faders(int bank,int raw) {
 smk_fbank=bank;fm1_in.frames=0;fm1_in.buttons=0;
 for(int j=0;j<4;j++) fm1_in.fader[j]=raw;
 smk_faders();fm1_in.frames=200;writes=0;
}
static pref_t dest(int bank,int i) {
 if(!bank)return PR(R_ENG,T_MIX,i,MX_LEVEL);
 if(i==0)return PR(R_ENG,T_MIX,PART_BRK,MX_LEVEL);
 return i<3?PR(R_ENG,T_FX,0,i==1?FX_RV_LEVEL:FX_DL_LEVEL):PR(R_ENG,T_MST,0,MST_DJF);
}
static void scan(int i,int value) { fm1_in.fader[i]=value;now_ms+=16;smk_faders(); }
int main(int argc,char **argv) {
 if(argc>1 && !strcmp(argv[1],"mute")) {
  /* Audit the owner-requested behavior, not an assertion that current code is
   * correct. Desired mismatches are printed and make this optional audit fail.
   * Ordinary scanner regression mode remains independent of this policy probe. */
  unsigned cases=0, mismatches=0;
  static const char *const kinds[]={"part","909-track","808-track","reverb","delay"};
  for(int kind=0;kind<5;kind++)for(int active=0;active<2;active++)for(int moved=0;moved<2;moved++)for(int epoch=0;epoch<3;epoch++) {
   int drum=kind==1||kind==2, kit=kind==2?PART_808:PART_909;
   setup(drum?V_SEQ:V_HOME,drum?kit:PART_303A);
   int strip=kind>=3?kind+2:0, raw=active?64:100;
   pref_t r=smc_target(strip);pref_set(r,64);smc_frame();
   fader(strip,active?0:raw);if(active)fader(strip,64);
   CHECK(pref_get(r)==64);int before_picked=!!(smc.picked&(1u<<strip));
   /* Freeze the exact muted identity before navigation. Locking does not grant
    * physical takeover; it only preserves which target/button we address. */
   if(epoch==1)CHECK(smc_lock_toggle());
   message(0x90,16+strip,127);message(0x80,16+strip,0);smc_frame();
   if(epoch==1){ui.part=PART_303B;set_view(V_PART);build_pages();smc_frame();CHECK(smc_same(r,smc_target(strip)));}
   writes=0;if(moved){raw=active?80:110;fader(strip,raw);}
   unsigned movement_writes=writes;int during=pref_get(r), during_picked=!!(smc.picked&(1u<<strip));
   int desired=moved?raw:64;
   /* A real external target edit must invalidate prior takeover. The following
    * physical change stays entirely on one side of90, so it must not write. */
   unsigned edited_writes=0;int after_edit=-1;
   if(epoch==2){
    pref_set(r,90);smc_frame();writes=0;raw=active?81:111;fader(strip,raw);
    edited_writes=writes;after_edit=pref_get(r);desired=90;
   }
   message(0x90,16+strip,127);message(0x80,16+strip,0);smc_frame();
   int resumed=pref_get(r), matches=resumed==desired;
   if(epoch==2)matches=matches&&edited_writes==0&&after_edit==90;
   cases++;mismatches+=!matches;
   printf("MUTE_AUDIT %s kind=%s acquired=%d moved=%d epoch=%s raw=%d before_picked=%d movement_writes=%u during_saved=%d during_picked=%d edited_writes=%u after_edit=%d resumed_saved=%d desired=%d cache=%d\n",
          matches?"MATCH":"DESIRED_MISMATCH",kinds[kind],active,moved,epoch==0?"base":epoch==1?"locked-navigation":"real-target-edit",raw,before_picked,movement_writes,during,during_picked,edited_writes,after_edit,resumed,desired,kind>=3?smc.fx_restore[kind-3]:0);
   /* Two further mute/unmute pairs, with no physical movement, must preserve
    * the desired resumed level. A previous mismatch stays a mismatch;
    * repetition cannot bless an already-wrong saved value. */
   for(int cycle=0;cycle<2;cycle++){
    for(int edge=0;edge<2;edge++){message(0x90,16+strip,127);message(0x80,16+strip,0);smc_frame();}
    int stable=pref_get(r)==desired;cases++;mismatches+=!stable;
    printf("MUTE_AUDIT %s kind=%s repeated_cycle=%d resumed_saved=%d previous_resume=%d desired=%d\n",stable?"MATCH":"DESIRED_MISMATCH",kinds[kind],cycle+1,pref_get(r),resumed,desired);
   }
  }
  /* M8 is FX_VOLUME, whereas fader8 is the independent master FILTER. Moving
   * that fader while volume is zero must not redefine the remembered volume. */
  for(int moved=0;moved<2;moved++) {
   setup(V_HOME,PART_303A);pref_t volume=PR(R_ENG,T_FX,0,FX_VOLUME), filter=smc_target(7);
   CHECK(smc_same(filter,PR(R_ENG,T_MST,0,MST_DJF)));
   pref_set(volume,64);pref_set(filter,64);smc_frame();fader(7,0);fader(7,64);
   message(0x90,23,127);message(0x80,23,0);smc_frame();CHECK(pref_get(volume)==0);
   if(moved)fader(7,80);
   int volume_during=pref_get(volume), filter_during=pref_get(filter);
   message(0x90,23,127);message(0x80,23,0);smc_frame();
   int matches=volume_during==0&&pref_get(volume)==64&&filter_during==(moved?80:64);
   cases++;mismatches+=!matches;
   printf("MUTE_AUDIT %s kind=volume/filter-distinction moved=%d volume_during=%d resumed_volume=%d filter=%d desired_volume=64 desired_filter=%d\n",matches?"MATCH":"DESIRED_MISMATCH",moved,volume_during,pref_get(volume),filter_during,moved?80:64);
  }
  printf("MUTE_AUDIT %s cases=%u desired_mismatches=%u; current runtime behavior is not claimed correct by this audit\n",mismatches?"FAIL":"PASS",cases,mismatches);
  return mismatches?1:0;
 }
 if(argc>1 && !strcmp(argv[1],"cold")) {
  int bank=argc>2?atoi(argv[2]):0, i=argc>3?atoi(argv[3]):0;
  int raw=argc>4?atoi(argv[4]):100, frames=argc>5?atoi(argv[5]):206;
  int held=argc>6?atoi(argv[6]):0;
  setup(V_HOME,PART_303A);pref_t r=dest(bank,i);pref_set(r,64);render();writes=0;
  smk_fbank=bank;fm1_in.frames=(uint32_t)frames;fm1_in.fader[i]=(uint8_t)raw;
  fm1_in.buttons=held?(1u<<SMK_BTN_FADER_BANK):0;
  smk_faders();render();
  printf("COLD_FIRST_SCAN bank=%d strip=%d frames=%u parked=%u writes=%u stored=%d\n",bank,i,fm1_in.frames,fm1_in.fader[i],writes,pref_get(r));
  CHECK(writes==0&&pref_get(r)==64&&smk_fbank==bank);
  fm1_in.frames=1000;smk_faders();CHECK(writes==0&&pref_get(r)==64&&smk_fbank==bank);
  fm1_in.buttons=0;smk_faders();CHECK(writes==0&&smk_fbank==bank);
  scan(i,64);CHECK(writes==1&&pref_get(r)==64&&wt==r.a&&wv==r.b&&wi==r.c&&wn==64);
  render();if(r.a==T_MIX)CHECK(fabsf(keyboard_test_mix_level(r.b)-.4096f)<.0001f);
  return 0;
 }

 unsigned cases=0;
 for(int bank=0;bank<2;bank++)for(int i=0;i<4;i++)for(int down=0;down<2;down++)for(int mode=0;mode<5;mode++) {
  setup(V_HOME,PART_303A);pref_t r=dest(bank,i);pref_set(r,64);render();reset_faders(bank,down?127:0);
  /* Page navigation, target edit while away, SMC target lock, SMC unlock. */
  if(mode==1||mode==2){ui.part=PART_808;set_view(V_SEQ);if(mode==2)pref_set(r,80);ui.part=PART_303A;set_view(V_HOME);}
  if(mode>=3){CHECK(smc_lock_toggle());if(mode==4)CHECK(smc_lock_toggle());}
  writes=0;scan(i,down?127:0);CHECK(writes==0);
  int target=mode==2?80:64;scan(i,target);CHECK(writes==1);CHECK(wt==r.a&&wv==r.b&&wi==r.c&&wn==target);CHECK(pref_get(r)==target);
  char num[16],unit[8];pref_value(r,num,unit);CHECK(strstr(ui.msg,num));render();
  if(r.a==T_MIX)CHECK(fabsf(keyboard_test_mix_level(r.b)-(float)(target*target)/10000.f)<0.0001f);
  writes=0;scan(i,target);CHECK(writes==0);
  for(int n=1;n<=(down?target:127-target);n++){int raw=target+(down?-n:n);scan(i,raw);CHECK(pref_get(r)==raw);CHECK(wn==raw);render();}
  cases++;
 }
 for(int bank=0;bank<2;bank++)for(int i=0;i<4;i++)for(int down=0;down<2;down++) {
  setup(V_HOME,PART_303A);pref_t r=dest(bank,i);pref_set(r,64);render();reset_faders(bank,down?127:0);
  /* Coarse crossing publishes the received end, with no hidden intermediate target write. */
  scan(i,down?20:100);CHECK(writes==1&&pref_get(r)==(down?20:100));render();
  /* An acquired parked fader stays parked across page changes and SMC lock. */
  writes=0;ui.part=PART_909;set_view(V_SEQ);scan(i,down?20:100);CHECK(!writes);
  CHECK(smc_lock_toggle());ui.part=PART_303B;set_view(V_PART);scan(i,down?20:100);CHECK(!writes);
  pref_set(r,80);writes=0;scan(i,down?20:100);CHECK(!writes&&pref_get(r)==80);
  /* Physical history survives that external edit; crossing80 works on its next real move. */
  scan(i,down?100:20);CHECK(writes==1&&pref_get(r)==(down?100:20));
  reset_faders(bank,down?127:0);pref_set(r,64);writes=0;
  for(int n=1;n<=127;n++) {
   int raw=down?127-n:n;unsigned before=writes;scan(i,raw);
   /* Current intentional tolerance claims at target +/-2; subsequent raw samples are absolute. */
   int acquired=down?raw<=66:raw>=62;
   CHECK(writes==before+(unsigned)acquired);CHECK(pref_get(r)==(acquired?raw:64));
  }
  cases++;
 }
 for(int down=0;down<2;down++) {
  setup(V_HOME,PART_303A);pref_t r=dest(0,0);pref_set(r,64);int part=engine_motion_part(r.a,r.b);
  int k=motion_alloc(proj.arr.lane,seq.ppat[part],part,r.a,r.b,r.c);CHECK(k>=0);
  for(int j=0;j<NSTEPS;j++)proj.arr.lane[k].val[j]=20;
  engine_motion_enable(1);seq_start(&seq);for(int j=0;j<100;j++)render();CHECK(engine_motion_value(k)==20);
  reset_faders(0,down?127:0);
  printf("TRACE before dir=%d raw=%d writes=%u stored=%d lane=%d gain=%.6f\n",down,fm1_in.fader[0],writes,pref_get(r),engine_motion_value(k),keyboard_test_mix_level(0));
  scan(0,64);CHECK(writes==1&&pref_get(r)==64);
  { char num[16],unit[8];pref_value(r,num,unit);
    printf("TRACE acquisition dir=%d raw=%d writes=%u stored=%d display=%s%s lane=%d gain=%.6f msg=%s\n",down,fm1_in.fader[0],writes,pref_get(r),num,unit,engine_motion_value(k),keyboard_test_mix_level(0),ui.msg);
    int prior=engine_motion_value(k);
    for(int j=0;j<30;j++){render();int lane=engine_motion_value(k);if(j<2||lane!=prior)printf("TRACE render dir=%d block=%d raw=%d writes=%u stored=%d lane=%d gain=%.6f\n",down,j+1,fm1_in.fader[0],writes,pref_get(r),lane,keyboard_test_mix_level(0));prior=lane;}
  }
  for(int j=0;j<100;j++)render();CHECK(engine_motion_value(k)==-1);CHECK(fabsf(keyboard_test_mix_level(0)-.4096f)<.0001f);
 }
 printf("PASS keyboard production UI/engine: %u mapping/direction/navigation/lock sequences, exact acquisitions and two actual motion holds, %u checks\n",cases,checks);
 return 0;
}
