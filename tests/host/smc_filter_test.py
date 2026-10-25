#!/usr/bin/env python3
"""Check production SMC home routing and master mute feedback independently."""
import os
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
source = Path(os.environ.get("SMC_SOURCE", root / "firmware/src/app/smc_mixer.c")).read_text()
functions = []
for name in ("smc_home_target", "smc_drum_mix", "smc_drum_track", "smc_lock_blocks", "smc_led"):
    match = re.search(r"^static [^\n]+ " + name + r"\([^\n]*\)\n\{.*?^\}", source, re.M | re.S)
    if name in ("smc_drum_mix", "smc_drum_track") and not match:
        continue
    assert match, name
    functions.append(match[0])
harness = r'''
#include <assert.h>
#include <stdint.h>
enum { R_ENG, T_MIX, T_FX, T_MST, MX_LEVEL, FX_RV_LEVEL, FX_DL_LEVEL,
       FX_VOLUME, MST_DJF, V_FX, V_MIX, V_HOME, V_GLO, V_PART, V_SEQ, O_NONE, NDRUM = 11, NPARTS = 5 };
typedef struct { int kind, a, b, c; } pref_t;
#define PR(k,a,b,c) ((pref_t){k,a,b,c})
static struct { int rec, part, view, overlay, help, sel[2]; } ui;
static struct { int rec_part, solo,locked,lock_kit,lock_bank; uint16_t drum_solo[2]; } smc;
static struct { int playing; } seq;
static int volume, filter;
static int pref_get(pref_t r) { return r.a == T_MST ? filter : volume; }
static int part_muted(int p) { (void)p; return 0; }
#define track_muted(k,t) ((void)(k),(void)(t),0)
static int smk_clip_mode_on(void) { return 0; }
#define is_drum() (ui.part < 2)
static int drum_mix_page(void) { return -1; }
static int cur_page(void) { return 0; }
'''
harness += "\n".join(functions)
harness += r'''
int main(void) {
    for (int i = 0; i < 5; i++) {
        pref_t r = smc_home_target(i);
        assert(r.a == T_MIX && r.b == i && r.c == MX_LEVEL);
    }
    assert(smc_home_target(5).c == FX_RV_LEVEL);
    assert(smc_home_target(6).c == FX_DL_LEVEL);
    pref_t r = smc_home_target(7);
    assert(r.a == T_MST && r.c == MST_DJF);
    volume = 100; filter = 0;
    assert(!smc_led(23)); /* Filter endpoint must not light the master mute. */
    volume = 0; filter = 64;
    assert(smc_led(23));
    return 0;
}
'''
with tempfile.TemporaryDirectory() as tmp:
    c = Path(tmp) / "test.c"
    exe = Path(tmp) / "test"
    c.write_text(harness)
    subprocess.run([os.environ.get("CC", "cc"), "-std=c99", "-Wall", "-Wextra", "-Werror", str(c), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print("PASS: SMC filter destination and independent master mute light")
