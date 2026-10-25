#!/usr/bin/env python3
"""Native clock/transport order regression using functions extracted from sequencer.c.

This is a scoped source-function test, not a complete firmware target build.
When multiple MIDI packets arrive in one audio block, clocks before an accepted
Start/Continue/PLAY belong to the previous transport epoch; clocks after it
belong to the new one. A rejected PLAY echo must not discard its preceding ticks.
"""
import argparse
import os
import pathlib
import re
import subprocess
import sys
import tempfile

FUNCTIONS = (
    "request_start", "request_continue", "seq_start", "seq_stop",
    "seq_continue", "seq_ext_clock", "seq_midi_rt",
)


def function_body(src, name):
    pattern = r"(?:static )?(?:void|int)\s+" + name + r"\s*\([^;]*\)\s*(?:/\*.*?\*/\s*)?\{"
    match = re.search(pattern, src, re.S)
    if not match:
        raise ValueError("missing production function: " + name)
    pos = src.index("{", match.start())
    depth = 0
    for idx in range(pos, len(src)):
        if src[idx] == "{":
            depth += 1
        elif src[idx] == "}":
            depth -= 1
            if depth == 0:
                return src[match.start():idx + 1]
    raise ValueError("unterminated production function: " + name)


def apply_block(src):
    begin = src.index("if (s->req_stop) {")
    end = re.search(r"s->midi_req\s*=\s*0\s*;[^\n]*", src[begin:])
    if not end:
        raise ValueError("missing production transport application block")
    return src[begin:begin + end.end()]


