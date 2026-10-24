#!/usr/bin/env python3
"""SMC BLE feedback reliability when the outgoing queue refuses a MIDI frame.

Extracts production smc_frame() and smc_ble_link() for actual retry/ack and
link-epoch behavior. BLE transmission and UI values are simulated. No hardware,
radio, installation, timing/preemption, or firmware-target claim is made.

SMC_SOURCE selects an exact earlier/mutated C source for a negative control.
CC and SAN_FLAGS select the native host compiler/sanitizer combination.
"""
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
SOURCE = Path(os.environ.get('SMC_SOURCE', ROOT / 'firmware/src/app/smc_mixer.c'))
src = SOURCE.read_text(encoding='utf-8')
match = re.search(r'^void smc_frame\(void\)\n\{.*?^\}', src, re.M | re.S)
if not match:
    raise RuntimeError(f'Production smc_frame not found: {SOURCE}')
link_match = re.search(r'^void smc_ble_link\(int up\)\n\{.*?^\}', src, re.M | re.S)
if not link_match:
    raise RuntimeError(f'Production smc_ble_link not found: {SOURCE}')

harness = r'''
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define PROJ_BT_SMC_CENTRAL 16u
/* Actual published-link/ISR state consumed by production link + frame paths. */
static volatile uint32_t smc_link_loss_seq, smc_link_ble_loss_boundary, smc_link_host_loss_boundary;
static volatile uint32_t smc_link_published, smc_ble_solo_release_lost, smc_host_solo_release_lost;
static volatile uint8_t smc_qw;
static uint32_t smc_link_seen;
void smc_ble_link(int up);
#define CHECK(x) do { ++checked; if (!(x)) { ++failed; \
    fprintf(stderr, "FAIL line %d: %s (failure=%d accepted=%d)\n", \
            __LINE__, #x, fail_at, accepted); } } while (0)

static struct { uint8_t up, known[128], lit[128], pending, value[8]; } smc;
static struct { struct { uint8_t bt; } set; } proj;
static uint8_t led[128], accepted_msg[128][3];
static int busy, fail_at, attempts, accepted, notice_calls, link_flip_at, coalesce_flip;
static unsigned checked, failed;
static int ui_project_busy(void) { return busy; }
static void smc_notice(int strip) { (void)strip; ++notice_calls; }
static int smc_led(int note) { return led[note]; }
static int plat_ble_smc_send(const uint8_t *msg, uint8_t len)
{
    if (len != 3u || accepted >= 128) return 0;
    int at = attempts++;
    if (at == link_flip_at) {
        smc_ble_link(0); /* TIMER5 edge mid-send */
        if (coalesce_flip) smc_ble_link(1); /* rapid reconnect before poll */
    }
    if (at == fail_at) return 0; /* temporarily full radio TX FIFO */
    memcpy(accepted_msg[accepted++], msg, 3);
    return 1;
}
'''

