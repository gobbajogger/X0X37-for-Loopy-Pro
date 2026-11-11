#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Exercise the real UTF-16 USB product descriptor and exact model build flags."""
import importlib.util
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
TOOLS = ROOT / "tools"
sys.path.insert(0, str(TOOLS))
spec = importlib.util.spec_from_file_location("x0x_usb_build", TOOLS / "build.py")
build = importlib.util.module_from_spec(spec)
spec.loader.exec_module(build)

HARNESS = r'''
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "usb_product_name.h"
#ifndef EXPECTED
#error EXPECTED is required
#endif
int main(void)
{
    const uint8_t *desc = (const uint8_t *)&STR2;
    const char *name = EXPECTED;
    size_t n = strlen(name), i;
    if (sizeof STR2 != 2u + 2u * n || desc[0] != sizeof STR2 || desc[1] != 3u)
        return 1;
    for (i = 0; i < n; i++)
        if (desc[2u + 2u * i] != (uint8_t)name[i] || desc[3u + 2u * i])
            return 2;
    printf("PASS USB descriptor: %s (%u bytes)\\n", name, desc[0]);
    return 0;
}
'''


def compile_one(source, dest, model, version=None, expected=None):
    args = ["cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-DBOARD_SMK37=1",
            f"-DX0X_USB_37{model.upper()}=1", "-Ifirmware/src"]
    if version is not None:
        args.append(f'-DX0X_VERSION="{version}"')
    if expected is not None:
        args.append(f'-DEXPECTED="{expected}"')
    args.extend([str(source), "-o", str(dest)])
    return subprocess.run(args, cwd=ROOT, capture_output=True, text=True)


def main():
    assert build.usb_product_define("smk37", "pro") == "-DX0X_USB_37PRO=1"
    assert build.usb_product_define("smk37", "elite") == "-DX0X_USB_37ELITE=1"
    assert build.usb_product_define("smk37", "mke") is None
    assert build.usb_product_define("fm1", "pro") is None

    with tempfile.TemporaryDirectory(prefix="x0x-usb-names-") as tmp:
        source, binary = Path(tmp) / "check.c", Path(tmp) / "check"
        source.write_text(HARNESS, encoding="utf-8")
        for model in ("pro", "elite"):
            for version in (None, "0.1.3", "99.99.99"):
                label = version if version is not None else "DEV"
                expected = f"DnBOX {label} 37{model.upper()}"
                result = compile_one(source, binary, model, version, expected)
                assert result.returncode == 0, result.stderr
                result = subprocess.run([str(binary)], cwd=ROOT, capture_output=True, text=True)
                assert result.returncode == 0, result.stdout + result.stderr
                assert expected in result.stdout, result.stdout
                print(result.stdout.strip())
        too_long = "1" * 125
        result = compile_one(source, binary, "elite", too_long, f"DnBOX {too_long} 37ELITE")
        assert result.returncode != 0 and "USB product name exceeds" in result.stderr, result.stderr
        print("PASS oversized USB product name rejected at compile time")

    usb = (ROOT / "firmware/src/usb.c").read_text(encoding="utf-8")
    assert '#include "usb_product_name.h"' in usb
    assert '*d = (const uint8_t *)&STR2;' in usb
    assert '0x09, 0x12, FELUCCA_USB_PID & 0xFF' in usb  # existing VID/PID unchanged
    assert "'F', 0, 'e', 0, 'l', 0, 'u'" in usb  # loader name unchanged
    print("PASS USB wiring, unchanged VID/PID and loader descriptor")


if __name__ == "__main__":
    main()
