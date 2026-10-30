#!/usr/bin/env python3
"""Interrupt production SMC consumers when they release a full queue slot.

The injected callback models the ISR producer running immediately after the
consumer publishes its read index. Queue code and producers come from the
selected production source; MIDI/UI actions alone are recorded substitutes.
SMC_SOURCE selects an unchanged base for the negative control.
"""
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
SOURCE = Path(os.environ.get("SMC_SOURCE", ROOT / "firmware/src/app/smc_mixer.c"))
source = SOURCE.read_text()


def function(name):
    match = re.search(r"^[^\n]*\b" + name + r"\([^\n]*\)\s*\{", source, re.M)
    if not match:
        raise RuntimeError(f"Production function not found: {name}")
    depth, end = 1, match.end()
    while depth:
        if end == len(source):
            raise RuntimeError(f"Unterminated production function: {name}")
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[match.start():end]


prefix = source[source.index("#define SMC_Q"):source.index("int smc_project_active")]
poll = function("smc_poll_peer" if "smc_poll_peer" in source else "smc_poll")
for index, name in enumerate(("smc_qr", "smc_host_r", "smc_mmc_r[src]")):
    statement = re.escape(name) + r" = \(uint8_t\)\(r \+ 1u\);"
    poll, matches = re.subn(statement, lambda m: m[0] + f" interleave({index});", poll)
    if matches != 1:
        raise RuntimeError(f"Expected one production publication for {name}, got {matches}")

harness = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "app/smc_protocol.h"
#include "smc_fx_harness_stubs.h"
#define X0X_COLD
#define PROJ_BT_SMC_CENTRAL 16u
#define PROJ_BT_SMC_USER 32u
typedef struct { uint8_t kind, a, b, c; } pref_t;
enum { O_NONE, O_ASK, NKIT = 2, NPARTS = 5 };
static struct { struct { uint8_t bt; } set; } proj;
static struct { uint8_t overlay, help; } ui;
static int locked, testing, inject, route, mmc_src, reset_on_first;
static unsigned count;
static uint8_t observed[64][7];
static int perf_testing(void) { return testing; }
static int ui_project_busy(void) { return locked; }
static void engine_smc_solo(int n) { (void)n; }
static void engine_smc_solo_targets(uint8_t p,uint16_t a,uint16_t b) { (void)a; (void)b; engine_smc_solo(p); }
static void ui_say(const char *a, const char *b) { (void)a; (void)b; }
''' + prefix + "\n" + "\n".join(function(name) for name in
    (name for name in ("smc_solo_union", "smc_peer_drop", "smc_ble_midi_slot", "smc_project_active", "smc_solo_clear") if re.search(r"\b" + name + r"\(", source))) + "\n" + "\n".join(function(name) for name in (
    "smc_project_drop", "smc_ble_midi", "smc_midi_host", "smc_mmc_host")) + r'''

static void record(const uint8_t *m, uint8_t n)
{
    assert(count < 64 && n <= 6);
    observed[count][0] = n;
    memcpy(observed[count] + 1, m, n);
    count++;
    if (reset_on_first && count == 1) smc_project_drop();
}
static uint8_t smc_message(const uint8_t *m, uint8_t n, int allow)
{
    assert(allow);
    record(m, n);
    return 0;
}
static void smc_guarded_button(uint8_t note, int mmc, int allow)
{
    assert(mmc && allow);
    record(&note, 1);
}
static void enqueue(unsigned i)
{
    if (route == 0) {
        /* Mixed lengths expose a torn length/payload as well as lost messages. */
        const uint8_t msg[6] = {0xF0, 0x7F, 0x7F, 6, (uint8_t)(i + 1), 0xF7};
        if ((i & 1) || i == SMC_Q) {
            const uint8_t cc[3] = {0xB0, 16, (uint8_t)(i + 1)};
            smc_ble_midi(cc, 3);
        } else smc_ble_midi(msg, 6);
    } else if (route == 1) {
        uint32_t pkt = 0x0Bu | (0xBFu << 8) | (16u << 16) | ((i + 1u) << 24);
        assert(smc_midi_host(pkt));
    } else smc_mmc_host((uint8_t)(i + 1), (uint8_t)mmc_src);
}
static void interleave(int queue)
{
    if (!inject || queue != (route == 4 ? 2 : route)) return;
    inject = 0;
    if (route == 4) {
        smc_mmc_host(33, (uint8_t)mmc_src); /* ISR refills the released slot */
        smc_mmc_host(34, (uint8_t)mmc_src); /* STOP overflows after PLAY */
    } else enqueue(route == 2 ? 16 : SMC_Q);
}
''' + function("smc_mmc_drain_stop") + "\n" + function("smc_spill_apply") + "\n" + poll + ("\n" + function("smc_poll") if "smc_poll_peer" in source else "") + r'''

