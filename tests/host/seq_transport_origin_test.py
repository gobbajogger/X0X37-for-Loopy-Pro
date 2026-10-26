#!/usr/bin/env python3
"""Native host regression for X0X37 transport request origin/phase.

Extracts the real request functions and transport-apply block from the supplied
sequencer.c; does not reinterpret the production request handler in Python.
Only the sequencer boundary is compiled, not a complete firmware target.
"""
import argparse
import os
import pathlib
import re
import subprocess
import sys
import tempfile

FUNCTIONS = ("seq_start", "seq_stop", "seq_continue", "seq_ext_clock", "seq_midi_rt")


def extract_function(source: str, name: str) -> str:
    m = re.search(r"(?:void|int)\s+" + name + r"\s*\([^;]*\)\s*(?:/\*.*?\*/\s*)?\{", source, re.S)
    if not m:
        raise ValueError(f"missing source function {name}")
    start = source.index("{", m.start())
    depth = 0
    for idx in range(start, len(source)):
        if source[idx] == "{":
            depth += 1
        elif source[idx] == "}":
            depth -= 1
            if depth == 0:
                return source[m.start():idx + 1]
    raise ValueError(f"unterminated source function {name}")


def extract_apply(source: str) -> str:
    begin = source.index("if (s->req_stop) {")
    end = re.search(r"s->midi_req\s*=\s*0\s*;[^\n]*", source[begin:])
    if not end:
        raise ValueError("missing transport apply boundary")
    return source[begin:begin + end.end()]


