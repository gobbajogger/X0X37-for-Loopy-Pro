#!/usr/bin/env python3
"""Compile actual ui.c commit helpers against a deferred-DMA framebuffer model."""
from pathlib import Path
import subprocess
import tempfile
root = Path(__file__).resolve().parents[2]
s = (root / 'firmware/src/app/ui.c').read_text()
helpers = s[s.index('#include "lcd_tiles.h"'):s.index('static uint16_t blend')]
preamble = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define BOARD_SMK37 1
#define HDR_H 20
#define BAND_H 69
#define KNOB_H 82
#define KNOB_Y 158
static uint16_t canvas[240*138], screen[240*240], pending_copy[768];
static uint16_t *cv_px=canvas;
static uint32_t cv_w=240,cv_h,now=7,advance=1;
static unsigned calls,keys,pads,clips,max_bytes;
static const uint16_t *pending;
static unsigned tx,ty,tw,th;
static uint32_t plat_ms(void){return now;}
static void input(void){keys++;}
static void division_pad_input(void){pads++;}
void smk_ck_poll(void){clips++;}
static void lcd_sync(void){
 if(!pending)return;
 assert(!memcmp(pending,pending_copy,tw*th*2));
 for(unsigned y=0;y<th;y++)memcpy(screen+(ty+y)*240+tx,pending+y*tw,tw*2);
 pending=0;
}
static void lcd_blit(uint32_t x,uint32_t y,uint32_t w,uint32_t h,const uint16_t *p){
 lcd_sync(); /* actual lcd_window/lcd_cmd serializes the preceding DMA */
 assert(w*h*2<=1536 && x+w<=240 && y+h<=240);
 tx=x;ty=y;tw=w;th=h;pending=p;memcpy(pending_copy,p,w*h*2);
 if(w*h*2>max_bytes)max_bytes=w*h*2;
 calls++;now+=advance;
}
'''
checks = r'''
static void fill(unsigned h,unsigned seed){
 lcd_sync();cv_h=h;
 for(unsigned i=0;i<240*h;i++)canvas[i]=(uint16_t)(i*31+seed);
}
static void equal(unsigned y){
 lcd_sync();for(unsigned row=0;row<cv_h;row++)assert(!memcmp(canvas+row*240,screen+(row+y)*240,480));
}
int main(void){
 fill(138,1);cv_commit_rect(1,0,20);equal(20);
 unsigned n=calls;cv_commit_rect(1,0,20);assert(calls==n);
 canvas[0]^=1;canvas[240*138-1]^=1;cv_commit_rect(1,0,20);equal(20);assert(calls==n+2);
 fill(20,2);part_header_snapshot_valid=1;cv_commit_header();equal(0);assert(!part_header_snapshot_valid);
 n=calls;cv_commit_header();cv_commit_header();assert(calls==n);
 canvas[4]^=1;cv_commit_header();equal(0);
 fill(82,3);cv_commit_knobs(3);equal(158);
 fill(82,4);blit_hash[3]=0;popup_knob_full_hash=0;cv_commit_popup_knobs();equal(158);
 n=calls;cv_commit_popup_knobs();cv_commit_popup_knobs();assert(calls==n);
 canvas[240*81+239]^=1;cv_commit_popup_knobs();equal(158);
 fill(82,5);cv_commit(3,0,158);equal(158);assert(!cv_tile_valid[3]);
 fill(82,4);cv_commit_popup_knobs();equal(158);assert(cv_tile_valid[3]);
 /* Fixed hardware tick must not dispatch for every tile. */
 advance=0;now=1234;n=keys;fill(138,6);cv_commit_rect(1,0,20);equal(20);assert(keys==n+1);
 fill(138,7);cv_commit_rect(1,0,20);equal(20);assert(keys==n+1);
 now++;cv_lcd_maybe_input();assert(keys==n+2);
 now=UINT32_MAX;cv_lcd_maybe_input();now=0;cv_lcd_maybe_input();assert(keys==n+4);
 assert(keys==pads && keys==clips);assert(max_bytes==1536);
 printf("PASS production commit helpers: exact pixels, idle, ownership, deferred DMA lifetime, <=1536 bytes, millisecond gating/wrap, key/pad/clip dispatch (%u transfers, %u dispatches)\n",calls,keys);
}
'''
with tempfile.TemporaryDirectory() as tmp:
    c=Path(tmp)/'test.c'; binary=Path(tmp)/'test'
    c.write_text(preamble+helpers+checks)
    subprocess.run(['cc','-std=c11','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-I'+str(root/'firmware/src/app'),str(c),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
