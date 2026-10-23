#!/usr/bin/env python3
"""Native regression: held SMC Solo must release even if an input ring is full.

The queue declarations and functions are extracted from firmware/src/app/smc_mixer.c
and compiled without firmware SDK dependencies.  MIDI/UI effects are mocked; these
are not device or full-firmware tests. SMC_SOURCE selects a negative-control source.
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


def extract(name):
    match = re.search(r"^[^\n]*\b" + re.escape(name) + r"\([^\n]*\)\s*\{", source, re.M)
    if not match:
        raise RuntimeError(f"Function missing: {name}")
    end, depth = match.end(), 1
    while depth:
        if end >= len(source):
            raise RuntimeError(f"Unterminated function: {name}")
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[match.start():end]


prefix = source[source.index("#define SMC_Q"):source.index("int smc_project_active")]
original = "\n".join(extract(n) for n in (
    "smc_project_active", "smc_solo_clear", "smc_solo_publish", "smc_solo_press",
    "smc_project_drop", "smc_project_reset", "smc_ble_midi", "smc_ble_link",
    "smc_ble_role_get", "smc_connected", "smc_midi_host", "smc_mmc_host",
    "smc_release", "smc_poll"))

harness = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "app/smc_protocol.h"
#define X0X_COLD
#define PROJ_BT_SMC_CENTRAL 16u
#define PROJ_BT_SMC_USER 32u
#define NKIT 2
#define NPARTS 5
#define NDRUM 11
#define O_NONE 0
#define O_ASK 1
/* Only the fields referenced by the actual queued functions matter. */
typedef struct { uint8_t kind, a, b, c; } pref_t;
static struct { struct { uint8_t bt; } set; } proj;
static struct { int overlay; } ui;
static int is_busy, is_testing, clear_calls, output_solo, enqueued;
static int perf_testing(void) { return is_testing; }
static int ui_project_busy(void) { return is_busy; }
static void engine_smc_solo(int parts) { output_solo = parts; clear_calls++; }
static void engine_smc_solo_targets(uint8_t parts, uint16_t a, uint16_t b)
{ output_solo = (int)parts | (int)(a != 0) << 8 | (int)(b != 0) << 9; }
static void ui_say(const char *a, const char *b) { (void)a; (void)b; }
''' + prefix + "\n" + original[:original.index("void smc_poll(void)")] + r'''
static uint8_t smc_message(const uint8_t *m, uint8_t n, int allow)
{
    if (!allow || n != 3) return 0;
    uint8_t type = m[0] & 0xF0u, note = m[1], vel = m[2];
    if (note < 8u || note >= 16u) return 0;
    if (type == 0x90u && vel) smc_solo_press((int)(note - 8u), 1, 0);
    else if (type == 0x80u || (type == 0x90u && !vel)) smc_release(note);
    return 0;
}
static void smc_guarded_button(uint8_t note, int mmc, int allow)
{ (void)note; (void)mmc; (void)allow; }
''' + extract("smc_poll") + r'''
static uint32_t packet(uint8_t st, uint8_t a, uint8_t b)
{ return 9u | (uint32_t)st << 8 | (uint32_t)a << 16 | (uint32_t)b << 24; }
static void prepare(void)
{
    smc_project_reset();
    smc_link_published = 0; smc_link_seen = 0; smc.up = 1;
    proj.set.bt = PROJ_BT_SMC_CENTRAL | PROJ_BT_SMC_USER;
    is_busy = is_testing = 0; ui.overlay = O_NONE;
    output_solo = 0; clear_calls = 0;
    smc_qr = smc_qw = smc_host_r = smc_host_w = 0;
    enqueued = 0;
}
static void press(int route)
{
    if (route == 0) assert(smc_midi_host(packet(0x9F, 8, 127)));
    else { const uint8_t m[3] = {0x90, 8, 127}; smc_ble_midi(m, 3); }
    smc_poll();
    assert(smc.solo_target[0] == 1);
    assert(output_solo != 0);
}
static void emit(int route, uint8_t st, uint8_t note, uint8_t vel)
{
    if (route == 0) assert(smc_midi_host(packet((uint8_t)(st | 15u), note, vel)));
    else { const uint8_t m[3] = {st, note, vel}; smc_ble_midi(m, 3); }
}
static void saturate(int route)
{
    for (unsigned i = 0; i < SMC_Q; i++) {
        if (route == 0) assert(smc_midi_host(packet(0xBF, 40, (uint8_t)i)));
        else { const uint8_t m[3] = {0xB0, 16, (uint8_t)i}; smc_ble_midi(m, 3); }
    }
}
static void scenario(int route, int note_zero, int full, int with_held)
{
    prepare();
    if (with_held) press(route);
    if (full) saturate(route);
    emit(route, note_zero ? 0x90 : 0x80, 8, 0);
    smc_poll();
    if (with_held && output_solo) {
        fprintf(stderr, "FAIL route=%d note_zero=%d full=%d: lost Solo release\n", route, note_zero, full);
        exit(1);
    }
    if (!full && with_held) assert(!smc.solo_target[0]);
}
static void no_false_release(int route)
{
    prepare();
    press(route);
    saturate(route);
    emit(route, 0x90, 9, 127); /* dropped NOTE ON must not cancel an earlier solo */
    smc_poll();
    assert(smc.solo_target[0] && output_solo);
}
static void queued_press_then_release(int route)
{
    prepare();
    emit(route, 0x90, 8, 127);
    for (unsigned i=1; i < SMC_Q; i++) {
        if (route == 0) assert(smc_midi_host(packet(0xBF, 40, (uint8_t)i)));
        else { const uint8_t m[3] = {0xB0, 16, (uint8_t)i}; smc_ble_midi(m, 3); }
    }
    emit(route, 0x80, 8, 0);
    smc_poll();
    assert(!smc.solo_target[0] && !output_solo);
}
static void malformed_host(void)
{
    prepare(); press(0); saturate(0);
    assert(!smc_midi_host(packet(0x8F, 0x88, 0)));
    assert(!smc_midi_host(packet(0x8F, 8, 0x80)));
    smc_poll();
    assert(smc.solo_target[0] && output_solo);
}
static void disabled_user_host(void)
{
    prepare(); press(0); saturate(0);
    proj.set.bt &= (uint8_t)~PROJ_BT_SMC_USER;
    assert(!smc_midi_host(packet(0x8F, 8, 0)));
    smc_poll();
    assert(smc.solo_target[0] && output_solo);
}
static void direct_wrong_channel(void)
{
    prepare(); press(1); saturate(1);
    proj.set.bt &= (uint8_t)~PROJ_BT_SMC_USER;
    emit(1, 0x8F, 8, 0);  /* channel 16 is not DAW's channel 1 */
    smc_poll();
    assert(smc.solo_target[0] && output_solo);
}
static void simultaneous_overflow(void)
{
    prepare(); press(0); saturate(0); saturate(1);
    emit(0, 0x80, 8, 0); emit(1, 0x80, 8, 0);
    smc_poll();
    assert(!smc.solo_target[0] && !output_solo);
}
static void reconnect_overflow(void)
{
    prepare(); press(1); saturate(1);
    emit(1, 0x80, 8, 0);
    smc_ble_link(0); smc_poll();
    assert(!smc.solo_target[0] && !output_solo);
}
static void project_reset_overflow(void)
{
    prepare(); press(0); saturate(0);
    emit(0, 0x80, 8, 0);
    is_busy = 1; smc_poll(); is_busy = 0; smc_poll();
    assert(!smc.solo_target[0] && !output_solo);
}
static void wrapped_generation(void)
{
    prepare(); press(0);
    smc_host_solo_release_lost = UINT32_MAX;
    smc_host_solo_release_seen = UINT32_MAX;
    saturate(0); emit(0, 0x80, 8, 0); smc_poll();
    assert(!smc.solo_target[0] && !output_solo);
}
int main(void)
{
    int cases = 0;
    for (int route = 0; route < 2; route++) {
        for (int note_zero = 0; note_zero < 2; note_zero++) {
            for (int full = 0; full < 2; full++) {
                scenario(route, note_zero, full, 1); cases++;
            }
        }
        no_false_release(route); cases++;
        queued_press_then_release(route); cases++;
    }
    malformed_host(); cases++;
    disabled_user_host(); cases++;
    direct_wrong_channel(); cases++;
    simultaneous_overflow(); cases++;
    reconnect_overflow(); cases++;
    project_reset_overflow(); cases++;
    wrapped_generation(); cases++;
    assert(cases == 19);
    puts("SMC Solo-release overflow: 19 positive/negative cases PASS");
    return 0;
}
'''

with tempfile.TemporaryDirectory(prefix="smc-solo-overflow-") as temp:
    test_c = Path(temp) / "test.c"
    exe = Path(temp) / "test"
    test_c.write_text(harness)
    flags = shlex.split(os.environ.get("SAN_FLAGS", ""))
    subprocess.run([os.environ.get("CC", "gcc"), "-std=c99", "-O1", "-g", "-Wall",
                    "-Wextra", "-Werror", *flags,
                    "-I" + str(ROOT / "firmware/src"), str(test_c), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
