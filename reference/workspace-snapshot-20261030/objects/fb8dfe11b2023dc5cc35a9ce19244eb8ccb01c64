#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Regression: never reuse generated font bytes across changed renderer inputs.

Run with: python3 tests/gen_cache_renderer_closure_test.py
The baseline without renderer runtime in its cache key must fail the Pillow test.
This tests the real gen_cache fingerprint/store/restore paths without vendor SDK.
"""
import os
import subprocess
import sys
import tempfile
from pathlib import Path
from unittest.mock import patch

from PIL import features
import PIL

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import gen_cache  # noqa: E402


def check_runtime_inputs():
    rendered = gen_cache._font_renderer_inputs()
    assert rendered["pillow"] == PIL.__version__
    assert rendered["freetype2"] == (features.version_module("freetype2") or "unavailable")
    assert rendered["raqm-enabled"] == ("1" if features.check("raqm") else "0")
    assert rendered["raqm-version"] == (
        (features.version_feature("raqm") or "unavailable") if features.check("raqm") else "disabled"
    )


def tests():
    # Fixed simulated compiler/source/asset closure isolates only Pillow and its
    # rendering features, so different key hashes prove explicit invalidation.
    with patch.dict(os.environ, {"CC": "cc"}, clear=True), \
         patch.object(gen_cache, "_sources", return_value=[]), \
         patch.object(gen_cache, "_font_files", return_value=[]), \
         patch.object(gen_cache, "_wavs", return_value=[]), \
         patch.object(gen_cache.shutil, "which", return_value="/usr/bin/cc"), \
         patch.object(gen_cache.subprocess, "run", return_value=subprocess.CompletedProcess(
             ["cc", "--version"], 0, stdout="cc-test 1\n", stderr="")):
        first = gen_cache.fingerprint()
        assert first == gen_cache.fingerprint(), "identical closure must reuse key"
        with patch.object(PIL, "__version__", "9.4.0"):
            old = gen_cache.fingerprint()
        with patch.object(PIL, "__version__", "12.3.0"):
            new = gen_cache.fingerprint()
        assert old != new, "Pillow upgrade must invalidate generated headers"

        with patch.object(features, "version_module", return_value="2.12.0"):
            old_ft = gen_cache.fingerprint()
        with patch.object(features, "version_module", return_value="2.14.3"):
            new_ft = gen_cache.fingerprint()
        assert old_ft != new_ft, "FreeType change must invalidate font glyph cache"

        with patch.object(features, "check", return_value=False):
            no_raqm = gen_cache.fingerprint()
        with patch.object(features, "check", return_value=True):
            has_raqm = gen_cache.fingerprint()
        assert no_raqm != has_raqm, "raqm/no-raqm digit layout may differ"

        with patch.object(features, "check", return_value=True), \
             patch.object(features, "version_feature", return_value="0.9.0"):
            raqm_v1 = gen_cache.fingerprint()
        with patch.object(features, "check", return_value=True), \
             patch.object(features, "version_feature", return_value="0.10.5"):
            raqm_v2 = gen_cache.fingerprint()
        assert raqm_v1 != raqm_v2, "raqm implementation update must invalidate glyphs"

        with tempfile.TemporaryDirectory(prefix="x0x-gen-cache-closure-") as tmp:
            os.environ["X0X_GEN_CACHE"] = str(Path(tmp) / "cache")
            dest = Path(tmp) / "gen"
            dest.mkdir()
            original = {}
            for name in gen_cache.HEADERS:
                data = (name + " original bytes\n").encode()
                (dest / name).write_bytes(data)
                original[name] = data
            baseline = gen_cache.fingerprint()
            gen_cache.store(dest)
            for name in gen_cache.HEADERS:
                (dest / name).unlink()
            with patch.object(PIL, "__version__", "99.0.0"):
                assert not gen_cache.restore(dest), "stale version must be cache miss"
                assert not any(dest.iterdir()), "a miss cannot restore stale files"
            assert gen_cache.restore(dest), "matching runtime must restore cached headers"
            assert all((dest / n).read_bytes() == data for n, data in original.items())
            # Truncated/tampered entry must fail despite unchanged key.
            entry = Path(os.environ["X0X_GEN_CACHE"]) / baseline
            (entry / gen_cache.HEADERS[0]).write_bytes(b"corrupt")
            assert not gen_cache.restore(dest), "corrupted entry must never be accepted"
            del os.environ["X0X_GEN_CACHE"]

        with patch.dict(sys.modules, {"PIL": None}):
            try:
                gen_cache._font_renderer_inputs()
            except OSError:
                pass
            else:
                raise AssertionError("unavailable renderer metadata must fail closed")
    print("PASS: Pillow/FreeType/raqm closure changes miss, match restores, corruption rejected")


if __name__ == "__main__":
    check_runtime_inputs()
    tests()
