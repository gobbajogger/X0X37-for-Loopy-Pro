#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Prove lossless tanh pack: audio bit-identical to pre-pack tip; table bits exact."""
import hashlib
import json
import math
import os
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
BASE = "092589b84fd455355216cb0de93c4782236f7524"  # nightly tip before this pack


def f32(x):
    return struct.unpack("<f", struct.pack("<f", x))[0]


def bits32(x):
    return struct.unpack("<I", struct.pack("<f", x))[0]


def run(args, **kw):
    return subprocess.run(args, check=True, cwd=ROOT, **kw)


def old(path):
    return subprocess.check_output(["git", "show", BASE + ":" + path], cwd=ROOT)


def main():
    sys.path.insert(0, str(ROOT / "tools"))
    import gen_drum_samples as gen  # noqa: E402

    tab = [f32(math.tanh(gen.TANH_MAX * i / gen.TANH_N)) for i in range(gen.TANH_N + 2)]
    runs, words, raw_b, packed_b = gen.pack_tanh_runs(tab)
    save_b = raw_b - packed_b
    if save_b < 600:
        print(json.dumps({"status": "NO_PR_SAVE_BELOW_FLOOR", "raw_b": raw_b,
                          "packed_b": packed_b, "save_b": save_b}))
        return 2

    ends = [end for *_, end in runs]

    def value(i):
        r = start = 0
        while i >= ends[r]:
            start = ends[r]
            r += 1
        base, offset, width, _end = runs[r]
        if width == 0:
            return base
        bit = (i - start) * width
        shift = bit & 31
        p = offset + (bit >> 5)
        v = words[p] >> shift
        if shift:
            v |= words[p + 1] << (32 - shift)
        return base | (v & ((1 << width) - 1))

    for i, want in enumerate(tab):
        got = value(i)
        if got != bits32(want):
            print(f"python reconstruct fail i={i} want={bits32(want):08x} got={got:08x}",
                  file=sys.stderr)
            return 1

    cc = os.environ.get("CC", "cc")
    dsp_dir = ROOT / "firmware" / "src" / "dsp"
    with tempfile.TemporaryDirectory() as tmp:
        tmp = Path(tmp)
        results = {}
        for kind in ("baseline", "packed"):
            out = tmp / kind
            gen = out / "gen"
            shadow = out / "shadow" / "dsp"
            gen.mkdir(parents=True)
            shadow.mkdir(parents=True)
            # Full DSP header shadow so quoted includes and dsp/*.h harness paths
            # both see the kind-specific drum909_dsp.h / generated tables.
            for hdr in dsp_dir.glob("*.h"):
                (shadow / hdr.name).write_bytes(hdr.read_bytes())
            dsp_path = "firmware/src/dsp/drum909_dsp.h"
            (shadow / "drum909_dsp.h").write_bytes(
                old(dsp_path) if kind == "baseline" else (ROOT / dsp_path).read_bytes())

            script = ROOT / "tools/gen_drum_samples.py"
            code = old("tools/gen_drum_samples.py") if kind == "baseline" else script.read_bytes()
            ns = {"__file__": str(script), "__name__": "generator_test"}
            exec(compile(code, str(script), "exec"), ns)
            argv = sys.argv[:]
            sys.argv = [str(script), str(gen / "x0x_drum_samples.h")]
            ns["main"]()
            sys.argv = argv
            # Tables live next to DSP headers for #include "x0x_drum_tables.h".
            (shadow / "x0x_drum_tables.h").write_bytes((gen / "x0x_drum_tables.h").read_bytes())
            (shadow / "x0x_drum_samples.h").write_bytes((gen / "x0x_drum_samples.h").read_bytes())

            objects = []
            for name in ("drum909", "fxbus"):
                path = "firmware/src/dsp/" + name + ".c"
                src = out / (name + ".c")
                src.write_bytes((ROOT / path).read_bytes())
                obj = out / (name + ".o")
                run([cc, "-O2", "-ffp-contract=off", "-std=c99",
                     "-Wall", "-Wextra", "-Werror", "-Wdouble-promotion",
                     "-I" + str(shadow), "-I" + str(gen),
                     "-c", str(src), "-o", str(obj)])
                objects.append(str(obj))

            exe = out / "audio"
            # pot_packing_audio_test.c includes "dsp/drum909.h" — shadow parent is -I.
            run([cc, "-O2", "-ffp-contract=off",
                 "-I" + str(out / "shadow"), "-I" + str(gen),
                 str(ROOT / "tests/host/pot_packing_audio_test.c"), *objects, "-lm",
                 "-o", str(exe)])
            with (out / "audio.bin").open("wb") as f:
                run([str(exe)], stdout=f)
            data = (out / "audio.bin").read_bytes()
            results[kind] = {"bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()}

            if kind == "packed":
                dump = out / "dump_tanh.c"
                dump.write_text(
                    "#define X0X_DRUM_TABLES_DEFINE\n"
                    "#include \"x0x_drum_tables.h\"\n"
                    "#include <stdio.h>\n"
                    "#include <string.h>\n"
                    "int main(void) {\n"
                    "  for (unsigned i = 0; i < (unsigned)X0X_TANH_N + 2u; ++i) {\n"
                    "    float f = x0x_tanh_value(i);\n"
                    "    unsigned u; memcpy(&u, &f, 4);\n"
                    "    if (fwrite(&u, 4, 1, stdout) != 1) return 1;\n"
                    "  }\n"
                    "  return 0;\n"
                    "}\n"
                )
                dump_exe = out / "dump_tanh"
                run([cc, "-O2", "-std=c99", "-Wall", "-Werror",
                     "-I" + str(gen), str(dump), "-o", str(dump_exe)])
                c_bits = subprocess.check_output([str(dump_exe)], cwd=ROOT)
                expect = b"".join(struct.pack("<I", bits32(v)) for v in tab)
                if c_bits != expect:
                    print("C accessor bits != Python float table", file=sys.stderr)
                    return 1

        assert results["baseline"] == results["packed"], results
        print(json.dumps({
            "status": "PASS_BIT_IDENTICAL_STEREO_AUDIO",
            "base": BASE,
            "tanh_raw_b": raw_b,
            "tanh_packed_b": packed_b,
            "tanh_save_b": save_b,
            "scope": "every 909 and FX parameter, values -1..128, 16 stereo samples per setter; "
                     "plus every tanh table index bit-identical via x0x_tanh_value",
            "results": results,
        }, indent=2))
    return 0


if __name__ == "__main__":
    sys.exit(main())
