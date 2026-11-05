#!/usr/bin/env python3
"""Check production SMC LED destinations and emitted feedback without a device.

Navigation and drum-strip expectations follow the actions in SMC_MIXER.md.
Production LED/frame functions are compiled verbatim; UI/sound/transport are substitutes.
SMC_SOURCE selects an older smc_mixer.c for a source-negative control.
"""
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
source = Path(os.environ.get("SMC_SOURCE", ROOT / "firmware/src/app/smc_mixer.c")).read_text()
ui_source = (ROOT / "firmware/src/app/ui.c").read_text()
app_header = (ROOT / "firmware/src/app/x0x.h").read_text()
fx_header = (ROOT / "firmware/src/dsp/fxbus.h").read_text()


def function(name):
    match = re.search(r"^[^\n]*\b" + name + r"\([^\n]*\)\n\{.*?^\}", source, re.M | re.S)
    if not match:
        raise RuntimeError(f"Missing production function: {name}")
    return match[0]


# Import production identifiers so view order, part count and kit size cannot drift.
views = re.search(r"^enum \{ V_HOME,.*?\};", ui_source, re.M)[0]
overlays = re.search(r"^enum \{ O_NONE,.*?\};", ui_source, re.M)[0]
central_bit = re.search(r"^#define PROJ_BT_SMC_CENTRAL\s+\d+u", app_header, re.M)[0]
# The full FX header needs generated audio tables; only its parameter enum is used here.
fx_params = re.search(r"^enum \{ FX_VOLUME,.*?FX_NPARAMS \};", fx_header, re.M | re.S)[0]
harness = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "app/engine.h"
#include "dsp/master.h"
''' + views + "\n" + overlays + "\n" + central_bit + "\n" + fx_params + r'''
enum { R_ENG };
typedef struct { int kind,a,b,c; } pref_t;
#define PR(k,a,b,c) ((pref_t){k,a,b,c})
seq_t seq;
static struct { int rec,part,view,overlay,help,sel[NKIT]; } ui;
static struct { uint8_t up,rec_part,solo,known[128],lit[128],pending,value[8],locked,lock_kit,lock_bank,lock_part,lock_view,lock_page,lock_pages,lock_page_kit[1],fx_down; uint16_t drum_solo[NKIT]; uint32_t beat_at,beat_period; float beat_internal; uint8_t beat_external; } smc_states[2];
#define SMC_PEERS 2u
static uint8_t smc_slot;
static uint32_t smc_link_published=1, smc_link_seen=1;
#define smc (smc_states[smc_slot])
static struct { struct { uint8_t bt; } set; } proj;
static int page,clip,locked,part_mutes,fx[3]={100,100,100},filter=64;
static uint16_t drum_mutes[NKIT];
static unsigned checks,failures,tx_count;
static uint8_t tx[128][3];
static int pref_get(pref_t r) {
    if(r.a==T_MST) return filter;
    if(r.a!=T_FX) return 100;
    return fx[r.c==FX_RV_LEVEL?0:r.c==FX_DL_LEVEL?1:2];
}
static int part_muted(int p) { return (part_mutes>>p)&1; }
static int track_muted(int kit,int track) {
    assert(kit>=0 && kit<NKIT && track>=0 && track<NDRUM);
    return (drum_mutes[kit]>>track)&1;
}
static int smk_clip_mode_on(void) { return clip; }
#define is_drum() (ui.part < NKIT)
static int cur_page(void) { return page; }
/* MIX pages: 0 part mixer, 1..NKIT the 909 and 808 kit mixers, 3 master.
 * smc_drum_mix reads the kit from the title; FX uses pages 0 and 1 only. */
