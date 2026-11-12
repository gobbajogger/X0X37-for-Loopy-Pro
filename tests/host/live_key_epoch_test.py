#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""25.1 live-key FIFO ignores stale owner generations and accepts current generation."""
import subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
PRE=r"""
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define NBASS 2
#define LIVE_KEY_Q 32u
#define TRK_BASS0 0
#define ORIGINAL_INPUT_KEYS 0
#define ORIGINAL_RUNTIME_OK 0
typedef struct { uint32_t generation, word; } live_key_command_t;
typedef struct { uint32_t generation, context; uint8_t gate, token, pitch, arp, blocked; } live_key_owner_t;
struct bass_dummy { int unused; };
static live_key_command_t live_key_q[LIVE_KEY_Q];
static live_key_owner_t live_key_owner[NBASS];
static volatile uint8_t live_key_w, live_key_r, live_key_cancel;
static uint8_t original_key_pitch[NBASS][64];
static uint8_t bass_held[NBASS], bass_held_count[NBASS], bass_held_src[NBASS], bass_slide[NBASS];
static struct bass_dummy original_bass[NBASS];
static int applied, retired;
static uint32_t plat_cq_lock(void) { return 0; }
static void plat_cq_unlock(uint32_t s) { (void)s; }
static void live_key_context_retire(int v, int release) { (void)v; (void)release; retired++; }
static void bass_adapt_flags(int v, int a, int s) { (void)v; (void)a; (void)s; }
static int SEQ_PART_OFF(void *s, int p) { (void)s; (void)p; return 0; }
static int seq;
static int original_runtime_key(struct bass_dummy *part, int input, unsigned pitch, unsigned velocity)
{ (void)part; (void)input; (void)pitch; if (velocity) applied++; return ORIGINAL_RUNTIME_OK; }
"""
HARNESS=r"""
static void push(unsigned slot, uint32_t generation, unsigned velocity) {
 live_key_q[slot].generation=generation;
 live_key_q[slot].word=(1u<<1)|(60u<<7)|(velocity<<14);
}
int main(void) {
 memset(live_key_q,0,sizeof live_key_q); memset(live_key_owner,0,sizeof live_key_owner);
 live_key_r=0; live_key_w=2; live_key_cancel=0; live_key_owner[0].generation=7;
 push(0,6,100); push(1,7,100);
 live_key_drain();
 if(applied!=1 || original_key_pitch[0][1]!=61 || !live_key_owner[0].gate || live_key_r!=live_key_w) {
  printf("FAIL live key epoch: applied=%d pitch=%u gate=%u r=%u w=%u\\n",applied,original_key_pitch[0][1],live_key_owner[0].gate,live_key_r,live_key_w); return 1;
 }
 live_key_cancel=1u; live_key_drain();
 if(retired!=1) { printf("FAIL live key cancel: retired=%d\\n",retired); return 1; }
 puts("live key epoch: 0 failures"); return 0;
}
"""
def extract(text, signature):
 start=text.index(signature); brace=text.index("{",start); depth=0
 for j in range(brace,len(text)):
  if text[j]=="{": depth+=1
  elif text[j]=="}":
   depth-=1
   if depth==0:return text[start:j+1]+"\\n"
 raise SystemExit("unclosed "+signature)
engine=(ROOT/"firmware/src/app/engine.c").read_text()
source=PRE+extract(engine,"static void live_key_drain(void)\\n{")+HARNESS
with tempfile.TemporaryDirectory(prefix="x0x-live-epoch-") as d:
 c=Path(d)/"test.c"; exe=Path(d)/"test"; c.write_text(source)
 subprocess.run(["cc","-std=c11","-O2","-Wall","-Wextra","-Werror",str(c),"-o",str(exe)],check=True)
 raise SystemExit(subprocess.call([str(exe)]))
