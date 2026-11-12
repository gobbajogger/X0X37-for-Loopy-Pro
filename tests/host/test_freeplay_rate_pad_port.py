#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Compile the real Free Play rate-pad HAL/UI handlers against a small host fake.

Reads the source from the requested checkout, not an approximate Python model.
Runs without the JieLi SDK, hardware or private DSP headers. It does not prove
that the firmware links, that the musical rate is bit-exact, or that pads work on
hardware. Also covers the bottom-row wobble SHAPE pads (9-12) and the empty
pads (13-16) that do nothing.
"""
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
UI = ROOT / "firmware/src/app/ui.c"
HAL = ROOT / "firmware/hal/smk_input.h"


def function(source, signature):
    """Extract a complete function by its exact signature and source braces."""
    match = re.search(re.escape(signature) + r"\s*\{", source)
    if not match:
        raise AssertionError(f"missing exact source function: {signature}")
    depth = 1
    i = match.end()
    # These routines have no comments or strings containing braces.
    while depth:
        if i >= len(source):
            raise AssertionError(f"unterminated source function: {signature}")
        depth += (source[i] == "{") - (source[i] == "}")
        i += 1
    return source[match.start():i]


def compile_and_run():
    ui = UI.read_text()
    hal = HAL.read_text()
    poll = function(hal, "static int smk__division_poll(unsigned owner)")
    owner = function(ui, "static int division_pad_owner(void)")
    index = function(ui, "static int division_pad_index(void)")
    label = function(ui, "static const char *division_pad_label(void)")
    handler = function(ui, "static void division_pad_input(void)")
    shape = function(ui, "static int division_shape_index(void)")
    shape_enum = re.search(r"enum \{ DIVISION_SHAPE_PADS = \d+ \};", ui).group(0)

    pad_func = hal.index("static void smk__padq(uint32_t pad, uint32_t vel)")
    pad_branch_start = hal.index("#if BOARD_SMK37 && X0X_ORIGINAL_SYNTH && X0X_DEV_WOBBLE", pad_func)
    pad_branch_end = hal.index("\n#endif", pad_branch_start) + len("\n#endif")
    pad_branch = hal[pad_branch_start:pad_branch_end]
    for token in ("smk_div_held", "smk_div_owner", "smk_div_pending", "smk_pad_note[pad]"):
        assert token in pad_branch, f"missing physical press/release ownership: {token}"

    # Compile exactly the source routines and physical-event routing (extracted
    # above) with fake firmware dependencies. No copied implementation of the
    # rate-selection logic occurs in this test.
    cpp = r'''#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define BOARD_SMK37 1
#define X0X_ORIGINAL_SYNTH 1
#ifndef X0X_DEV_WOBBLE
#define X0X_DEV_WOBBLE 1
#endif
#define X0X_DEV_PADCAL 0
#define ORIGINAL_SOUND_MIAMI 1
#define V_SEQ 1
#define V_PART 2
#define O_NONE 0
#define O_LIST 1
#define O_ASK 2
#define B_HOME 1
#define B_SAVE 2
#define R_ENG 1
#define T_303 2
#define ORIGINAL_MIAMI_WOBBLE_RATE 13
#define ORIGINAL_MIAMI_WOBBLE_SHAPE 11
#define SMK_NPAD 16u
#define PR(k,t,v,p) ((pref_t){k,t,v,p})
typedef struct { int kind, target, voice, parameter; } pref_t;
static int project_busy, perftest, clip, smc_active;
static int banks[2] = {ORIGINAL_SOUND_MIAMI, ORIGINAL_SOUND_MIAMI};
static int rate[2], wshape[2], shape_edits, rate_edits, note_down, note_up, feedbacks;
static const char *const names[] = {"1/2","1/4","1/8","1/16","1/3","1/6","1/12","1/24"};
static uint8_t smk_pad_note[16];
static struct {int rec,help,overlay,btn,view,part,freeplay[2],dirty;unsigned act_t;} ui;
static int perf_testing(void) {return perftest;}
static int is_303(void) {return ui.part == 0 || ui.part == 1;}
static int bidx(void) {return ui.part;}
static int engine_bass_bank(int v) {return banks[v];}
uint8_t smk_clip_mode_on(void) {return (uint8_t)clip;}
static int smc_ui_active(void) {return smc_active;}
static int pref_get(pref_t p) { assert(p.kind==R_ENG && p.target==T_303 && (p.parameter==13 || p.parameter==11));return p.parameter==13?rate[p.voice]:wshape[p.voice]; }
static void pref_set(pref_t p, int v) {assert(p.kind==R_ENG && p.target==T_303 && (p.parameter==13 || p.parameter==11)); assert(v>=0&&v<=127);
    if(p.parameter==13){rate[p.voice]=v;rate_edits++;}else{wshape[p.voice]=v;shape_edits++;}}
static void mark_dirty(void) {ui.dirty=1;}
static unsigned plat_ms(void) {return 1234u;}
static const char *said;
static void say(const char *a,const char *b) {assert(!strcmp(a,"WOBBLE "));assert(b && *b);said=b;feedbacks++;}
static volatile uint8_t smk_div_owner, smk_div_pending;
static uint16_t smk_div_held;
''' + poll + r'''
static void smk__padq(uint32_t pad, uint32_t vel) {
''' + pad_branch + r'''
 if(vel){smk_pad_note[pad]=1;note_down++;}
 else if(smk_pad_note[pad]){smk_pad_note[pad]=0;note_up++;}
}
static int plat_division_pad(unsigned owner) {return smk__division_poll(owner);}
''' + owner + '\n' + index + '\n' + label + '\n' + shape_enum + '\n' + shape + '\n' + handler + r'''
int main(void) {
    ui.view=V_PART; ui.part=0; ui.freeplay[0]=1;
#if X0X_DEV_WOBBLE
    assert(division_pad_owner()==1);
    for(int pad=0;pad<8;pad++) {
        division_pad_input();             /* register the owner before input ISR */
        assert(smk_div_owner==1);
        smk__padq((uint32_t)pad,80);
        assert(smk_div_pending==(uint8_t)(pad+1));
        division_pad_input();
        assert(rate_edits==pad+1);
        assert(division_pad_index()==pad);
        assert(strstr(division_pad_label(), names[pad]));
        assert(ui.dirty==1 && ui.act_t==1234u);
        assert(feedbacks==pad+1);
        smk__padq((uint32_t)pad,0);
        assert(smk_div_held==0);
    }
    assert(note_down==0 && note_up==0); /* rate hits never play notes */

    /* Bottom row: pads 9-12 pick the wobble shape TRI, DOWN, UP, SQR; pads 13-16
     * are empty. No pad of the bottom row plays a note or edits the rate. */
    static const char *const shapes[4]={"SHAPE TRI","SHAPE DOWN","SHAPE UP","SHAPE SQR"};
    static const int values[4]={0,42,85,127};
    for (int pad=8;pad<16;pad++) {
        int rates=rate_edits, edits=shape_edits, fb=feedbacks, before=division_shape_index();
        division_pad_input();
        smk__padq((uint32_t)pad,80);
        assert(smk_pad_note[pad]==0 && (smk_div_held & (uint16_t)(1u<<pad)));
        division_pad_input();
        assert(rate_edits==rates);
        if (pad<12) {
            assert(shape_edits==edits+1 && wshape[0]==values[pad-8]);
            assert(division_shape_index()==pad-8 && !strcmp(said,shapes[pad-8]));
            assert(feedbacks==fb+1);
        } else {
            assert(shape_edits==edits && feedbacks==fb && division_shape_index()==before);
        }
        smk__padq((uint32_t)pad,0);
        assert(smk_div_held==0 && smk_pad_note[pad]==0);
    }
    assert(note_down==0 && note_up==0);
    project_busy=1;division_pad_input();assert(smk_div_owner==0);
    project_busy=0;division_pad_input();
    note_down=note_up=0; /* keep later mixed ownership tests independent */

    /* Existing press retains its original release owner across mode change. */
    division_pad_input();smk__padq(0,80);project_busy=1;
    division_pad_input();assert(smk_div_owner==0);
    smk__padq(0,0);assert(note_down==0 && note_up==0);
    project_busy=0;ui.freeplay[0]=0;division_pad_input();
    smk__padq(1,80);assert(note_down==1);
    ui.freeplay[0]=1;division_pad_input();
    smk__padq(1,0);assert(note_up==1); /* an original note is not stolen */

    /* A press pending for A must never edit B after the owner switches. */
    ui.part=0;division_pad_input();smk__padq(2,80);
    ui.part=1;ui.freeplay[1]=1;division_pad_input();
    assert(smk_div_owner==2 && smk_div_pending==0 && rate_edits==8);
    smk__padq(2,0);
    smk__padq(7,80);division_pad_input();
    assert(rate_edits==9 && division_pad_index()==7);
    ui.part=0;assert(division_pad_index()==7); /* previous rate was 1/24 */
    smk__padq(7,0);

    /* A settings list or HELP does not take the pads: each rate pad still sets
     * the rate it shows (owner request: pads work as labelled under settings). */
    for (int cx=0;cx<2;cx++) {
        if (cx) ui.help=1; else ui.overlay=O_LIST;
        assert(division_pad_owner()==1);
        for (int pad=0;pad<8;pad++) {
            int edits=rate_edits;
            division_pad_input();smk__padq((uint32_t)pad,90);division_pad_input();
            assert(rate_edits==edits+1 && division_pad_index()==pad);
            assert(strstr(division_pad_label(), names[pad]));
            smk__padq((uint32_t)pad,0);
        }
        for (int pad=8;pad<16;pad++) {
            int edits=shape_edits, rates=rate_edits;
            division_pad_input();smk__padq((uint32_t)pad,90);division_pad_input();
            assert(rate_edits==rates && shape_edits==edits+(pad<12));
            if (pad<12) assert(division_shape_index()==pad-8);
            smk__padq((uint32_t)pad,0);
        }
        ui.help=0;ui.overlay=O_NONE;
    }
    assert(note_down==1 && note_up==1);   /* no stray notes from rate pads */
    /* UI ownership goes away under a question, recording, clip/SMC ownership. */
    ui.rec=1;assert(!division_pad_owner());ui.rec=0;
    ui.overlay=O_ASK;assert(!division_pad_owner());ui.overlay=O_NONE;
    ui.btn=1u<<B_HOME;assert(!division_pad_owner());ui.btn=0;
    ui.btn=1u<<B_SAVE;assert(!division_pad_owner());ui.btn=0;
    perftest=1;assert(!division_pad_owner());perftest=0;
    smc_active=1;assert(!division_pad_owner());smc_active=0;
    clip=1;assert(!division_pad_owner());clip=0;
    banks[0]=0;assert(!division_pad_owner());banks[0]=ORIGINAL_SOUND_MIAMI;
    ui.freeplay[0]=0;assert(!division_pad_owner());ui.freeplay[0]=1;
    ui.view=0;assert(!division_pad_owner());ui.view=V_SEQ;
    assert(division_pad_owner()==1);
#else
    /* Explicit opt-out restores ordinary note presses on all physical pads. */
    assert(division_pad_owner()==0);
    division_pad_input(); /* disabled selector must not claim the HAL owner */
    assert(smk_div_owner==0 && smk_div_pending==0);
    for (int pad=0;pad<16;pad++) {
        smk__padq((uint32_t)pad,80);
        assert(smk_pad_note[pad]!=0);
        smk__padq((uint32_t)pad,0);
        assert(smk_pad_note[pad]==0);
    }
    assert(note_down==16 && note_up==16);
    assert(rate_edits==0 && smk_div_pending==0 && smk_div_held==0);
    assert(!ui.dirty && !ui.act_t);
    (void)names;
    (void)project_busy; (void)said; (void)shape_edits;
#endif
    puts("PASS: physical Free Play rate pads: eight rates, four wobble shapes, four empty pads and opt-out note behavior");
    return 0;
}
'''
    compiler = os.environ.get("CC", "cc")
    with tempfile.TemporaryDirectory(prefix="x0x-rate-pad-") as temp:
        c = Path(temp)/"rate_pad.c"
        c.write_text(cpp)
        for enabled in ("1", "0"):
            exe = Path(temp)/f"rate_pad_{enabled}"
            subprocess.run([compiler,"-std=c11","-O1","-Wall","-Wextra","-Werror", "-Wno-unused-function", "-fsanitize=undefined","-fno-sanitize-recover=all", f"-DX0X_DEV_WOBBLE={enabled}",str(c),"-o",str(exe)],check=True)
            subprocess.run([str(exe)],check=True)


if __name__ == "__main__":
    compile_and_run()
