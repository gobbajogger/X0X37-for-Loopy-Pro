#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Fail-closed file backup tests; mocks filesystem faults, never accesses hardware."""
import importlib.util
import os
from pathlib import Path
import sys
import tempfile
import unittest
from unittest import mock

source_path = Path(sys.argv.pop(1)) if len(sys.argv) > 1 else Path(__file__).resolve().parents[1] / "tools" / "x0x_slots.py"
spec = importlib.util.spec_from_file_location("x0x_slots_under_test", source_path)
slots = importlib.util.module_from_spec(spec)
spec.loader.exec_module(slots)


class BackupAtomicTest(unittest.TestCase):
    def setUp(self):
        self.td = tempfile.TemporaryDirectory()
        self.addCleanup(self.td.cleanup)
        self.root = Path(self.td.name)
        self.dest = self.root / "samples.x0xslots"
        lay = {"count": 1, "size": 81920, "slots": [{"size": 81920, "nz": 0, "off": None}]}
        self.original = slots.make_archive(lay, [None], "2026-10-22T00:00:00Z")
        self.replacement = slots.make_archive(lay, [None], "2026-10-23T00:00:00Z")
        self.assertNotEqual(self.original, self.replacement)
        slots.read_archive(self.original)
        slots.read_archive(self.replacement)

    def assert_no_stage(self):
        self.assertEqual(list(self.root.glob(".samples.x0xslots.*.tmp")), [])

    def test_new_archive_valid_and_private(self):
        slots.save_backup_archive(self.dest, self.replacement)
        self.assertEqual(self.dest.read_bytes(), self.replacement)
        self.assertEqual(self.dest.stat().st_mode & 0o777, 0o600)
        self.assert_no_stage()

    def test_force_atomically_replaces_existing_archive(self):
        self.dest.write_bytes(self.original)
        slots.save_backup_archive(self.dest, self.replacement, replace=True)
        self.assertEqual(self.dest.read_bytes(), self.replacement)
        self.assert_no_stage()

    def test_default_does_not_overwrite_archive(self):
        self.dest.write_bytes(self.original)
        with self.assertRaises(FileExistsError):
            slots.save_backup_archive(self.dest, self.replacement)
        self.assertEqual(self.dest.read_bytes(), self.original)
        self.assert_no_stage()

    def test_fsync_failure_preserves_archive(self):
        self.dest.write_bytes(self.original)
        with mock.patch.object(slots.os, "fsync", side_effect=OSError("simulated fsync failure")):
            with self.assertRaisesRegex(OSError, "simulated fsync"):
                slots.save_backup_archive(self.dest, self.replacement, replace=True)
        self.assertEqual(self.dest.read_bytes(), self.original)
        self.assert_no_stage()

    def test_replace_failure_preserves_archive(self):
        self.dest.write_bytes(self.original)
        with mock.patch.object(slots.os, "replace", side_effect=OSError("simulated rename failure")):
            with self.assertRaisesRegex(OSError, "simulated rename"):
                slots.save_backup_archive(self.dest, self.replacement, replace=True)
        self.assertEqual(self.dest.read_bytes(), self.original)
        self.assert_no_stage()

    def test_concurrent_creator_wins_without_force(self):
        def concurrent(*args):
            self.dest.write_bytes(self.original)
            raise FileExistsError("raced with another backup")
        with mock.patch.object(slots.os, "link", side_effect=concurrent):
            with self.assertRaises(FileExistsError):
                slots.save_backup_archive(self.dest, self.replacement)
        self.assertEqual(self.dest.read_bytes(), self.original)
        self.assert_no_stage()

    def test_invalid_archive_cannot_replace_good_backup(self):
        self.dest.write_bytes(self.original)
        with self.assertRaises(slots.SlotError):
            slots.save_backup_archive(self.dest, self.replacement[:-4] + b"bad!", replace=True)
        self.assertEqual(self.dest.read_bytes(), self.original)
        self.assert_no_stage()

    def test_temporary_readback_mismatch_preserves_backup(self):
        self.dest.write_bytes(self.original)
        read = Path.read_bytes
        def tampered(p):
            v = read(p)
            return b"tampered" if p.name.startswith(".samples.x0xslots.") else v
        with mock.patch.object(Path, "read_bytes", tampered):
            with self.assertRaisesRegex(slots.SlotError, "failed readback"):
                slots.save_backup_archive(self.dest, self.replacement, replace=True)
        self.assertEqual(self.dest.read_bytes(), self.original)
        self.assert_no_stage()

    def test_symlink_destination_does_not_modify_target(self):
        target = self.root / "other-valid-backup.x0xslots"
        target.write_bytes(self.original)
        self.dest.symlink_to(target)
        slots.save_backup_archive(self.dest, self.replacement, replace=True)
        self.assertEqual(target.read_bytes(), self.original)
        self.assertFalse(self.dest.is_symlink())
        self.assertEqual(self.dest.read_bytes(), self.replacement)
        self.assert_no_stage()

    def test_cli_force_does_not_truncate_on_disk_full(self):
        """The original main() calls Path.write_bytes, which corrupts the old archive."""
        import types
        self.dest.write_bytes(self.original)
        lay = {"count": 1, "size": 81920, "slots": [{"size": 81920, "nz": 0, "off": None}]}
        with mock.patch.dict(sys.modules, {"fm1_sample_upload": types.SimpleNamespace(Link=lambda: object())}), \
             mock.patch.object(slots, "backup", return_value=(lay, [None])), \
             mock.patch.object(slots, "make_archive", return_value=self.replacement):
            old_write = Path.write_bytes
            def broken_write(p, data):
                # Only the pre-fix direct destination overwrite uses this operation.
                with p.open("wb") as fd:
                    fd.write(data[:len(data)//2])
                raise OSError("simulated disk full writing destination")
            with mock.patch.object(Path, "write_bytes", broken_write):
                try:
                    slots.main(["backup", str(self.dest), "--force"])
                except OSError:
                    pass  # The old source failed after truncating the destination.
        self.assertEqual(self.dest.read_bytes(), self.replacement)
        self.assert_no_stage()

    def test_partial_write_preserves_original(self):
        self.dest.write_bytes(self.original)
        original_named = slots.tempfile.NamedTemporaryFile
        def partial_writer(*args, **kwargs):
            f = original_named(*args, **kwargs)
            real_write = f.write
            def partial(b):
                real_write(b[:max(1, len(b) // 2)])
                raise OSError("simulated disk full during write")
            f.write = partial
            return f
        with mock.patch.object(slots.tempfile, "NamedTemporaryFile", side_effect=partial_writer):
            with self.assertRaisesRegex(OSError, "simulated disk full"):
                slots.save_backup_archive(self.dest, self.replacement, replace=True)
        self.assertEqual(self.dest.read_bytes(), self.original)
        self.assert_no_stage()


if __name__ == "__main__":
    unittest.main(verbosity=2)
