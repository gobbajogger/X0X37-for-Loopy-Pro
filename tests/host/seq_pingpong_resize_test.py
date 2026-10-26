#!/usr/bin/env python3
"""Compile the exact production next_pos() source window and test live 303 length edits.

Standalone native host regression: python3 tests/host/seq_pingpong_resize_test.py
Uses firmware/src/seq/sequencer.c from the invoking repository by default.
An extracted production function (not whole-target firmware) is the scope.
"""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile

START = 'static int next_pos(seq_t *s, int t, int p, int commit)'
END = '\n__attribute__((noinline)) static void midi('
PREFIX = r'''
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define TRK_BASS0 2
#define NBASS 2
#define NTRACKS 5
enum { DIR_FWD, DIR_REV, DIR_PINGPONG, DIR_RANDOM };
typedef struct { int8_t pp_dir; uint32_t rng; } seq_track_t;
typedef struct { int dir; } bpart_t;
typedef struct { seq_track_t t[NTRACKS]; bpart_t bass[NBASS]; int len[NTRACKS]; } seq_t;
static int is_bass(int t) { return t >= TRK_BASS0 && t < TRK_BASS0+NBASS; }
static bpart_t *bpart(seq_t *s, int b) { return &s->bass[b]; }
static int seq_part_len(seq_t *s, int t) {
  int len=s->len[t]; return len<1?1:len>64?64:len;
}
static uint32_t rng_next(uint32_t *r) {
  uint32_t x=*r?*r:1; x^=x<<13; x^=x>>17; x^=x<<5; return *r=x;
}
'''
SUFFIX = r'''
static seq_t s;
static int passed, failed;
#define CHECK(cond,desc) do {if(cond){passed++;}else{failed++;fprintf(stderr,"FAIL %s\n",desc);}}while(0)
static void setup(int len,int dir,int pp_dir) {
    memset(&s,0,sizeof s);
    s.len[TRK_BASS0]=len;
    s.bass[0].dir=dir;
    s.t[TRK_BASS0].pp_dir=(int8_t)pp_dir;
    s.t[TRK_BASS0].rng=123;
}
int main(void) {
    int n;
    /* Changing from 16 to 4 while at step 16 must never sound steps 5..16. */
    setup(4,DIR_PINGPONG,+1);
    n=next_pos(&s,TRK_BASS0,15,1);
    CHECK(n==3,"shrink 16->4 from last position (+1): choose last valid step");
    CHECK(s.t[TRK_BASS0].pp_dir==-1,"shrink 16->4 (+1): reverse toward active range");
    CHECK(next_pos(&s,TRK_BASS0,n,1)==2,"after shrink resume in-range reverse sequence");
    setup(4,DIR_PINGPONG,-1);
    n=next_pos(&s,TRK_BASS0,12,1);
    CHECK(n==3,"shrink while already reversing: return to last valid step");
    CHECK(s.t[TRK_BASS0].pp_dir==-1,"shrink while reversing retains reverse direction");
    setup(4,DIR_PINGPONG,+1);
    n=next_pos(&s,TRK_BASS0,31,0);
    CHECK(n==3,"look-ahead of shortened part must stay in bounds");
    CHECK(s.t[TRK_BASS0].pp_dir==+1,"look-ahead never commits a direction flip");
    setup(4,DIR_PINGPONG,+1);
    CHECK(next_pos(&s,TRK_BASS0,2,1)==3,"unmodified ping-pong interior forward");
    CHECK(next_pos(&s,TRK_BASS0,3,1)==2 && s.t[TRK_BASS0].pp_dir==-1,"normal right edge reversal");
    CHECK(next_pos(&s,TRK_BASS0,0,1)==1 && s.t[TRK_BASS0].pp_dir==+1,"normal left edge reversal");
    setup(1,DIR_PINGPONG,+1);
    CHECK(next_pos(&s,TRK_BASS0,15,1)==0,"shrink to one step never emits out-of-range step");
    setup(8,DIR_PINGPONG,+1);
    CHECK(next_pos(&s,TRK_BASS0,3,1)==4,"length expansion retains normal progression");
    setup(4,DIR_REV,-1);
    n=next_pos(&s,TRK_BASS0,3,1);
    CHECK(n==2,"reverse mode unaffected");
    setup(4,DIR_FWD,+1);
    CHECK(next_pos(&s,TRK_BASS0,3,1)==0,"forward mode wraps unaffected");
    setup(4,DIR_RANDOM,+1);
    CHECK(next_pos(&s,TRK_BASS0,2,0)==3,"random mode look-ahead unaffected");
    for(int i=0;i<32;i++)CHECK(next_pos(&s,TRK_BASS0,2,1)<4,"random mode stays within bounds");
    /* Exhaust every position of every shortened pattern, both directions. */
    for(int old_len=2;old_len<=64;old_len++)
      for(int new_len=1;new_len<=old_len;new_len++)
        for(int p=0;p<old_len;p++)
          for(int dir=-1;dir<=1;dir+=2) {
            setup(new_len,DIR_PINGPONG,dir);
            int next=next_pos(&s,TRK_BASS0,p,1);
            CHECK(next>=0 && next<new_len,"all shortened ping-pong positions remain in pattern");
          }
    printf("ping-pong resize source-window: %d pass, %d fail\n",passed,failed);
    return failed?1:0;
}
'''

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument('--source',type=Path,default=Path('firmware/src/seq/sequencer.c'))
    ap.add_argument('--cc',default=os.environ.get('CC','gcc'))
    ap.add_argument('--sanitize',choices=('none','ubsan','asan-ubsan'),default='none')
    args=ap.parse_args()
    text=args.source.read_text(encoding='utf8')
    if text.count(START)!=1 or text.count(END)!=1 or text.index(END)<text.index(START):
        raise SystemExit('SOURCE WINDOW MISMATCH: next_pos signature/boundary')
    section=text[text.index(START):text.index(END)]
    with tempfile.TemporaryDirectory(prefix='seq-pingpong-') as t:
        c=Path(t)/'test.c'; exe=Path(t)/'test'
        c.write_text(PREFIX+'\n'+section+'\n'+SUFFIX,encoding='utf8')
        flags=['-std=c11','-O1','-g','-Wall','-Wextra','-Werror','-Wno-unused-function']
        if args.sanitize=='ubsan':flags+=['-fsanitize=undefined','-fno-sanitize-recover=all']
        if args.sanitize=='asan-ubsan':flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all']
        command=[args.cc,*flags,str(c),'-o',str(exe)]
        print('compile:', ' '.join(command),flush=True)
        subprocess.run(command,check=True)
        completed=subprocess.run([str(exe)],check=False)
        raise SystemExit(completed.returncode)

if __name__=='__main__':main()
