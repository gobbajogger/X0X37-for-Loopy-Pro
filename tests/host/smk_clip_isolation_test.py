#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Run real clip output, frame and pad input functions with host hardware stubs.

SRC may point to another firmware/src tree to demonstrate the regression.
Build products live in a temporary directory. No device or shared queue is used.
"""
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
SRC = Path(os.environ.get("SRC", str(ROOT / "firmware/src"))).resolve()
main = (SRC / "app/main_fm1.c").read_text()
plat = (SRC / "app/plat_fm1.c").read_text()
hal = (SRC.parent / "hal/smk_input.h").read_text()


def function(source, name):
    match = re.search(r"^[^\n;]*\b" + name + r"\([^;]*?\)\s*(?:/\*[^\n]*\*/)?\n\{.*?^\}", source, re.M | re.S)
    if not match:
        raise AssertionError(f"Missing function {name}")
    return match.group()


prefix = r'''
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "app/smk_apc40.h"
#define CK_FN
#define BOARD_SMK37 1
#define X0X_DEV_PADCAL 0
#define SMK_NPAD 16
#define SMK_KEY_F3 5
#define SMK_BTN_KNOB_BANK 0
#define SMK_WHITE_OF_PAD(p) (smk_pad_grid ? (p) : (p) < 8u ? (p) + 8u : (p) - 8u)
enum {B_REC, B_PLAY, B_OCTDN, B_OCTUP};
static struct {uint8_t btn[4];} panel = {{1,2,3,4}};
static uint8_t clip_grid_mode, knob_bank_prev, smk_pad_piano, smk_pad_grid = 1;
static uint8_t apc_cols=8, apc_rows=5, apc_origin_col, apc_origin_row, apc_view;
static uint8_t apc_led_velocity[40];
static uint8_t smk_pad_note[16];
static uint32_t clip_pad_keys;
static uint8_t clip_pad_note[SMK_APC40_PAD_COUNT], clip_pad_active, project_busy;
static int ui_project_busy(void) {return project_busy;}
static const uint8_t SMK_WHITE_NOTE[16] = {0,2,4,6,7,9,11,12,14,16,18,19,21,23,24,26};
static const int8_t SMK_PIANO_NOTE[16] = {7,9,11,12,14,16,18,19,8,10,-1,13,15,17,-1,20};
static struct {
 uint32_t buttons,pnotes,knotes,notes,notes_pressed;
 uint64_t keys;  /* hal/smk_input.h: smk_frame reads the outer keys for the BREAK pages */
 uint8_t key_vel[37],pad_vel[16];
} fm1_in;
/* smk_frame calls it for the BREAK page keys (no BREAK UI in this test) */
void ui_brk_page_keys(unsigned keys, int allowed) {(void)keys;(void)allowed;}
static unsigned failed, irq, require_lock, usb_n, ble_n, flush_n, flush_mode[8];
static uint32_t usb_packets[256], ble_packets[256];
#define CHECK(x) do {if (!(x)) {fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x); failed++;}} while(0)
/* Match fm1_flash.h: CLI then return 0; restore always STI. Nesting is invalid. */
static uint32_t irq_save(void) {CHECK(!irq); irq=1; return 0;}
static void irq_restore(uint32_t old) {(void)old; CHECK(irq); irq=0;}
static void midi_out_event(uint32_t p) {if(require_lock) CHECK(irq); usb_packets[usb_n++]=p;}
static void ble_out(uint32_t p) {if(require_lock) CHECK(irq); ble_packets[ble_n++]=p;}
static uint8_t smk_clip_mode_on(void) {return clip_grid_mode;}
static void say(const char *s, unsigned n) {(void)s;(void)n;}
static int smk_extkey_take(void) {return -1;}
static void smk_arrows(void) {}
static int smk_clip_map_refresh(void) {return 0;}
static void smk_faders(void) {}
static void smk_wheels(void) {}
static void smk_grid(void) {}
static void smk_pads(void) {}
void smk_clip_enter_cleanup(void) {}
static void smk_apc_clip_entered(void) {}
void engine_clip_midi_flush(void) {CHECK(irq); flush_mode[flush_n++]=clip_grid_mode;}
static void clear_packets(void) {usb_n=ble_n=0;}
'''

body = "\n".join(function(main, n) for n in ("apc_out", "apc_note"))
body += "\n" + function(plat, "plat_midi_out")
body += "\n" + function(main, "smk_clip_history_button")
body += "\n" + function(hal, "smk__padq")
if "static void smk_clip_mode_poll(void)" in main:
    body += "\n" + function(main, "smk_clip_mode_poll")
else:
    body += "\nstatic void smk_clip_mode_poll(void) {}\n"
body += "\n" + function(main, "smk_frame")
body += "\nstatic void frame(void) {smk_clip_mode_poll(); smk_frame();}\n"

suffix = r'''
int main(void) {
 /* Generic engine notes, CC, program, clock and transport never enter the clip stream. */
 const uint32_t messages[]={0x7f5d9009,0x005d8008,0x7f30b00b,0x0001c00c,0x0000f80f,0x0000fa0f,0x0000fb0f,0x0000fc0f};
 clip_grid_mode=1;
 for(unsigned i=0;i<sizeof(messages)/sizeof(messages[0]);i++) plat_midi_out(messages[i]);
 CHECK(usb_n==0 && ble_n==0);
 clip_grid_mode=0; clear_packets();
 for(unsigned i=0;i<sizeof(messages)/sizeof(messages[0]);i++) plat_midi_out(messages[i]);
 CHECK(usb_n==8 && ble_n==(X0X_BLE ? 8u:0u));
 for(unsigned i=0;i<8;i++) CHECK(usb_packets[i]==messages[i]);
 /* Main-loop USB and BLE producers must both run inside the same critical section. */
 clear_packets(); require_lock=1;
 apc_note(3,37); CHECK(usb_n==1 && usb_packets[0]==0x25039009u);
 CHECK(ble_n==(X0X_BLE ? 1u:0u)); CHECK(!irq);
 apc_note(3,0); CHECK(!irq);
 clip_grid_mode=1; clear_packets();
 CHECK(smk_clip_history_button(B_REC,1,1));
 clip_grid_mode=0; CHECK(smk_clip_history_button(B_REC,0,0));
 CHECK(usb_n==2 && ble_n==0); CHECK(!irq);
 CHECK(usb_packets[0]==smk_apc40_history_packet(0,1));
 CHECK(usb_packets[1]==smk_apc40_history_packet(0,0));
 /* Entry flush precedes ownership transfer. A pad already held at entry does not fire. */
 memset(apc_led_velocity,1,sizeof(apc_led_velocity));
 clear_packets(); smk__padq(0,31); fm1_in.buttons=1; frame();
 CHECK(clip_grid_mode && flush_n==1 && flush_mode[0]==0 && !irq);
 CHECK(usb_n==0); fm1_in.buttons=0; smk__padq(0,0); frame(); CHECK(usb_n==0);
 /* Every physical pad carries its own velocity even if its matching keybed key changed. */
 for(unsigned p=0;p<16;p++) {
   for(unsigned v=1;v<=127;v+=(v==1 ? 36:90)) {
     clear_packets(); smk__padq(p,v);
     fm1_in.key_vel[SMK_KEY_F3+SMK_WHITE_NOTE[p]]=99;
     frame();
     unsigned note=smk_apc40_note_for_pad_view(8,5,0,0,0,smk_apc40_logical_pad_for_physical(p));
     CHECK(usb_n==1); CHECK(usb_packets[0]==(9u|0x90u<<8|note<<16|v<<24));
     CHECK(ble_n==(X0X_BLE ? 1u:0u));
     frame(); CHECK(usb_n==1);
     smk__padq(p,0); frame();
     CHECK(usb_n==2 && ((usb_packets[1]>>8)&255)==0x80 && ((usb_packets[1]>>16)&127)==note);
   }
 }
 /* Blank cells cannot start recording and mode exit releases only sent notes. */
 clear_packets(); unsigned note=smk_apc40_note_for_pad_view(8,5,0,0,0,smk_apc40_logical_pad_for_physical(0));
 apc_led_velocity[note]=0; smk__padq(0,64); frame(); CHECK(usb_n==0);
 smk__padq(0,0); frame(); CHECK(usb_n==0);
 apc_led_velocity[note]=1; smk__padq(0,64); frame(); CHECK(usb_n==1);
 fm1_in.buttons=1; frame();
 CHECK(!clip_grid_mode && flush_n==2 && flush_mode[1]==1 && !irq);
 CHECK(usb_n==2 && ((usb_packets[1]>>8)&255)==0x80);
 smk__padq(0,0); fm1_in.buttons=0; frame(); CHECK(usb_n==2);
 /* A project hold consumes mode/pad edges without forwarding them after release. */
 clear_packets(); project_busy=1; fm1_in.buttons=1; frame();
 CHECK(!clip_grid_mode && flush_n==2 && usb_n==0);
 project_busy=0; frame(); CHECK(!clip_grid_mode && flush_n==2 && usb_n==0);
 fm1_in.buttons=0; frame(); fm1_in.buttons=1; frame();
 CHECK(clip_grid_mode && flush_n==3 && usb_n==0);
 fm1_in.buttons=0; frame(); project_busy=1; smk__padq(0,80); frame();
 CHECK(usb_n==0); project_busy=0; frame(); CHECK(usb_n==0);
 smk__padq(0,0); frame(); CHECK(usb_n==0);
 smk__padq(0,80); frame(); CHECK(usb_n==1);
 smk__padq(0,0); frame(); CHECK(usb_n==2);
 if(failed) return 1;
 printf("PASS clip isolation, pad velocity and mode ownership (BLE=%d)\n",X0X_BLE);
 return 0;
}
'''
# The sequencer flush must end both sides of a slide without stopping local audio.
seq_source = (SRC / "seq/sequencer.c").read_text()
flush_prefix = r'''
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#define NBASS 2
typedef struct {
 uint8_t bass_slide_valid[2], bass_slide_ch[2], bass_slide_old[2];
 uint8_t bass_out_valid[2], bass_out_ch[2], bass_notes_out[2];
 unsigned bass_slide_off[2];
 uint8_t playing;
} seq_t;
typedef struct {void (*midi)(void *,uint8_t,uint8_t,uint8_t);void *ctx;} seq_sink_t;
static unsigned count;
static uint32_t output[4];
static void capture(void *ctx,uint8_t st,uint8_t d1,uint8_t d2) {
 (void)ctx; assert(count<4); output[count++]=st|d1<<8|d2<<16;
}
'''
# #521/#528 shared Note Off helpers out of bass_midi_flush; extract them too.
flush_helpers = ("midi", "midi_note_off", "bass_slide_midi_off", "bass_midi_flush")
flush_body = "\n".join(function(seq_source, n) for n in flush_helpers)
if "void seq_midi_flush(" in seq_source:
    flush_body += "\n" + function(seq_source, "seq_midi_flush")
else:
    flush_body += "\nvoid seq_midi_flush(seq_t *s,const seq_sink_t *o) {(void)s;(void)o;}\n"
flush_suffix = r'''
int main(void) {
 seq_t s={.bass_slide_valid={1,1},.bass_slide_ch={2,5},.bass_slide_old={40,50},
          .bass_out_valid={1,1},.bass_out_ch={3,6},.bass_notes_out={42,52},
          .bass_slide_off={17,23},.playing=1};
 seq_sink_t sink={capture,0};
 seq_midi_flush(&s,&sink);
 assert(count==4 && output[0]==(0x82|40<<8) && output[1]==(0x83|42<<8));
 assert(output[2]==(0x85|50<<8) && output[3]==(0x86|52<<8));
 for(int i=0;i<2;i++) assert(!s.bass_slide_valid[i] && !s.bass_out_valid[i] && !s.bass_slide_off[i]);
 assert(s.playing==1); seq_midi_flush(&s,&sink); assert(count==4);
 puts("PASS sequencer MIDI flush preserves playback and ends both slide notes");
 return 0;
}
'''

with tempfile.TemporaryDirectory(prefix="smk-clip-isolation-") as tmp:
    source = Path(tmp) / "test.c"
    source.write_text(prefix + body + suffix)
    for ble in (0, 1):
        binary = Path(tmp) / f"test-{ble}"
        subprocess.run([os.environ.get("CC", "cc"), "-std=c99", "-Wall", "-Wextra", "-Wno-unused-function",
                        f"-DX0X_BLE={ble}", "-I", str(SRC), str(source), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
    source.write_text(flush_prefix + flush_body + flush_suffix)
    binary = Path(tmp) / "flush-test"
    subprocess.run([os.environ.get("CC", "cc"), "-std=c99", str(source), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)

# A simultaneous mode press must establish routing before either UI input path.
dispatch = function(main, "fm1_main")
poll = dispatch.index("smk_clip_mode_poll();")
branch = dispatch.index("if (fm1_ms - last_frame >= 16u)")
assert poll < branch < dispatch.index("ui_frame();") < dispatch.index("ui_input_only();")
print("PASS clip ownership changes before both UI dispatch paths")
