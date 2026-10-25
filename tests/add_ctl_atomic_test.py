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

            # The rename has already committed before parent-directory finalization.
            # A nonzero result must NOT suggest the old package was preserved.
            original_fsync = os.fsync
            calls = [0]

            def fail_post_replace_sync(fd):
                calls[0] += 1
                if calls[0] == 2:
                    raise OSError("injected parent directory fsync failure")
                return original_fsync(fd)

            pkg.write_bytes(before)
            failure_text = io.StringIO()
            with patch("os.fsync", side_effect=fail_post_replace_sync):
                with contextlib.redirect_stderr(failure_text):
                    rc = add_ctl.main(command)
            verify(rc == 1 and calls[0] == 2 and pkg.read_bytes() == after,
                   "failed directory fsync occurs after replacement and keeps new output")
            verify("replacement completed" in failure_text.getvalue()
                   and "durability is uncertain" in failure_text.getvalue(),
                   "post-replace fsync failure explicitly warns about uncertain durability")
            verify(not list(folder.glob(".original.fwsc.*.tmp")),
                   "post-replace fsync failure leaves no temporary file")

            # Parent directory may also fail to open after rename.
            original_open = os.open
            def fail_parent_directory_open(path, flags, *args, **kwargs):
                if Path(path) == folder:
                    raise OSError("injected parent directory open failure")
                return original_open(path, flags, *args, **kwargs)

            pkg.write_bytes(before)
            failure_text = io.StringIO()
            with patch("os.open", side_effect=fail_parent_directory_open):
                with contextlib.redirect_stderr(failure_text):
                    rc = add_ctl.main(command)
            verify(rc == 1 and pkg.read_bytes() == after,
                   "failed directory open occurs after replacement and keeps new output")
            verify("replacement completed" in failure_text.getvalue()
                   and "durability is uncertain" in failure_text.getvalue(),
                   "post-replace directory open failure warns about uncertain durability")
            verify(not list(folder.glob(".original.fwsc.*.tmp")),
                   "post-replace directory open failure leaves no temporary file")

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
