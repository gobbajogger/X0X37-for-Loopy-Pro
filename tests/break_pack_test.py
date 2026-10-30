#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""The optional break pack: full loops, a free tail, and the same bytes in C and Python."""
import struct
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import break_pack as bp  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]
fail = 0


def check(cond, what):
    global fail
    print(f"{what:64} {'ok' if cond else 'FAIL'}")
    if not cond:
        fail += 1


def main():
    hdr = ROOT / "build" / "empty-break-bank.h"
    bp.write_empty_header(hdr)
    text = hdr.read_text()
    check("X0X_NBREAKS 0" in text and "X0X_BREAK_0" not in text, "default header has no loop bytes")

    adpcm = bytes([i & 0xFF for i in range(40)])
    pack = bp.encode_pack([{"name": "TONE", "adpcm": adpcm, "nsamples": 80, "bars": 1}])
    parsed = bp.parse_pack(pack)
    check(parsed is not None and parsed[1][0]["adpcm"] == adpcm, "pack keeps the whole ADPCM")
    check(parsed[1][0]["nsamples"] == 80 and parsed[1][0]["bars"] == 1, "pack keeps length and bars")
    check(bp.parse_pack(pack[:-1]) is None, "a cut pack does not parse")

    code_max = 4096
    app = bytearray(b"\xA5" * 100 + b"\x00\xff\xff\xff" + b"\xff" * (code_max - 104))
    placed = bp.place_pack(app, pack, code_max)
    check(placed[:100] == b"\xA5" * 100 and placed[100] == 0, "the image and its tail mark stay")
    check(len(placed) == code_max, "place stays inside the code region")
    again = bp.parse_pack_prefix(placed)
    # the pack is not at offset 0
    found = None
    for o in range(0, code_max - 20, 16):
        got = bp.parse_pack_prefix(placed[o:])
        if got and all(b == 0xFF for b in placed[o + got[2]:code_max]):
            found = got
            break
    check(found is not None and found[1][0]["name"] == "TONE", "the pack sits in the free tail")
    check(placed[code_max - 1] == 0xFF or placed[code_max - 1] in range(256), "no write past code_max")
    try:
        bp.place_pack(bytearray(b"\x00" + b"\xff" * 30), pack, 32)
        check(False, "a pack bigger than the tail is refused")
    except ValueError:
        check(True, "a pack bigger than the tail is refused")

    export = ROOT / "assets" / "factory-breaks"
    if (export / "pack.txt").is_file():
        loops = bp.loops_from_export(export)
        factory = bp.encode_pack(loops)
        got = bp.parse_pack(factory)
        check(got is not None and [L["name"] for L in got[1]] == ["909 GR", "909 FL"], "factory names")
        check(all(L["nsamples"] == 220500 and L["bars"] == 2 for L in got[1]), "factory loops are 5 seconds, 2 bars")
        check(got[1][0]["adpcm"] != got[1][1]["adpcm"][:32] or got[1][0]["adpcm"][:32] != got[1][1]["adpcm"][:32],
              "the two loops do not share a prefix")
        check(got[1][0]["adpcm"] == (export / "909GR.adpcm").read_bytes(), "pack is the generator's ADPCM")
        raw = (export / "factory-break-pack.bin").read_bytes()
        check(raw == factory, "the saved factory pack matches")
    else:
        check(False, "factory export is present")

    blob = ROOT / "build" / "brk-pack-blob.bin"
    blob.write_bytes(placed)
    exe = ROOT / "build" / "brk_pack_test"
    src = ROOT / "tests" / "brk_pack_test.c"
    c = subprocess.run(["cc", "-O2", "-Wall", "-Ifirmware/src", "-o", str(exe), str(src)],
                       capture_output=True, text=True)
    check(c.returncode == 0, "C parser compiles")
    if c.returncode == 0:
        r = subprocess.run([str(exe), str(blob)], capture_output=True, text=True)
        check(r.returncode == 0 and r.stdout.splitlines()[:1] == ["1 TONE 80 1 40"], "C parser reads the same pack")
        if r.returncode != 0:
            print(r.stdout, r.stderr)
    else:
        print(c.stderr)
    return fail


if __name__ == "__main__":
    sys.exit(main())
