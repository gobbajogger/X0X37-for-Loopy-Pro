#!/usr/bin/env python3
"""Native regression: MIDI TRS/invalid sources must bypass host SMC routing.

Uses the exact SMC routing block extracted from firmware/src/app/engine.c and
the actual retired smc_midi_host() body from smc_mixer.c. No physical controller needed.
ENGINE_SOURCE may select a historical pre-fix file for a negative control.
"""
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
engine = Path(os.environ.get("ENGINE_SOURCE", root / "firmware/src/app/engine.c")).read_text()
start = engine.find("extern int smc_midi_host(uint32_t pkt);")
assert start >= 0, "SMC host routing call removed without regression update"
opening = engine.rfind("{", 0, start)
assert opening >= 0 and not engine[opening + 1:start].strip(), "Unexpected SMC routing block"
closing = engine.find("\n    }", start)
assert closing >= 0, "SMC routing block is missing a closing brace"
block = engine[opening:closing + len("\n    }")]
assert block.count("smc_midi_host(pkt)") == 1, "SMC called more than once or routed elsewhere"

source = Path(os.environ.get("SMC_SOURCE", root / "firmware/src/app/smc_mixer.c")).read_text()
m = re.search(r"^int smc_midi_host\(uint32_t pkt\)\n\{.*?^\}", source, re.M | re.S)
assert m, "Missing actual host hook"
actual_hook = m[0].replace("smc_midi_host(", "actual_smc_midi_host(", 1)

harness = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "smc_protocol.h"
#define PLAT_MIDI_SRC(pkt) (((pkt) >> 4) & 3u)
''' + actual_hook + r'''
static int user_mode = 1;
static unsigned smc_calls, instrument_events;
int smc_midi_host(uint32_t pkt) {
    smc_calls++;
    return actual_smc_midi_host(pkt);
}
static void route(uint32_t pkt) {
    int src = (int)PLAT_MIDI_SRC(pkt);
    (void)src;
'''
harness += block + r'''
    instrument_events++;
}
static void check(unsigned src, unsigned status, unsigned data1,
                  int enabled, unsigned expect_smc, unsigned expect_pass) {
    uint32_t pkt = ((uint32_t)src << 4) | 0x0bu |
                   ((uint32_t)status << 8) | ((uint32_t)data1 << 16) |
                   (0x7fu << 24);
    user_mode = enabled;
    smc_calls = instrument_events = 0;
    route(pkt);
    if (smc_calls != expect_smc || instrument_events != expect_pass) {
        fprintf(stderr, "source=%u status=%02x note=%u user=%d: SMC calls=%u (expected %u), passthrough=%u (expected %u)\\n",
                src, status, data1, enabled, smc_calls, expect_smc,
                instrument_events, expect_pass);
        assert(0 && "SMC source isolation violated");
    }
}
int main(void) {
    check(0, 0xbfu, 40, 1, 1, 1); /* USB old User CC stays ordinary instrument MIDI */
    check(1, 0xbfu, 40, 1, 1, 1); /* BLE peripheral old User CC remains unconsumed */
    check(0, 0x9fu, 8, 1, 1, 1);  /* USB User Solo press */
    check(1, 0x8fu, 8, 1, 1, 1);  /* BLE User Solo release */
    check(2, 0xbfu, 40, 1, 0, 1); /* TRS channel 16 must stay instrument MIDI */
    check(2, 0x9fu, 8, 1, 0, 1);  /* TRS must not activate SMC Solo */
    check(3, 0xbfu, 40, 1, 0, 1); /* Unknown source must not route to SMC */
    check(0, 0xb0u, 40, 1, 1, 1); /* USB channel 1 not admitted by User mode */
    check(1, 0xbfu, 40, 0, 1, 1); /* User disabled still bypasses SMC */
    puts("PASS: 9 host MIDI source isolation cases");
    return 0;
}
'''

with tempfile.TemporaryDirectory() as temp:
    path = Path(temp) / "test.c"
    exe = Path(temp) / "test"
    path.write_text(harness)
    cmd = [os.environ.get("CC", "cc"), "-std=c11", "-O1", "-g", "-Wall", "-Wextra", "-Werror"]
    cmd += shlex.split(os.environ.get("SAN_FLAGS", ""))
    cmd += ["-I", str(root / "firmware/src/app"), str(path), "-o", str(exe)]
    subprocess.run(cmd, check=True)
    subprocess.run([str(exe)], check=True)
