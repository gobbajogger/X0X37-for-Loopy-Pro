#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Lossless font storage: byte-run boundaries and production rendering equivalence."""
import os
from pathlib import Path
import random
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import gen_font  # noqa: E402


def unpack(data):
    out, i = [], 0
    while i < len(data):
        token = data[i]
        i += 1
        count = (token & 127) + 1
        if token & 128:
            out.extend([data[i]] * count)
            i += 1
        else:
            out.extend(data[i:i + count])
            i += count
    assert i == len(data)
    return out


def main():
    rng = random.Random(9327)
    for count in (0, 1, 2, 3, 127, 128, 129, 255, 256, 257, 600):
        for source in ([0] * count, [255] * count, [i & 255 for i in range(count)],
                       [rng.randrange(256) for _ in range(count)]):
            assert unpack(gen_font.pack_glyph(source)) == source
    for fontset in ("barlow", "terminus"):
        gen_font.FONTSET = fontset
        gen_font.SIZES = gen_font.FONTSETS[fontset]
        with tempfile.TemporaryDirectory(prefix="x0x-font-") as directory:
            path = Path(directory)
            gen_font.main(path / "felucca_font.h")
            gen_font.main(path / "font_raw.h", compressed=False)
            original = (path / "font_raw.h").read_text()
            packed = (path / "felucca_font.h").read_text()
            size = lambda text: sum(map(int, re.findall(r"_DATA\[(\d+)\]", text)))
            assert size(packed) < size(original)
            if fontset == "barlow":
                assert size(original) - size(packed) >= 16000
            (path / "font_raw.h").write_text(original.replace("FONT_", "RAW_FONT_"))
            executable = path / "font_pack_test"
            subprocess.run([os.environ.get("CC", "cc"), "-std=c99", "-O2", "-Wall", "-Wextra", "-Werror",
                            "-Wno-unused-function", "-I" + directory, str(ROOT / "tests/host/font_pack_test.c"),
                            "-o", str(executable)], check=True)
            subprocess.run([str(executable)], check=True)
            print(f"{fontset}: {size(original) - size(packed)} bytes saved; all rendered pixels retained")


if __name__ == "__main__":
    main()
