#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Verify the Barlow renderer's tabular-number fallback without libraqm."""
import sys
import tempfile
from pathlib import Path
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import gen_font  # noqa: E402


def main():
    with tempfile.TemporaryDirectory() as tmp, patch.object(gen_font.PILFeatures, "check", return_value=False):
        output = Path(tmp) / "felucca_font.h"
        gen_font.main(output)
        header = output.read_text()
    start = header.index("static const uint8_t FONT_M_ADV[")
    values = header[start:].split("{", 1)[1].split("}", 1)[0]
    advances = [int(value.strip()) for value in values.split(",") if value.strip()]
    digits = advances[ord("0") - 32:ord("9") - 31]
    assert len(digits) == 10 and len(set(digits)) == 1, digits
    print("PASS font generation without libraqm retains tabular number cells")


if __name__ == "__main__":
    main()
