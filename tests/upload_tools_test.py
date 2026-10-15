#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""tools/upload_breaks.py and tools/fm1_sample_upload.py without hardware:
- upload_breaks.py against tests/x0x_slots_test.py's mocked device: it takes the slot count and
  size from SMP_INFO (no fixed slots 1,2,3), writes 44.1 kHz by default and 22.05 kHz with --rate;
- the one-shot loader trims the silence by default (--no-trim keeps it); loops are not trimmed.
  tests/upload_tools_test.py"""
import contextlib
import io
import math
import struct
import sys
from pathlib import Path

SRC = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SRC / "tools"))
sys.path.insert(0, str(SRC / "tests"))
import sampleio as sio  # noqa: E402
import x0x_slots as xs  # noqa: E402
import fm1_sample_upload as up  # noqa: E402
import upload_breaks as ub  # noqa: E402
from x0x_slots_test import MockDevice  # noqa: E402

FAILS = 0
OUT = SRC / "build" / "host" / "upload_tools"


def check(cond, what):
    global FAILS
    print(f"  {'ok  ' if cond else 'FAIL'} {what}")
    FAILS += not cond


def wav(path, seconds, sr=44100, pad=0.0, f=180.0):
    n, npd = int(sr * seconds), int(sr * pad)
    x = [0.0] * npd + [0.7 * math.sin(2 * math.pi * f * i / sr) * (1.0 if (i // (sr // 8)) % 2 == 0 else 0.3)
                       for i in range(n)] + [0.0] * npd
    pcm = b"".join(struct.pack("<h", int(v * 32767)) for v in x)
    path.write_bytes(b"RIFF" + struct.pack("<I", 36 + len(pcm)) + b"WAVEfmt " +
                     struct.pack("<IHHIIHH", 16, 1, 1, sr, sr * 2, 2, 16) + b"data" + struct.pack("<I", len(pcm)) + pcm)
    return path


def run_upload(dev, args):
    up.Link = lambda: dev                         # the mocked device instead of mido
    old = sys.argv
    sys.argv = ["upload_breaks.py"] + args
    try:
        with contextlib.redirect_stdout(io.StringIO()):
            ub.main()
    finally:
        sys.argv = old


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    loops = [wav(OUT / f"l{k}.wav", 2.0) for k in range(3)]       # one bar at 120 BPM each: 88200 samples at 44.1 kHz

    dev = MockDevice(3, 120 * 1024)                                # a layout other than 2 or 3 x 80 KiB
    run_upload(dev, [str(p) for p in loops])
    hs = [xs.parse_header(bytes(dev.flash[k][:xs.HDR_LEN])) for k in range(3)]
    zs = [z for h in hs if h for z in h["zones"]]
    check([h["nz"] if h else 0 for h in hs] == [2, 1, 0] and len(zs) == 3 and all(z["rate"] == 44100 and z["n"] == 88200 for z in zs),
          "3 x 120 KiB device: 2 + 1 loops of 44.1 kHz (two 43 KiB loops fit one 120 KiB slot; layout from SMP_INFO)")
    check(dev.log[0] == xs.CMD_INFO, "upload_breaks.py asks SMP_INFO first")

    dev = MockDevice(2, 80 * 1024)
    run_upload(dev, ["--rate", "22050"] + [str(p) for p in loops])
    h = xs.parse_header(bytes(dev.flash[0][:xs.HDR_LEN]))
    h2 = xs.parse_header(bytes(dev.flash[1][:xs.HDR_LEN]))
    check(h and h["nz"] == 3 and all(z["rate"] == 22050 for z in h["zones"]) and h2 is None,
          "2 x 80 KiB device, --rate 22050: three 22.05 kHz loops in slot 1, slot 2 untouched")

    dev = MockDevice(2, 80 * 1024)
    run_upload(dev, ["--slots", "2", str(loops[0])])
    check(xs.parse_header(bytes(dev.flash[0][:xs.HDR_LEN])) is None and dev.nz[1] == 1, "--slots 2: only slot 2 written")

    # one-shots: trimmed by default; loops: not trimmed unless --trim
    shot = wav(OUT / "shot.wav", 0.3, pad=0.5)
    h_t, _ = up.build("SHOT", [str(shot)])
    h_n, _ = up.build("SHOT", [str(shot)], trim=False)
    nt, nn = struct.unpack_from("<I", h_t, 36)[0], struct.unpack_from("<I", h_n, 36)[0]
    check(nt < nn * 0.4 and abs(nt - (0.3 + 0.006) * sio.SLOT_RATE) < 30, f"one-shot: trimmed by default ({nn} -> {nt} samples)")
    full = ub.cut(shot, False)
    trimmed = ub.cut(shot, False, 44100, True)
    check(len(full) == len(sio.resample(sio.read_any_wav(shot)[1], 44100, 44100)) and len(trimmed) < len(full),
          f"loops: not trimmed by default ({len(full)} samples), --trim cuts ({len(trimmed)})")
    print("upload tools: all ok" if not FAILS else "UPLOAD TOOLS FAILED")
    return 1 if FAILS else 0


if __name__ == "__main__":
    sys.exit(main())
