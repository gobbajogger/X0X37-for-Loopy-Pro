#!/usr/bin/env python3
"""Actual SMC action/lock/feedback bodies; UI, sequencer and DSP endpoints simulated."""
import argparse,os,re,subprocess,tempfile
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--source',type=Path,default=Path(__file__).resolve().parents[2]/'firmware/src/app/smc_mixer.c')
p.add_argument('--reference',type=Path,required=True)
p.add_argument('--fx-support',type=Path,required=True)
a=p.parse_args();source=a.source.read_text();reference=a.reference.read_text()
def fn(name,s=source):
 m=re.search(r'^[^\n]*\b'+name+r'\([^\n{}]*\)[^;\n{}]*(?:\n\s*)?\{',s,re.M)
 if not m:return None
 i=m.end();d=1
 while d:d+=(s[i]=='{')-(s[i]=='}');i+=1
 return s[m.start():i]
for name in ['smc_lock_button','smc_lock_ready','smc_lock_tick','smc_fx_release_note','smc_ble_midi_slot','smc_fx_publish']:
 assert fn(name)==fn(name,reference),name+' unexpectedly changed'
for name in ['smc_poll_peer','smc_pad_diag_boundary']:
 assert fn(name).replace('        smc.ui_held = 0;\n','')==fn(name,reference),name+' changed beyond display inventory cleanup'
