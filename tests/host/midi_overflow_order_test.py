#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Exercise real USB/BLE queue code: ordered retention, wrap and IRQ boundaries.

The optional source root permits the same regression to run against a pristine
baseline. No production source is changed. Interrupt injection adds observation
points to extracted statements, and uses the actual source-mask calls to defer
the simulated TIMER5 callback. This models CPU0 scheduling, not device timing.
"""
from pathlib import Path
import argparse
import os
import re
import subprocess
import tempfile


def function(source, name):
    match = re.search(r"^[^\n]*\b" + re.escape(name) + r"\([^\n]*\)[^\n]*\n\{", source, re.M)
    if not match:
        raise ValueError("Missing " + name)
    depth, end = 1, match.end()
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[match.start():end]


parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--source-root", type=Path, default=Path(__file__).resolve().parents[2])
parser.add_argument("--sanitize", action="store_true")
args = parser.parse_args()
usb = (args.source_root / "firmware/src/usb.c").read_text()
ble = (args.source_root / "firmware/src/ble.c").read_text()
plat = (args.source_root / "firmware/src/app/plat_fm1.c").read_text()
mq = int(re.search(r"^#define MQ (\d+)u", usb, re.M).group(1))
boq = int(re.search(r"^#define BLE_OQ (\d+)u", ble, re.M).group(1))
ring = usb[usb.index("/* MIDI rings: 4-byte USB-MIDI event packets */"):
           usb.index("static void midi_out_event(uint32_t pkt)")]
ring += function(usb, "midi_out_event") + "\n" + function(plat, "plat_midi_in")
ring += "\n" + "\n".join(function(ble, name) for name in (
    "ble_out_is_note_off", "ble_out_is_rt_transport", "ble_out_pkt_is_keep", "ble_out"))
# Observe the actual slot and cursor accesses, including every shifted slot.
# Do not insert inside unbraced loops: the comma expression remains one statement.
ring = re.sub(r"(\b(?:q|ble_oq)\[[^;\n]+?\]\s*=\s*[^;\n]+);",
              r"\1, boundary();", ring)
ring = ring.replace("*pkt = midi_in_q[mi_r % MQ];",
                    "*pkt = midi_in_q[mi_r % MQ]; boundary();")
ring = ring.replace("uint32_t p = q[i % MQ];", "uint32_t p = q[i % MQ]; boundary();")
ring = ring.replace("uint32_t p = ble_oq[i % BLE_OQ];", "uint32_t p = ble_oq[i % BLE_OQ]; boundary();")

preamble = r"""
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define FM1_IRQ_TIMER5 63u
static unsigned enabled = 1, global_enabled = 1, masks, restores;
static unsigned observations, trigger, pending, fired, in_callback;
static void (*timer_callback)(void);
static void service_pending(void) {
    if (pending && enabled && global_enabled && !in_callback) {
        pending = 0; fired++; in_callback = 1;
        timer_callback();
        in_callback = 0;
    }
}
static void boundary(void) {
    if (!in_callback && trigger && ++observations == trigger) pending = 1;
    service_pending();
}
static inline uint32_t fm1_irq_mask_save(unsigned irq) {
    if (irq != FM1_IRQ_TIMER5) { puts("wrong IRQ source"); return 0; }
    unsigned old = enabled; enabled = 0; masks++; return old;
}
static inline void fm1_irq_mask_restore(unsigned irq, uint32_t old) {
    if (irq != FM1_IRQ_TIMER5) { puts("wrong IRQ source"); return; }
    enabled = old; restores++; service_pending();
}
#define RING_PUBLISH() boundary()
static struct { unsigned config; } usb = {1};
static uint32_t ble_oq[BLE_OQ];
static struct { volatile uint32_t ow, orr; union { int h; } host; } ble_r;
static int ble_central_role, ble_host_on = 1, ble_want = 1;
static int smc_ble_role_get(void) { return 0; }
static int bleh_notifying(void *h) { (void)h; return 1; }
"""

checks = r"""
static unsigned checks, failures;
static void check(int ok, const char *name) {
    checks++;
    if (!ok && failures++ < 16) printf("FAIL %s\n", name);
}
static uint32_t pkt(unsigned st, unsigned a, unsigned b) {
    unsigned cin = st >= 0xf8 ? 15 : st >> 4;
    return cin | (st << 8) | (a << 16) | (b << 24);
}
static unsigned capacity(unsigned path) { return path == 2 ? BLE_OQ : MQ; }
static uint32_t *queue(unsigned path) {
    return path == 0 ? midi_in_q : path == 1 ? midi_out_q : ble_oq;
}
static volatile uint32_t *rd(unsigned path) {
    return path == 0 ? &mi_r : path == 1 ? &mo_r : &ble_r.orr;
}
static volatile uint32_t *wr(unsigned path) {
    return path == 0 ? &mi_w : path == 1 ? &mo_w : &ble_r.ow;
}
static void put(unsigned path, uint32_t p) {
    if (path == 0) (void)midi_in_put(p);
    else if (path == 1) midi_out_event(p);
    else ble_out(p);
}
static uint32_t pop(unsigned path) {
    uint32_t p = 0;
    if (path == 0) check(plat_midi_in(&p), "ingress pop exists");
    else { p = queue(path)[*rd(path) % capacity(path)]; (*rd(path))++; }
    return p;
}
static void reset(unsigned path, uint32_t start) {
    *rd(path) = *wr(path) = start;
    trigger = pending = fired = observations = in_callback = 0;
    enabled = global_enabled = 1;
    masks = restores = 0;
}
static void verify(unsigned path, const uint32_t *expected, unsigned count, const char *name) {
    check(*wr(path) - *rd(path) == count, "queue depth");
    for (unsigned i = 0; i < count; i++) check(pop(path) == expected[i], name);
    check(masks == restores, "balanced source masks");
    check(enabled == 1 && global_enabled == 1, "IRQ state retained");
}
static void order_cases(void) {
    const uint32_t arrivals[] = {0x003c8008u, 0x003c9009u, 0x0000fa0fu, 0x0000fb0fu, 0x0000fc0fu};
    const uint32_t starts[] = {0u, 43u, UINT32_MAX - 20u};
    for (unsigned path = 0; path < 3; path++) {
        unsigned n = capacity(path);
        for (unsigned wrap = 0; wrap < 3; wrap++) {
            for (unsigned prior = 0; prior < n; prior++) {
                for (unsigned a = 0; a < sizeof arrivals / sizeof arrivals[0]; a++) {
                    uint32_t before[128], expected[128];
                    reset(path, starts[wrap]);
                    for (unsigned i = 0; i < n; i++) {
                        before[i] = i == prior ? pkt(a < 2 ? 0x90 : a == 4 ? 0xfa : 0xfc, a < 2 ? 60 : 0, a < 2 ? 100 : 0) : pkt(0xb0, 1, i);
                        put(path, before[i]);
                    }
                    unsigned victim = prior == 0 ? 1 : 0, out = 0;
                    for (unsigned i = 0; i < n; i++) if (i != victim) expected[out++] = before[i];
                    expected[out++] = arrivals[a];
                    put(path, arrivals[a]);
                    verify(path, expected, out, "protected arrival remains after earlier events");
                }
            }
        }
    }
}
static void policy_cases(void) {
    for (unsigned path = 0; path < 3; path++) {
        unsigned n = capacity(path);
        uint32_t before[128], expected[128], off = pkt(0x80, 60, 0);
        /* Prefer a CC over an earlier Note On; then remove the first ordinary
         * event if there is no CC. Existing critical events retain their order. */
        for (unsigned cc = 0; cc < 2; cc++) {
            reset(path, 51);
            for (unsigned i = 0; i < n; i++) {
                before[i] = i == 0 ? pkt(0x90, 61, 100) : cc && i == 10 ? pkt(0xb0, 1, 99) : pkt(0x80, i, 0);
                put(path, before[i]);
            }
            unsigned victim = cc ? 10 : 0, out = 0;
            for (unsigned i = 0; i < n; i++) if (i != victim) expected[out++] = before[i];
            expected[out++] = off; put(path, off);
            verify(path, expected, out, "priority and retained critical order");
        }
        for (unsigned incoming = 0; incoming < 3; incoming++) {
            reset(path, 27);
            for (unsigned i = 0; i < n; i++) {
                before[i] = pkt(0x80, i, 0); put(path, before[i]);
            }
            put(path, incoming == 0 ? off : incoming == 1 ? pkt(0xb0, 1, 1) : pkt(0xf8, 0, 0));
            verify(path, before, n, "full protected queue is unchanged");
        }
        /* Every path restores a previously masked source, without enabling
         * global interrupts. Include both ordinary and saturated enqueue. */
        for (unsigned full = 0; full < 2; full++) {
            reset(path, 0);
            for (unsigned i = 0; full && i < n; i++) put(path, pkt(0xb0, 1, i));
            enabled = global_enabled = 0;
            put(path, off);
            check(!enabled && !global_enabled && masks == restores, "disabled caller state retained");
        }
    }
    reset(0, 0);
    uint32_t p = 0x12345678u;
    enabled = global_enabled = 0;
    check(!plat_midi_in(&p) && p == 0x12345678u, "empty ingress unchanged");
    check(!enabled && !global_enabled && masks == restores, "empty ingress restores source state");
}
static unsigned callback_path;
static uint32_t emitted, incoming;
static void consume_timer(void) { emitted = pop(callback_path); }
static void produce_timer(void) { put(0, incoming); }
static void interrupt_cases(void) {
    for (unsigned path = 1; path < 3; path++) {
        unsigned n = capacity(path);
        for (unsigned point = 1; point <= n + 2; point++) {
            uint32_t expected[128];
            reset(path, 43);
            for (unsigned i = 0; i < n; i++) {
                uint32_t p = i == 0 ? pkt(0xb0, 1, 99) : pkt(0x90, i, 100);
                put(path, p);
                if (i) expected[i - 1] = p;
            }
            incoming = pkt(0x80, 60, 0); expected[n - 1] = incoming;
            callback_path = path; timer_callback = consume_timer;
            observations = 0; trigger = point;
            put(path, incoming);
            trigger = 0;
            check(fired == 1, "pending TIMER5 consumed after producer transaction");
            check(emitted == expected[0], "TIMER5 never sees a partially shifted ring");
            verify(path, expected + 1, n - 1, "producer interrupt preserves retained sequence");
        }
    }
    /* TIMER5 arrives between the real ingress slot read and cursor advance.
     * Without source masking it compacts that slot and the resumed read skips
     * the next note. With masking, the freed slot admits the release normally. */
    reset(0, 43);
    uint32_t expected[MQ], first;
    for (unsigned i = 0; i < MQ; i++) {
        uint32_t p = i == 0 ? pkt(0xb0, 1, 99) : pkt(0x90, i, 100);
        put(0, p); if (i) expected[i - 1] = p;
    }
    incoming = pkt(0x80, 60, 0); expected[MQ - 1] = incoming;
    timer_callback = produce_timer; observations = 0; trigger = 1;
    check(plat_midi_in(&first) && first == pkt(0xb0, 1, 99), "ingress reads original head");
    trigger = 0;
    check(fired == 1, "pending ingress producer executes after consume");
    verify(0, expected, MQ, "consumer interrupt does not skip a retained note");
}
static void rollover_case(void) {
    uint32_t expected[128];
    reset(2, UINT32_MAX - 20u);
    for (unsigned i = 0; i < BLE_OQ; i++) { expected[i] = pkt(0xb0, 1, i); put(2, expected[i]); }
    verify(2, expected, BLE_OQ, "BLE uint32 rollover preserves every packet");
}
int main(void) {
    rollover_case(); order_cases(); policy_cases(); interrupt_cases();
    printf("midi-overflow-order: %u checks, %u failures (MQ=%u BLE_OQ=%u)\n", checks, failures, MQ, BLE_OQ);
    return failures != 0;
}
"""

with tempfile.TemporaryDirectory(prefix="x0x-midi-order-") as tmp:
    c = Path(tmp) / "test.c"
    exe = Path(tmp) / "test"
    c.write_text(f"#define BLE_OQ {boq}u\n" + preamble + ring + checks)
    flags = ["-std=c99", "-O1", "-g", "-Wall", "-Wextra", "-Werror"]
    if args.sanitize:
        flags += ["-fsanitize=address,undefined", "-fno-sanitize-recover=all"]
    subprocess.run([os.environ.get("CC", "cc"), *flags, str(c), "-o", str(exe)], check=True)
    raise SystemExit(subprocess.call([str(exe)]))
