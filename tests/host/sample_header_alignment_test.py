#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Check the actual slots accessor and its debug misalignment trap."""
import os
from pathlib import Path
import resource
import signal
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
compiler = os.environ.get("CC", "cc")
version = subprocess.run([compiler, "--version"], capture_output=True, text=True, check=True).stdout
alignment_warning = "-Wcast-align" if "clang" in version.lower() else "-Wcast-align=strict"
unit = r'''
#include <stdint.h>
#include <assert.h>
#define BOARD_SMK37 1
#define X0X_HOST 1
static uint8_t image[0x100000] __attribute__((aligned(4)));
#define SMP_USER_XIP(k) (image + SMP_USER_OFF(k))
#include "firmware/src/app/slots.c"
int main(int argc, char **argv) {
    (void)argv;
    const uint8_t *p = image + SMP_USER_OFF(0);
    if (argc > 1) { (void)smp_hdr(p + 1); return 1; }
    assert((const void *)smp_hdr(p) == (const void *)p);
    p = image + SMP_USER_OFF(1);
    assert((const void *)smp_hdr(p) == (const void *)p);
    return 0;
}
'''
with tempfile.TemporaryDirectory(prefix="sample-align-") as temporary:
    path = Path(temporary)
    (path / "test.c").write_text(unit)
    subprocess.run([compiler, "-std=c11", "-O2", "-Wall", "-Wextra",
                    "-Wshadow", alignment_warning, "-Werror", "-Wno-unused-function",
                    "-I", str(ROOT), str(path / "test.c"), "-o", str(path / "test")], check=True)
    subprocess.run([str(path / "test")], check=True)
    bad = subprocess.run([str(path / "test"), "misaligned"])
    assert bad.returncode in (-signal.SIGILL, -signal.SIGABRT), bad.returncode
    print("sample header: both slots aligned; misalignment trapped PASS")
