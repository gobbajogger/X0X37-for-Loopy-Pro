#!/usr/bin/env python3
"""Native SMC MMC STOP FIFO ordering under repeated overflow and ISR refill.

The production queue, STOP-drain helper, project drop, and full smc_poll are
extracted unmodified from SMC_SOURCE, with mocked transport/UI effects.
"""
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
source = Path(os.environ.get('SMC_SOURCE', ROOT / 'firmware/src/app/smc_mixer.c')).read_text()


def extract(name):
    match = re.search(r'^[^\n]*\b' + re.escape(name) + r'\([^\n]*\)\s*\{', source, re.M)
    if not match:
        raise RuntimeError('Missing production function ' + name)
    pos, depth = match.end(), 1
    while depth:
        if pos >= len(source):
            raise RuntimeError('Unterminated function ' + name)
        depth += (source[pos] == '{') - (source[pos] == '}')
        pos += 1
    return source[match.start():pos]


prefix = source[source.index('#define SMC_Q'):source.index('static int smc_is_solo_release')]
code = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "app/smc_protocol.h"
#define X0X_COLD
#define PROJ_BT_SMC_CENTRAL 16u
#define PROJ_BT_SMC_USER 32u
#define NKIT 2
#define NPARTS 5
#define NDRUM 11
#define O_ASK 1
#define O_NONE 0
typedef struct { uint8_t kind,a,b,c; } pref_t;
static struct { struct { uint8_t bt; } set; } proj;
static struct { int overlay; } ui;
static struct { uint8_t up, fresh, picked, seen, knob_i, pending, known[128]; } smc;
static int busy, testing, inject, injected, inject_source, log_count;
static int reset_stop_n, stop_seen;
static int reset_cross_source, reset_fresh_other, primary_source;
static uint8_t log_notes[64];
static int perf_testing(void) { return testing; }
static int ui_project_busy(void) { return busy; }
static int smc_project_active(void) { return 0; }
static void smc_solo_clear(void) {}
static void ui_say(const char *a, const char *b) { (void)a; (void)b; }
''' + prefix + '\n' + extract('smc_project_drop') + '\n' + extract('smc_mmc_host') + r'''
static uint8_t smc_message(const uint8_t *msg, uint8_t len, int allow)
{ (void)msg; (void)len; (void)allow; return 0; }
static void smc_guarded_button(uint8_t note, int mmc, int allow)
{
 (void)allow;
 assert(mmc == 1);
 assert(log_count < 64);
 log_notes[log_count++] = note;
 if (note == 0x5Du) {
  ++stop_seen;
  if (reset_cross_source && stop_seen == 1) {
   /* The producer fills the other source while this source's STOP waits.
    * It is OLD-project input and must be discarded by a subsequent reset. */
   for (unsigned i=0;i<16;i++) smc_mmc_host(33,(uint8_t)(primary_source ^ 1));
   smc_mmc_host(34,(uint8_t)(primary_source ^ 1));
  }
  if (stop_seen == reset_stop_n) {
   smc_project_drop();
   if (reset_fresh_other) {
    /* An ISR callback after the project's drop is NEW input and survives. */
    smc_mmc_host(33,(uint8_t)(primary_source ^ 1));
   }
  }
 }
 if (note == 0x5Eu && injected < inject) {
  injected++;
  /* ISR runs after queue reader frees a slot and dispatch begins. */
  smc_mmc_host(33, (uint8_t)inject_source); /* a fresh PLAY enqueues */
  smc_mmc_host(34, (uint8_t)inject_source); /* a second STOP overflows */
 }
}
''' + extract('smc_mmc_drain_stop') + '\n' + extract('smc_poll') + r'''
static void prep(unsigned src, uint8_t start)
{
 memset(&smc,0,sizeof smc); memset(&ui,0,sizeof ui);
 proj.set.bt=PROJ_BT_SMC_USER;busy=testing=inject=injected=log_count=0;
 reset_stop_n=stop_seen=reset_cross_source=reset_fresh_other=primary_source=0;
 smc_qw=smc_qr=smc_host_w=smc_host_r=0;
 smc_mmc_w[0]=smc_mmc_r[0]=0;
 smc_mmc_w[1]=smc_mmc_r[1]=0;
 smc_project_drop();
 smc_mmc_w[src]=smc_mmc_r[src]=start;
 memset(log_notes,0,sizeof log_notes);
}
static int test(unsigned src, uint8_t start, int mode)
{
 prep(src,start);
 if (mode == 8 || mode == 9) {
  /* The USB/peripheral BLE SysEx parser can receive MMC while User mode is
   * disabled. Enqueue-time filtering must stop those old commands from
   * executing if User mode is enabled before the next consumer poll. */
  if (mode == 9)
   for (unsigned i=0;i<16;i++) smc_mmc_host(33,(uint8_t)src);
  proj.set.bt = 0;
  if (mode == 8) smc_mmc_host(33,(uint8_t)src); /* disabled, free FIFO */
  smc_mmc_host(34,(uint8_t)src); /* disabled STOP, free or saturated */
  proj.set.bt = PROJ_BT_SMC_USER;
  smc_poll(); smc_poll();
  int expected = mode == 9 ? 16 : 0;
  if (log_count != expected ||
      smc_mmc_r[src] != smc_mmc_w[src] ||
      smc_mmc_stop_r[src] != smc_mmc_stop_w[src]) {
   fprintf(stderr,"FAIL disabled MMC replay mode=%d src=%u cursor=%u log=%d expected=%d\n",
           mode,src,start,log_count,expected); return 0;
  }
  smc_mmc_host(34,(uint8_t)src); /* current enabled User must still work */
  smc_poll();
  if (log_count != expected+1 || log_notes[expected] != 0x5D ||
      smc_mmc_r[src] != smc_mmc_w[src]) {
   fprintf(stderr,"FAIL enabled User STOP after disabled frame mode=%d src=%u\n",mode,src);
   return 0;
  }
  return 1;
 }
 for(unsigned i=0;i<16;i++)smc_mmc_host(33,(uint8_t)src);
 smc_mmc_host(34,(uint8_t)src); /* FIRST STOP at cutoff start+16 */
 if(mode==0){
  inject=1;inject_source=(int)src;
  smc_poll();smc_poll();
  if (log_count!=19) { /* 16 PLAY, two STOP, one later PLAY */
   fprintf(stderr,"FAIL count %d instead of 19 src=%u start=%u\n",log_count,src,start);return 0;
  }
  for(int i=0;i<16;i++) if(log_notes[i]!=0x5E) return 0;
  if(log_notes[16]!=0x5D || log_notes[17]!=0x5E || log_notes[18]!=0x5D){
   fprintf(stderr,"FAIL double STOP ordering src=%u start=%u: pos16=%02x pos17=%02x pos18=%02x\n",
           src,start,log_notes[16],log_notes[17],log_notes[18]);return 0;
  }
 }else if(mode==4){
  inject=16;inject_source=(int)src;
  smc_poll();smc_poll();smc_poll();
  if(log_count!=49 || injected!=16){
   fprintf(stderr,"FAIL max-cutoff stress count=%d injections=%d\n",log_count,injected);return 0;
  }
  for(unsigned j=0;j<16;j++)if(log_notes[j]!=0x5E)return 0;
  if(log_notes[16]!=0x5D)return 0;
  for(unsigned j=0;j<16;j++) {
   if(log_notes[17+j*2]!=0x5E || log_notes[18+j*2]!=0x5D) {
    fprintf(stderr,"FAIL 17 unique STOP cutoffs j=%u\n",j);return 0;
   }
  }
 }else if(mode==5){
  /* On the second overflow STOP callback, a project change discards the
   * remaining queue. Never consume an obsolete command or poison the ring. */
  inject=16; inject_source=(int)src; reset_stop_n=2;
  smc_poll(); smc_poll();
  if(log_count!=19 || smc_mmc_r[src]!=smc_mmc_w[src] ||
     smc_mmc_stop_r[src]!=smc_mmc_stop_w[src]) {
   fprintf(stderr,"FAIL reset-in-STOP resurrected old MMC item: count=%d q=%u/%u stop=%u/%u src=%u start=%u\n",
     log_count,smc_mmc_r[src],smc_mmc_w[src],
     smc_mmc_stop_r[src],smc_mmc_stop_w[src],src,start);return 0;
  }
  inject=0;
  smc_mmc_host(33,(uint8_t)src); smc_poll();
  if(log_count!=20 || log_notes[19]!=0x5E || smc_mmc_r[src]!=smc_mmc_w[src]) {
   fprintf(stderr,"FAIL reset poisoned next project MMC queue: count=%d q=%u/%u\n",
      log_count,smc_mmc_r[src],smc_mmc_w[src]);return 0;
  }
 }else if(mode==6){
  /* Source A overflows its MMC ring and receives an ordered second STOP.
   * Its first STOP's callback then fills source B with OLD-project PLAYs
   * and an overflow STOP; the current STOP then resets BOTH rings. Only
   * post-reset source B PLAY, published after the drop, may survive. */
  inject=16; inject_source=(int)src; reset_stop_n=1;
  reset_cross_source=reset_fresh_other=1; primary_source=(int)src;
  smc_poll(); smc_poll(); smc_poll();
  if(log_count!=18 || injected!=16 || stop_seen!=1) {
   fprintf(stderr,"FAIL cross-source project reset log=%d injected=%d stops=%d src=%u start=%u\n",
           log_count,injected,stop_seen,src,start);return 0;
  }
  for(unsigned i=0;i<16;i++)if(log_notes[i]!=0x5E)return 0;
  if(log_notes[16]!=0x5D || log_notes[17]!=0x5E) {
   fprintf(stderr,"FAIL cross-source stale or fresh event ordering src=%u start=%u\n",src,start);
   return 0;
  }
  for(unsigned q=0;q<2;q++) {
   if(smc_mmc_r[q]!=smc_mmc_w[q] || smc_mmc_stop_r[q]!=smc_mmc_stop_w[q]) {
    fprintf(stderr,"FAIL cross-source queue poisoned src=%u other=%u\n",src,q);
    return 0;
   }
  }
 }else if(mode==7){
  /* A non-overflowed, queued STOP is equally capable of replacing the
   * project while an ISR is publishing on the OTHER source. The queued
   * old-project PLAY following STOP must not be replayed. */
  smc_project_drop();
  smc_mmc_host(33,(uint8_t)src); /* PLAY */
  smc_mmc_host(34,(uint8_t)src); /* queued STOP */
  smc_mmc_host(33,(uint8_t)src); /* stale PLAY after project reset */
  reset_stop_n=1;
  reset_cross_source=reset_fresh_other=1; primary_source=(int)src;
  smc_poll(); smc_poll(); smc_poll();
  if(log_count!=3 || stop_seen!=1 ||
     log_notes[0]!=0x5E || log_notes[1]!=0x5D || log_notes[2]!=0x5E) {
   fprintf(stderr,"FAIL queued-STOP cross-source project reset log=%d stops=%d src=%u start=%u\n",
           log_count,stop_seen,src,start);return 0;
  }
  for(unsigned q=0;q<2;q++) {
   if(smc_mmc_r[q]!=smc_mmc_w[q] || smc_mmc_stop_r[q]!=smc_mmc_stop_w[q]) {
    fprintf(stderr,"FAIL queued-STOP cross-source ring poison src=%u other=%u\n",src,q);
    return 0;
   }
  }
 }else if(mode==1){
  for(unsigned j=0;j<200;j++)smc_mmc_host(34,(uint8_t)src);
  smc_poll();smc_poll();
  if(log_count!=17 || log_notes[16]!=0x5D){
   fprintf(stderr,"FAIL same-cutoff repeated STOP coalescing count=%d\n",log_count);return 0;
  }
 }else if(mode==2){
  smc_project_drop();smc_poll();
  if(log_count!=0) {fprintf(stderr,"FAIL stale STOP after reset\n");return 0;}
 }else if(mode==3){
  proj.set.bt=0;
  smc_poll();
  if(log_count!=0) {fprintf(stderr,"FAIL disabled User MMC dispatch\n");return 0;}
 }
 smc_poll();
 if(mode!=3 && mode!=2 && ((uint8_t)(smc_mmc_w[src]-smc_mmc_r[src])!=0)){
  fprintf(stderr,"FAIL queue not drained\n");return 0;
 }
 return 1;
}
int main(void)
{
 int count=0;
 for(unsigned src=0;src<2;src++)for(unsigned i=0;i<4;i++)for(unsigned mode=0;mode<10;mode++){
  uint8_t start=(const uint8_t[]){0,15,240,255}[i];
  if(!test(src,start,mode))return 1;
  count++;
 }
 assert(count==80);
 printf("SMC MMC STOP FIFO source-window: %d/80 PASS\n",count);
 return 0;
}
'''

with tempfile.TemporaryDirectory(prefix='smc-mmc-stop-') as tmp:
    c = Path(tmp) / 'test.c'
    exe = Path(tmp) / 'test'
    c.write_text(code)
    command = [os.environ.get('CC', 'cc'), '-std=c99', '-O1', '-g',
               '-Wall', '-Wextra', '-Werror',
               *shlex.split(os.environ.get('SAN_FLAGS', '')),
               '-I' + str(ROOT / 'firmware/src'), str(c), '-o', str(exe)]
    subprocess.run(command, check=True)
    subprocess.run([str(exe)], check=True)