int main(int argc, char **argv)
{
    assert(argc == 5);
    route = atoi(argv[1]); mmc_src = atoi(argv[2]);
    unsigned start = (unsigned)atoi(argv[3]);
    reset_on_first = atoi(argv[4]);
    assert(route >= 0 && route <= 4 && mmc_src >= 0 && mmc_src < 2);
    smc_qr = smc_qw = smc_host_r = smc_host_w = (uint8_t)start;
    for (int i = 0; i < 2; i++) smc_mmc_r[i] = smc_mmc_w[i] = (uint8_t)start;
    smc.up = 1;
    proj.set.bt = PROJ_BT_SMC_CENTRAL | PROJ_BT_SMC_USER;
    if (route == 4) {
        /* ISR fills a freed slot with PLAY, then STOP overflows. The extra
         * PLAY predates STOP and must not be replayed after it next poll. */
        for (unsigned i = 0; i < 16; ++i) smc_mmc_host(33, (uint8_t)mmc_src);
        inject = 1;
        smc_poll();
        assert(!inject && count == 16u);
        for (unsigned i = 0; i < 16; ++i)
            assert(observed[i][0] == 1 && observed[i][1] == 0x5Eu);
        smc_mmc_host(33, (uint8_t)mmc_src); /* later PLAY belongs after STOP */
        smc_poll();
        assert(count == 19u);
        assert(observed[16][0] == 1 && observed[16][1] == 0x5Eu);
        assert(observed[17][0] == 1 && observed[17][1] == 0x5Du);
        assert(observed[18][0] == 1 && observed[18][1] == 0x5Eu);
        smc_poll();
        assert(count == 19u); /* STOP acknowledged only once */
        return 0;
    }
    if (route == 3) {
        /* First sixteen queued PLAY commands occupy the whole source ring.
         * The seventeenth STOP must run last, never be silently lost. */
        for (unsigned i = 0; i < 16; ++i) smc_mmc_host(33, (uint8_t)mmc_src);
        smc_mmc_host(34, (uint8_t)mmc_src);
        if (reset_on_first) {
            smc_project_drop(); /* a discarded project must not inherit STOP */
            smc_poll();
            assert(count == 0);
            return 0;
        }
        smc_poll();
        assert(count == 17);
        for (unsigned i = 0; i < 16; ++i)
            assert(observed[i][0] == 1 && observed[i][1] == 0x5E);
        assert(observed[16][0] == 1 && observed[16][1] == 0x5D);
        smc_poll();
        assert(count == 17); /* the lost STOP is acknowledged only once */
        return 0;
    }
    /* Only legitimate seven-bit payload bytes may become SMC controls.
     * Before this guard an invalid data1 0x88 was silently decoded as
     * Solo 1 (0x08), and invalid CC values were still accepted. */
    if (route == 1) {
        const uint32_t base = 0x0Bu | (0xBFu << 8);
        const uint32_t note = 0x09u | (0x9Fu << 8);
        uint8_t before = smc_host_w;
        assert(!smc_midi_host(base | (0x90u << 16) | (1u << 24)));
        assert(!smc_midi_host(base | (16u << 16) | (0x81u << 24)));
        assert(!smc_midi_host(note | (0x88u << 16) | (127u << 24)));
        assert(!smc_midi_host(note | (8u << 16) | (0xFFu << 24)));
        assert(smc_host_w == before);
        proj.set.bt &= (uint8_t)~PROJ_BT_SMC_USER;
        assert(!smc_midi_host(base | (16u << 16) | (1u << 24)));
        assert(smc_host_w == before);
        proj.set.bt |= PROJ_BT_SMC_USER;
    }
    unsigned capacity = route == 2 ? 16 : SMC_Q;
    for (unsigned i = 0; i < capacity; i++) enqueue(i);
    inject = 1;
    smc_poll();
    smc_poll();
    assert(!inject);
    unsigned expected_count = reset_on_first ? 1 : capacity + 1;
    assert(count == expected_count);
    for (unsigned i = 0; i < count; i++) {
        uint8_t expected[7] = {0};
        if (route == 0 && !(i & 1) && i != SMC_Q) {
            const uint8_t mmc[7] = {6, 0xF0, 0x7F, 0x7F, 6, (uint8_t)(i + 1), 0xF7};
            memcpy(expected, mmc, sizeof mmc);
        } else if (route == 2) {
            expected[0] = 1; expected[1] = smc_mmc_note((uint8_t)(i + 1));
        } else {
            expected[0] = 3; expected[1] = route == 0 ? 0xB0 : 0xBF;
            expected[2] = 16; expected[3] = (uint8_t)(i + 1);
        }
        if (memcmp(observed[i], expected, sizeof expected)) {
            fprintf(stderr, "FAIL: route=%d source=%d start=%u reset=%d event=%u: "
                    "queue slot was overwritten before dispatch\n",
                    route, mmc_src, start, reset_on_first, i);
            return 1;
        }
    }
    assert(smc_qr == smc_qw && smc_host_r == smc_host_w);
    for (int i = 0; i < 2; i++) assert(smc_mmc_r[i] == smc_mmc_w[i]);
    return 0;
}
'''

with tempfile.TemporaryDirectory(prefix="smc-queue-") as tmp:
    c = Path(tmp) / "test.c"
    executable = Path(tmp) / "test"
    c.write_text(harness)
    flags = shlex.split(os.environ.get("SAN_FLAGS", ""))
    subprocess.run([os.environ.get("CC", "cc"), "-std=c99", "-O2", "-g",
                    "-Wall", "-Wextra", "-Werror", *flags,
                    "-I" + str(ROOT / "firmware/src"),
                    "-I" + str(ROOT / "tests/host"),
                    str(c), "-o", str(executable)], check=True)
    failures = []
    cases = 0
    for route, src in ((0, 0), (1, 0), (2, 0), (2, 1), (3, 0), (3, 1), (4, 0), (4, 1)):
        for start in (0, 240, 255):
            for reset in (0, 1):
                if route == 4 and reset: continue
                args = [str(route), str(src), str(start), str(reset)]
                result = subprocess.run([str(executable), *args], check=False)
                cases += 1
                if result.returncode:
                    failures.append((args, result.returncode))
    if failures:
        raise SystemExit(f"FAIL: {len(failures)}/{cases} SMC queue interleavings: {failures}")
print(f"PASS: {cases} full-queue ISR interleavings, MMC STOP priority, wrap and reset discard")
