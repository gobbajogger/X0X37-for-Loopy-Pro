#!/usr/bin/env python3
"""Exercise production SMC link publication across a TIMER5 interruption.

The real edge notifier, link producer, queue producer, project reset and poller
are compiled together. UI/platform calls schedule an interrupt after the poller
has sampled the link. MIDI actions are recorded at the dispatch boundary.
SMC_SOURCE selects the unchanged source for the negative control.
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
transport = (ROOT / "firmware/src/ble.c").read_text()


def function(text, name):
    match = re.search(r"^[^\n]*\b" + name + r"\([^\n]*\)\s*\{", text, re.M)
    if not match:
        raise RuntimeError(f"Production function not found: {name}")
    depth = 1
    end = match.end()
    while depth:
        if end == len(text):
            raise RuntimeError(f"Unterminated production function: {name}")
        depth += (text[end] == "{") - (text[end] == "}")
        end += 1
    return text[match.start():end]


prefix = source[source.index("#define SMC_Q"):source.index("int smc_project_active")]
has_generation = "smc_link_published" in prefix
harness = r'''
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
static int locked, interrupt_at, interrupt_up;
static unsigned dispatched, notices, solo;
static uint8_t observed[3][3];
static int perf_testing(void);
static int ui_project_busy(void);
static void engine_smc_solo(int n) { solo = (unsigned)n; }
static void engine_smc_solo_targets(uint8_t p,uint16_t a,uint16_t b) { (void)a; (void)b; engine_smc_solo(p); }
static void ui_say(const char *a, const char *b) { (void)a; (void)b; notices++; }
#define smc_ui_cancel() ((void)0) /* Presentation is exercised by smc_ui_test.c. */
''' + prefix + "\n" + "\n".join(function(source, name) for name in
    (name for name in ("smc_solo_union", "smc_peer_drop", "smc_ble_midi_slot", "smc_ble_link_slot", "smc_project_active", "smc_solo_clear") if re.search(r"\b" + name + r"\(", source))) + "\n" + "\n".join(function(source, name) for name in (
    "smc_project_drop", "smc_project_reset", "smc_ble_midi", "smc_ble_link")) + r'''
static uint8_t ble_central_link_seen[2];
''' + function(transport, "ble_central_link_notify_slot") + r'''

static void interrupt(int site)
{
    if (interrupt_at != site) return;
    interrupt_at = 0;
    ble_central_link_notify_slot(0, interrupt_up);
}
static int perf_testing(void) { interrupt(1); return 0; }
static int ui_project_busy(void) { interrupt(2); return locked; }
static uint8_t smc_message(const uint8_t *m, uint8_t n, int allow)
{
    if (!allow || n != 3 || dispatched >= 3) abort();
    memcpy(observed[dispatched++], m, 3);
    return 0;
}
static void smc_guarded_button(uint8_t n, int mmc, int allow)
{ (void)n; (void)mmc; (void)allow; abort(); }
''' + function(source, "smc_mmc_drain_stop") + "\n" + function(source, "smc_spill_apply") + "\n" + ((function(source, "smc_poll_peer") + "\n") if "smc_poll_peer" in source else "") + function(source, "smc_poll") + r'''

static int check_input(int up)
{
    static const uint8_t controls[3][3] = {
        {0xE0, 0, 100}, {0xB0, 16, 1}, {0x90, 0x18, 127}
    };
    /* The transport already reported this state. It does not retransmit it. */
    ble_central_link_notify_slot(0, up);
    for (unsigned i = 0; i < 3; i++) smc_ble_midi(controls[i], 3);
    smc_poll();
    if (smc.up != up || dispatched != (up ? 3u : 0u) ||
        (up && memcmp(controls, observed, sizeof controls)) ||
        (!up && (smc_rx_diag() >> 24) != 2u)) {
        fprintf(stderr, "FAIL: transport=%d application=%u dispatched=%u reason=%u\n",
                up, smc.up, dispatched, (unsigned)(smc_rx_diag() >> 24));
        return 1;
    }
    return 0;
}

int main(int argc, char **argv)
{
    if (argc != 3) return 2;
    int mode = atoi(argv[1]), site = atoi(argv[2]);
    proj.set.bt = PROJ_BT_SMC_CENTRAL;
    if (mode <= 1) {
        /* A newer reconnect/disconnect arrives after the old state was read. */
        int up = mode == 0;
        ble_central_link_notify_slot(0, up); smc_poll();
        ble_central_link_notify_slot(0, !up);
        interrupt_at = site; interrupt_up = up;
        smc_poll(); smc_poll();
        if (interrupt_at) return 3;
        return check_input(up);
    }
    ble_central_link_notify_slot(0, 1); smc_poll();
    smc.picked = 0x55; smc.seen = 0xAA; smc.known[5] = 1;
    if (mode == 2) {
        /* Unchanged connection must not reset pickup or republish notices. */
        unsigned before = notices;
        for (unsigned i = 0; i < 32; i++) smc_poll();
        if (notices != before || smc.picked != 0x55 || smc.seen != 0xAA ||
            !smc.known[5]) return 4;
    } else if (mode == 3) {
        /* Coalesced down/up remains a new link even if the final state is up. */
        ble_central_link_notify_slot(0, 0); ble_central_link_notify_slot(0, 1); smc_poll();
        if (smc.picked || smc.seen || smc.known[5]) return 5;
    } else if (mode == 4) {
        /* A project lock/reset must leave a newer link publication available. */
        locked = 1;
        ble_central_link_notify_slot(0, 0); smc_poll(); smc_project_reset();
        ble_central_link_notify_slot(0, 1); smc_poll();
        locked = 0; smc_poll();
    } else if (mode == 5) {
        smc.solo = 1; solo = 1;
        ble_central_link_notify_slot(0, 0); smc_poll();
        if (smc.solo || solo || smc.picked || smc.seen) return 6;
        return check_input(0);
    } else if (mode == 6) {
#ifdef LINK_GENERATION
        /* Zero is a valid disconnected publication after unsigned wrap. */
        smc_link_published = smc_link_seen = UINT32_MAX;
        ble_central_link_notify_slot(0, 0); smc_poll();
        if (smc_link_published != 0 || smc.up) return 7;
        ble_central_link_notify_slot(0, 1); smc_poll();
#else
        return 77;
#endif
    } else if (mode == 7) {
#ifdef LINK_GENERATION
        ble_central_link_notify_slot(0, 0); smc_poll();
        smc_link_published = smc_link_seen = UINT32_MAX - 1u;
        ble_central_link_notify_slot(0, 1); smc_poll();
        if (smc_link_published != 1u || !smc.up) return 8;
#else
        return 77;
#endif
    } else return 2;
    return check_input(1);
}
'''

cases = [("reconnect", 0, 1), ("reconnect-late", 0, 2),
         ("disconnect", 1, 1), ("disconnect-late", 1, 2),
         ("stable-pickup", 2, 0), ("coalesced-edges", 3, 0),
         ("project-reset", 4, 0), ("disconnect-clears-solo", 5, 0)]
if has_generation:
    cases += [("wrap-down", 6, 0), ("wrap-up", 7, 0)]

with tempfile.TemporaryDirectory(prefix="smc-link-") as tmp:
    c = Path(tmp) / "test.c"
    executable = Path(tmp) / "test"
    c.write_text(harness)
    flags = shlex.split(os.environ.get("SAN_FLAGS", ""))
    if has_generation:
        flags.append("-DLINK_GENERATION")
    subprocess.run([os.environ.get("CC", "cc"), "-std=c99", "-O2", "-g",
                    "-Wall", "-Wextra", "-Werror", *flags,
                    "-I" + str(ROOT / "firmware/src"),
                    "-I" + str(ROOT / "tests/host"),
                    str(c), "-o", str(executable)], check=True)
    failures = []
    for label, mode, site in cases:
        result = subprocess.run([str(executable), str(mode), str(site)], check=False)
        print(f'{"PASS" if result.returncode == 0 else "FAIL"} {label}', flush=True)
        if result.returncode:
            failures.append((label, result.returncode))
    if failures:
        raise SystemExit(f"FAIL: {len(failures)}/{len(cases)} SMC link cases: {failures}")
print(f"PASS: {len(cases)} SMC link cases preserve reconnect, disconnect, pickup, reset and wrap")
