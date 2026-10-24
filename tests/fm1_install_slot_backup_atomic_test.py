#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Host-only installer backup preservation checks; never opens real MIDI or flash.

Run in a complete source checkout:
    python3 tests/fm1_install_slot_backup_atomic_test.py
For explicitly sourced isolated function-window fixtures:
    python3 tests/fm1_install_slot_backup_atomic_test.py /path/to/fixture
"""
import io
from pathlib import Path
import sys
import tempfile
import unittest
from unittest import mock

root = Path(__file__).resolve().parents[1]
source_dir = Path(sys.argv.pop(1)) if len(sys.argv) > 1 else root / 'tools'
sys.path.insert(0, str(source_dir))
import x0x_slots as xs  # noqa: E402
import fm1_install as install  # noqa: E402


class InstallBackupAtomicTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.folder = Path(self.tmp.name)
        self.path = self.folder / 'preinstall.x0xslots'
        self.lay = {'count': 1, 'size': 81920, 'slots': [
            {'size': 81920, 'nz': 0, 'name': '', 'used': 0, 'off': None}]}
        self.old = xs.make_archive(self.lay, [None], created='2026-10-22T00:00:00Z')
        self.new = xs.make_archive(self.lay, [None], created='2026-10-23T00:00:00Z')
        self.assertNotEqual(self.old, self.new)
        self.assertEqual(xs.read_archive(self.old)[1], [None])
        self.assertEqual(xs.read_archive(self.new)[1], [None])
        self.move = install.SlotMove(self.lay, [], [], [0], 'smk')

    def save(self):
        # This mocks only the read-only MIDI retrieval and image packing; the
        # actual firmware installer SlotMove.backup() and archive publication run.
        with mock.patch.object(xs, 'backup', return_value=(self.lay, [None])) as read, \
             mock.patch.object(xs, 'make_archive', return_value=self.new):
            message = io.StringIO()
            self.move.backup(object(), self.path, message)
            read.assert_called_once()
            return message.getvalue()

    def assert_no_temp(self):
        self.assertEqual(list(self.folder.glob('.preinstall.x0xslots.*.tmp')), [])

    def test_fresh_install_backup_exists_and_validates(self):
        line = self.save()
        self.assertEqual(self.path.read_bytes(), self.new)
        self.assertIn('slots saved:', line)
        self.assertEqual(self.move.path, self.path)
        self.assert_no_temp()

    def test_preexisting_archive_cannot_be_overwritten(self):
        self.path.write_bytes(self.old)
        with self.assertRaises(FileExistsError):
            self.save()
        self.assertEqual(self.path.read_bytes(), self.old)
        self.assert_no_temp()

    def test_repeated_default_filename_preserves_previous_backup(self):
        self.save()
        saved = self.path.read_bytes()
        with self.assertRaises(FileExistsError):
            self.save()
        self.assertEqual(self.path.read_bytes(), saved)
        self.assert_no_temp()

    def test_concurrent_backup_creator_wins_without_clobber(self):
        def raced_link(*_args):
            self.path.write_bytes(self.old)
            raise FileExistsError('racing process created the backup name')
        with mock.patch.object(xs.os, 'link', side_effect=raced_link):
            with self.assertRaises(FileExistsError):
                self.save()
        self.assertEqual(self.path.read_bytes(), self.old)
        self.assert_no_temp()

    def test_symlink_backup_destination_cannot_modify_its_target(self):
        other = self.folder / 'owner-good-backup.x0xslots'
        other.write_bytes(self.old)
        self.path.symlink_to(other)
        with self.assertRaises(FileExistsError):
            self.save()
        self.assertTrue(self.path.is_symlink())
        self.assertEqual(other.read_bytes(), self.old)
        self.assert_no_temp()

    def test_archive_preflight_rejects_corruption_before_file_publish(self):
        self.path.write_bytes(self.old)
        bad = self.new[:-4] + b'bad!'
        with mock.patch.object(xs, 'backup', return_value=(self.lay, [None])), \
             mock.patch.object(xs, 'make_archive', return_value=bad):
            with self.assertRaises(xs.SlotError):
                self.move.backup(object(), self.path, io.StringIO())
        self.assertEqual(self.path.read_bytes(), self.old)
        self.assert_no_temp()

    def test_device_read_failure_never_changes_previous_backup(self):
        self.path.write_bytes(self.old)
        with mock.patch.object(xs, 'backup', side_effect=xs.SlotError('read timed out')):
            with self.assertRaisesRegex(xs.SlotError, 'read timed out'):
                self.move.backup(object(), self.path, io.StringIO())
        self.assertEqual(self.path.read_bytes(), self.old)
        self.assert_no_temp()

    def test_directory_destination_is_refused_without_clobbering(self):
        self.path.mkdir()
        with self.assertRaises(OSError):
            self.save()
        self.assertTrue(self.path.is_dir())
        self.assert_no_temp()

    def test_disk_full_after_partial_stage_write_does_not_truncate_old_backup(self):
        self.path.write_bytes(self.old)
        original = xs.tempfile.NamedTemporaryFile

        def partial_destination(p, b):
            with p.open('wb') as out:
                out.write(b[:max(1, len(b) // 2)])
            raise OSError('disk full while replacing existing backup')

        def stage_failure(*args, **kwargs):
            fd = original(*args, **kwargs)
            actual = fd.write
            def partial(b):
                actual(b[:max(1, len(b) // 2)])
                raise OSError('disk full writing stage')
            fd.write = partial
            return fd

        with mock.patch.object(Path, 'write_bytes', partial_destination), \
             mock.patch.object(xs.tempfile, 'NamedTemporaryFile', side_effect=stage_failure):
            with self.assertRaisesRegex(OSError, 'disk full'):
                self.save()
        self.assertEqual(self.path.read_bytes(), self.old)
        self.assert_no_temp()

    def test_fsync_failure_does_not_overwrite_existing_backup(self):
        self.path.write_bytes(self.old)
        with mock.patch.object(xs.os, 'fsync', side_effect=OSError('simulated fsync failure')):
            with self.assertRaisesRegex(OSError, 'fsync'):
                self.save()
        self.assertEqual(self.path.read_bytes(), self.old)
        self.assert_no_temp()

    def test_temporary_file_readback_failure_does_not_publish(self):
        original = Path.read_bytes
        def corrupt_stage(p):
            value = original(p)
            return value[:-1] if p.name.startswith('.preinstall.x0xslots.') else value
        with mock.patch.object(Path, 'read_bytes', corrupt_stage):
            with self.assertRaisesRegex(xs.SlotError, 'readback'):
                self.save()
        self.assertFalse(self.path.exists())
        self.assert_no_temp()


if __name__ == '__main__':
    unittest.main(verbosity=2)
