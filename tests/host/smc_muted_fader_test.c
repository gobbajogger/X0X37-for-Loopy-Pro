/* SPDX-License-Identifier: GPL-3.0-only */
/* Real UI/store/SMC input and real engine queue. The recorder forwards writes;
 * no pickup, mute, motion, target, or sound implementation is substituted. */
#define engine_set muted_record_set
#define SMC_UI_MAIN unused_smc_ui_main
#include "smc_ui_test.c"
#undef engine_set
extern void engine_set(int,int,int,int);
extern float muted_test_fx_gain(int);
extern float muted_test_signal(int,int);
enum { SMK_BTN_FADER_BANK=1 };
static struct { uint32_t frames,buttons; uint8_t fader[4]; } fm1_in;
static uint8_t smk_fbank;
#include "smk_muted_faders.inc"
static unsigned writes;
static int wt,wv,wi,wn;
void muted_record_set(int t,int v,int i,int n) { writes++;wt=t;wv=v;wi=i;wn=n;engine_set(t,v,i,n); }
static void render(void) { int32_t out[512]; engine_render(out,256); }
static void sample(int strip,int v,int user) {
 if(user)message(0xB0,40+strip,v);else fader(strip,v);
}
static void toggle(int strip) { message(0x90,16+strip,127);message(0x80,16+strip,0);smc_frame();render(); }
static int strip_of(int kind) { return kind<3?0:kind+2; }
static pref_t prepare(int kind,int acquired,int user) {
 setup(kind==1||kind==2?V_SEQ:V_HOME,kind==2?PART_808:kind==1?PART_909:PART_303A);
 if(user)proj.set.bt|=PROJ_BT_SMC_USER;
 int strip=strip_of(kind);pref_t r=smc_target(strip);
 CHECK(r.kind==R_ENG&&pref_desc(r)->max==127);
 pref_set(r,64);smc_frame();render();sample(strip,acquired?0:100,user);
 if(acquired)sample(strip,64,user);
 CHECK(pref_get(r)==64);CHECK(!!(smc.picked&(1u<<strip))==acquired);
 writes=0;return r;
}
static void muted(int kind,int strip,int yes) {
 CHECK(!!smc_led(16+strip)==yes);
 if(kind==0)CHECK(!!part_muted(0)==yes);
 if(kind==1||kind==2)CHECK(!!track_muted(kind-1,0)==yes);
 if(kind>=3){render();if(yes)CHECK(muted_test_fx_gain(kind-3)==0.0f);else CHECK(muted_test_fx_gain(kind-3)>0.0f);}
}
static void basic_matrix(void) {
 unsigned cases=0;
 for(int kind=0;kind<5;kind++)for(int acquired=0;acquired<2;acquired++)for(int move=0;move<2;move++)for(int user=0;user<2;user++) {
  int strip=strip_of(kind), raw=acquired?64:100;pref_t r=prepare(kind,acquired,user);
  toggle(strip);muted(kind,strip,1);CHECK(pref_get(r)==64);CHECK(!writes);
  /* A parked repeat never seizes an unacquired destination. */
  sample(strip,raw,user);CHECK(pref_get(r)==64);CHECK(!writes);
  if(move){raw=acquired?80:110;sample(strip,raw,user);CHECK(writes==1);CHECK(wt==r.a&&wv==r.b&&wi==r.c&&wn==raw);}
  CHECK(pref_get(r)==(move?raw:64));muted(kind,strip,1);
  writes=0;toggle(strip);muted(kind,strip,0);CHECK(!writes);CHECK(pref_get(r)==(move?raw:64));
  /* Accepted movement remains acquired through unmute: no second pickup. */
  if(move){sample(strip,raw+1,user);CHECK(writes==1&&pref_get(r)==raw+1);raw++;}
  writes=0;
  for(int cycle=0;cycle<10;cycle++){toggle(strip);muted(kind,strip,1);sample(strip,raw,user);toggle(strip);muted(kind,strip,0);CHECK(!writes&&pref_get(r)==(move?raw:64));}
  cases++;
 }
 printf("PASS %u mute periods: part/both drum kits/reverb/delay, acquired/pending, moved/untouched, DAW/User; 400 rapid toggle pairs\n",cases);
}
static void identity_epochs(void) {
 unsigned cases=0;
 for(int kind=0;kind<5;kind++)for(int epoch=0;epoch<7;epoch++) {
  int strip=strip_of(kind);pref_t r=prepare(kind,0,0);toggle(strip);muted(kind,strip,1);
  if(epoch==0){ /* Lock preserves exact destination through page/part navigation. */
   CHECK(smc_lock_toggle());ui.part=PART_303B;set_view(V_PART);build_pages();smc_frame();CHECK(smc_same(r,smc_target(strip)));
   writes=0;fader(strip,110);CHECK(writes==1&&pref_get(r)==110);muted(kind,strip,1);
  }else if(epoch==1){ /* A genuine external base edit rearms takeover. */
   pref_set(r,90);smc_frame();writes=0;fader(strip,111);CHECK(!writes&&pref_get(r)==90);
  }else if(epoch==2){ /* Target change to another muted identity must seed there. */
   if(kind==1||kind==2){ui.sel[kind-1]=8;}else{ui.part=PART_909;set_view(V_SEQ);}
   build_pages();pref_t other=smc_target(strip);CHECK(!smc_same(r,other));pref_set(other,64);
   if(kind==1||kind==2)mute_track(kind-1,8+strip);else mute_track(0,strip);
   smc_frame();writes=0;fader(strip,111);CHECK(!writes&&pref_get(other)==64&&pref_get(r)==64);
  }else if(epoch==3){ /* New connection has no physical history. */
   smc_ble_link(0);smc_poll();smc_ble_link(1);smc_poll();writes=0;fader(strip,110);CHECK(!writes&&pref_get(r)==64);
   fader(strip,111);CHECK(writes==1&&pref_get(r)==111);
  }else if(epoch==4){ /* Exact-match acquisition still publishes through mute. */
   writes=0;fader(strip,64);CHECK(writes==1&&pref_get(r)==64&&(smc.picked&(1u<<strip)));
  }else if(epoch==5){ /* Entering lock does not turn a parked packet into a gesture. */
   CHECK(smc_lock_toggle());writes=0;fader(strip,100);CHECK(!writes&&pref_get(r)==64);
  }else{ /* An unmute/remute with no samples cannot manufacture a gesture. */
   toggle(strip);toggle(strip);writes=0;fader(strip,100);CHECK(!writes&&pref_get(r)==64);
   fader(strip,110);CHECK(writes==1&&pref_get(r)==110);
  }
  cases++;
 }
 printf("PASS %u identity epochs: locked navigation, external edit, exact other target, reconnect, exact match, parked lock, rapid mute epoch\n",cases);
}
static void queued_epochs(void) {
 for(int kind=0;kind<5;kind++) {
  int strip=strip_of(kind);pref_t r=prepare(kind,0,0);toggle(strip);pref_set(r,90);smc_frame();writes=0;fader(strip,111);CHECK(!writes&&pref_get(r)==90);
  /* Two explicit press edges can share a frame. They still constitute a new
   * mute epoch; frame cadence must not decide who owns the intended level. */
  message(0x90,16+strip,127);message(0x80,16+strip,0);
  message(0x90,16+strip,127);message(0x80,16+strip,0);
  writes=0;fader(strip,112);CHECK(writes==1&&pref_get(r)==112);CHECK(volume_muted(r));
 }
 puts("PASS rapid press pairs without an intervening display frame rearm a new explicit mute epoch");
}
static void filter_and_master(void) {
 for(int move=0;move<2;move++) {
  setup(V_HOME,PART_303A);pref_t volume=PR(R_ENG,T_FX,0,FX_VOLUME),filter=smc_target(7);
  CHECK(smc_same(filter,PR(R_ENG,T_MST,0,MST_DJF)));
  pref_set(volume,64);pref_set(filter,64);smc_frame();fader(7,0);fader(7,64);render();writes=0;
  toggle(7);CHECK(smc_led(23)&&pref_get(volume)==64);CHECK(muted_test_fx_gain(2)==0.0f);
  if(move)fader(7,80);
  CHECK(pref_get(filter)==(move?80:64));CHECK(pref_get(volume)==64);CHECK(muted_test_fx_gain(2)==0.0f);
  toggle(7);CHECK(!smc_led(23)&&pref_get(volume)==64);CHECK(muted_test_fx_gain(2)>0.0f);
 }
 puts("PASS M8 master-volume mute remains distinct from fader8 master FILTER");
}
static void audio_gate(void) {
 for(int stereo=0;stereo<2;stereo++)for(int which=0;which<3;which++) {
  setup(V_HOME,PART_303A);
  /* Isolate one real DSP path at a time; gain values remain authoritative. */
  pref_set(PR(R_ENG,T_FX,0,FX_RV_LEVEL),which==0?85:0);
  pref_set(PR(R_ENG,T_FX,0,FX_DL_LEVEL),which==1?85:0);
  pref_set(PR(R_ENG,T_FX,0,FX_VOLUME),100);
  pref_set(PR(R_ENG,T_FX,0,FX_COMP),0);pref_set(PR(R_ENG,T_FX,0,FX_DIST),0);render();
  CHECK(muted_test_signal(which,stereo)>0.1f);
  toggle(which+5);CHECK(smc_led(21+which));CHECK(muted_test_signal(which,stereo)==0.0f);
  pref_t r=PR(R_ENG,T_FX,0,which==0?FX_RV_LEVEL:which==1?FX_DL_LEVEL:FX_VOLUME);
  pref_set(r,110);render();CHECK(pref_get(r)==110);CHECK(muted_test_signal(which,stereo)==0.0f);
  toggle(which+5);CHECK(!smc_led(21+which));CHECK(pref_get(r)==110);CHECK(muted_test_signal(which,stereo)>0.1f);
 }
 puts("PASS actual mono/stereo DSP: reverb/delay/master output silent through edits and tails while muted, audible on unmute");
}
static void all_mappings(void) {
 for(int part=0;part<NPARTS;part++) {
  setup(V_HOME,PART_303A);pref_t r=smc_target(part);CHECK(smc_same(r,PR(R_ENG,T_MIX,part,MX_LEVEL)));
  pref_set(r,64);smc_frame();fader(part,100);toggle(part);CHECK(part_muted(part));writes=0;
  fader(part,110);CHECK(writes==1&&pref_get(r)==110&&part_muted(part));toggle(part);CHECK(!part_muted(part)&&pref_get(r)==110);
 }
 for(int kit=0;kit<2;kit++)for(int track=0;track<NDRUM;track++) {
  setup(V_SEQ,kit);ui.sel[kit]=track;build_pages();smc_frame();int strip=track%8;pref_t r=smc_target(strip);
  CHECK(r.a==(kit?T_808:T_909)&&r.b==track&&!strcmp(pref_name(r),"Level"));
  pref_set(r,64);smc_frame();fader(strip,100);toggle(strip);CHECK(track_muted(kit,track));writes=0;
  fader(strip,110);CHECK(writes==1&&pref_get(r)==110&&track_muted(kit,track));toggle(strip);CHECK(!track_muted(kit,track)&&pref_get(r)==110);
 }
 puts("PASS actual maps: all five parts and all eleven voices in each drum kit, including final partial bank");
}
static void motion_ownership(void) {
 for(int kind=0;kind<5;kind++) {
  int strip=strip_of(kind);pref_t r=prepare(kind,0,0);int part=engine_motion_part(r.a,r.b);
  int k=motion_alloc(proj.arr.lane,seq.ppat[part],part,r.a,r.b,r.c);CHECK(k>=0);
  pref_t other=PR(R_ENG,T_MIX,PART_303B,MX_LEVEL);int opart=engine_motion_part(other.a,other.b);
  int other_k=motion_alloc(proj.arr.lane,seq.ppat[opart],opart,other.a,other.b,other.c);CHECK(other_k>=0&&other_k!=k);
  for(int j=0;j<NSTEPS;j++){proj.arr.lane[k].val[j]=20;proj.arr.lane[other_k].val[j]=30;}
  lane_t saved=proj.arr.lane[other_k];
  engine_motion_enable(1);seq_start(&seq);for(int j=0;j<100;j++)render();CHECK(engine_motion_value(k)==20);CHECK(engine_motion_value(other_k)==30);
  toggle(strip);writes=0;fader(strip,110);CHECK(writes==1&&pref_get(r)==110);muted(kind,strip,1);
  for(int j=0;j<100;j++)render();CHECK(engine_motion_value(k)==-1);CHECK(engine_motion_value(other_k)==30);CHECK(!memcmp(&saved,&proj.arr.lane[other_k],sizeof saved));
  for(int j=0;j<NSTEPS;j++)CHECK(proj.arr.lane[k].val[j]==20);
  toggle(strip);CHECK(pref_get(r)==110);muted(kind,strip,0);
 }
 puts("PASS deliberate muted takeover uses existing target motion hold and leaves target lane data/unrelated automation intact");
}
static pref_t keyboard_dest(int bank,int strip) {
 if(!bank)return PR(R_ENG,T_MIX,strip,MX_LEVEL);
 return strip==0?PR(R_ENG,T_MIX,PART_BRK,MX_LEVEL):strip<3?PR(R_ENG,T_FX,0,strip==1?FX_RV_LEVEL:FX_DL_LEVEL):PR(R_ENG,T_MST,0,MST_DJF);
}
static void keyboard_scan(int strip,int value) {fm1_in.fader[strip]=value;now_ms+=16;smk_faders();}
static void keyboard_seed(int bank,int raw) {
 smk_fbank=bank;fm1_in.buttons=0;fm1_in.frames=0;for(int j=0;j<4;j++)fm1_in.fader[j]=raw;
 /* Model a real project-busy sampling boundary to clear static input epochs. */
 project_busy=1;smk_faders();project_busy=0;smk_faders();fm1_in.frames=206;smk_faders();writes=0;
}
static void keyboard_mute(int bank,int strip) {toggle(bank?strip==0?4:strip+4:strip);smk_faders();render();}
static void keyboard_rapid_epochs(void) {
 for(int bank=0;bank<2;bank++)for(int strip=0;strip<4;strip++) {
  if(bank&&strip==3)continue;
  setup(V_HOME,PART_303A);pref_t r=keyboard_dest(bank,strip);pref_set(r,64);keyboard_seed(bank,100);keyboard_mute(bank,strip);
  pref_set(r,90);writes=0;keyboard_scan(strip,111);CHECK(!writes&&pref_get(r)==90);
  /* No keyboard scans between explicit mute button edges. */
  int button=bank?strip==0?4:strip+4:strip;
  message(0x90,16+button,127);message(0x80,16+button,0);message(0x90,16+button,127);message(0x80,16+button,0);
  writes=0;keyboard_scan(strip,112);CHECK(writes==1&&pref_get(r)==112&&volume_muted(r));
  pref_set(r,90);writes=0;keyboard_scan(strip,113);CHECK(!writes&&pref_get(r)==90);
 }
 puts("PASS keyboard rapid mute edges rearm exact target without scans; subsequent external edit revokes exception");
}
static void keyboard_matrix(void) {
 unsigned cases=0;
 for(int bank=0;bank<2;bank++)for(int strip=0;strip<4;strip++)for(int acquired=0;acquired<2;acquired++)for(int moved=0;moved<2;moved++) {
  if(bank&&strip==3)continue; /* fader8 FILTER has no volume mute mapping. */
  setup(V_HOME,PART_303A);pref_t r=keyboard_dest(bank,strip);pref_set(r,64);render();keyboard_seed(bank,acquired?0:100);
  if(acquired)keyboard_scan(strip,64);
  CHECK(pref_get(r)==64);keyboard_mute(bank,strip);CHECK(volume_muted(r));writes=0;
  keyboard_scan(strip,acquired?64:100);CHECK(!writes&&pref_get(r)==64);
  int raw=acquired?80:110;if(moved)keyboard_scan(strip,raw);
  CHECK(pref_get(r)==(moved?raw:64));CHECK(writes==(unsigned)moved);CHECK(volume_muted(r));
  writes=0;keyboard_mute(bank,strip);CHECK(!volume_muted(r));CHECK(!writes&&pref_get(r)==(moved?raw:64));
  if(moved){keyboard_scan(strip,raw+1);CHECK(writes==1&&pref_get(r)==raw+1);}
  cases++;
 }
 for(int bank=0;bank<2;bank++)for(int strip=0;strip<4;strip++) {
  if(bank&&strip==3)continue;
  setup(V_HOME,PART_303A);pref_t r=keyboard_dest(bank,strip);pref_set(r,64);keyboard_seed(bank,100);keyboard_mute(bank,strip);
  pref_set(r,90);writes=0;keyboard_scan(strip,111);CHECK(!writes&&pref_get(r)==90);
  /* Physical crossing after an outside edit retains normal pickup safety. */
  keyboard_scan(strip,80);CHECK(writes==1&&pref_get(r)==80);CHECK(volume_muted(r));
  /* Fixed keyboard mapping survives UI page/part and SMC lock navigation. */
  writes=0;CHECK(smc_lock_toggle());ui.part=PART_808;set_view(V_SEQ);build_pages();keyboard_scan(strip,81);CHECK(writes==1&&pref_get(r)==81&&volume_muted(r));
  cases++;
 }
 /* A bank switch into an already muted unrelated target does not inherit an
  * old target's gesture, even if both targets happen to have equal levels. */
 for(int strip=0;strip<3;strip++) {
  setup(V_HOME,PART_303A);pref_t old=keyboard_dest(0,strip),r=keyboard_dest(1,strip);pref_set(old,64);pref_set(r,64);keyboard_seed(0,100);
  toggle(strip);toggle(strip==0?4:strip+4);smk_faders();CHECK(volume_muted(old)&&volume_muted(r));
  fm1_in.buttons=1u<<SMK_BTN_FADER_BANK;smk_faders();fm1_in.buttons=0;smk_faders();CHECK(smk_fbank==1);writes=0;
  keyboard_scan(strip,111);CHECK(!writes&&pref_get(old)==64&&pref_get(r)==64);
  keyboard_scan(strip,64);CHECK(writes==1&&pref_get(r)==64);cases++;
 }
 setup(V_HOME,PART_303A);pref_t filter=keyboard_dest(1,3);pref_set(filter,64);keyboard_seed(1,100);toggle(7);smk_faders();writes=0;
 keyboard_scan(3,110);CHECK(!writes&&pref_get(filter)==64);keyboard_scan(3,64);CHECK(writes==1&&pref_get(filter)==64);CHECK(engine_fx_muted()&4u);
 printf("PASS %u keyboard muted periods/epochs: seven exact levels, moved/untouched, acquired/pending, external edit, UI/lock navigation, bank changes; FILTER excluded\n",cases);
}
static void recording_ownership(void) {
 for(int kind=0;kind<5;kind++) {
  int strip=strip_of(kind);pref_t r=prepare(kind,0,0);int part=engine_motion_part(r.a,r.b);
  int k=motion_alloc(proj.arr.lane,seq.ppat[part],part,r.a,r.b,r.c);CHECK(k>=0);
  pref_t other=PR(R_ENG,T_MIX,PART_303B,MX_LEVEL);int opart=engine_motion_part(other.a,other.b);
  int other_k=motion_alloc(proj.arr.lane,seq.ppat[opart],opart,other.a,other.b,other.c);CHECK(other_k>=0&&other_k!=k);
  for(int j=0;j<NSTEPS;j++){proj.arr.lane[k].val[j]=20;proj.arr.lane[other_k].val[j]=30;}lane_t saved=proj.arr.lane[other_k];
  engine_motion_enable(1);seq_start(&seq);for(int j=0;j<100;j++)render();ui.rec=1;toggle(strip);writes=0;fader(strip,110);
  CHECK(writes==1&&pref_get(r)==110&&volume_muted(r));for(int j=0;j<100;j++)render();
  int recorded=0,retained=0;for(int j=0;j<NSTEPS;j++){recorded+=proj.arr.lane[k].val[j]==110;retained+=proj.arr.lane[k].val[j]==20;}
  CHECK(recorded>0&&retained>0);CHECK(!memcmp(&saved,&proj.arr.lane[other_k],sizeof saved));
 }
 puts("PASS REC muted edits use existing target motion recording and preserve untouched target steps/unrelated lanes");
}
static void held_parameter_locks(void) {
 for(int kind=0;kind<5;kind++) {
  int strip=strip_of(kind);pref_t r=prepare(kind,0,0);CHECK(smc_lock_toggle());
  int part=engine_motion_part(r.a,r.b);ui.part=(uint8_t)part;set_view(V_PART);build_pages();
  ui.hold_step=0;ui.hold_drum=part*NDRUM+ui.sel[part];ui.hold_pat=seq.ppat[part];
  int k=motion_alloc(proj.arr.lane,seq.ppat[part],part,r.a,r.b,r.c);CHECK(k>=0);proj.arr.lane[k].val[0]=20;
  CHECK(lock_step()==0&&lock_of(r,0)==20);lane_t saved=proj.arr.lane[k];
  toggle(strip);writes=0;fader(strip,110);CHECK(writes==1&&pref_get(r)==110&&volume_muted(r));
  /* Faders retain their established base-volume ownership; a held parameter
   * lock stays a lock and is neither rewritten nor erased by muted takeover. */
  CHECK(lock_step()==0&&lock_of(r,0)==20);CHECK(!memcmp(&saved,&proj.arr.lane[k],sizeof saved));
 }
 puts("PASS held parameter locks remain byte-identical while an exact locked fader edits only its base volume");
}
static void zero_and_project_reset(void) {
 setup(V_HOME,PART_303A);
 for(int which=0;which<3;which++) {
  pref_t r=PR(R_ENG,T_FX,0,which==0?FX_RV_LEVEL:which==1?FX_DL_LEVEL:FX_VOLUME);
  pref_set(r,0);render();writes=0;toggle(5+which);CHECK(smc_led(21+which)&&pref_get(r)==0&&!writes);
  toggle(5+which);CHECK(!smc_led(21+which)&&pref_get(r)==0&&!writes);
  toggle(5+which);CHECK(engine_fx_muted()&(1u<<which));
 }
 CHECK(engine_fx_muted()==7);smc_project_reset();CHECK(!engine_fx_muted());
 CHECK(pref_get(PR(R_ENG,T_FX,0,FX_RV_LEVEL))==0&&pref_get(PR(R_ENG,T_FX,0,FX_DL_LEVEL))==0&&pref_get(PR(R_ENG,T_FX,0,FX_VOLUME))==0);
 puts("PASS initially zero FX levels never resurrect a fallback100; project reset clears only explicit mute state");
}
int main(int argc,char **argv) {
 if(argc>1&&!strcmp(argv[1],"late")) {
  setup(V_HOME,PART_303A);for(int j=0;j<4;j++)pref_set(keyboard_dest(0,j),120);render();toggle(0);
  smk_fbank=0;fm1_in.frames=206;fm1_in.buttons=0;for(int j=0;j<4;j++)fm1_in.fader[j]=100;writes=0;
  smk_faders();CHECK(!writes&&pref_get(keyboard_dest(0,0))==120);smk_faders();CHECK(!writes);
  keyboard_scan(0,110);CHECK(writes==1&&pref_get(keyboard_dest(0,0))==110&&part_muted(0));
  puts("PASS fresh late first scan: noncrossing parked sample cannot seize muted target; subsequent genuine movement can");return 0;
 }
 basic_matrix();identity_epochs();queued_epochs();filter_and_master();all_mappings();motion_ownership();recording_ownership();held_parameter_locks();zero_and_project_reset();audio_gate();keyboard_matrix();keyboard_rapid_epochs();printf("PASS muted fader production regression: %u checks\n",checks);return 0; }
