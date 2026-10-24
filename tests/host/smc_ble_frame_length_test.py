#!/usr/bin/env python3
"""Reject partial/extended direct BLE control frames (exact smc_message C)."""
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
source_path = Path(os.environ.get('SMC_SOURCE', root / 'firmware/src/app/smc_mixer.c'))
source = source_path.read_text()
function = re.search(r'^static uint8_t smc_message\([^\n]*\)\n\{.*?^\}', source, re.M | re.S)
if not function:
    raise SystemExit('smc_message not found in ' + str(source_path))

prefix = r'''
#include <stdint.h>
#include <stdio.h>
#define PROJ_BT_SMC_USER 32u
static struct { struct { uint8_t bt; } set; } proj;
static unsigned buttons, releases, faders, knobs, mmcs, guarded;
static int testing;
static int perf_testing(void) { return testing; }
static uint8_t smc_mmc_note(uint8_t v) { return v == 34u ? 0x5Du : 0xFFu; }
static int smc_relative(uint8_t v) { return v == 1u ? 1 : 0; }
static void smc_guarded_button(uint8_t n, int mmc, int allow)
{ (void)n; (void)allow; ++guarded; if (mmc) ++mmcs; }
static void smc_fader(int n, uint8_t v) { (void)n; (void)v; ++faders; }
static void smc_knob(int n, int d) { (void)n; (void)d; ++knobs; }
static void smc_button(uint8_t n, int mmc)
{ (void)n; (void)mmc; ++buttons; }
static void smc_release(uint8_t n) { (void)n; ++releases; }
'''
suffix = r'''
#define CHECK(cond) do { if (!(cond)) { \
 fprintf(stderr, "FAIL frame length line %d: %s\n", __LINE__, #cond); return 1; \
 } ++checks; } while (0)
int main(void)
{
 unsigned checks=0;
 const uint8_t on[7] = {0x9F,8,127,0,0,0,0};
 const uint8_t off[7] = {0x8F,8,0,0,0,0,0};
 const uint8_t cc[7] = {0xBF,40,77,0,0,0,0};
 const uint8_t pitch[7] = {0xE0,2,4,0,0,0,0};
 const uint8_t daw_on[3] = {0x90,8,127};
 const uint8_t daw_off[3] = {0x80,8,0};
 const uint8_t daw_knob[3] = {0xB0,16,1};
 const uint8_t foreign_on[3] = {0x91,8,127};
 const uint8_t foreign_knob[3] = {0xB1,16,1};
 const uint8_t invalid_status[3] = {0xF8,8,127};
 const uint8_t invalid_data[3] = {0x90,0x88,127};
 const uint8_t mmc[6] = {0xF0,0x7F,0x7F,0x06,34,0xF7};
 proj.set.bt = PROJ_BT_SMC_USER;
 CHECK(smc_message(on,3,1)==0 && buttons==1);
 CHECK(smc_message(off,3,1)==0 && releases==1);
 CHECK(smc_message(cc,3,1)==0 && faders==1);
 CHECK(smc_message(mmc,6,1)==0 && mmcs==1);
 CHECK(smc_message(on,2,1)==4);
 for (unsigned len=4;len<=7;len++) {
  CHECK(smc_message(on,(uint8_t)len,1)==4);
  CHECK(smc_message(off,(uint8_t)len,1)==4);
  CHECK(smc_message(cc,(uint8_t)len,1)==4);
 }
 CHECK(buttons==1 && releases==1 && faders==1 && mmcs==1);
 CHECK(smc_message(on,4,0)==4 && guarded==1);
 testing=1;
 CHECK(smc_message(on,5,1)==4 && guarded==1);
 testing=0;
 proj.set.bt=0;
 CHECK(smc_message(pitch,3,1)==0 && faders==2);
 CHECK(smc_message(pitch,4,1)==4 && faders==2);
 CHECK(smc_message(mmc,6,1)==4 && mmcs==1);
 /* With User preset disabled, DAW controls (channel 1) still work, but
  * channel-16 User keys and unrelated MIDI channels must not move controls.
  * Test the real production dispatch, not only the protocol helper. */
 CHECK(smc_message(on,3,1)==4 && buttons==1);
 CHECK(smc_message(off,3,1)==4 && releases==1);
 CHECK(smc_message(cc,3,1)==4 && faders==2);
 CHECK(smc_message(foreign_on,3,1)==4 && buttons==1);
 CHECK(smc_message(foreign_knob,3,1)==4 && knobs==0);
 CHECK(smc_message(invalid_status,3,1)==4 && buttons==1);
 CHECK(smc_message(invalid_data,3,1)==4 && buttons==1);
 CHECK(smc_message(daw_on,3,1)==0 && buttons==2);
 CHECK(smc_message(daw_off,3,1)==0 && releases==2);
 CHECK(smc_message(daw_knob,3,1)==0 && knobs==1);
 CHECK(smc_message(pitch,3,1)==0 && faders==3);
 CHECK(buttons==2 && releases==2 && knobs==1 && faders==3 && guarded==1);
 printf("SMC BLE exact-frame length: %u/%u PASS\n",checks,checks);
 return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='smc-ble-frame-') as directory:
    c = Path(directory) / 'test.c'
    exe = Path(directory) / 'test'
    c.write_text(prefix + function.group(0) + suffix)
    command = [os.environ.get('CC', 'cc'), '-std=c99', '-O1', '-g', '-Wall',
               '-Wextra', '-Werror', *shlex.split(os.environ.get('SAN_FLAGS','')),
               str(c), '-o', str(exe)]
    subprocess.run(command, check=True)
    subprocess.run([str(exe)], check=True)
