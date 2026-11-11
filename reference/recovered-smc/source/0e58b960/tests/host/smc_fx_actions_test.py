#!/usr/bin/env python3
"""Extract production dispatch/FX bodies; simulate UI and the engine publish endpoint."""
import argparse,ast,os,re,subprocess,tempfile
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--source',type=Path,default=Path(__file__).resolve().parents[2]/'firmware/src/app/smc_mixer.c')
p.add_argument('--fx-support',type=Path,required=True)
p.add_argument('--lock-chord-only',action='store_true')
p.add_argument('--lifetime-only',action='store_true')
a=p.parse_args();source=a.source.read_text()
def fn(name):
 m=re.search(r'^[^\n]*\b'+name+r'\([^\n{}]*\)[^;\n{}]*(?:\n\s*)?\{',source,re.M)
 if not m:
  if name=='smc_button_bit':return ''
  raise ValueError(name)
 i=m.end();d=1
 while d:d+=(source[i]=='{')-(source[i]=='}');i+=1
 return source[m.start():i]
# Reuse the explicit endpoint fixture, never a substitute for the production FX bodies.
tree=ast.parse(Path(__file__).with_name('smc_locked_actions_test.py').read_text())
prefix=next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='prefix' for t in n.targets))
for name in ['smc_fx_publish','smc_fx_clear','smc_fx_main_nav','smc_home_held','smc_fx_try']:
 prefix=re.sub(r'^static [^\n]*\b'+name+r'\([^\n]*\n','',prefix,flags=re.M)
