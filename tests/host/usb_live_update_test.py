#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""L1 source-backed USB request regression. Authored without execution.

Breaks caught: accepting OTA/UBOOT during any live-state input; accidentally
rejecting stopped requests; retaining a refused request as an editor/OTA frame;
gating unrelated SysEx; importing application engine symbols into the loader.

Compiles the complete production sysex_byte region, its production preprocessor
conditions, real usb struct, real OTA receive declarations, and production MIDI
ring declarations/predicates/writer. Only the IRQ mask, engine note/transport
state, and downstream inquiry/MMC notification boundaries are doubles. This
does not emulate USB endpoint hardware or run the sequencer.

The tests use usb.ota_refused from the actual production struct, never a fixture
field. No red/green result is claimed: execution is delegated to the orchestrator.
"""
import argparse
import os
from pathlib import Path
import shlex
import subprocess
import tempfile


def unique_index(source, marker):
    if source.count(marker) != 1:
        raise SystemExit(f"expected exactly one extraction marker: {marker!r}")
    return source.index(marker)


def between(source, begin, end):
    lo = unique_index(source, begin)
    hi = unique_index(source, end)
    if hi <= lo:
        raise SystemExit(f"reversed extraction markers: {begin!r}, {end!r}")
    return source[lo:hi]


# A narrow boundary double, deliberately unavailable to the loader. The volatile
# transport flag types match seq_t; seq_t layout and engine behavior are not under
# test. Loader includes or references to these application symbols must fail.
ENGINE_BOUNDARY = r'''
#ifndef USB_LIVE_UPDATE_ENGINE_BOUNDARY_H
#define USB_LIVE_UPDATE_ENGINE_BOUNDARY_H
#ifdef FELUCCA_LOADER
#error USB loader must not import the application engine/transport boundary
#endif
#include <stdint.h>
typedef struct {
    volatile uint8_t playing;
    volatile uint8_t req_start, req_stop, req_cont;
} seq_t;
extern seq_t seq;
int engine_project_notes(void);
#endif
'''

HARNESS = r'''
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define RING_PUBLISH() ((void)0)
#ifndef FELUCCA_LOADER
enum { FM1_IRQ_TIMER5 = 5 };
static uint32_t fm1_irq_mask_save(unsigned irq) { (void)irq; return 0; }
static void fm1_irq_mask_restore(unsigned irq, uint32_t state)
{ (void)irq; (void)state; }
#endif
#if BOARD_SMK37 && !defined(FELUCCA_LOADER)
#include "engine_boundary.h"
seq_t seq;
static int note_probe;
int engine_project_notes(void) { return note_probe; }
#endif
/* These are the external delivery boundaries of the real SysEx parser. */
static unsigned inquiries, introductions, host_seen, mmc_calls;
static uint8_t inquiry_device, mmc_command, mmc_source;
void apc40_device_inquiry(uint8_t device)
{ inquiries++; inquiry_device = device; }
void apc40_introduction_reply(void) { introductions++; }
void smk_apc_host_seen(void) { host_seen++; }
void smk_apc_host_generic(uint8_t ble) { (void)ble; }
void smc_mmc_host(uint8_t command, uint8_t source)
{ mmc_calls++; mmc_command = command; mmc_source = source; }
'''

CASES = r'''
static const uint8_t uboot[] = {0xF0, 0x22, 0x24, 0x35, 0x7D, 0xF7};
static const uint8_t ota[] = {0xF0, 0x22, 0x24, 0x35, 0x7F, 0xF7};
static const uint8_t inquiry[] = {0xF0, 0x7E, 0x7F, 0x06, 0x01, 0xF7};
static const uint8_t intro[] = {0xF0, 0x47, 0x7F, 0x29, 0x60, 0, 4, 0x41, 0, 0, 0, 0xF7};
#if FELUCCA_OTA
/* Actual editor PING envelope: F0, private 7D 46 4C, command 25, F7. */
static const uint8_t editor_ping[] = {0xF0, 0x7D, 0x46, 0x4C, 25, 0xF7};
#endif
static int failures;
static void check(const char *label, int condition)
{
    if (!condition) {
        printf("FAIL %s [uboot=%u ota=%u]\n", label,
               (unsigned)usb.uboot_req, (unsigned)usb.ota_req);
        failures++;
    }
}
static void send_bytes(const uint8_t *p, size_t n)
{ for (size_t i = 0; i < n; i++) sysex_byte(p[i]); }
static void reset_case(void)
{
    memset(&usb, 0, sizeof usb);
    memset(midi_in_q, 0, sizeof midi_in_q);
    mi_w = mi_r = 0;
    inquiries = introductions = host_seen = mmc_calls = 0;
    inquiry_device = mmc_command = mmc_source = 0;
#if BOARD_SMK37 && !defined(FELUCCA_LOADER)
    memset(&seq, 0, sizeof seq);
    note_probe = 0;
#endif
#if FELUCCA_OTA
    memset(sx_frame, 0, sizeof sx_frame);
    sx_pos = sx_frame_len = 0;
    sx_ready = sx_collect = sx_busy = 0;
#endif
}
static int no_request(void) { return !usb.uboot_req && !usb.ota_req; }
static int no_receive_frame(void)
{
#if FELUCCA_OTA
    return !sx_ready && !sx_collect;
#else
    return 1;
#endif
}
static int exact_request(int want_ota)
{ return want_ota ? (usb.ota_req == 1 && !usb.uboot_req)
                  : (usb.uboot_req == 1 && !usb.ota_req); }

/* These fixtures reach the real production ring writer before the real parser,
 * matching ep1_rx order when one endpoint packet contains MIDI then SysEx. */
static const uint32_t raw_harmless[] = {
    0x0000FC0Fu, /* Stop */
    0x0000F80Fu, /* clock */
    0x7F408308u, /* NoteOff, channel 3 */
    0x00409309u, /* velocity-zero NoteOn, channel 3 */
    0x7F07B00Bu, /* CC volume */
};
static const uint32_t raw_live[] = {
    0x0000FA0Fu, /* Start */
    0x0000FB0Fu, /* Continue */
    0x01409309u, /* NoteOn, channel 3, minimum nonzero velocity */
    0x7F409F09u, /* NoteOn, channel 15, maximum velocity */
};
static void fill_raw_ring(uint32_t base, int live_slot, uint32_t live_packet)
{
    mi_r = mi_w = base;
    for (unsigned i = 0; i < MQ; i++)
        check("production ring writer accepts each event within capacity",
              midi_in_put((int)i == live_slot ? live_packet :
                  raw_harmless[i % (sizeof raw_harmless / sizeof raw_harmless[0])]));
}
static void raw_pending_cases(const uint8_t *command, size_t n, int want_ota)
{
    const uint32_t bases[] = {0u, MQ - 2u, UINT32_MAX - 1u};
    const unsigned positions[] = {0u, MQ / 2u, MQ - 1u};
    uint32_t saved[MQ];
    for (unsigned b = 0; b < sizeof bases / sizeof bases[0]; b++) {
        reset_case();
        fill_raw_ring(bases[b], -1, 0u);
        memcpy(saved, midi_in_q, sizeof saved);
        send_bytes(command, n);
        check("Stop/NoteOff/velocity-zero/clock/CC alone allow stopped update",
              exact_request(want_ota) && no_receive_frame());
        check("admitting harmless pending MIDI preserves every raw event and index",
              mi_r == bases[b] && mi_w == bases[b] + MQ &&
              !memcmp(saved, midi_in_q, sizeof saved));
#if BOARD_SMK37 && !defined(FELUCCA_LOADER)
        check("harmless pending MIDI does not count as a refusal", !usb.ota_refused);
#endif
        for (unsigned p = 0; p < sizeof positions / sizeof positions[0]; p++) {
            for (unsigned k = 0; k < sizeof raw_live / sizeof raw_live[0]; k++) {
                reset_case();
                fill_raw_ring(bases[b], (int)positions[p], raw_live[k]);
                memcpy(saved, midi_in_q, sizeof saved);
                send_bytes(command, n);
#if BOARD_SMK37 && !defined(FELUCCA_LOADER)
                check("accepted undrained Start/Continue/NoteOn refuses an otherwise idle update",
                      no_request() && no_receive_frame() && usb.ota_refused == 1u &&
                      !seq.playing && !seq.req_start && !seq.req_cont && !note_probe);
                send_bytes(command, n);
                check("one refusal per complete repeated key with raw intent pending",
                      no_request() && usb.ota_refused == 2u);
#else
                check("loader update admission remains independent of raw application MIDI intent",
                      exact_request(want_ota) && no_receive_frame());
#endif
                check("admission never consumes/reorders pending raw MIDI, including index rollover",
                      mi_r == bases[b] && mi_w == bases[b] + MQ &&
                      !memcmp(saved, midi_in_q, sizeof saved));
#if BOARD_SMK37 && !defined(FELUCCA_LOADER)
                /* Model the consumer advancing its cursor. Stale live bytes in
                 * consumed slots must not gate a fresh fully stopped request. */
                mi_r = mi_w;
                sysex_byte(0xF7);
                check("draining raw MIDI cannot resurrect a refused update", no_request());
                send_bytes(command, n);
                check("explicit replay after raw MIDI drains succeeds despite stale live slots",
                      exact_request(want_ota) && no_receive_frame() && usb.ota_refused == 2u);
#endif
            }
        }
    }
}

static void raw_full_ring_cases(const uint8_t *command, size_t n, int want_ota)
{
    uint32_t saved[MQ];
    reset_case();
    fill_raw_ring(UINT32_MAX - 1u, -1, 0u);
    memcpy(saved, midi_in_q, sizeof saved);
    check("full raw ring rejects a new unprotected nonzero NoteOn", !midi_in_put(raw_live[2]));
    send_bytes(command, n);
    check("an unaccepted NoteOn cannot block a stopped update",
          exact_request(want_ota) && no_receive_frame() &&
          mi_r == UINT32_MAX - 1u && mi_w == 62u && !memcmp(saved, midi_in_q, sizeof saved));
#if BOARD_SMK37 && !defined(FELUCCA_LOADER)
    check("dropped NoteOn is not counted as an update refusal", !usb.ota_refused);
#endif
    for (unsigned k = 0; k < 2u; k++) {
        reset_case();
        mi_r = mi_w = UINT32_MAX - 1u;
        for (unsigned i = 0; i < MQ; i++)
            check("CC fills the production ring", midi_in_put(0x7F07B00Bu));
        check("protected Start/Continue displaces a CC even in a full ring", midi_in_put(raw_live[k]));
        memcpy(saved, midi_in_q, sizeof saved);
        send_bytes(command, n);
#if BOARD_SMK37 && !defined(FELUCCA_LOADER)
        check("new protected transport at the full ring tail gates the update",
              no_request() && no_receive_frame() && usb.ota_refused == 1u);
#else
        check("loader admission ignores even protected application transport",
              exact_request(want_ota) && no_receive_frame());
#endif
        check("update admission preserves the ring writer's accepted FIFO order",
              mi_r == UINT32_MAX - 1u && mi_w == 62u && !memcmp(saved, midi_in_q, sizeof saved));
    }
}

#if BOARD_SMK37 && !defined(FELUCCA_LOADER)
static void raw_completion_cases(const uint8_t *command, size_t n, int want_ota)
{
    for (unsigned k = 0; k < sizeof raw_live / sizeof raw_live[0]; k++) {
        reset_case();
        mi_r = mi_w = UINT32_MAX;
        send_bytes(command, n - 1u);
        check("raw live intent accepted between update key bytes", midi_in_put(raw_live[k]));
        sysex_byte(0xF7);
        check("new raw intent at the key terminator blocks update once",
              no_request() && no_receive_frame() && usb.ota_refused == 1u &&
              mi_r == UINT32_MAX && mi_w == 0u);

        reset_case();
        mi_r = mi_w = UINT32_MAX;
        check("raw live intent initially accepted", midi_in_put(raw_live[k]));
        check("harmless Stop accepted behind live intent", midi_in_put(0x0000FC0Fu));
        send_bytes(command, n - 1u);
        mi_r++; /* only the live event has been drained; Stop remains queued */
        sysex_byte(0xF7);
        check("consumed live slot cannot block a key completed with only harmless traffic pending",
              exact_request(want_ota) && no_receive_frame() && !usb.ota_refused &&
              mi_r == 0u && mi_w == 1u && midi_in_q[0] == 0x0000FC0Fu);
    }
}

/* Held and queued are two scenarios for the same external nonzero note probe;
 * engine_project_notes itself is intentionally outside this USB parser test. */
static void set_live(int state)
{
    seq.playing = state == 1;
    seq.req_start = state == 2;
    seq.req_cont = state == 3;
    note_probe = state == 4 ? 1 : state == 5 ? 2 : 0;
}
static void refused_request(const char *kind, const uint8_t *command, size_t n,
                            int want_ota, int state, const char *state_name)
{
    char label[160];
    reset_case();
    set_live(state);
    send_bytes(command, n);
    snprintf(label, sizeof label, "%s ignored with %s alone", kind, state_name);
    check(label, no_request());
    snprintf(label, sizeof label, "%s refusal is not routed as an OTA/editor frame", kind);
    check(label, no_receive_frame() && !usb.sx_on);
    check("one denied command increments the real refusal counter once", usb.ota_refused == 1u);
    check("refusal preserves pending transport flags",
          seq.req_start == (state == 2) && seq.req_cont == (state == 3) && !seq.req_stop);
    /* No polling helper here: USB has no deferred update consumer. A stop plus
     * unrelated traffic may not resurrect a refused command in request/frame state. */
    set_live(0);
    sysex_byte(0xF7); /* stray terminator after refusal */
    send_bytes(inquiry, sizeof inquiry);
    snprintf(label, sizeof label, "%s remains refused after stop and unrelated traffic", kind);
    check(label, no_request() && inquiries == 1u);
#if FELUCCA_OTA
    check("stopped inquiry transports its own frame, not the refused command",
          sx_ready == 1 && sx_frame_len == 4u &&
          !memcmp(sx_frame, (const uint8_t[]){0x7E, 0x7F, 0x06, 0x01}, 4u));
    /* Simulate frame consumer release; no production update flag is touched. */
    sx_ready = 0;
#endif
    check("stop/inquiry do not count another refusal", usb.ota_refused == 1u);
    send_bytes(command, n);
    snprintf(label, sizeof label, "%s fresh replay accepted after fully stopping", kind);
    check(label, exact_request(want_ota));
    check("accepted replay does not count a refusal", usb.ota_refused == 1u);
}
static void live_cases(void)
{
    const char *states[] = {"stopped", "playing", "req_start", "req_cont", "held-note probe", "queued-note probe"};
    for (int state = 1; state <= 5; state++) {
        refused_request("UBOOT", uboot, sizeof uboot, 0, state, states[state]);
#if FELUCCA_OTA
        refused_request("OTA", ota, sizeof ota, 1, state, states[state]);
#endif
    }
}
static void completion_and_pending_cases(const uint8_t *command, size_t n, int want_ota)
{
    for (int state = 1; state <= 5; state++) {
        reset_case();
        send_bytes(command, n - 1u);
        check("an incomplete request has no request or refusal side effects",
              no_request() && !usb.ota_refused);
        set_live(state);
        sysex_byte(0xF7);
        check("admission observes live state at completion, including held/queued notes",
              no_request() && no_receive_frame() && usb.ota_refused == 1u);

        reset_case(); set_live(state);
        send_bytes(command, n - 1u);
        set_live(0);
        sysex_byte(0xF7);
        check("a request completed after stopping is admitted without refusal",
              exact_request(want_ota) && no_receive_frame() && !usb.ota_refused);

        for (unsigned pending = 0; pending <= 3u; pending++) {
            reset_case(); set_live(state);
            seq.req_stop = 1;
            usb.uboot_req = pending & 1u;
            usb.ota_req = pending >> 1;
            send_bytes(command, n);
            check("denial preserves preexisting update requests and transport requests",
                  usb.uboot_req == (pending & 1u) && usb.ota_req == (pending >> 1) &&
                  seq.req_start == (state == 2) && seq.req_cont == (state == 3) && seq.req_stop &&
                  usb.ota_refused == 1u && no_receive_frame());
            send_bytes(command, n);
            check("each complete denied command is counted once, including repeated requests",
                  usb.ota_refused == 2u);
        }
    }
}
static void unrelated_live_cases(void)
{
    for (int state = 1; state <= 5; state++) {
        reset_case(); set_live(state);
        send_bytes(inquiry, sizeof inquiry);
        check("valid device inquiry still reaches its reply boundary while live",
              inquiries == 1u && inquiry_device == 0x7Fu && host_seen == 1u &&
              no_request() && !usb.ota_refused);
        reset_case(); set_live(state);
        send_bytes(intro, sizeof intro);
        check("valid APC introduction still reaches its reply boundary while live",
              introductions == 1u && host_seen == 1u && no_request() && !usb.ota_refused);
        for (unsigned command = 1; command <= 43u; command++) {
            uint8_t mmc[] = {0xF0, 0x7F, 0x7F, 0x06, (uint8_t)command, 0xF7};
            reset_case(); set_live(state);
            send_bytes(mmc, sizeof mmc);
            check("valid MMC transport/control remains routed while live",
                  mmc_calls == 1u && mmc_command == command && mmc_source == 0u &&
                  no_request() && !usb.ota_refused);
        }
#if FELUCCA_OTA
        reset_case(); set_live(state);
        send_bytes(editor_ping, sizeof editor_ping);
        check("editor PING still reaches the shared frame transport while live",
              sx_ready == 1u && sx_frame_len == 4u &&
              !memcmp(sx_frame, (const uint8_t[]){0x7D, 0x46, 0x4C, 25}, 4u) && no_request());
#else
        reset_case(); set_live(state);
        send_bytes(ota, sizeof ota);
        check("OTA-disabled live application ignores the unsupported OTA request",
              no_request() && !usb.ota_refused);
#endif
        check("editor/inquiry/MMC traffic does not count update refusals", usb.ota_refused == 0u);
    }
}
#if FELUCCA_OTA
static void pending_frame_case(const uint8_t *command, size_t n)
{
    reset_case();
    send_bytes(editor_ping, sizeof editor_ping);
    seq.playing = 1;
    send_bytes(command, n);
    check("refusing an update preserves an unrelated pending editor frame",
          no_request() && usb.ota_refused == 1u && sx_ready == 1u && !sx_collect &&
          sx_frame_len == 4u &&
          !memcmp(sx_frame, (const uint8_t[]){0x7D, 0x46, 0x4C, 25}, 4u));
}
#endif
#endif

static void stopped_and_loader_cases(void)
{
#if !FELUCCA_OTA
    reset_case(); send_bytes(ota, sizeof ota);
    check("OTA-disabled application/loader cannot acquire an OTA request", no_request());
#endif
    reset_case(); send_bytes(uboot, sizeof uboot);
    check("fully stopped UBOOT request accepted exactly once", exact_request(0) && no_receive_frame());
#if BOARD_SMK37 && !defined(FELUCCA_LOADER)
    check("accepted stopped UBOOT is not counted as refused", usb.ota_refused == 0u);
#endif
#if FELUCCA_OTA
    reset_case(); send_bytes(ota, sizeof ota);
    check("fully stopped OTA request accepted exactly once", exact_request(1) && no_receive_frame());
#if BOARD_SMK37 && !defined(FELUCCA_LOADER)
    check("accepted stopped OTA is not counted as refused", usb.ota_refused == 0u);
#endif
#endif
    reset_case(); send_bytes(inquiry, sizeof inquiry);
    check("inquiry remains routed in application and engine-free loader",
          inquiries == 1u && inquiry_device == 0x7Fu && no_request());
    reset_case(); send_bytes(intro, sizeof intro);
    check("APC introduction remains routed in application and engine-free loader",
          introductions == 1u && no_request());
#if FELUCCA_OTA
    reset_case(); send_bytes(editor_ping, sizeof editor_ping);
    check("frame transport remains available in engine-free loader",
          sx_ready == 1u && sx_frame_len == 4u &&
          !memcmp(sx_frame, (const uint8_t[]){0x7D, 0x46, 0x4C, 25}, 4u) && no_request());
#endif
#if BOARD_SMK37 && !defined(FELUCCA_LOADER)
    reset_case(); seq.req_stop = 1; send_bytes(uboot, sizeof uboot);
    check("req_stop alone does not block an otherwise stopped UBOOT request", exact_request(0));
#if FELUCCA_OTA
    reset_case(); seq.req_stop = 1; send_bytes(ota, sizeof ota);
    check("req_stop alone does not block an otherwise stopped OTA request", exact_request(1));
#endif
#endif
}
int main(void)
{
    raw_pending_cases(uboot, sizeof uboot, 0);
    raw_full_ring_cases(uboot, sizeof uboot, 0);
#if FELUCCA_OTA
    raw_pending_cases(ota, sizeof ota, 1);
    raw_full_ring_cases(ota, sizeof ota, 1);
#endif
#if BOARD_SMK37 && !defined(FELUCCA_LOADER)
    raw_completion_cases(uboot, sizeof uboot, 0);
    live_cases();
    completion_and_pending_cases(uboot, sizeof uboot, 0);
#if FELUCCA_OTA
    raw_completion_cases(ota, sizeof ota, 1);
    completion_and_pending_cases(ota, sizeof ota, 1);
    pending_frame_case(uboot, sizeof uboot);
    pending_frame_case(ota, sizeof ota);
#endif
    unrelated_live_cases();
#endif
    stopped_and_loader_cases();
    printf("USB LIVE UPDATE %s (%d failures)\n", failures ? "FAIL" : "PASS", failures);
    return failures != 0;
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[2],
                        help="source tree root containing firmware/src/usb.c")
    args = parser.parse_args()
    source = (args.root / "firmware/src/usb.c").read_text()
    # Preserve whole production ranges without replacing branches or rewriting
    # the function body. Missing/ambiguous boundaries are harness errors.
    prefix = between(source, "#ifndef FELUCCA_CDC\n", "enum { S_FADDR =")
    declarations = between(source,
        "static struct {\n    uint8_t up, config, pend_addr, has_pend_addr, e0_tx, e0_zlp;",
        "/* MIDI rings: 4-byte USB-MIDI event packets */")
    ingress = between(source, "/* MIDI rings: 4-byte USB-MIDI event packets */",
                      "static void midi_out_event(uint32_t pkt)")
    assembly = between(source, "/* SysEx assembly: only short commands matter here */",
        "\n#ifndef FELUCCA_LOADER\n/* MID-15, BLE-16:")
    if assembly.count("static void sysex_byte(uint8_t b)") != 1:
        raise SystemExit("expected one intact production sysex_byte definition")
    fixture = HARNESS + prefix + declarations + ingress + assembly + CASES
    failures = 0
    with tempfile.TemporaryDirectory(prefix="x0x-usb-live-update-") as td:
        out = Path(td)
        (out / "engine_boundary.h").write_text(ENGINE_BOUNDARY)
        # Preserve production include statements: headers resolve to the narrow
        # boundary double instead of importing DSP/platform dependencies.
        (out / "app").mkdir()
        (out / "seq").mkdir()
        (out / "app/engine.h").write_text('#include "../engine_boundary.h"\n')
        (out / "seq/sequencer.h").write_text('#include "../engine_boundary.h"\n')
        cpath = out / "test.c"
        cpath.write_text(fixture)
        variants = [("application-ota", 1, False), ("application-no-ota", 0, False),
                    ("loader-ota", 1, True), ("loader-no-ota", 0, True)]
        for name, ota, loader in variants:
            print(f"=== {name} ===", flush=True)
            exe = out / name
            argv = shlex.split(os.environ.get("CC", "cc")) + [
                "-std=c99", "-O2", "-Wall", "-Wextra", "-Werror", "-Wno-unused-variable",
                "-DBOARD_SMK37=1", f"-DFELUCCA_OTA={ota}", "-I", str(out),
                str(cpath), "-o", str(exe)]
            if loader:
                argv.append("-DFELUCCA_LOADER=1")
            compiled = subprocess.run(argv, check=False)
            if compiled.returncode:
                print(f"HARNESS/COMPILE ERROR: {name}; this is not a behavioral RED", flush=True)
                failures += 1
                continue
            failures += subprocess.run([str(exe)], check=False).returncode != 0
    return int(failures != 0)


if __name__ == "__main__":
    raise SystemExit(main())
