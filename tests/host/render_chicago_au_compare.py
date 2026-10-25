#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Chicago Acid AU vs native303 offline compare (C-SND-2 scaffold).

When CHICAGO_AU_REFS is unset or empty, exits 0 after printing a skip line
(run_tests.sh treats this via not-run on the env var).

When set, future revisions will:
  - render candidate PCM via x0x_host + scenario battery under tests/scenarios/chicago_au/
  - score pitch (<=2 cents), steady envelope (<=1 dB), strong spectrum (<=1 dB),
    waveform correlation (>=0.99) vs files under CHICAGO_AU_REFS
  - print F_agg for sign-off (>=90% min, 99% target)

Env:
  CHICAGO_AU_REFS  directory of AU reference WAVs (gitignored corpus)
"""
from __future__ import annotations

import os
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SCENARIO_DIR = ROOT / "tests" / "scenarios" / "chicago_au"


def main() -> int:
    refs = os.environ.get("CHICAGO_AU_REFS", "").strip()
    if not refs:
        print("skip chicago-au-compare (CHICAGO_AU_REFS unset)")
        return 0
    ref_dir = Path(refs)
    if not ref_dir.is_dir():
        print(f"FAIL chicago-au-compare: CHICAGO_AU_REFS not a directory: {ref_dir}", file=sys.stderr)
        return 1
    if not SCENARIO_DIR.is_dir():
        print(f"FAIL chicago-au-compare: missing {SCENARIO_DIR}", file=sys.stderr)
        return 1
    scenarios = sorted(SCENARIO_DIR.glob("chicago_au_*.x0x"))
    if not scenarios:
        print(
            "ok chicago-au-compare (stub: CHICAGO_AU_REFS set; "
            "no chicago_au_*.x0x scenarios yet — C-SND-1)"
        )
        return 0
    print(
        "FAIL chicago-au-compare: scenarios present but render+score not implemented (C-SND-2)",
        file=sys.stderr,
    )
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
