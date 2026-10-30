#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Smoke test for au_compare_align lag search."""
import math
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import au_compare_align


def _sine(n: int, phase: float = 0.0) -> list[float]:
    return [math.sin(2.0 * math.pi * 440.0 * i / 48000.0 + phase) for i in range(n)]


def main() -> int:
    ref = _sine(2000)
    shift = 37
    cand = [0.0] * shift + ref[:-shift]
    lag, r = au_compare_align.lag_aligned_correlation(ref, cand, lag_min=-64, lag_max=64)
    if lag != shift:
        print(f"FAIL lag expected {shift} got {lag}")
        return 1
    if r < 0.99:
        print(f"FAIL correlation expected >=0.99 got {r}")
        return 1
    print("ok au_compare_align lag search")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