static struct { int n; char title[4][16]; } pg = {4, {"MIXER", "909 MIX", "808 MIX", "MASTER"}};
static int ui_project_busy(void) { return locked; }
static void smc_notice(int n) { (void)n; }
int plat_ble_smc_send(const uint8_t *msg,uint8_t len) {
    assert(len==3 && tx_count<128);memcpy(tx[tx_count++],msg,3);return 1;
}
'''
harness += re.search(r"^static const uint8_t PART_ORDER.*?;", ui_source, re.M)[0] + "\n"
harness += re.search(r"^static int part_at\(.*", ui_source, re.M)[0] + "\n"
harness += 'int plat_ble_smc_send_slot(unsigned p,const uint8_t *m,uint8_t n) { assert(p==0); return plat_ble_smc_send(m,n); }\n'
harness += "\n".join(function(name) for name in (
    "smc_home_target", "smc_page_now", "smc_drum_mix", "smc_drum_track", "smc_lock_blocks", "smc_beat_lit", "smc_led", "smc_frame_peer", "smc_frame"))
harness += r'''
static const int expected_bus[5]={PART_909,PART_808,PART_BRK,PART_303B,PART_303A};
static int expected_strip(int part) { for(int n=0;n<5;n++) if(expected_bus[n]==part) return n; abort(); }
static void expect(const char *label,int note,int expected) {
    int got=smc_led(note)!=0;checks++;
    if(got!=expected) {
        failures++;
        fprintf(stderr,"FAIL %s: note=%d view=%d page=%d got=%d expected=%d\n",label,note,ui.view,page,got,expected);
    }
}
static void navigation(void) {
    /* Destinations from existing R8/S6/S7/S8 and SELECT6/7/8 actions. */
    /* PART page 0 is S8's entry; every PART page is the selected part's sound workspace. */
    static const int state[][2]={{V_HOME,0},{V_PART,0},{V_PART,2},{V_FX,0},{V_FX,1},
        {V_MIX,0},{V_MIX,3},{V_GLO,0},{V_SEQ,0}};
    for(unsigned i=0;i<sizeof state/sizeof state[0];i++) {
        ui.view=state[i][0];page=state[i][1];ui.overlay=O_NONE;
        expect("R8 HOME",7,ui.view==V_HOME);
        expect("S6 MIX",13,ui.view==V_MIX);
        expect("S7 GLOBAL",14,ui.view==V_GLO);
        expect("S8 PART",15,ui.view==V_PART);
        expect("SELECT6 REVERB",29,ui.view==V_FX && page==0);
        expect("SELECT7 DELAY",30,ui.view==V_FX && page==1);
        expect("SELECT8 MASTER",31,ui.view==V_MIX && page==3);
    }
    for(int overlay=O_NONE;overlay<=O_ASK;overlay++) {
        ui.overlay=overlay;
        expect("R6 overlay",5,overlay!=O_NONE);
        expect("R7 momentary back",6,0);
    }
    ui.overlay=O_NONE;
}
static void main_strips(void) {
    ui.view=V_HOME;page=0;
    for(int selected=0;selected<NPARTS;selected++) {
        ui.part=selected;ui.rec=1;smc.rec_part=(selected+1)%NPARTS;
        /* Panel REC and part changes need not update the last SMC REC cache. */
        part_mutes=smc.solo=1u<<selected;
        for(int strip=0;strip<NPARTS;strip++) {
            expect("main REC",strip,expected_bus[strip]==selected);
            expect("main SOLO",8+strip,expected_bus[strip]==selected);
            expect("main MUTE",16+strip,expected_bus[strip]==selected);
            expect("main SELECT",24+strip,expected_bus[strip]==selected);
        }
    }
    ui.rec=smc.solo=part_mutes=0;
    for(int strip=0;strip<NPARTS;strip++) {
        expect("main REC off",strip,0);expect("main SOLO off",8+strip,0);expect("main MUTE off",16+strip,0);
    }
    /* Assigned source effects own these lamps only in the FX view. Each bus
     * uses its actual strip bit, including 303B (3) and BREAK (4). */
    ui.view=V_FX;
    for(int held=0;held<NPARTS;held++) {
        smc.fx_down=(uint8_t)(1u<<held);
        for(int strip=0;strip<NPARTS;strip++)
            expect("FX source held",24+strip,expected_bus[strip]==held);
    }
    smc.fx_down=0;
    for(int strip=0;strip<NPARTS;strip++) expect("FX sources released",24+strip,0);
    ui.view=V_HOME;
    for(int strip=0;strip<NPARTS;strip++) expect("ordinary selection returns",24+strip,expected_bus[strip]==ui.part);
    for(int which=0;which<3;which++) {
        fx[which]=0;expect("FX/master muted",21+which,1);
        fx[which]=100;expect("FX/master audible",21+which,0);
    }
    filter=0;expect("filter endpoint does not mute master",23,0);filter=64;
}
static void drums(void) {
    for(int mode=0;mode<2;mode++) for(int kit=0;kit<NKIT;kit++) for(int selected=0;selected<NDRUM;selected++) {
        ui.view=mode?V_SEQ:V_MIX;page=mode?0:kit+1;ui.sel[kit]=selected;
        ui.part=kit;ui.rec=1;smc.rec_part=3;smc.solo=4;
        smc.drum_solo[kit]=(uint16_t)(1u<<selected);
        drum_mutes[kit]=(uint16_t)(1u<<selected);
        for(int strip=0;strip<8;strip++) {
            int track=(selected/8)*8+strip;
            expect("drum REC bank",strip,track==selected);
            expect("drum SOLO bank",8+strip,track==selected);
            expect("drum MUTE bank",16+strip,track==selected);
            expect("drum SELECT takes priority over shortcuts",24+strip,track==selected);
        }
        /* A visible kit can differ from the actual recording part. */
        if(!mode) {
            ui.part=2;
            for(int strip=0;strip<8;strip++) expect("other part records: no drum REC lamp",strip,0);
        }
    }
    ui.rec=smc.solo=0;drum_mutes[0]=drum_mutes[1]=0;
    smc.drum_solo[0]=smc.drum_solo[1]=0;
    /* Overlay/clip ownership keeps the existing main-map projection. */
    ui.view=V_MIX;page=1;ui.part=3;part_mutes=8;
    for(int owner=0;owner<2;owner++) {
        ui.overlay=owner?O_NONE:O_LIST;clip=owner;
        for(int strip=0;strip<NPARTS;strip++) {
            expect("overlay/clip main MUTE",16+strip,expected_bus[strip]==3);
            expect("overlay/clip main SELECT",24+strip,expected_bus[strip]==3);
        }
    }
    clip=0;ui.overlay=O_NONE;part_mutes=0;
}
static void transport(void) {
    for(int mode=0;mode<3;mode++) for(int playing=0;playing<2;playing++) for(int rec=0;rec<2;rec++) {
        ui.view=mode?V_MIX:V_HOME;page=mode;seq.playing=playing;ui.rec=rec;
        for(int captured=0;captured<2;captured++) for(int phase=0;phase<2;phase++) {
            smc.locked=captured;seq.ext_now=(uint32_t)phase*10584u;
            expect("Pause lock/follow lamp",0x5d,captured||!phase);
        }
        smc.locked=0;
        expect("shared PLAY",0x5e,playing);expect("shared REC",0x5f,rec);
    }
    ui.rec=seq.playing=0;seq.ext_now=0;
}
static void wire_value(const char *label,int note,int expected) {
    int found=-1;for(unsigned i=0;i<tx_count;i++) if(tx[i][0]==0x90 && tx[i][1]==note) found=tx[i][2];
    checks++;if(found!=expected) { failures++;fprintf(stderr,"FAIL %s: wire note=%d got=%d expected=%d\n",label,note,found,expected); }
}
static void frames(void) {
    memset(&smc,0,sizeof smc);ui.part=PART_909;ui.view=V_HOME;page=0;ui.overlay=O_NONE;ui.help=0;ui.rec=0;seq.playing=0;
    proj.set.bt=PROJ_BT_SMC_CENTRAL;smc.up=1;tx_count=0;smc_frame();
    assert(tx_count==43);wire_value("HOME active",7,127);wire_value("MIX inactive",13,0);wire_value("PART inactive",15,0);
    wire_value("selected 909 lights",24+expected_strip(PART_909),127);
    tx_count=0;smc_frame();assert(tx_count==0);
    ui.part=PART_BRK;smc_frame();assert(tx_count==2);
    wire_value("old source selection extinguishes",24+expected_strip(PART_909),0);
    wire_value("selected BREAK lights",24+expected_strip(PART_BRK),127);
    tx_count=0;ui.view=V_FX;smc.fx_down=1u<<PART_303B;smc_frame();
    wire_value("FX view removes ordinary source selection",24+expected_strip(PART_BRK),0);
    wire_value("303B FX hold lights its own strip",24+expected_strip(PART_303B),127);
    tx_count=0;smc.fx_down=0;smc_frame();
    wire_value("303B FX release extinguishes",24+expected_strip(PART_303B),0);
    tx_count=0;ui.view=V_HOME;smc_frame();
    wire_value("leaving FX restores selected source",24+expected_strip(PART_BRK),127);
    tx_count=0;smc_frame();assert(tx_count==0);
    ui.view=V_MIX;page=0;smc_frame();
    wire_value("HOME extinguishes",7,0);wire_value("MIX lights",13,127);
    tx_count=0;seq.playing=1;smc_frame();assert(tx_count==1);
    wire_value("PLAY lights",0x5e,127);
    tx_count=0;seq.ext_now=10584u;smc_frame();assert(tx_count==1);wire_value("following beat off",0x5d,0);
    tx_count=0;seq.ext_now=21168u;smc_frame();assert(tx_count==1);wire_value("following beat on",0x5d,127);
    tx_count=0;seq.ext_now=31752u;smc_frame();assert(tx_count==1);wire_value("following blink off",0x5d,0);
    tx_count=0;smc.locked=1;smc_frame();wire_value("locked steady on",0x5d,127);
    tx_count=0;seq.ext_now=42336u;smc_frame();assert(tx_count==0);
    tx_count=0;seq.ext_now=52920u;smc_frame();assert(tx_count==0);
    smc.locked=0;tx_count=0;smc_frame();wire_value("unlock resumes blink off",0x5d,0);
    tx_count=0;locked=1;seq.playing=0;smc_frame();assert(tx_count==0);locked=0;
    smc.up=0;smc_frame();assert(tx_count==0);smc.up=1;
    proj.set.bt=0;smc_frame();assert(tx_count==0);proj.set.bt=PROJ_BT_SMC_CENTRAL;
    smc_frame();assert(tx_count==1);tx_count=0;
    smc.pending=0x81;smc.value[0]=10;smc.value[7]=120;smc_frame();
    assert(tx_count==2 && !smc.pending);
    assert(tx[0][0]==0xe0 && tx[0][1]==10 && tx[0][2]==10);
    assert(tx[1][0]==0xe7 && tx[1][1]==120 && tx[1][2]==120);
}
int main(void) {
    seq.bpm=125;navigation();main_strips();drums();transport();frames();
    printf("%s: %u SMC feedback expectations, %u failures; navigation, main/drum banks, transport and emitted MIDI\n",failures?"FAIL":"PASS",checks,failures);
    return failures?1:0;
}
'''
with tempfile.TemporaryDirectory(prefix="smc-feedback-") as tmp:
    c = Path(tmp) / "feedback.c"
    binary = Path(tmp) / "feedback"
    c.write_text(harness)
    subprocess.run([*shlex.split(os.environ.get("CC", "cc")), "-std=c99", "-Wall", "-Wextra", "-Werror",
                    "-I" + str(ROOT / "firmware/src"), str(c), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
