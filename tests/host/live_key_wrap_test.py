#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Live-key panic order stays a signed compare across the free-running wrap.

The host FIFO never reaches a wrapped accept position, so this harness calls
the firmware drain with the counters set on either side of 2^32. A pre-wrap
note is dropped. A post-wrap note is kept even when its generation is stale.
Device and native builds are not this test.
"""
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]

PRE = r"""
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define NBASS 2
#define LIVE_KEY_Q 32u
#define ORIGINAL_INPUT_KEYS 0
#define ORIGINAL_RUNTIME_OK 0
typedef struct { uint32_t generation, word; } live_key_command_t;
typedef struct { uint32_t generation, context; uint8_t gate, token, pitch, arp, blocked; } live_key_owner_t;
struct bass_dummy { int unused; };
static live_key_command_t live_key_q[LIVE_KEY_Q];
static live_key_owner_t live_key_owner[NBASS];
static volatile uint8_t live_key_w, live_key_r, live_key_cancel;
static uint32_t live_key_at;
static uint32_t notes_off_live;
static uint8_t notes_off_drop, live_panic_pass;
static uint8_t original_key_pitch[NBASS][64];
static uint8_t bass_held[NBASS], bass_held_count[NBASS], bass_held_src[NBASS];
static struct bass_dummy original_bass[NBASS];
static int applied;
static uint32_t plat_cq_lock(void) { return 0; }
static void plat_cq_unlock(uint32_t s) { (void)s; }
static void live_key_context_retire(int v, int release) { (void)v; (void)release; }
static int original_runtime_key(struct bass_dummy *part, int input, unsigned pitch, unsigned velocity)
{
    (void)part; (void)input; (void)pitch;
    if (velocity)
        applied++;
    return ORIGINAL_RUNTIME_OK;
}
"""

HARNESS = r"""
static void reset_key(uint32_t ord, uint32_t limit, int panic_pass, uint32_t owner_gen, uint32_t cmd_gen)
{
    memset(live_key_owner, 0, sizeof live_key_owner);
    memset(live_key_q, 0, sizeof live_key_q);
    memset(original_key_pitch, 0, sizeof original_key_pitch);
    memset(bass_held, 0, sizeof bass_held);
    memset(bass_held_count, 0, sizeof bass_held_count);
    memset(bass_held_src, 0, sizeof bass_held_src);
    live_key_r = 0;
    live_key_w = 1;
    live_key_cancel = 0;
    live_key_at = ord;
    notes_off_live = limit;
    notes_off_drop = 1;
    live_panic_pass = (uint8_t)panic_pass;
    live_key_owner[0].generation = owner_gen;
    live_key_q[0].generation = cmd_gen;
    /* voice 0, token 1, pitch 60, velocity 100 */
    live_key_q[0].word = (1u << 1) | (60u << 7) | (100u << 14);
    applied = 0;
}

static int expect_wrap(void)
{
    /* ord 0xFFFFFFF0 is before limit 5. Unsigned ord < limit keeps it. */
    reset_key(0xFFFFFFF0u, 5u, 0, 4u, 4u);
    live_key_drain();
    if (applied != 0)
        return 1;
    /* ord 2 is after limit 0xFFFFFFF0. Unsigned post misses it and the stale
     * generation is then rejected. */
    reset_key(2u, 0xFFFFFFF0u, 1, 9u, 4u);
    live_key_drain();
    if (applied != 1)
        return 2;
    return 0;
}

int main(void)
{
    int rc = expect_wrap();
    if (rc) {
        printf("FAIL live key wrap rc=%d applied=%d\n", rc, applied);
        return 1;
    }
    printf("live key wrap: 0 failures\n");
    return 0;
}
"""


def extract_fn(text, signature):
    start = text.index(signature)
    i = text.index("{", start)
    depth = 0
    for j in range(i, len(text)):
        if text[j] == "{":
            depth += 1
        elif text[j] == "}":
            depth -= 1
            if depth == 0:
                return text[start:j + 1] + "\n"
    raise SystemExit("unclosed " + signature)


def main():
    engine = (ROOT / "firmware/src/app/engine.c").read_text()
    drain = extract_fn(engine, "static void live_key_drain(void)\n{")
    source = PRE + drain + HARNESS
    with tempfile.TemporaryDirectory(prefix="x0x-live-wrap-") as tmp:
        c = Path(tmp) / "test.c"
        exe = Path(tmp) / "test"
        c.write_text(source)
        subprocess.run(
            ["cc", "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", str(c), "-o", str(exe)],
            check=True,
        )
        return subprocess.run([str(exe)]).returncode


if __name__ == "__main__":
    raise SystemExit(main())
