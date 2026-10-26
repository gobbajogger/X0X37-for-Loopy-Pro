#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
from pathlib import Path
import subprocess, hashlib

root = Path(__file__).resolve().parents[2]
q = root / "build/wobble-independent-mix"
q.mkdir(parents=True, exist_ok=True)

engine = (root / "firmware/src/app/engine.c").read_text()
wrapper = q / "exact_engine_wrapper.c"
wrapper.write_text(
    '#include "'
    + str(root / "firmware/src/app/engine.c")
    + '"\nvoid qa_stereo(void){wobble_set(&dev_wobble,W_STEREO,1.0f);}\n'
)

dev_decl = "static wobble_t dev_wobble POOL;"
if dev_decl not in engine:
    raise SystemExit("FAIL: unexpected dev_wobble declaration in engine.c")
ref = engine.replace(dev_decl, dev_decl + "\nstatic float qa_right[2][256];")
ref = ref.replace("bb[1 - b]", "qa_right[b]")
if "bb[1 - b]" in ref:
    raise SystemExit("FAIL: bb[1 - b] still present after reference rewrite")
ref = (
    ref.replace('#include "../', '#include "')
    .replace('#include "engine.h"', '#include "app/engine.h"')
    .replace('#include "plat.h"', '#include "app/plat.h"')
)
ref += "\nvoid qa_stereo(void){wobble_set(&dev_wobble,W_STEREO,1.0f);}\n"
(q / "reference_engine.c").write_text(ref)

host = (root / "host/x0x_host.c").read_text().replace(
    '"../firmware/', '"' + str(root / "firmware") + "/"
)
host = host.replace(
    '        if (!strcmp(cmd, "wait"))',
    '        if (!strcmp(cmd,"qa_stereo")) qa_stereo();\n        else if (!strcmp(cmd, "wait"))',
)
host = "void qa_stereo(void);\n" + host
(q / "qa_host.c").write_text(host)

scenario_path = root / "tests/scenarios/synth_engine_regression.x0x"
if scenario_path.is_file():
    sc = scenario_path.read_text()
else:
    sc = """wait 300
load 0
wav adapter-equivalence.wav
param 2 0 0 99
param 2 0 1 84
param 2 0 8 96
param 2 0 9 2
param 2 1 5 127
param 2 1 0 110
param 2 1 1 90
key w2 down
wait 160
key w6 down
wait 90
key w2 up
wait 90
key w6 up
wait 600
load 99
key w4 down
wait 400
key w4 up
wait 1200
load 0
param 2 0 12 100
param 2 0 13 80
param 2 0 14 110
key w3 down
wait 600
key w3 up
wait 1700
wavstop
"""
sc = sc.replace("load 0", "load 0\nqa_stereo", 1)
sc = sc.replace("turn ALGO 2\n", "")
sc = sc.replace("tap PLAY\nwait 2000\ntap PLAY\n", "")
(q / "side-sends.x0x").write_text(sc)

units = "firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c firmware/src/dsp/x0x_param.c firmware/src/dsp/bass303.c firmware/src/dsp/breaks.c firmware/src/dsp/fxbus.c firmware/src/dsp/master.c firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c firmware/src/seq/pattern.c firmware/src/seq/motion.c experiments/wobble/wobble.c".split()
flags = [
    "-O2",
    "-ffp-contract=off",
    "-std=c99",
    "-Wall",
    "-Wextra",
    "-Wno-unused-function",
    "-Wno-unused-parameter",
    "-DX0X_HOST",
    "-D_DARWIN_C_SOURCE",
    "-DX0X_DEV_PADCAL=1",
    "-DX0X_DEV_WOBBLE=1",
    "-Ifirmware/src",
    "-Ifirmware/src/app",
    "-Ifirmware/src/dsp",
    "-Ibuild/gen",
]
for name, src in [("side_exact", wrapper), ("side_reference", q / "reference_engine.c")]:
    exe = q / name
    out = q / (name + "-render")
    out.mkdir(exist_ok=True)
    subprocess.run(
        ["clang"] + flags + [str(q / "qa_host.c")] + units + [str(src), "-lm", "-o", str(exe)],
        check=True,
    )
    subprocess.run([str(exe), str(q / "side-sends.x0x"), str(out)], check=True)
a = (q / "side_exact-render/adapter-equivalence.wav").read_bytes()
b = (q / "side_reference-render/adapter-equivalence.wav").read_bytes()
assert a == b
print(
    "PASS exact sequential scratch mixer equals independent dedicated planar arrays with nonzero side and sends:",
    len(a),
    hashlib.sha256(a).hexdigest(),
)
