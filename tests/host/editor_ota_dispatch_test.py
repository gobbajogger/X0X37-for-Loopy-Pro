#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Compile actual editor service + main dispatch block with the existing OTA fixture.

Only editor handlers/platform boundaries are sentinels. No hardware or packages.
SRC permits the same regression to exercise an untouched source snapshot.
"""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
SRC = Path(os.environ.get("SRC", ROOT / "firmware/src")).resolve()
editor = (SRC / "app/editor.c").read_text()
main = (SRC / "app/main_fm1.c").read_text()
# Fail closed on missing/ambiguous extraction markers, keeping production bodies intact.
signatures = [s for s in ("static void ed_service(void)", "static int ed_service(void)") if s in editor]
if len(signatures) != 1:
    raise SystemExit("expected one editor service signature")
signature = signatures[0]
service = editor[editor.index(signature):]
if service.count("ed_service(void)") != 1 or not service.rstrip().endswith("}"):
    raise SystemExit("unexpected editor service extent")
begin = main.index("        {   /* Project transfers cannot queue an update across COMMIT or ABORT. */")
end = main.index("#if X0X_CDC", begin)
block = main[begin:end]
if block.count("ed_service()") != 1 or block.count("ota_service()") != 1:
    raise SystemExit("unexpected main dispatch block")
# Stop before boot_screen.h: that header is for ED_BOOT_* handlers, not this
# dispatch fixture. Ending at BOARD_SMK37 used to work; boot-screen install
# commands put the include inside that window and broke the host compile.
definitions = editor[editor.index("#ifndef ED_MAINTENANCE_ADMIT"):editor.index('#include "boot_screen.h"')]
with tempfile.TemporaryDirectory(prefix="editor-ota-dispatch-") as td:
    out = Path(td)
    (out / "editor_service.inc").write_text(service)
    (out / "editor_definitions.inc").write_text(definitions)
    (out / "main_dispatch.inc").write_text(block)
    # Copy the unchanged fixture with its production OTA include pinned to SRC.
    fixture = (ROOT / "tests/ota_test.c").read_text()
    fixture = fixture.replace('#include "../firmware/src/ota.c"', f'#include "{SRC / "ota.c"}"')
    if str(SRC / "ota.c") not in fixture:
        raise SystemExit("OTA fixture include marker missing")
    (out / "ota_fixture.inc").write_text(fixture)
    argv = shlex.split(os.environ.get("CC", "cc")) + [
        "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", "-Wno-unused-function",
        "-Wno-unused-variable", "-Wno-sign-compare", "-DBOARD_SMK37=1", "-DT_MODEL=1",
        f"-DED_SERVICE_RETURNS_PENDING={int(signature.startswith('static int'))}",
        "-I", str(out), str(ROOT / "tests/host/editor_ota_dispatch_test.c"),
        "-o", str(out / "test"),
    ] + shlex.split(os.environ.get("SAN_FLAGS", ""))
    subprocess.run(argv, check=True)
    subprocess.run([str(out / "test")], check=True)
