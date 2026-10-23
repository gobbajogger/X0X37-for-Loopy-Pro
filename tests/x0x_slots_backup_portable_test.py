#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Native no-device tests for linkless atomic backup and fail-closed handling."""
import errno
import importlib.util
import os
from pathlib import Path
import tempfile
import unittest
from unittest import mock

source = Path(os.environ.get("X0X_SLOTS_SOURCE", Path(__file__).resolve().parents[1] / "tools" / "x0x_slots.py"))
spec = importlib.util.spec_from_file_location("x0x_slots_atomic_under_test", source)
slots = importlib.util.module_from_spec(spec)
spec.loader.exec_module(slots)


class PortableBackupTest(unittest.TestCase):
    def setUp(self):
        self.work = tempfile.TemporaryDirectory()
        self.addCleanup(self.work.cleanup)
        self.root = Path(self.work.name)
        self.path = self.root / "samples.x0xslots"
        lay = {"count": 1, "size": 81920, "slots": [{"size": 81920, "nz": 0, "off": None}]}
        self.a = slots.make_archive(lay, [None], "2026-10-23T00:00:00Z")
        self.b = slots.make_archive(lay, [None], "2026-10-24T00:00:00Z")
        slots.read_archive(self.a)
        slots.read_archive(self.b)

    def stages(self):
        return list(self.root.glob(".samples.x0xslots.*.tmp"))

    @staticmethod
    def no_hardlinks(*args, **kwargs):
        raise OSError(errno.EOPNOTSUPP, "simulated unsupported link")

    @unittest.skipUnless(os.name == "posix" and os.uname().sysname == "Linux", "native Linux renameat2 test")
    def test_linkless_atomic_publish_creates_readable_backup(self):
        with mock.patch.object(slots.os, "link", side_effect=self.no_hardlinks):
            slots.save_backup_archive(self.path, self.b)
        self.assertEqual(slots.read_archive(self.path.read_bytes())[0]["count"], 1)
        self.assertEqual(self.path.read_bytes(), self.b)
        self.assertEqual(self.stages(), [])

    def test_hardlink_path_unchanged(self):
        slots.save_backup_archive(self.path, self.a)
        self.assertEqual(self.path.read_bytes(), self.a)
        self.assertEqual(self.stages(), [])

    def test_concurrent_creator_wins_on_linkless_filesystem(self):
        def raced(*args, **kwargs):
            self.path.write_bytes(self.a)
            raise OSError(errno.EOPNOTSUPP, "simulated missing hardlinks")
        with mock.patch.object(slots.os, "link", side_effect=raced):
            with self.assertRaises(FileExistsError):
                slots.save_backup_archive(self.path, self.b)
        self.assertEqual(self.path.read_bytes(), self.a)
        self.assertEqual(self.stages(), [])

    def test_symlink_conflict_is_not_followed(self):
        target = self.root / "precious.x0xslots"
        target.write_bytes(self.a)
        self.path.symlink_to(target)
        with mock.patch.object(slots.os, "link", side_effect=self.no_hardlinks):
            with self.assertRaises(FileExistsError):
                slots.save_backup_archive(self.path, self.b)
        self.assertTrue(self.path.is_symlink())
        self.assertEqual(target.read_bytes(), self.a)
        self.assertEqual(self.stages(), [])

    def test_unsupported_atomic_rename_explains_remediation_and_cleans_stage(self):
        with mock.patch.object(slots.os, "link", side_effect=self.no_hardlinks), \
             mock.patch.object(slots, "_rename_backup_noreplace", side_effect=slots.SlotError("filesystem cannot atomically publish backup")):
            with self.assertRaisesRegex(slots.SlotError, "filesystem cannot atomically publish"):
                slots.save_backup_archive(self.path, self.b)
        self.assertFalse(self.path.exists())
        self.assertEqual(self.stages(), [])

    def test_permission_error_never_falls_back(self):
        with mock.patch.object(slots.os, "link", side_effect=PermissionError(errno.EPERM, "permission denied")), \
             mock.patch.object(slots, "_rename_backup_noreplace") as native:
            with self.assertRaises(PermissionError):
                slots.save_backup_archive(self.path, self.b)
        native.assert_not_called()
        self.assertFalse(self.path.exists())
        self.assertEqual(self.stages(), [])

    def test_existing_destination_not_modified_by_linkless_path(self):
        self.path.write_bytes(self.a)
        with mock.patch.object(slots.os, "link", side_effect=self.no_hardlinks):
            with self.assertRaises(FileExistsError):
                slots.save_backup_archive(self.path, self.b)
        self.assertEqual(self.path.read_bytes(), self.a)
        self.assertEqual(self.stages(), [])

    def test_corrupted_archive_rejected_before_stage_or_link(self):
        with mock.patch.object(slots.os, "link", side_effect=AssertionError("must not link")):
            with self.assertRaises(slots.SlotError):
                slots.save_backup_archive(self.path, self.b[:-4] + b"bad!")
        self.assertFalse(self.path.exists())
        self.assertEqual(self.stages(), [])

    def test_replace_path_does_not_depend_on_link(self):
        self.path.write_bytes(self.a)
        with mock.patch.object(slots.os, "link", side_effect=self.no_hardlinks):
            slots.save_backup_archive(self.path, self.b, replace=True)
        self.assertEqual(self.path.read_bytes(), self.b)
        self.assertEqual(self.stages(), [])


if __name__ == "__main__":
    unittest.main(verbosity=2)
