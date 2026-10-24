#!/usr/bin/env python3
"""BF-027: exercise engine's actual motion-tick call-site with real motion.c."""
import argparse
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

P = argparse.ArgumentParser()
P.add_argument('root', type=Path, nargs='?', default=Path(__file__).resolve().parents[2])
args = P.parse_args()
root = args.root.resolve()
engine = (root / 'firmware/src/app/engine.c').read_text()
matches = re.findall(r'if\s*\(\s*mot_on(?:\s*&&\s*seq\.playing)?(?:\s*&&\s*!seq_clock_waiting\s*\(\s*&seq\s*\))?\s*\)\s*motion_tick\s*\(\s*&mot\s*,\s*n\s*\)\s*;', engine)
if len(matches) != 1:
    raise SystemExit(f'Expected one engine motion_tick guard; found {len(matches)}')
guard = matches[0]
harness = r'''
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "motion.h"
static motion_t mot;
static int mot_on;
static struct { int playing; } seq;
static int seq_clock_waiting(const void *s) { (void)s; return 0; }  /* idle test has no external clock */
static int applied;
static int base(void *ctx,int t,int v,int i) { (void)ctx;(void)t;(void)v;(void)i; return 0; }
static void apply(void *ctx,int t,int v,int i,int val) {
    (void)ctx;(void)t;(void)v;(void)i; applied=val;
}
static void app_block(uint32_t n) {
    GUARD
}
static int errors;
#define CHECK(x, msg) do { if (!(x)) { fprintf(stderr,"FAIL %s (line %d)\n",msg,__LINE__); errors++; } } while(0)
int main(void) {
    static lane_t lanes[NLANE];
    motion_init(&mot,lanes);
    mot.base=base; mot.apply=apply;
    int slot=motion_alloc(lanes,0,0,1,0,0);
    CHECK(slot == 0,"create automation lane");
    lanes[slot].val[0]=30;
    lanes[slot].val[1]=100;
    mot_on=1;
    seq.playing=0;
    app_block(500);
    CHECK(mot.age[0] == 0,"idle blocks do not measure a musical step");
    seq.playing=1;
    motion_step(&mot,0,0,0,1,16);
    CHECK(applied == 30,"first p-lock applied on playback start");
    CHECK(mot.len[0][0] == 0,"first step did not inherit stopped-time interval");
    app_block(200);
    CHECK(applied == 30,"first step ramp waits for actual step-duration evidence");
    CHECK(mot.age[0] == 200,"running clock increments motion age");
    motion_step(&mot,0,0,1,2,16);
    CHECK(applied == 100,"next p-lock fires on time");
    CHECK(mot.len[0][0] == 200,"actual step measures 200 samples");
    motion_release(&mot);
    CHECK(applied == 0,"stop restores base");
    seq.playing=0;
    app_block(1000);
    CHECK(mot.age[0] == 0,"stopped interval does not contaminate next start");
    printf("BF027: %d failures\n",errors);
    return errors ? 1 : 0;
}
'''.replace('GUARD',guard)
compiler = os.environ.get('CC','cc')
flags = os.environ.get('CFLAGS','-O2 -Wall -Wextra -Werror').split()
with tempfile.TemporaryDirectory(prefix='bf027-') as temp:
    source=Path(temp)/'case.c'
    exe=Path(temp)/'case'
    source.write_text(harness)
    cmd=[compiler,*flags,'-I'+str(root/'firmware/src/seq'),str(source),str(root/'firmware/src/seq/motion.c'),'-o',str(exe)]
    built=subprocess.run(cmd,text=True,capture_output=True)
    if built.returncode:
        print(built.stdout+built.stderr, file=sys.stderr)
        raise SystemExit(f'{compiler} compile exit {built.returncode}')
    run=subprocess.run([str(exe)],text=True,capture_output=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
    sys.stdout.write(run.stdout)
    sys.stderr.write(run.stderr)
    sys.exit(run.returncode)