checks = r'''
static const uint8_t globals[11] = {
    0x2E, 0x2F, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F, 0x60, 0x61, 0x62, 0x63
};
static int note_at(int i) { return i < 32 ? i : globals[i - 32]; }
static void reset(void)
{
    memset(&smc, 0, sizeof smc);
    memset(led, 0, sizeof led);
    memset(accepted_msg, 0, sizeof accepted_msg);
    smc.up = 1;
    smc.pending = 255u;
    proj.set.bt = PROJ_BT_SMC_CENTRAL;
    for (int i=0; i<43; ++i) led[note_at(i)] = (uint8_t)((i & 1) != 0);
    for (int i=0; i<8; ++i) smc.value[i] = (uint8_t)(10 + i * 13);
    busy = attempts = accepted = notice_calls = 0;
    fail_at = -1; link_flip_at = -1; coalesce_flip = 0;
    smc_link_loss_seq = 0; smc_link_published = smc_link_seen = 1;
    smc_link_ble_loss_boundary = smc_link_host_loss_boundary = 0;
    smc_ble_solo_release_lost = smc_host_solo_release_lost = 0;
    smc_qw = 0;
}
static void expect_message(int idx, int seqidx)
{
    CHECK(idx < accepted);
    if (idx >= accepted) return;
    const uint8_t *m = accepted_msg[idx];
    if (seqidx < 43) {
        int n = note_at(seqidx);
        CHECK(m[0] == 0x90u && m[1] == (uint8_t)n &&
              m[2] == (uint8_t)(led[n] ? 127 : 0));
    } else {
        int strip = seqidx - 43;
        CHECK(m[0] == (uint8_t)(0xE0u + strip) &&
              m[1] == smc.value[strip] && m[2] == smc.value[strip]);
    }
}
static void cold_drop(int drop_at)
{
    reset();
    fail_at = drop_at;
    smc_frame();
    CHECK(accepted == drop_at);
    CHECK(attempts == drop_at + 1);
    CHECK(notice_calls == 8);
    for (int i=0; i<43; ++i) {
        int n = note_at(i);
        CHECK(smc.known[n] == (i < drop_at));
        if (i < drop_at) CHECK(smc.lit[n] == led[n]);
    }
    for (int i=0; i<8; ++i)
        CHECK(!!(smc.pending & (1u << i)) == (43 + i >= drop_at));
    for (int i=0; i<drop_at; ++i) expect_message(i, i);
    /* Recompute a value BEFORE the retry: never transmit stale feedback. */
    if (drop_at < 43) {
        int n = note_at(drop_at);
        led[n] ^= 1u;
    } else {
        smc.value[drop_at-43] = (uint8_t)(smc.value[drop_at-43] + 1u);
    }
    fail_at = -1; attempts = 0;
    smc_frame();
    CHECK(accepted == 51);
    CHECK(attempts == 51-drop_at);
    for (int i=drop_at; i<51; ++i) expect_message(i, i);
    for (int i=0; i<43; ++i) {
        int n = note_at(i);
        CHECK(smc.known[n] && smc.lit[n] == led[n]);
    }
    CHECK(smc.pending == 0);
    CHECK(notice_calls == 16);
    attempts = 0;
    smc_frame();
    CHECK(attempts == 0 && accepted == 51); /* no duplicate after recovery */
}
static void current_state_and_gates(void)
{
    reset(); smc_frame(); CHECK(accepted == 51 && smc.pending == 0);
    attempts = accepted = 0;
    smc_frame(); CHECK(!attempts && !accepted);

    /* An already acknowledged change and a pending knob-fader packet must
     * still be retried, without resending unrelated acknowledged LEDs. */
    led[4] ^= 1; led[0x5E] ^= 1;
    smc.value[0] = 101; smc.value[7] = 32;
    smc.pending = (uint8_t)((1u << 0) | (1u << 7));
    fail_at = 1;
    smc_frame();
    CHECK(accepted == 1 && attempts == 2);
    expect_message(0, 4); /* changed LED 4 only */
    CHECK(smc.lit[4] == led[4] && smc.lit[0x5E] != led[0x5E]);
    CHECK(smc.pending == (uint8_t)((1u << 0) | (1u << 7)));
    led[0x5E] ^= 1; /* return to previously sent value before recovery */
    smc.value[0] = 102;
    fail_at = -1; attempts = 0;
    smc_frame();
    CHECK(accepted == 3 && attempts == 2); /* only faders, LED is no longer stale */
    CHECK(accepted_msg[1][0] == 0xE0 && accepted_msg[1][1] == 102);
    CHECK(accepted_msg[2][0] == 0xE7 && accepted_msg[2][1] == 32);
    CHECK(smc.pending == 0);

    /* Disconnected, mode-off and project-locked must not emit stale control. */
    led[2] ^= 1; smc.pending = 1;
    for (int mode=0; mode<3; ++mode) {
        busy = mode == 0;
        smc.up = mode != 1;
        proj.set.bt = mode == 2 ? 0 : PROJ_BT_SMC_CENTRAL;
        attempts = 0;
        smc_frame();
        CHECK(attempts == 0 && smc.pending == 1 && smc.lit[2] != led[2]);
    }
    busy = 0; smc.up = 1; proj.set.bt = PROJ_BT_SMC_CENTRAL;
    attempts = 0;
    smc_frame();
    CHECK(attempts == 2 && smc.pending == 0 && smc.lit[2] == led[2]);
}
/* A successful BLE enqueue can race TIMER5 disconnect while the callback
 * runs. It is accepted for the OLD link, but not a valid acknowledgement for
 * the NEW link. Abort the frame after the ISR publishes its edge. */
static void link_change_during_feedback(int edge_at, int coalesced)
{
    reset();
    link_flip_at = edge_at; coalesce_flip = coalesced;
    smc_frame();
    CHECK((smc_link_published & 1u) == (uint32_t)coalesced);
    CHECK(accepted == edge_at + 1);       /* NO post-edge stale feedback */
    CHECK(attempts == edge_at + 1);
    for (int i=0; i<43; ++i) {
        int n = note_at(i);
        CHECK(smc.known[n] == (i < edge_at));
    }
    for (int i=0; i<8; ++i)
        CHECK(!!(smc.pending & (1u << i)) == (43 + i >= edge_at));
    /* Before smc_poll consumes the new link, even a coalesced up/down/up
     * must NOT transmit using acknowledgement state from the old session. */
    attempts = 0;
    smc_frame(); CHECK(attempts == 0);
    /* Model the main-loop smc_poll link acknowledgement and forced resync
     * explicitly; this source-window test is not a whole smc_poll test. */
    if (!coalesced) smc_ble_link(1);
    smc_link_seen = smc_link_published;
    smc.up = 1;
    memset(smc.known, 0, sizeof smc.known);
    smc.pending = 255u;
    led[note_at(edge_at < 43 ? edge_at : 0)] ^= 1u;
    for (int i=0; i<8; ++i) smc.value[i] = (uint8_t)(110+i);
    fail_at = link_flip_at = -1;
    attempts = accepted = 0;
    smc_frame();
    CHECK(accepted == 51 && attempts == 51);
    for (int i=0; i<51; ++i) expect_message(i, i);
    CHECK(smc.pending == 0u);
}
int main(void)
{
    for (int drop=0; drop<51; ++drop) cold_drop(drop);
    current_state_and_gates();
    for (int coalesced=0; coalesced<2; ++coalesced)
        for (int edge=0; edge<51; ++edge)
            link_change_during_feedback(edge, coalesced);
    printf("SMC BLE retry/feedback: 51 injected TX backpressure and 102 ISR disconnect/reconnect positions + "
           "state/connection gates, %u checks, %u failed\n", checked, failed);
    return failed ? 1 : 0;
}
'''

with tempfile.TemporaryDirectory(prefix='smc-feedback-retry-') as folder:
    cfile = Path(folder) / 'feedback_retry.c'
    binary = Path(folder) / 'feedback_retry'
    cfile.write_text(harness + link_match[0] + match[0] + checks, encoding='utf-8')
    cmd = [*shlex.split(os.environ.get('CC', 'gcc')), '-std=c99', '-O1', '-g',
           '-Wall', '-Wextra', '-Werror', *shlex.split(os.environ.get('SAN_FLAGS', '')),
           str(cfile), '-o', str(binary)]
    print('COMPILE:', shlex.join(cmd), flush=True)
    subprocess.run(cmd, check=True)
    subprocess.run([str(binary)], check=True)