PREFIX = r'''
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#define SEQ_CLK_INT 1u
#define TRK_DRUM 0
#define FS 44100u

typedef struct {
    uint8_t req_start, req_cont, req_stop, req_rt, req_stop_rt, midi_req;
    uint8_t clk_src, playing, start_local;
    uint32_t ext_pending, ext_now, start_at, req_prior_ticks;
    int count_start, count_continue, count_stop;
} seq_t;
typedef struct { int ignored; } seq_sink_t;
static uint32_t seq_step_q8(seq_t *s, int rate) {
    (void)s; (void)rate; return 11025u * 256u;
}
static int part_rate(seq_t *s, int t) { (void)s; (void)t; return 0; }
static void do_start(seq_t *s, int top, const seq_sink_t *o, uint8_t remote) { (void)top; (void)o; (void)remote; (void)o; s->playing = 1;
    if (top) s->count_start++; else s->count_continue++;
}
static void do_stop(seq_t *s, const seq_sink_t *o, uint8_t remote) { (void)o; (void)remote; (void)o; s->playing = 0; s->count_stop++;
}
'''
SUFFIX = r'''
static void apply_transport(seq_t *s) {
    seq_sink_t sink = {0};
    const seq_sink_t *o = &sink;
    TRANSPORT_BLOCK
}
static int failed = 0, checked = 0;
#define CHECK(expr, why) do { checked++; if (!(expr)) { failed++; fprintf(stderr, "FAIL %s\n", why); } } while (0)
static seq_t fresh(void) { seq_t s = {0}; return s; }
int main(void) {
    seq_t s;
    /* A locally pressed PLAY wins over an incoming Start waiting in the block. */
    s = fresh(); seq_midi_rt(&s, 0xFA); seq_start(&s); apply_transport(&s);
    CHECK(s.playing && s.start_local && s.count_start == 1, "remote Start then local PLAY must count as local");
    s.ext_now += 100; seq_midi_rt(&s, 0xFA); apply_transport(&s);
    CHECK(s.count_start == 1, "PLAY echo within a step must not cause duplicate Start");

    /* A canceled remote start cannot tag the next local operation as remote. */
    s = fresh(); seq_midi_rt(&s, 0xFA); seq_stop(&s); seq_start(&s); apply_transport(&s);
    CHECK(s.playing && s.start_local && s.count_start == 1, "remote Start/Stop/local Start preserves origin");
    s = fresh(); seq_midi_rt(&s, 0xFA); seq_stop(&s); seq_continue(&s); apply_transport(&s);
    CHECK(s.playing && s.start_local && s.count_continue == 1, "remote Start/Stop/local Continue preserves origin");
    s = fresh(); seq_midi_rt(&s, 0xFA); seq_stop(&s);
    CHECK(!s.req_rt && !s.req_start && !s.req_cont, "Stop clears canceled remote origin");

    /* Last request still wins, but remote requests retain their provenance. */
    s = fresh(); seq_start(&s); seq_midi_rt(&s, 0xFA); apply_transport(&s);
    CHECK(!s.start_local && s.count_start == 1, "incoming Start overrides local Start origin");
    s = fresh(); seq_continue(&s); seq_midi_rt(&s, 0xFB); apply_transport(&s);
    CHECK(!s.start_local && s.count_continue == 1, "incoming Continue overrides local Continue origin");
    s = fresh(); seq_midi_rt(&s, 0xFA); seq_continue(&s); apply_transport(&s);
    CHECK(s.start_local && s.count_continue == 1 && !s.count_start, "local Continue overrides incoming Start");
    s = fresh(); seq_midi_rt(&s, 0xFB); seq_start(&s); apply_transport(&s);
    CHECK(s.start_local && s.count_start == 1 && !s.count_continue, "local Start overrides incoming Continue");
    s = fresh(); seq_start(&s); seq_midi_rt(&s, 0xFB); apply_transport(&s);
    CHECK(!s.start_local && s.count_continue == 1 && !s.count_start, "incoming Continue overrides local Start");
    s = fresh(); seq_continue(&s); seq_midi_rt(&s, 0xFA); apply_transport(&s);
    CHECK(!s.start_local && s.count_start == 1 && !s.count_continue, "incoming Start overrides local Continue");

    /* The fix must not affect external clock, Note-Off, INTERNAL policy or midi_req. */
    s = fresh(); seq_midi_rt(&s, 0xF8);
    CHECK(s.ext_pending == 1 && !s.req_rt, "MIDI clocks do not modify origin");
    CHECK(seq_midi_rt(&s, 0x80) == 0 && !s.req_rt, "Note Off remains outside realtime transport");
    s = fresh(); s.clk_src = SEQ_CLK_INT; seq_midi_rt(&s, 0xFA); seq_start(&s); apply_transport(&s);
    CHECK(s.start_local && s.count_start == 1, "INTERNAL ignores remote transport but not local PLAY");
    s = fresh(); s.midi_req = 1; seq_midi_rt(&s, 0xFA);
    CHECK(s.midi_req == 1, "existing clip-mode echo suppression is unchanged before apply");
    apply_transport(&s); CHECK(!s.midi_req, "midi_req is consumed at audio boundary");

    printf("%d/%d transport provenance checks passed\n", checked - failed, checked);
    return failed != 0;
}
'''


def run(source_path: pathlib.Path, cc: str, flags: str) -> int:
    src = source_path.read_text()
    functions = "\n\n".join(extract_function(src, name) for name in (("request_start", "request_continue") if "request_start(seq_t" in src else ()) + FUNCTIONS)
    apply = extract_apply(src)
    code = PREFIX + functions + "\n" + SUFFIX.replace("TRANSPORT_BLOCK", apply)
    with tempfile.TemporaryDirectory(prefix="seq-origin-host-") as tmp:
        source = pathlib.Path(tmp) / "test.c"
        binary = pathlib.Path(tmp) / "test"
        source.write_text(code)
        cmd = [cc, "-std=c11", "-Wall", "-Wextra", "-Werror", "-O1"] + flags.split() + [str(source), "-o", str(binary)]
        print("COMPILE", " ".join(cmd), flush=True)
        c = subprocess.run(cmd, check=False)
        print("COMPILE_EXIT", c.returncode, flush=True)
        if c.returncode:
            return c.returncode
        test = subprocess.run([str(binary)], check=False)
        print("TEST_EXIT", test.returncode, flush=True)
        return test.returncode


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", type=pathlib.Path, required=True)
    parser.add_argument("--cc", default=os.environ.get("CC", "gcc"))
    parser.add_argument("--flags", default="")
    args = parser.parse_args()
    sys.exit(run(args.source, args.cc, args.flags))