prefix=prefix.replace('fx_ready,fx_effect','fx_ready,fx_page,fx_effect')
prefix=prefix.replace('static void button_tap(int key){last_tap=key;}','static void button_tap(int key){last_tap=key;if(key==B_HOME)ui.view=V_HOME;}')
prefix+="""
#define SMC_PEERS 2
#define smc_states states
#define SMCFX_NPAGE 3
static unsigned published_down,dirty;static uint8_t published_effect[8];
static struct{struct{uint8_t v[16][16][16];}sound;}proj;
static void engine_smc_busfx(uint8_t down,const uint8_t*effects){published_down=down;memcpy(published_effect,effects,8);}
static void pref_set(pref_t r,int v){(void)r;(void)v;}
static void mark_dirty(void){dirty++;}
static int smc_lock_blocks(uint8_t);
static void smc_fx_clear(void);
"""
functions='\n'.join(fn(n) for n in ['smc_is_solo_release','smc_drum_mix','smc_drum_track'])
if re.search(r'^static int smc_strip_part\(',source,re.M):functions+='\n'+fn('smc_strip_part')
else:functions+='\nstatic int smc_strip_part(int n){return part_at(n);}'
functions+='\n'+source[source.index('static const uint8_t SMC_VIEW'):source.index('static int smc_nav_key')]
functions+='\n'+'\n'.join(fn(n) for n in ['smc_fx_bus','smc_fx_bus_now','smc_lock_blocks','smc_nav_key','smc_record_toggle','smc_select_part','smc_select_drum','smc_bank','smc_fx_publish','smc_fx_load_page','smc_home_held','smc_fx_reset_cuts','smc_fx_save_preset','smc_fx_show','smc_fx_main_nav','smc_fx_main_tick','smc_fx_try','smc_fx_release_note','smc_fx_clear','smc_button_apply','smc_button_bit','smc_button','smc_release'])
body=r''' 
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static void reset(void){memset(&ui,0,sizeof ui);memset(states,0,sizeof states);memset(&seq,0,sizeof seq);memset(&smc_ui,0,sizeof smc_ui);for(int i=0;i<5;i++)order[i]=i;slot=0;shown_kit=-1;clip=0;ui.overlay=O_NONE;ui.view=V_FX;ui.part=2;ui.outline_ok=1;last_tap=-1;published_down=dirty=notices=0;page=0;now=100;pg.n=4;}
static void lock_home(void){smc.locked=1;smc.lock_part=2;for(int i=0;i<5;i++)smc.lock_fader[i]=PR(R_ENG,T_MIX,i,MX_LEVEL);}
int main(void){
 for(unsigned peer=0;peer<2;peer++)for(unsigned locked=0;locked<2;locked++){
  reset();slot=peer;if(locked)lock_home();
  smc_button(6,0);CHECK(ui.view==V_FX&&last_tap==-1); /* red: immediate Back leaves FX */
  smc_button(0x63,0);CHECK(smc.fx_page==1&&ui.view==V_FX);smc_release(6);CHECK(ui.view==V_FX&&last_tap==-1);
  smc_button(6,0);smc_button(0x62,0);CHECK(smc.fx_page==0);smc_release(6);CHECK(ui.view==V_FX);
  smc_button(6,0);smc_button(0x60,0);CHECK(dirty==1);smc_release(6);CHECK(ui.view==V_FX);
  smc_button(6,0);smc_button(0x61,0);smc_release(6);CHECK(ui.view==V_FX&&!published_down);
  for(unsigned bus=0;bus<8;bus++){
   unsigned bit=1u<<bus;smc_fx_load_page(0);
   for(unsigned e=0;e<SMCFX_SUIT_N[bus];e++){
    unsigned old=smc.fx_effect[bus];smc_button(6,0);smc_button(24+bus,0);
    CHECK(smc.fx_effect[bus]==smcfx_cycle(bus,old)&&!published_down&&ui.view==V_FX);
    smc_release(24+bus);smc_release(6);CHECK(!published_down&&ui.view==V_FX);
    smc_button(24+bus,0);smc_button(24+bus,0); /* repeated press has no second action */
    if(bus>=5){CHECK(!(published_down&bit));smc_ui.on=0;now+=180;smc_fx_main_tick();CHECK(smc_ui.on);}
    CHECK((published_down&bit)&&published_effect[bus]==smc.fx_effect[bus]);
    smc_release(24+bus);CHECK(!(published_down&bit)&&!smc.fx_owned);
   }
  }
  smc_button(6,0);smc_release(6);CHECK(last_tap==B_HOME);
 }
 for(unsigned peer=0;peer<2;peer++)for(unsigned locked=0;locked<2;locked++)for(unsigned bus=5;bus<8;bus++){
  reset();slot=peer;if(locked)lock_home();smc_button(24+bus,0);now+=100;smc_release(24+bus);
  CHECK(!published_down&&!smc.fx_owned&&ui.view==(bus==7?V_MIX:V_FX)&&page==(bus==7?3:(int)bus-5));
 }
 /* Mode gating: ordinary SELECT selects; only V_FX runs held effects. */
 unsigned views[]={V_HOME,V_PART,V_SEQ,V_GEN,V_MIX,V_GLO,V_SONG};
 for(unsigned v=0;v<sizeof views/sizeof *views;v++)for(unsigned peer=0;peer<2;peer++){
  reset();slot=peer;ui.view=views[v];smc_button(24,0);CHECK(!published_down&&!smc.fx_owned);smc_release(24);CHECK(!published_down);
 }
 /* Overlay, clip and captured drum bank never become bus FX. */
 for(unsigned mode=0;mode<3;mode++){reset();if(mode==0)ui.overlay=O_ASK;if(mode==1)clip=1;if(mode==2){lock_home();smc.lock_kit=1;}smc_button(24,0);CHECK(!published_down);smc_release(24);}
 /* Two independently held assignments: peer 0 wins conflict, release reveals peer 1. */
 reset();smc_fx_load_page(0);smc_button(26,0);CHECK(published_down==4);
 slot=1;smc_fx_load_page(0);smc.fx_effect[2]=SMCFX_LP;smc_button(26,0);CHECK(published_effect[2]==SMCFX_HP);
 slot=0;smc_release(26);CHECK(published_down==4&&published_effect[2]==SMCFX_LP);
 slot=1;smc_release(26);CHECK(!published_down);
 /* Pending Back is revoked by a context change or existing lost-release cleanup. */
 reset();smc_button(6,0);ui.view=V_PART;smc_release(6);CHECK(last_tap==-1&&ui.view==V_PART);
 reset();smc_button(6,0);smc.home_down=0;smc_release(6);CHECK(last_tap==-1);
 reset();smc_button(6,0);ui.view=V_PART;smc_fx_main_tick();ui.view=V_FX;smc_release(6);CHECK(last_tap==-1);
 reset();smc_release(29);CHECK(ui.view==V_FX&&!published_down); /* foreign/stale release cannot navigate */
 reset();slot=0;smc_button(6,0);slot=1;smc_button(26,0);CHECK(states[1].fx_effect[2]==SMCFX_HP&&published_down==4);smc_release(26);slot=0;smc_release(6);CHECK(last_tap==B_HOME);
 reset();lock_home();order[0]=4;smc_button(24,0);CHECK(published_down==1);order[0]=3;smc_release(24);CHECK(!published_down);
 reset();lock_home();smc_button(0x63,0);CHECK(smc.fx_page==0&&!notices); /* ordinary navigation never selects an FX assignment preset */
 printf("PASS %u actual FX dispatch checks: both peers, locks, all suited effects/pages, Home chords/tap, ownership and mode negatives\n",checks);
}
'''
if a.lifetime_only:
 prefix+="""
static unsigned context=10,diag;
static uint32_t smc_ui_context(void){return context;}
static int ui_pad_diag_active(void){return diag;}
"""
 functions+='\n'+fn('smc_ui_active')
 body=body[:body.index('int main(void)')]+r'''
int main(void){
 unsigned notes[]={0,8,16,24,5,6,7,13,14,15,29,30,31,0x2e,0x2f,0x30,0x31,0x5b,0x5c,0x5d,0x5e,0x5f,0x60,0x61,0x62,0x63};
 for(unsigned peer=0;peer<2;peer++)for(unsigned n=0;n<sizeof notes/sizeof *notes;n++){
  reset();slot=peer;ui.view=V_PART;smc_button(notes[n],0);
  smc_ui.on=1;smc_ui.peer=peer;smc_ui.context=context;smc_ui.until=now+1200;
  now+=5000;CHECK(smc_ui_active()); /* red: old active expires while held */
  CHECK(smc_is_solo_release(0x80,notes[n],64)&&smc_is_solo_release(0x90,notes[n],0)&&!smc_is_solo_release(0x90,notes[n],127));
  smc_release(notes[n]);CHECK(smc_ui_active());now+=1199;CHECK(smc_ui_active());now++;CHECK(!smc_ui_active());
 }
 for(unsigned n=0;n<128;n++){
  int known=n<32||(n>=0x2e&&n<=0x31)||(n>=0x5b&&n<=0x63);
  CHECK(smc_is_solo_release(0x80,n,0)==known);
  CHECK(!smc_is_solo_release(0xb0,n,0)&&!smc_is_solo_release(0x90,n,127));
 }
 CHECK(!smc_is_solo_release(0x80,128,0)&&!smc_is_solo_release(0x80,8,128));
 reset();smc_button(8,0);smc_ui.on=1;smc_ui.peer=0;smc_ui.context=context;smc_ui.until=now+1200;
 slot=1;smc_release(8);CHECK(states[0].ui_held);slot=0;now+=5000;CHECK(smc_ui_active());
 context++;CHECK(!smc_ui_active()); /* navigation still cancels immediately */
 reset();smc_button(8,0);smc_ui.on=1;smc_ui.peer=0;smc_ui.context=context;diag=1;CHECK(!smc_ui_active());diag=0;
 reset();now=0xfffffff0u;smc_button(8,0);smc_ui.on=1;smc_ui.peer=0;smc_ui.context=context;now+=5000;CHECK(smc_ui_active());smc_release(8);now+=1200;CHECK(!smc_ui_active());
 reset();smc_ui.on=1;smc_ui.peer=0;smc_ui.context=context;smc_ui.until=now+1200;now+=500;smc_release(8);CHECK(smc_ui.until==1300); /* unmatched release cannot extend */
 printf("PASS %u held-display checks: every key class/two peers, release grace, foreign releases, context/diagnostic cancellation and timer wrap\n",checks);
}
'''
if a.lock_chord_only:
 prefix=prefix.replace('static uint32_t plat_buttons(void){return 0;}','static uint32_t plat_buttons(void){return 1u<<B_HOME;}')
 body=r'''int main(void){ui.view=V_FX;ui.overlay=O_NONE;shown_kit=-1;smc.locked=1;for(int i=0;i<5;i++){order[i]=i;smc.lock_fader[i]=PR(R_ENG,T_MIX,i,MX_LEVEL);}smc_button(0x63,0);if(smc.fx_page!=1){fprintf(stderr,"physical Home+Right was blocked by target lock\n");return 1;}puts("PASS locked physical Home+Right reaches existing FX page selector");}'''
with tempfile.TemporaryDirectory(prefix='smc-fx-actions-') as d:
 c=Path(d)/'test.c';exe=Path(d)/'test';c.write_text(prefix+functions+body)
 subprocess.run([os.getenv('CC','cc'),'-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-unused-variable','-fsanitize=address,undefined','-fno-sanitize-recover=all','-I'+str(a.fx_support),str(c),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
