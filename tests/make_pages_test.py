#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Release identity checks and multi-version site generation for the web installer."""
import json
import shutil
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from make_pages import installer_model, main as make_pages_main, model_key  # noqa: E402

assert installer_model("FM-1_9001000") == "FM-1"
assert installer_model("SMK-37 Pro_9001000") == "SMK-37 Pro"
assert installer_model("SMK-37 Elite_9001000") == "SMK-37 Elite"
assert installer_model("MKE-P37_9001000") == "MKE-P37"
assert model_key("SMK-37 Elite_9001000") == "elite"
for invalid in ("FM-1_900", "SMK-37 Pro_900", "SMK-37+VA_9001000", "other_9001000"):
    try:
        installer_model(invalid)
    except SystemExit:
        pass
    else:
        raise AssertionError(f"accepted unsupported identity {invalid}")

html = (ROOT / "web/x0x_installer.html").read_text(encoding="utf-8")
assert 'id="stock"' in html and 'id="version"' in html and 'id="models"' in html
assert "/*CATALOG*/" in html and "/*LIB*/" in html
assert "mergeGitHubReleases" in html
assert "installKeepingSlots" in html or "/*LIB*/" in html
assert 'id="stockdrop"' in html

# Synthetic packages (identity + FELUCCA-LOADER-1)
BLOCKS, BLK, KEEP = 20, 0x30, 0x2F


def make_pkg(product: str, path: Path):
    raw = bytearray(0x2000)
    for i, ch in enumerate(product[:20]):
        raw[i * BLK + KEEP] = (ord(ch) + i + 1) & 0xFF
    for i in range(len(product), BLOCKS):
        raw[i * BLK + KEEP] = 0x7D
    raw[0x400:0x400 + 15] = b"FELUCCA-LOADER-1"
    path.write_bytes(raw)


with tempfile.TemporaryDirectory() as td:
    td = Path(td)
    fixtures = td / "fw"
    fixtures.mkdir()
    make_pkg("SMK-37 Pro_9002000", fixtures / "pro-0.2.fwsc")
    make_pkg("SMK-37 Elite_9002000", fixtures / "elite-0.2.fwsc")
    make_pkg("MKE-P37_9002000", fixtures / "mke-0.2.fwsc")
    make_pkg("SMK-37 Pro_9001000", fixtures / "pro-0.1.fwsc")
    make_pkg("SMK-37 Elite_9001000", fixtures / "elite-0.1.fwsc")

    out1 = td / "site1"
    make_pages_main([
        "--version", "0.1", "--out", str(out1),
        "--pro", str(fixtures / "pro-0.1.fwsc"),
        "--elite", str(fixtures / "elite-0.1.fwsc"),
    ])
    cat1 = json.loads((out1 / "versions.json").read_text())
    assert cat1["latest"] == "0.1"
    assert set(cat1["versions"][0]["models"]) == {"pro", "elite"}
    assert (out1 / "firmware" / "x0x37-0.1.fwsc").is_file()
    assert (out1 / "firmware" / "x0x37-0.1-elite.fwsc").is_file()
    assert (out1 / "install" / "index.html").is_file()
    assert (out1 / "breaks" / "index.html").is_file()
    inst = (out1 / "install" / "index.html").read_text(encoding="utf-8")
    assert "SMK-37 Pro_9001000" in inst
    assert "/*CATALOG*/" not in inst and "/*LIB*/" not in inst
    assert "productOf" in inst and "Updater" in inst

    out2 = td / "site2"
    make_pages_main([
        "--version", "0.2", "--out", str(out2),
        "--pro", str(fixtures / "pro-0.2.fwsc"),
        "--elite", str(fixtures / "elite-0.2.fwsc"),
        "--mke", str(fixtures / "mke-0.2.fwsc"),
        "--merge", str(out1),
    ])
    cat2 = json.loads((out2 / "versions.json").read_text())
    assert cat2["latest"] == "0.2"
    assert {v["version"] for v in cat2["versions"]} == {"0.1", "0.2"}
    assert (out2 / "firmware" / "x0x37-0.1.fwsc").is_file(), "merge must keep older firmware"
    assert (out2 / "firmware" / "x0x37-0.2-mke.fwsc").is_file()
    assert set(next(v for v in cat2["versions"] if v["version"] == "0.2")["models"]) == {"pro", "elite", "mke"}

    # Legacy single-package argv
    out3 = td / "site3"
    make_pkg("FM-1_9001000", fixtures / "fm1.fwsc")
    make_pages_main([str(fixtures / "fm1.fwsc"), "1.0", str(out3)])
    cat3 = json.loads((out3 / "versions.json").read_text())
    assert cat3["latest"] == "1.0"
    assert "fm1" in cat3["versions"][0]["models"]
    assert (out3 / "firmware" / "x0x-1.0.fwsc").is_file()
    assert "X0X37-for-Loopy-Pro" in (out2 / "index.html").read_text(encoding="utf-8")

    out_shell = td / "shell"
    make_pages_main(["--shell", "--out", str(out_shell)])
    assert (out_shell / "install" / "index.html").is_file()
    assert (out_shell / "breaks" / "index.html").is_file()
    assert (out_shell / "projects" / "index.html").is_file()
    assert (out_shell / "projects" / "x0x_projects.js").is_file()
    shell_cat = json.loads((out_shell / "versions.json").read_text())
    assert shell_cat["versions"] == []
    shell_inst = (out_shell / "install" / "index.html").read_text(encoding="utf-8")
    assert "noversion" in shell_inst and "stockdrop" in shell_inst
    assert "confirmWrite" in shell_inst or "confirm" in shell_inst

print("PASS installer identities, multi-version merge, sample-importer page, legacy FM-1 site, shell site")
