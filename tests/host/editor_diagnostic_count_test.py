#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""The production count assertions must accept 127 and reject 128 words."""
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "firmware/src/app/editor.c").read_text()
checks = re.findall(r'_Static_assert\([^;]+"ED_BT_TEST op [34] count fits 7 bits"\);', source)
assert len(checks) == 2, "both diagnostic count assertions are required"
with tempfile.TemporaryDirectory(prefix="editor-count-") as temporary:
    path = Path(temporary)
    # (timer words, stopped-core words, canary words, accepted)
    for timer, core, canary, accepted in ((126, 63, 64, True), (127, 63, 64, False),
                                          (126, 64, 64, False)):
        unit = ("#include <stdint.h>\n"
                f"static uint32_t fm1_t5w_prev[{timer}], fm1_c0_dead[{core}], fm1_can[{canary}];\n"
                + "\n".join(checks) + "\n")
        (path / "test.c").write_text(unit)
        result = subprocess.run([os.environ.get("CC", "cc"), "-std=c11", "-c",
                                 str(path / "test.c"), "-o", str(path / "test.o")],
                                capture_output=True, text=True)
        assert (result.returncode == 0) == accepted, result.stderr
        if not accepted:
            assert "static assertion failed" in result.stderr, result.stderr
    print("editor diagnostics: count 127 accepted; both count-128 variants rejected PASS")
