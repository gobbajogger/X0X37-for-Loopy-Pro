#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""No-device libc contract checks for exclusive atomic sample-backup rename."""
import importlib.util
import os
from pathlib import Path
import unittest
from unittest import mock

source = Path(os.environ.get("X0X_SLOTS_SOURCE", Path(__file__).resolve().parents[1] / "tools" / "x0x_slots.py"))
spec = importlib.util.spec_from_file_location("x0x_slots_rename_under_test", source)
slots = importlib.util.module_from_spec(spec)
spec.loader.exec_module(slots)

class _FakeFunction:
    def __init__(self):
        self.args = None
    def __call__(self, *args):
        self.args = args
        return 0

class _Library:
    def __init__(self, fn, name):
        setattr(self, name, fn)

class PlatformRenameTest(unittest.TestCase):
    def test_darwin_uses_native_cwd_descriptor_and_exclusive_flag(self):
        fn = _FakeFunction()
        with mock.patch.object(slots.sys, "platform", "darwin"), mock.patch("ctypes.CDLL", return_value=_Library(fn, "renameatx_np")):
            slots._rename_backup_noreplace(Path("sample.tmp"), Path("backup.x0xslots"))
        self.assertEqual((fn.args[0], fn.args[2], fn.args[4]), (-2, -2, 4))

    def test_linux_uses_native_cwd_descriptor_and_noreplace_flag(self):
        fn = _FakeFunction()
        with mock.patch.object(slots.sys, "platform", "linux"), mock.patch("ctypes.CDLL", return_value=_Library(fn, "renameat2")):
            slots._rename_backup_noreplace(Path("sample.tmp"), Path("backup.x0xslots"))
        self.assertEqual((fn.args[0], fn.args[2], fn.args[4]), (-100, -100, 1))

    def test_darwin_missing_exclusive_rename_fails_closed(self):
        with mock.patch.object(slots.sys, "platform", "darwin"), mock.patch("ctypes.CDLL", return_value=object()):
            with self.assertRaisesRegex(slots.SlotError, "cannot atomically publish"):
                slots._rename_backup_noreplace(Path("sample.tmp"), Path("backup.x0xslots"))

if __name__ == "__main__":
    unittest.main(verbosity=2)
