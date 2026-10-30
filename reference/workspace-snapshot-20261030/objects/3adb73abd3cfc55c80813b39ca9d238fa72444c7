#!/usr/bin/env python3
"""Guard: engine_brk_outline must clamp abs(-32768) before uint8 store."""
from __future__ import annotations

import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
ENGINE = ROOT / "firmware/src/app/engine.c"


def fail(msg: str) -> None:
    print(f"FAIL: {msg}", file=sys.stderr)
    raise SystemExit(1)


def main() -> None:
    text = ENGINE.read_text()
    if "brk_outline_peak_u8" not in text:
        fail("engine.c missing brk_outline_peak_u8 clamp helper")

    m = re.search(
        r"static\s+uint8_t\s+brk_outline_peak_u8\s*\(\s*int32_t\s+pk\s*\)\s*\{(.*?)\}",
        text,
        re.S,
    )
    if not m:
        fail("brk_outline_peak_u8 definition not found")
    body = m.group(1)
    if "32768" not in body or "255" not in body:
        fail("brk_outline_peak_u8 must clamp pk >= 32768 to 255")

    m = re.search(
        r"X0X_COLD\s+int\s+engine_brk_outline\s*\(.*?\)\s*\{(.*?)^\}",
        text,
        re.S | re.M,
    )
    if not m:
        fail("engine_brk_outline not found")
    fn = m.group(1)
    if fn.count("brk_outline_peak_u8") < 2:
        fail("engine_brk_outline must store peaks through brk_outline_peak_u8 twice")
    if re.search(r"peaks\[[^\]]+\]\s*=\s*\(uint8_t\)\s*\(\s*pk\s*>>\s*7\s*\)", fn):
        fail("engine_brk_outline still has unclamped (uint8_t)(pk >> 7) store")

    print("PASS: engine_brk_outline clamps negative full-scale peaks")


if __name__ == "__main__":
    main()
