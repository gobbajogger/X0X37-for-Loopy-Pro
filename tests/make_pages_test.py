#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Release identity checks for the generated web installer."""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from make_pages import installer_model

assert installer_model("FM-1_9001000") == "FM-1"
assert installer_model("SMK-37 Pro_9001000") == "SMK-37 Pro"
assert installer_model("SMK-37 Elite_9001000") == "SMK-37 Elite"
assert installer_model("MKE-P37_9001000") == "MKE-P37"
for invalid in ("FM-1_900", "SMK-37 Pro_900", "SMK-37+VA_9001000", "other_9001000"):
    try:
        installer_model(invalid)
    except SystemExit:
        pass
    else:
        raise AssertionError(f"accepted unsupported identity {invalid}")

html = (Path(__file__).resolve().parents[1] / "web/x0x_installer.html").read_text()
assert 'id="stock"' in html and 'id="recovery"' in html
assert 'if (!meta.stock) { $("stock").hidden = true; $("recovery").hidden = true; }' in html
print("PASS FM-1 and SMK release identities; FM-1-only stock restore hidden for SMK builds")
