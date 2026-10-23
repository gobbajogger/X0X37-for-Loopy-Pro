#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Controller package writer must never damage its input on interrupted output."""
import contextlib
import io
import os
import sys
import tempfile
from pathlib import Path
from unittest.mock import patch

SRC = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SRC / "tools"))
import add_ctl  # noqa: E402


def verify(condition, description):
    if not condition:
        raise AssertionError(description)
    print("PASS", description)


def main():
    with tempfile.TemporaryDirectory(prefix="x0x-ctl-atomic-") as tmp:
        folder = Path(tmp)
        pkg = folder / "original.fwsc"
        ctl = folder / "input.ctl.img"
        before = b"VALIDATED-ORIGINAL-INPUT-PACKAGE" * 30
        after = b"VALIDATED-NEW-CONTROLLER-PACKAGE" * 32
        ctl.write_bytes(b"test-controller")
        command = [str(pkg), str(ctl), "-o", str(pkg)]
        pkg.write_bytes(before)

        with patch.object(add_ctl, "add", return_value=after):
            with patch("os.replace", side_effect=OSError("injected rename failure")) as replace:
                with contextlib.redirect_stderr(io.StringIO()):
                    rc = add_ctl.main(command)
            verify(rc == 1 and replace.called, "failed atomic publication must be reported")
            verify(pkg.read_bytes() == before, "failed publication never truncates original firmware")
            verify(not list(folder.glob(".original.fwsc.*.tmp")), "failed publication cleans temporary files")

            with patch("os.fsync", side_effect=OSError("injected disk sync failure")) as sync:
                with contextlib.redirect_stderr(io.StringIO()):
                    rc = add_ctl.main(command)
            verify(rc == 1 and sync.called, "disk flush failure must abort before publishing")
            verify(pkg.read_bytes() == before, "disk flush failure preserves original firmware")
            verify(not list(folder.glob(".original.fwsc.*.tmp")), "sync failure cleans temporary files")

            with contextlib.redirect_stdout(io.StringIO()):
                rc = add_ctl.main(command)
            verify(rc == 0 and pkg.read_bytes() == after, "successful in-place controller package replace")
            verify(not list(folder.glob(".original.fwsc.*.tmp")), "successful replace cleans temporary files")

            out = folder / "different.fwsc"
            out.write_bytes(before)
            with contextlib.redirect_stdout(io.StringIO()):
                rc = add_ctl.main([str(pkg), "--remove", "-o", str(out)])
            verify(rc == 0 and out.read_bytes() == after,
                   "explicit output path safely receives validated package bytes")

    print("add_ctl_atomic_test: PASS")


if __name__ == "__main__":
    main()
