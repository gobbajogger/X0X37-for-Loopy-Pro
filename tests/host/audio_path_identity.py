#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Capture production audio-path identity, or compare with a pre-change capture.
Run both captures with the same compiler, flags and generated assets.
  python3 tests/host/audio_path_identity.py --output DIR [--baseline DIR]
No device access. A mismatch exits nonzero; raw files remain inspectable.
"""
import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import struct
import subprocess
import sys
import wave

SCENARIOS = ["all", "solo-mute", "mix-sweeps", "preset-fade",
             *[f"djf{m}-{state}" for m in (1, 2, 3) for state in ("static", "sweep")],
             "limiter-off", "comp", "usb-full", "usb-normal", "dense", "idle", "mono"]

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--output", required=True, type=Path)
    ap.add_argument("--baseline", type=Path)
    args = ap.parse_args()
    root = Path(__file__).resolve().parents[2]
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=True)
    compiler = os.environ.get("CC", "cc")
    # Reuse the production host build's source closure and exact floating flags.
    # Only its simulator entry point is replaced by this whole-engine fixture.
    wrapper = out / "compiler.py"
    wrapper.write_text("#!" + sys.executable + "\n"
                       "import os,sys\n"
                       "args = [os.environ.get('OPT_A_ENGINE_SOURCE', x) if x == "
                       "'firmware/src/app/engine.c' else "
                       "os.environ.get('OPT_A_MASTER_SOURCE', x) if x == "
                       "'firmware/src/dsp/master.c' else "
                       "'tests/host/audio_path_identity.c' if x == 'host/x0x_host.c' else x "
                       "for x in sys.argv[1:]]\n"
                       "os.execvp(os.environ['OPT_A_CC'], [os.environ['OPT_A_CC'], *args])\n")
    wrapper.chmod(0o755)
    env = dict(os.environ, CC=str(wrapper), OPT_A_CC=compiler,
               X0X_HOST_OUT=str(out / "identity"), X0X_DEV_WOBBLE="0")
    with (out / "build.log").open("w") as log:
        subprocess.run(["sh", "host/build_host.sh"], cwd=root, env=env,
                       stdout=log, stderr=subprocess.STDOUT, check=True)
    fixture = out / "fixture.wav"
    # A synthetic loop, independently generated and free of recorded sample inputs.
    with wave.open(str(fixture), "wb") as wav:
        wav.setparams((1, 2, 22050, 0, "NONE", "not compressed"))
        wav.writeframes(b"".join(struct.pack("<h", int(10000 * math.sin(2 * math.pi * 173 * i / 22050)))
                                 for i in range(44100)))
    hashes = {}
    for name in SCENARIOS:
        directory = out / name
        directory.mkdir(exist_ok=True)
        subprocess.run([str(out / "identity"), name, str(directory), str(fixture)],
                       cwd=root, check=True, stdout=subprocess.DEVNULL)
        for path in sorted(directory.iterdir()):
            hashes[f"{name}/{path.name}"] = {
                "sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
                "bytes": path.stat().st_size}
        print("captured", name, flush=True)
    (out / "hashes.json").write_text(json.dumps(hashes, indent=2) + "\n")
    if args.baseline:
        expected = json.loads((args.baseline / "hashes.json").read_text())
        changed = sorted(k for k in expected.keys() | hashes.keys() if expected.get(k) != hashes.get(k))
        if changed:
            print("FAIL: identity mismatch: " + ", ".join(changed), file=sys.stderr)
            return 1
        print(f"PASS: {len(SCENARIOS)} scenarios, {len(hashes)} audio/state files bit-identical")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