assert fn('smc_lock_toggle') in (fn('smc_lock_toggle',reference),fn('smc_lock_toggle',reference).replace('    smc.locked ^= 1u;', '    smc_fx_clear(); /* a routing-epoch change ends owned FX before key-up can use a new mapping */\n    smc.locked ^= 1u;'))
functions='\n'.join(fn(n) for n in ['smc_drum_mix','smc_drum_track'])
functions+='\n'+(fn('smc_strip_part') or 'static int smc_strip_part(int n){return part_at(n);}')
functions+='\n'+'\n'.join(fn(n) for n in ['smc_fx_bus','smc_fx_bus_now','smc_lock_blocks','smc_nav_key','smc_record_toggle','smc_select_part','smc_select_drum','smc_bank','smc_lock_toggle'])
functions+='\n'+source[source.index('static const uint8_t SMC_VIEW'):source.index('static int smc_nav_key')]
# The actual action implementation is called through the unchanged edge wrapper.
functions+='\n'+fn('smc_button_apply')+'\n'+(fn('smc_button_bit') or '')+'\n'+fn('smc_button')+'\n'+fn('smc_fx_release_note')+'\n'+fn('smc_release')+'\n'+fn('smc_led')+'\n'+fn('smc_button_caption')
prefix=r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "smc_fx_map_tables.c"
enum {NKIT=2,NPARTS=5,NPAT=16,NDRUM=11,NBASS=2,V_FX=20,V_HOME,V_MIX,V_PART,V_SEQ,V_GEN,V_SONG,V_GLO,O_NONE,O_LIST,O_ASK,B_SEL=0,B_HOME,B_GLO,R_NONE,R_ENG,T_MIX,T_FX,T_909,T_808,MX_LEVEL,MX_CUT,FX_VOLUME};
#define SMC_FX_HOLD_MS 180u
#define SMC_UI_MS 1200u
#define smc_slot slot
typedef struct{uint8_t kind,a,b,c;}pref_t;
#define PR(k,x,y,z) ((pref_t){k,x,y,z})
static struct{uint8_t part,mixsel,outline_ok,overlay,view,help,rec,wpos[2],sel[2],song_sel,btn;char msg[80];uint32_t msg_until;}ui;
static struct{unsigned locked,lock_kit,lock_bank,lock_part,lock_view,lock_page,lock_song_revision,lock_song_len,picked,knob_i,pending,rec_part,solo,home_down,select_down,play_down,lock_down,fx_down,fx_owned,fx_main_wait,fx_ready,fx_effect[8];uint16_t drum_solo[2];uint8_t solo_target[8];pref_t lock_fader[8],lock_knob[8];uint32_t fx_main_t0[3];uint64_t ui_held;}states[2];
static unsigned slot;
#define smc (states[slot])
static struct{unsigned action,on,peer,context;uint32_t until;}smc_ui;
static struct{int ppat[5],playing,req_start,req_stop,req_cont,song_start;}seq;
static struct{unsigned len;}song_data;
#define song() (&song_data)
static struct{int n;}pg;
static int order[5],shown_kit,clip,muted[5],drum_muted[2][11],page,last_tap,last_fx,nav_calls,notices,bass_cancels,drum_cancels,fx_calls,fx_nav;
static unsigned checks;static uint32_t now=100;
static const char*const PART_N[]={"909","808","A","B","BREAK"};
static unsigned song_structure_revision=1;
static int ui_project_busy(void){return 0;}
static int perf_testing(void){return 0;}
static pref_t smc_target(int n){return PR(R_ENG,T_MIX,n,MX_LEVEL);}
static pref_t smc_knob_target(int n){return PR(R_ENG,T_MIX,n,MX_CUT);}
static pref_t smc_bind(pref_t r){return r;}
static int part_at(int n){return order[n];}
static int part_position(int p){for(int i=0;i<5;i++)if(order[i]==p)return i;return 0;}
static int part_neighbor(int p,int d){return order[part_position(p)+d];}
static int is_drum(void){return ui.part<2;}
static int is_303(void){return ui.part>=2&&ui.part<4;}
static int bidx(void){return (int)ui.part-2;}
static int drum_mix_page(void){return shown_kit;}
static int smk_clip_mode_on(void){return clip;}
static uint32_t plat_ms(void){return now;}
static uint32_t plat_buttons(void){return 0;}
static int cur_page(void){return page;}
static uint8_t page_storage;
static uint8_t*page_of(int v){(void)v;return &page_storage;}
static void build_pages(void){}
static void set_view(int v){ui.view=v;}
static void bass_step_cancel(void){bass_cancels++;}
static void drum_step_cancel(void){drum_cancels++;}
static void say(const char*a,const char*b){(void)a;(void)b;}
static void ui_say(const char*a,const char*b){(void)a;(void)b;notices++;}
static void button_tap(int key){last_tap=key;}
static int smk_ck_smc_nav(uint8_t n){(void)n;nav_calls++;return 0;}
static void smc_view(int v,int p){ui.view=v;page=p;}
static void smc_toggle_fx(int n){last_fx=n;}
static void mute_part(int p){muted[p]^=1;}
static void mute_track(int k,int t){drum_muted[k][t]^=1;}
static int part_muted(int p){return muted[p];}
static int track_muted(int k,int t){return drum_muted[k][t];}
static const char*engine_voice_name(int k,int t){(void)k;(void)t;return "drum";}
static void list_move(int n){page+=n;}
static void ui_menu_arrow(int n){page+=n;}
static void seq_cue_part(void*s,int p,int n){(void)s;seq.ppat[p]=n;}
static void seq_stop(void*s){(void)s;seq.req_stop=1;}
static void seq_start(void*s){(void)s;seq.req_start=1;}
static int pref_get(pref_t r){(void)r;return 0;}
static pref_t smc_home_target(int n){return PR(R_ENG,T_MIX,n,MX_LEVEL);}
static void smc_solo_publish(void){smc.solo=0;smc.drum_solo[0]=smc.drum_solo[1]=0;for(int i=0;i<8;i++){int t=smc.solo_target[i];if(t>0&&t<=5)smc.solo|=1u<<(t-1);else if(t>5){t-=6;smc.drum_solo[t/11]|=1u<<(t%11);}}}
static void smc_solo_press(int strip,int target,int mmc){(void)mmc;smc.solo_target[strip]=target;smc_solo_publish();}
static void smc_fx_publish(void){}
static void smc_fx_clear(void){smc.fx_down=smc.fx_owned=smc.fx_main_wait=0;}
static void smc_fx_main_nav(int n){fx_nav=n;}
static int smc_home_held(void){return smc.home_down;}
static int smc_lock_button(int pressed){(void)pressed;return 0;}
static void smc_ui_cancel(void){smc_ui.on=0;}
static void smc_ui_begin(int n,int k){(void)n;(void)k;smc_ui.on=1;}
static int smc_beat_lit(void){return 0;}
static int smc_fx_bus_now(uint8_t n);
static int smc_fx_try(uint8_t note,int mmc){(void)mmc;int bus=smc_fx_bus_now(note);if(bus<0)return 0;smc.fx_down|=1u<<bus;smc.fx_owned|=1u<<bus;fx_calls++;return 1;}
'''
body=r'''
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static void reset(void){memset(&ui,0,sizeof ui);memset(&smc_ui,0,sizeof smc_ui);memset(states,0,sizeof states);memset(&seq,0,sizeof seq);memset(muted,0,sizeof muted);memset(drum_muted,0,sizeof drum_muted);for(int i=0;i<5;i++)order[i]=i;ui.view=V_PART;ui.overlay=O_NONE;ui.part=3;ui.outline_ok=1;shown_kit=-1;slot=0;notices=nav_calls=fx_calls=0;last_tap=last_fx=fx_nav=-1;pg.n=4;song_data.len=4;page=0;}
static void lock_home(void){smc.locked=1;smc.lock_part=2;for(int i=0;i<5;i++){smc.lock_fader[i]=PR(R_ENG,T_MIX,i,MX_LEVEL);smc.lock_knob[i]=PR(R_ENG,T_MIX,i,MX_CUT);}}
int main(void){
 for(slot=0;slot<2;slot++){} /* each iteration below explicitly selects its peer */
 for(unsigned peer=0;peer<2;peer++)for(unsigned strip=0;strip<5;strip++){
  reset();slot=peer;lock_home();pref_t f[8],k[8];memcpy(f,smc.lock_fader,sizeof f);memcpy(k,smc.lock_knob,sizeof k);
  order[strip]=(strip+1)%5;ui.part=4;smc_button(strip,0);
  CHECK(ui.rec&&ui.part==strip&&smc.rec_part==strip); /* red: blanket note<8 lock */
  CHECK(smc_led(strip)&&!strcmp(smc_button_caption(strip),"R")&&!notices);
  CHECK(!memcmp(f,smc.lock_fader,sizeof f)&&!memcmp(k,smc.lock_knob,sizeof k));
  smc_button(16+strip,0);CHECK(muted[strip]&&!muted[(strip+1)%5]);
  smc_button(8+strip,0);CHECK(smc.solo_target[strip]==strip+1);ui.part=(strip+2)%5;smc_release(8+strip);CHECK(!smc.solo_target[strip]);
  smc_button(24+strip,0);CHECK(ui.part==strip);smc_release(24+strip);CHECK(!smc.select_down);
  ui.view=V_FX;smc_button(24+strip,0);CHECK(smc.fx_owned&(1u<<strip));
  order[strip]=(strip+3)%5;smc_release(24+strip);CHECK(!(smc.fx_owned&(1u<<strip))&&!(smc.fx_down&(1u<<strip)));
  CHECK(!memcmp(f,smc.lock_fader,sizeof f)&&!memcmp(k,smc.lock_knob,sizeof k));
 }
 for(unsigned peer=0;peer<2;peer++)for(unsigned kit=0;kit<2;kit++)for(unsigned bank=0;bank<2;bank++)for(unsigned strip=0;strip<8;strip++){
  reset();slot=peer;smc.locked=1;smc.lock_kit=kit+1;smc.lock_bank=bank*8;ui.part=1-kit;ui.sel[kit]=bank?0:8;
  unsigned track=bank*8+strip;smc_button(strip,0);
  if(track>=NDRUM){CHECK(!ui.rec&&ui.part==1-kit&&!smc_ui.on);continue;}
  CHECK(ui.rec&&ui.part==kit&&ui.sel[kit]==track&&smc_led(strip));
  smc_button(16+strip,0);CHECK(drum_muted[kit][track]&&!drum_muted[1-kit][track]);
  smc_button(8+strip,0);CHECK(smc.solo_target[strip]==6+kit*NDRUM+track);ui.part=1-kit;smc_release(8+strip);CHECK(!smc.solo_target[strip]);
  smc_button(24+strip,0);CHECK(ui.part==kit&&ui.sel[kit]==track);smc_release(24+strip);CHECK(!smc.select_down);
  CHECK(smc.lock_kit==kit+1&&smc.lock_bank==bank*8);
 }
 reset();lock_home();unsigned nav[]={5,6,7,13,14,15,29,30,31,0x2e,0x2f,0x30,0x31,0x60,0x61,0x62,0x63};
 for(unsigned i=0;i<sizeof nav/sizeof *nav;i++){
  reset();lock_home();pref_t f[8],k[8];memcpy(f,smc.lock_fader,sizeof f);memcpy(k,smc.lock_knob,sizeof k);
  CHECK(!smc_lock_blocks(nav[i]));smc_button(nav[i],0);smc_release(nav[i]);CHECK(!notices);
  CHECK(!memcmp(f,smc.lock_fader,sizeof f)&&!memcmp(k,smc.lock_knob,sizeof k));
 }
 reset();lock_home();
 ui.view=V_FX;for(unsigned n=29;n<32;n++){smc_button(n,0);CHECK(smc.fx_owned&(1u<<(n-24)));CHECK(smc_led(n));now+=200;smc_release(n);CHECK(!smc.fx_owned);CHECK(!smc_led(n));}
 ui.part=4;smc_button(0x5f,0);CHECK(ui.rec&&smc.rec_part==4);smc_button(0x5e,0);CHECK(seq.req_start);smc_release(0x5e);smc_button(0x5e,0);CHECK(seq.req_stop);
 seq.ppat[2]=2;smc_button(0x5c,0);CHECK(seq.ppat[2]==3&&ui.part==4);smc_button(0x5b,0);CHECK(seq.ppat[2]==2);
 reset();lock_home();ui.overlay=O_ASK;smc_button(0,0);CHECK(!ui.rec);smc_button(5,0);CHECK(last_tap==B_SEL);smc_button(6,0);CHECK(last_tap==B_HOME);
 reset();lock_home();ui.help=1;smc_button(6,0);CHECK(!ui.help);
 reset();lock_home();smc.lock_fader[0].kind=R_NONE;smc_button(0,0);CHECK(!ui.rec);smc_button(16,0);CHECK(!muted[0]);
 reset();lock_home();ui.view=V_FX;smc_button(24,0);CHECK(smc.fx_owned==1);order[0]=4;CHECK(smc_lock_toggle());CHECK(!smc.locked&&!smc.fx_owned&&!smc.fx_down);smc_release(24);CHECK(!smc.fx_owned&&!smc.fx_down);
 reset();order[0]=4;smc_button(0,0);CHECK(ui.rec&&ui.part==4); /* unlocked behavior remains presentation-based */
 printf("PASS %u locked action checks: two peers, captured parts/drum banks, REC/Solo/Mute/Select/FX, releases, live navigation without target reassignment and global controls\n",checks);
}
'''
with tempfile.TemporaryDirectory(prefix='smc-locked-actions-') as d:
 c=Path(d)/'test.c';exe=Path(d)/'test';c.write_text(prefix+functions+body)
 subprocess.run([os.getenv('CC','cc'),'-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-Wno-unused-function','-fsanitize=address,undefined','-fno-sanitize-recover=all','-I'+str(a.fx_support),str(c),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
