#!/usr/bin/env python3
"""Native SMC BLE link-epoch queue regression (source-extracted, no keyboard).

Pass SMC_SOURCE=/path/to/smc_mixer.c for an exact checkout, and CC/SAN_FLAGS
for toolchain/sanitizers. The harness extracts unmodified production functions.
It models the TIMER5 producer and the main-loop consumer synchronously; it does
not establish preemption safety or firmware/device correctness.
"""
import os
from pathlib import Path
import re
import shlex
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
SOURCE = Path(os.getenv("SMC_SOURCE", ROOT / "firmware/src/app/smc_mixer.c"))
source = SOURCE.read_text(encoding="utf-8")


def function(name):
    match = re.search(r"^[^\n]*\b" + re.escape(name) + r"\([^\n]*\)\s*\{", source, re.M)
    if not match:
        raise RuntimeError(f"missing source function: {name}")
    depth, pos = 1, match.end()
    while depth and pos < len(source):
        depth += (source[pos] == "{") - (source[pos] == "}")
        pos += 1
    if depth:
        raise RuntimeError(f"unterminated source function: {name}")
    return source[match.start():pos]


prefix = source[source.index("#define SMC_Q"):source.index("int smc_project_active")]
helpers = ("smc_solo_union", "smc_peer_drop", "smc_ble_midi_slot", "smc_ble_link_slot") if "smc_poll_peer" in source else ()
functions = "\n".join(function(n) for n in helpers + (
    "smc_project_active", "smc_solo_clear", "smc_project_drop", "smc_ble_midi", "smc_ble_link",
    "smc_mmc_drain_stop",
) + (("smc_poll_peer",) if helpers else ()) + ("smc_poll",))
harness = r'''
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "smc_fx_harness_stubs.h"
int ui_pad_diag_active(void) { return 0; } /* independent nonmodal regression */
#define X0X_COLD
#define PROJ_BT_SMC_CENTRAL 16u
#define PROJ_BT_SMC_USER 32u
enum { NKIT = 2, O_NONE, O_ASK };
typedef struct { uint8_t kind, a, b, c; } pref_t;
static struct { struct { uint8_t bt; } set; } proj;
static struct { uint8_t overlay; } ui;
static uint8_t observed[64];
static unsigned nobs, mmc_count;
static int busy;
static int perf_testing(void) { return 0; }
static int ui_project_busy(void) { return busy; }
static void engine_smc_solo(int mask) { (void)mask; }
static void engine_smc_solo_targets(uint8_t p,uint16_t a,uint16_t b) { (void)a; (void)b; engine_smc_solo(p); }
static void ui_say(const char *s, int unused) { (void)s; (void)unused; }
static uint8_t smc_message(const uint8_t *m, uint8_t len, int allow)
{
    if (!allow || len != 3u || nobs >= 64u) abort();
    observed[nobs++] = m[1];
    return 0;
}
''' + prefix + "\n" + functions + r'''
static void reset_state(unsigned start)
{
    memset(&smc, 0, sizeof smc);
    memset(observed, 0, sizeof observed);
    nobs = mmc_count = 0u;
    busy = 0;
    proj.set.bt = PROJ_BT_SMC_CENTRAL | PROJ_BT_SMC_USER;
    ui.overlay = O_NONE;
    smc_qr = smc_qw = smc_host_r = smc_host_w = (uint8_t)start;
    for (int i = 0; i < 2; ++i) smc_mmc_r[i] = smc_mmc_w[i] = (uint8_t)start;
    smc_link_published = smc_link_seen = 0u;
}
static void send_ble(uint8_t marker)
{
    const uint8_t msg[3] = {0x90u, marker, 0x7Fu};
    smc_ble_midi(msg, 3);
}
static void send_host(uint8_t marker)
{
    uint8_t w = smc_host_w;
    (void)marker; /* retired User backlog has no payload storage */
    smc_host_w = (uint8_t)(w + 1u);
}
static int check(const uint8_t *expected, size_t n, unsigned test)
{
    if (nobs != n || memcmp(observed, expected, n)) {
        fprintf(stderr, "FAIL case=%u got=%u expected=%zu:", test, nobs, n);
        for (unsigned i = 0; i < nobs; i++) fprintf(stderr," %u",observed[i]);
        fputc('\n',stderr);
        return 1;
    }
    if (smc_qr != smc_qw || smc_host_r != smc_host_w) {
        fprintf(stderr,"FAIL case=%u unfinished queue\n",test);
        return 2;
    }
    return 0;
}
int main(int argc, char **argv)
{
    if (argc != 3) return 99;
    unsigned t = (unsigned)strtoul(argv[1], NULL, 10);
    unsigned start = (unsigned)strtoul(argv[2], NULL, 10);
    reset_state(start);
    uint8_t want[64] = {0};
    unsigned nw = 0;
    switch(t) {
    case 0:  /* The first connected packet must NOT be dropped. */
        smc_ble_link(1); send_ble(9); smc_poll(); want[nw++] = 9; break;
    case 1:  /* Normal in-session dispatch. */
        smc_ble_link(1); smc_poll(); send_ble(3); send_ble(4); smc_poll();
        want[nw++] = 3; want[nw++] = 4; break;
    case 2:  /* Stale press must not run after disconnect/reconnect coalesces. */
        smc_ble_link(1); smc_poll(); send_ble(7);
        smc_ble_link(0); smc_ble_link(1); send_ble(9); smc_poll();
        want[nw++] = 9; break;
    case 3: /* Packets from a closing link and a new link must be separated. */
        smc_ble_link(1); smc_poll(); send_ble(7);
        smc_ble_link(0); send_ble(8); smc_ble_link(1); send_ble(9); smc_poll();
        want[nw++] = 9; break;
    case 4:  /* Ordinary explicit link-down poll still drops stale events. */
        smc_ble_link(1); smc_poll(); send_ble(7); smc_ble_link(0);
        smc_poll(); smc_ble_link(1); send_ble(9); smc_poll();
        want[nw++] = 9; break;
    case 5:  /* A boundary already consumed may never rewind the read cursor. */
        smc_ble_link(1); smc_poll(); send_ble(7); smc_ble_link(0);
        smc_ble_link(1); send_ble(9); smc_qr = (uint8_t)(smc_qr + 1u);
        smc_poll(); want[nw++] = 9; break;
    case 6:  /* A reset may also advance read cursor beyond link watermark. */
        smc_ble_link(1); smc_poll(); send_ble(7); smc_ble_link(0);
        smc_ble_link(1); smc_project_drop(); send_ble(9); smc_poll();
        want[nw++] = 9; break;
    case 7:  /* Saturated prior-session ring must not replay on reconnect. */
        smc_ble_link(1); smc_poll();
        for(unsigned i=0;i<SMC_Q;i++) send_ble((uint8_t)(i+1u));
        smc_ble_link(0); smc_ble_link(1); smc_poll();
        send_ble(9); smc_poll(); want[nw++] = 9; break;
    case 8: /* New session events survive multiple coalesced link edges. */
        smc_ble_link(1); smc_poll(); send_ble(7);
        smc_ble_link(0); smc_ble_link(1); smc_ble_link(0);
        smc_ble_link(1); send_ble(9); send_ble(10); smc_poll();
        want[nw++] = 9; want[nw++] = 10; break;
    case 9: /* Retired SMC User backlog is discarded; ordinary USB routing is separate. */
        smc_ble_link(1); smc_poll(); send_ble(7); send_host(11);
        smc_ble_link(0); smc_ble_link(1); send_ble(9); smc_poll();
        want[nw++] = 9; break;
    case 10: /* Project busy drains prior and current inputs. */
        smc_ble_link(1); smc_poll(); send_ble(7);
        smc_ble_link(0); smc_ble_link(1); busy=1; smc_poll(); busy=0;
        send_ble(9); smc_poll(); want[nw++] = 9; break;
    case 11: /* Stale Note Off/STOP cannot escape across reconnect. */
        smc_ble_link(1); smc_poll(); send_ble(0x5Du); send_ble(8);
        smc_ble_link(0); smc_ble_link(1); send_ble(9); smc_poll();
        want[nw++] = 9; break;
    default: return 98;
    }
    return check(want,nw,t);
}
'''

with tempfile.TemporaryDirectory(prefix="smc-link-epoch-") as tmp:
    path = Path(tmp) / "test.c"
    exe = Path(tmp) / "test"
    path.write_text(harness, encoding="utf-8")
    args = [os.getenv("CC", "gcc"), "-std=c99", "-O1", "-g", "-Wall",
            "-Wextra", "-Werror", *shlex.split(os.getenv("SAN_FLAGS", "")),
            "-I" + str(ROOT / "firmware/src"),
            "-I" + str(ROOT / "tests/host"),
            str(path), "-o", str(exe)]
    subprocess.run(args, check=True)
    total = failures = 0
    for start in (0, 240, 254, 255):
        for case in range(12):
            total += 1
            result = subprocess.run([str(exe), str(case), str(start)], check=False,
                                    capture_output=True, text=True)
            if result.returncode:
                failures += 1
                print(result.stderr or result.stdout or f"FAIL {case}/{start}: exit {result.returncode}",
                      file=sys.stderr, end="\n")
    if failures:
        sys.exit(f"FAIL {failures}/{total} BLE link-epoch cases on {SOURCE}")
    print(f"PASS {total}/{total} BLE link-epoch cases on {SOURCE}")
