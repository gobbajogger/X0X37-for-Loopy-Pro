#!/usr/bin/env python3
"""SMC direct BLE MIDI must reject high-bit channel data without aliasing a control.

Extract the actual smc_message() body from the selected firmware file. The C
harness replaces UI/DSP effects with counters, never a simulated device pass.
SMC_SOURCE can select an exact historical source for a red regression.
"""
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
source_path = Path(os.environ.get("SMC_SOURCE", root / "firmware/src/app/smc_mixer.c"))
source = source_path.read_text()
match = re.search(r"^static uint8_t smc_message\([^\n]*\)\n\{.*?^\}", source, re.M | re.S)
if not match:
    raise SystemExit("NO-GO: exact smc_message() function not present in " + str(source_path))

prefix = r'''
#include <stdint.h>
#include <stdio.h>
#define PROJ_BT_SMC_USER 1u
static struct { struct { uint8_t bt; } set; } proj;
static unsigned buttons, releases, faders, knobs, mmcs, checks, failures;
static unsigned last_note, last_fader, last_value;
static int perf_testing(void) { return 0; }
static uint8_t smc_mmc_note(uint8_t v) { return v == 16u ? 8u : 0xFFu; }
static void smc_guarded_button(uint8_t n, int mmc, int allow)
{ if (mmc && allow) { ++mmcs; last_note = n; } }
static void smc_fader(int n, uint8_t v)
{ ++faders; last_fader = (unsigned)n; last_value = v; }
static int smc_relative(uint8_t v) { return v == 1u ? 1 : v == 65u ? -1 : 0; }
static void smc_knob(int n, int d) { (void)n; (void)d; ++knobs; }
static void smc_button(uint8_t n, int mmc)
{ (void)mmc; ++buttons; last_note = n; }
static void smc_release(uint8_t n) { ++releases; last_note = n; }
#define CHECK(expr) do { ++checks; if (!(expr)) { ++failures; \
    fprintf(stderr,"FAIL line %d: %s\\n",__LINE__,#expr); } } while (0)
'''

suffix = r'''
int main(void)
{
    const uint8_t valid_press[3] = {0x9Fu, 8u, 127u};
    const uint8_t bad_press_note[3] = {0x9Fu, 0x88u, 127u};
    const uint8_t bad_press_vel[3] = {0x9Fu, 8u, 0xFFu};
    const uint8_t bad_release_note[3] = {0x8Fu, 0x88u, 0u};
    const uint8_t bad_release_vel[3] = {0x8Fu, 8u, 0xFFu};
    const uint8_t valid_release[3] = {0x8Fu, 8u, 0u};
    const uint8_t valid_cc[3] = {0xBFu, 40u, 77u};
    const uint8_t bad_cc_no[3] = {0xBFu, 0xA8u, 77u};
    const uint8_t bad_cc_data[3] = {0xBFu, 40u, 0xCDu};
    const uint8_t valid_mmc[6] = {0xF0u, 0x7Fu, 0x7Fu, 6u, 16u, 0xF7u};
    const uint8_t valid_daw[3] = {0xE0u, 2u, 4u};
    const uint8_t bad_daw[3] = {0xE0u, 0x82u, 4u};
    const uint8_t invalid_status[3] = {0xF4u, 8u, 127u};
    proj.set.bt = PROJ_BT_SMC_USER;
    CHECK(smc_message(valid_press, 3u, 1) == 0u);
    CHECK(buttons == 1u && last_note == 8u);
    CHECK(smc_message(bad_press_note, 3u, 1) == 4u);
    CHECK(smc_message(bad_press_vel, 3u, 1) == 4u);
    CHECK(buttons == 1u);
    CHECK(smc_message(bad_release_note, 3u, 1) == 4u);
    CHECK(smc_message(bad_release_vel, 3u, 1) == 4u);
    CHECK(releases == 0u);
    CHECK(smc_message(valid_release, 3u, 1) == 0u);
    CHECK(releases == 1u && last_note == 8u);
    CHECK(smc_message(bad_cc_no, 3u, 1) == 4u);
    CHECK(smc_message(bad_cc_data, 3u, 1) == 4u);
    CHECK(faders == 0u);
    CHECK(smc_message(valid_cc, 3u, 1) == 0u);
    CHECK(faders == 1u && last_fader == 0u && last_value == 77u);
    CHECK(smc_message(valid_mmc, 6u, 1) == 0u);
    CHECK(mmcs == 1u && last_note == 8u);
    CHECK(smc_message(invalid_status, 3u, 1) == 4u);
    CHECK(buttons == 1u && faders == 1u);
    proj.set.bt = 0u;
    CHECK(smc_message(valid_daw, 3u, 1) == 0u);
    CHECK(faders == 2u);
    CHECK(smc_message(bad_daw, 3u, 1) == 4u);
    CHECK(faders == 2u);
    CHECK(smc_message(valid_press, 2u, 1) == 4u);
    CHECK(smc_message(valid_press, 0u, 1) == 4u);
    CHECK(buttons == 1u && mmcs == 1u && releases == 1u && knobs == 0u);
    printf("checks=%u failed=%u\n", checks, failures);
    return failures ? 1 : 0;
}
'''
with tempfile.TemporaryDirectory(prefix="smc-ble-payload-") as d:
    c = Path(d) / "test.c"
    exe = Path(d) / "test"
    c.write_text(prefix + "\n" + match[0] + "\n" + suffix)
    compiler = shlex.split(os.environ.get("CC", "gcc"))
    flags = shlex.split(os.environ.get("CFLAGS", "-std=c11 -O1 -g -Wall -Wextra -Werror"))
    cmd = compiler + flags + [str(c), "-o", str(exe)]
    print("source:", source_path, "compiler:", " ".join(cmd), flush=True)
    subprocess.run(cmd, check=True)
    subprocess.run([str(exe)], check=True)
