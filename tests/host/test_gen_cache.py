#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""The nightly header cache matches a fresh generate and skips the slow rebuild."""
import os
import shutil
import sys
import tempfile
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
os.environ["X0X_GEN_CACHE"] = tempfile.mkdtemp(prefix="x0x-gen-cache-")
os.environ.pop("X0X_BREAK_BANK", None)
os.environ.pop("X0X_FONTSET", None)
os.environ.pop("X0X_FONT_DIRS", None)

import build  # noqa: E402
import gen_cache  # noqa: E402


def timed(dest):
    build.GEN = Path(dest)
    if build.GEN.exists():
        shutil.rmtree(build.GEN)
    started = time.perf_counter()
    build.generate()
    return time.perf_counter() - started


def main():
    work = Path(tempfile.mkdtemp(prefix="x0x-gen-work-"))
    cold_dir = work / "cold"
    cold = timed(cold_dir)
    warm_dir = work / "warm"
    warm = timed(warm_dir)
    for name in gen_cache.HEADERS:
        cold_bytes = (cold_dir / name).read_bytes()
        warm_bytes = (warm_dir / name).read_bytes()
        if not cold_bytes or cold_bytes != warm_bytes:
            raise SystemExit(f"{name}: cached header does not match a fresh generate")
    print(f"gen-cache cold {cold:.3f}s warm {warm:.3f}s")
    if not warm * 3 < cold:
        raise SystemExit(f"cached generate was not faster: cold {cold:.3f}s warm {warm:.3f}s")

    cached = Path(os.environ["X0X_GEN_CACHE"]) / gen_cache.fingerprint()
    (cached / "felucca_font.h").write_bytes(b"damaged")
    if gen_cache.restore(work / "damaged"):
        raise SystemExit("a damaged cache entry was reused")

    os.environ["X0X_FONTSET"] = "terminus"
    other = work / "terminus"
    timed(other)
    if (other / "felucca_font.h").read_bytes() == (cold_dir / "felucca_font.h").read_bytes():
        raise SystemExit("font set change reused the previous header")
    print("PASS gen cache")


if __name__ == "__main__":
    main()
