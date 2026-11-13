#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Exercise production BREAK control/CQ and the real renderer without synth assets.

The C harness replaces only unrelated owners, FX/master and sequencer timing.
No copied gate or queue model is used: complete production functions, including
engine_render, are extracted verbatim. Run on the designated build/test box.
"""
from pathlib import Path
import os
import re
import shlex
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]


def function(source, masked, signature):
    """Retain complete bodies; braces in comments and strings do not count."""
    start = source.index(signature)
    brace = masked.index("{", start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (masked[end] == "{") - (masked[end] == "}")
        end += 1
    return source[start:end] + "\n"


def main():
    source = (ROOT / "firmware/src/app/engine.c").read_text()
    api = (ROOT / "firmware/src/app/engine.h").read_text()
    masked = re.sub(
        r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',
        lambda match: " " * len(match.group()), source, flags=re.S,
    )
    cq_start = source.index("enum { C_PARAM_T0")
    cq_end = source.index("static volatile uint32_t cq_w, cq_r;", cq_start)
    cq_end += len("static volatile uint32_t cq_w, cq_r;")
    if "static void cq_mark_fresh(" in source:
        cq_end = source.index("static int cq_replace(", cq_start)
    declarations = source[cq_start:cq_end]
    if "static volatile uint8_t notes_off_req;" in source:
        start = source.index("static volatile uint8_t notes_off_req;")
        declarations += "\n" + source[start:source.index("void engine_request_notes_off(", start)]
    for first in ("enum { T_909,", "enum { PART_909,"):
        start = api.index(first)
        declarations += "\n" + api[start:api.index(";", start) + 1]
    # After 53fe37d6, engine_render uses the same-block FX/master tail
    # pipeline symbols. Extract them verbatim. Reviewed: BREAK mute still
    # holds; the official suite failed only because these were missing.
    if "static float eng_vol;" in source:
        start = source.index("static float eng_vol;")
        end = source.index("static volatile uint32_t eng_fx_done;", start)
        declarations += "\n" + source[start:end]
    if "#define ENG_TAIL_CS " in source:
        start = source.index("#define ENG_TAIL_CS ")
        end = source.index("\n", start)
        declarations += "\n" + source[start:end + 1]
    signatures = [
        "static int cq_replace(",
        "__attribute__((noinline)) static void cq_put(",
        "static void s_brk(",
        "static void brk_local_event(",
        "static void drain(",
        "void engine_render(",
    ]
    # The exact base has no helper; retaining that case permits a meaningful
    # audible RED on the base engine_render rather than an extraction failure.
    if "static void brk_mute_boundary(" in source:
        signatures.insert(2, "static void brk_mute_boundary(")
    # Extraction is deliberately strict: an upstream refactor must be reviewed.
    generated = declarations + "\n" + "\n".join(
        function(source, masked, signature) for signature in signatures
    )
    with tempfile.TemporaryDirectory(prefix="engine-brk-mute-") as directory:
        work = Path(directory)
        (work / "production_engine_brk_control.inc").write_text(generated)
        cc = shlex.split(os.environ.get("CC", "cc"))
        flags = [
            "-std=c99", "-O2", "-ffp-contract=off", "-DX0X_HOST",
            "-Wall", "-Wextra", "-Werror", "-Wno-unused-parameter",
            "-Wno-unused-function", "-Wno-error=attributes",
            "-I" + str(work),
        ]
        harness = ROOT / "tests/host/engine_brk_mute_test.c"
        for control in (False, True):
            executable = work / ("abrupt-control" if control else "mute-test")
            extra = ["-DBRK_FADE=1", "-DENGINE_BRK_ABRUPT_CONTROL"] if control else []
            subprocess.run(cc + flags + extra + [str(harness), "-lm", "-o", str(executable)],
                           cwd=ROOT, check=True)
            result = subprocess.run([str(executable)], cwd=ROOT, check=False)
            expected = 3 if control else 0
            if result.returncode != expected:
                raise SystemExit(f"{'abrupt-cut control' if control else 'mute regression'}: "
                                 f"exit {result.returncode}, expected {expected}")
    print("engine-brk-mute: production control/renderer and abrupt-cut control PASS")


if __name__ == "__main__":
    main()
