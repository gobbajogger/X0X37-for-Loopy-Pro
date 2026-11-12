#!/usr/bin/env python3
"""Guard: engine_brk_loops must not mute the break while rewriting loops (#354).

Chicago land f701ae34 reintroduced brk_hold around engine_brk_loops and dropped
breaks_flush. Any audio ISR between hold=1 and hold=0 skips break render for a
whole buffer — audible dropout when changing loops. Owner: gap must be gone.
"""
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
    if re.search(r"\bbrk_hold\b", text):
        fail("engine.c still has brk_hold — mute around loop rewrite is back")

    m = re.search(
        r"X0X_COLD\s+void\s+engine_brk_loops\s*\(\s*void\s*\)\s*\{(.*?)^\}",
        text,
        re.S | re.M,
    )
    if not m:
        fail("engine_brk_loops not found")
    body = m.group(1)
    if "breaks_flush" not in body:
        fail("engine_brk_loops must call breaks_flush so deferred bank publish lands on device")
    if re.search(r"brk_hold\s*=\s*1", body):
        fail("engine_brk_loops must not arm brk_hold")

    # Step / live / render paths must not gate on a hold flag.
    if re.search(r"if\s*\(\s*brk_hold\s*\)", text):
        fail("s_brk (or similar) still early-returns on brk_hold")
    if re.search(r"C_BRK\s*&&\s*!brk_hold", text):
        fail("C_BRK live path still gated on !brk_hold")
    if re.search(r"if\s*\(\s*!brk_hold\b", text):
        fail("render/mix still gated on !brk_hold")

    print("PASS: engine_brk_loops seamless (no brk_hold; breaks_flush present)")


if __name__ == "__main__":
    main()
