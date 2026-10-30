#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Sample-backup durability negative tests (host-only, no device or flash)."""
import errno
import os
import stat
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import x0x_slots as xs


class BackupDirectorySync(unittest.TestCase):
    def setUp(self):
        temp = tempfile.TemporaryDirectory(prefix="x0x-backup-sync-")
        self.addCleanup(temp.cleanup)
        self.parent = Path(temp.name)
        self.target = self.parent / "samples.x0xslots"
        lay = {"count": 1, "size": 0x14000, "slots": [{"size": 0x14000, "off": None}]}
        self.data = xs.make_archive(lay, [None], created="2026-10-24T00:00:00Z")
        xs.read_archive(self.data)

    def test_fresh_backup_fsyncs_published_name(self):
        observed = []
        real = os.fsync
        def check(fd):
            if stat.S_ISDIR(os.fstat(fd).st_mode):
                observed.append(self.target.read_bytes())
            return real(fd)
        with patch.object(xs.os, "fsync", side_effect=check):
            xs.save_backup_archive(self.target, self.data)
        self.assertEqual(observed, [self.data])
        self.assertEqual(self.target.read_bytes(), self.data)

    def test_post_publish_dirsync_failure_refuses_success(self):
        real = os.fsync
        def fail(fd):
            if stat.S_ISDIR(os.fstat(fd).st_mode):
                raise OSError(errno.EIO, "injected directory failure")
            return real(fd)
        with patch.object(xs.os, "fsync", side_effect=fail):
            with self.assertRaisesRegex(xs.SlotError, "durability unknown"):
                xs.save_backup_archive(self.target, self.data)
        self.assertEqual(self.target.read_bytes(), self.data)
        self.assertEqual(list(self.parent.glob(".samples.x0xslots.*.tmp")), [])

    def test_directory_open_failure_refuses_success(self):
        real = os.open
        def fail(path, flags, *args, **kwargs):
            if Path(path) == self.parent:
                raise OSError(errno.EACCES, "injected directory open failure")
            return real(path, flags, *args, **kwargs)
        with patch.object(xs.os, "open", side_effect=fail):
            with self.assertRaisesRegex(xs.SlotError, "durability unknown"):
                xs.save_backup_archive(self.target, self.data)
        self.assertEqual(self.target.read_bytes(), self.data)

    def test_replace_path_also_fsyncs_directory(self):
        self.target.write_bytes(b"previous archive")
        dirs = []
        real = os.fsync
        def check(fd):
            if stat.S_ISDIR(os.fstat(fd).st_mode):
                dirs.append(self.target.read_bytes())
            return real(fd)
        with patch.object(xs.os, "fsync", side_effect=check):
            xs.save_backup_archive(self.target, self.data, replace=True)
        self.assertEqual(dirs, [self.data])

    def test_pre_publish_fsync_failure_preserves_original(self):
        self.target.write_bytes(b"previous archive")
        with patch.object(xs.os, "fsync", side_effect=OSError(errno.ENOSPC, "disk full")):
            with self.assertRaises(OSError):
                xs.save_backup_archive(self.target, self.data, replace=True)
        self.assertEqual(self.target.read_bytes(), b"previous archive")
        self.assertEqual(list(self.parent.glob(".samples.x0xslots.*.tmp")), [])

    def test_existing_name_and_corrupt_archive_fail_closed(self):
        self.target.write_bytes(b"previous archive")
        with self.assertRaises(FileExistsError):
            xs.save_backup_archive(self.target, self.data)
        self.assertEqual(self.target.read_bytes(), b"previous archive")
        with self.assertRaises(xs.SlotError):
            xs.save_backup_archive(self.parent / "bad.x0xslots", b"invalid")
        self.assertFalse((self.parent / "bad.x0xslots").exists())


if __name__ == "__main__":
    unittest.main(verbosity=2)