PREFIX = r'''
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define SEQ_CLK_INT 1u
#define TRK_DRUM 0
#define FS 44100u

typedef struct {
    uint8_t req_start, req_cont, req_stop, req_rt, req_stop_rt, midi_req;
    uint8_t clk_src, playing, start_local;
    uint32_t ext_pending, ext_now, start_at, req_prior_ticks;
    uint32_t starts, continues, stops;
} seq_t;
typedef struct { int unused; } seq_sink_t;
static uint32_t seq_step_q8(const seq_t *s, int rate)
{ (void)s; (void)rate; return 11025u * 256u; }
static int part_rate(const seq_t *s, int track)
{ (void)s; (void)track; return 0; }
static void do_start(seq_t *s, int top, const seq_sink_t *o, uint8_t remote)
{ (void)top; (void)o; (void)remote; (void)o; s->playing = 1; if (top) s->starts++; else s->continues++; }
static void do_stop(seq_t *s, const seq_sink_t *o, uint8_t remote)
{ (void)o; (void)remote; (void)o; s->playing = 0; s->stops++; }
'''
SUFFIX = r'''
static void apply_transport(seq_t *s)
{
    seq_sink_t sink = {0};
    const seq_sink_t *o = &sink;
    TRANSPORT_APPLY
}
static unsigned checks = 0, failures = 0;
#define CHECK(expr, text) do { checks++; if (!(expr)) { failures++; fprintf(stderr, "FAIL %s\n", text); } } while (0)
int main(void)
{
    seq_t s = {0};
    for (int i = 0; i < 6; i++) seq_midi_rt(&s, 0xF8);
    seq_midi_rt(&s, 0xFA); apply_transport(&s);
    CHECK(s.ext_pending == 0, "F8 packets before remote Start cannot belong to new transport");
    CHECK(s.starts == 1, "Start still starts playback");

    memset(&s, 0, sizeof s);
    seq_midi_rt(&s, 0xFA); seq_midi_rt(&s, 0xF8); apply_transport(&s);
    CHECK(s.ext_pending == 1, "F8 after remote Start must survive");

    memset(&s, 0, sizeof s);
    for (int i = 0; i < 6; i++) seq_midi_rt(&s, 0xF8);
    seq_midi_rt(&s, 0xFA);
    seq_midi_rt(&s, 0xF8); seq_midi_rt(&s, 0xF8); apply_transport(&s);
    CHECK(s.ext_pending == 2, "mixed packet ordering preserves only post-Start F8");

    memset(&s, 0, sizeof s);
    for (int i = 0; i < 6; i++) seq_midi_rt(&s, 0xF8);
    seq_midi_rt(&s, 0xFB); apply_transport(&s);
    CHECK(s.ext_pending == 0, "F8 before remote Continue cannot advance resumed phase");

    memset(&s, 0, sizeof s);
    for (int i = 0; i < 6; i++) seq_midi_rt(&s, 0xF8);
    seq_start(&s); apply_transport(&s);
    CHECK(s.ext_pending == 0, "F8 before local PLAY cannot advance new phase");
    CHECK(s.start_local == 1, "local PLAY provenance is preserved");

    memset(&s, 0, sizeof s);
    seq_start(&s); apply_transport(&s);
    seq_midi_rt(&s, 0xF8); seq_midi_rt(&s, 0xF8);
    seq_midi_rt(&s, 0xFA); apply_transport(&s);
    CHECK(s.ext_pending == 2, "ignored remote PLAY echo MUST preserve queued F8");
    CHECK(s.starts == 1, "ignored PLAY echo cannot restart step one");

    memset(&s, 0, sizeof s);
    seq_midi_rt(&s, 0xF8); seq_midi_rt(&s, 0xFA);
    seq_midi_rt(&s, 0xF8); seq_midi_rt(&s, 0xFB);
    seq_midi_rt(&s, 0xF8); apply_transport(&s);
    CHECK(s.ext_pending == 1, "last Continue wins and keeps only later F8");
    CHECK(s.starts == 0 && s.continues == 1, "latest Continue determines transport");

    memset(&s, 0, sizeof s);
    for (int i = 0; i < 5; i++) seq_midi_rt(&s, 0xF8);
    seq_midi_rt(&s, 0xFA);
    s.ext_pending = 1; /* clock ownership change can clear an older queue */
    apply_transport(&s);
    CHECK(s.ext_pending == 0, "queued-tick subtraction must not underflow");

    memset(&s, 0, sizeof s);
    s.clk_src = SEQ_CLK_INT;
    seq_midi_rt(&s, 0xFA);
    CHECK(s.req_start == 0, "INTERNAL ignores MIDI Start");
    seq_start(&s); apply_transport(&s);
    CHECK(s.starts == 1 && s.start_local, "INTERNAL still accepts local PLAY");
    CHECK(seq_midi_rt(&s, 0x80) == 0, "Note Off is not intercepted by MIDI realtime handler");


    /* Boundary: most recent Start supersedes a queued Continue and owns its ticks. */
    memset(&s, 0, sizeof s);
    seq_midi_rt(&s, 0xFA);
    for (int i = 0; i < 3; i++) seq_midi_rt(&s, 0xF8);
    seq_midi_rt(&s, 0xFB);
    seq_midi_rt(&s, 0xF8);
    seq_start(&s);
    seq_midi_rt(&s, 0xF8); seq_midi_rt(&s, 0xF8);
    apply_transport(&s);
    CHECK(s.ext_pending == 2, "last local Start retains only its two following clocks");
    CHECK(s.starts == 1 && s.continues == 0, "last Start supersedes remote Continue");
    CHECK(s.start_local == 1, "last local Start keeps local origin after two remote requests");

    /* Stop cancels pending remote Start, but a later local Continue survives Stop. */
    memset(&s, 0, sizeof s);
    seq_midi_rt(&s, 0xFA);
    seq_midi_rt(&s, 0xF8);
    seq_stop(&s);
    seq_midi_rt(&s, 0xF8);
    seq_continue(&s);
    seq_midi_rt(&s, 0xF8);
    apply_transport(&s);
    CHECK(s.ext_pending == 1, "Stop/local Continue discards older epoch clocks");
    CHECK(s.starts == 0 && s.continues == 1, "Stop/local Continue executes only Continue");
    CHECK(s.start_local == 1, "Stop/local Continue is not misclassified as remote");

    /* MIDI Note Off is never consumed by the realtime clock/transport dispatcher. */
    memset(&s, 0, sizeof s);
    seq_midi_rt(&s, 0xFA);
    seq_midi_rt(&s, 0xF8);
    uint32_t before = s.ext_pending;
    CHECK(seq_midi_rt(&s, 0x80) == 0, "Note Off remains available to MIDI note routing");
    CHECK(s.ext_pending == before && s.req_start == 1,
          "Note Off leaves queued Start and clock phase unchanged");

    printf("%u/%u clock epoch native checks passed\n", checks - failures, checks);
    return failures ? 1 : 0;
}
'''


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", required=True, type=pathlib.Path)
    parser.add_argument("--cc", default=os.environ.get("CC", "cc"))
    parser.add_argument("--flags", default="")
    args = parser.parse_args()
    src = args.source.read_text(encoding="utf-8")
    code = PREFIX + "\n\n".join(function_body(src, n) for n in FUNCTIONS)
    code += "\n" + SUFFIX.replace("TRANSPORT_APPLY", apply_block(src))
    with tempfile.TemporaryDirectory(prefix="x0x-clock-epoch-") as tmp:
        test_src = pathlib.Path(tmp, "clock_epoch.c")
        test_bin = pathlib.Path(tmp, "clock_epoch")
        test_src.write_text(code, encoding="utf-8")
        command = [args.cc, "-std=c11", "-Wall", "-Wextra", "-Werror", "-O1"]
        command += args.flags.split() + [str(test_src), "-o", str(test_bin)]
        print("COMPILE", " ".join(command), flush=True)
        compiled = subprocess.run(command, check=False)
        print("COMPILE_EXIT", compiled.returncode, flush=True)
        if compiled.returncode:
            return compiled.returncode
        ran = subprocess.run([str(test_bin)], check=False)
        print("TEST_EXIT", ran.returncode, flush=True)
        return ran.returncode


if __name__ == "__main__":
    sys.exit(main())
