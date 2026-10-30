#!/usr/bin/env python3
"""Source-matched host regression: changed SMP_INFO during prewrite backup must replan preservation.

No MIDI device or firmware flash. Run from repository root:
  python3 tests/fm1_install_backup_snapshot_test.py
"""
import ast
import io
import os
from pathlib import Path
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]

def extract(src, predicate, scope):
    tree = ast.parse(src.read_text(), filename=str(src))
    chosen = [n for n in tree.body if predicate(n)]
    assert chosen, f"required production functions absent from {src}"
    exec(compile(ast.Module(body=chosen, type_ignores=[]), str(src), 'exec'), scope)

XSSCOPE = {}
# SlotMove.restore now calls guard_migration_loop_menu (LOOP menu fail-closed); pull it
# and its helpers so the mock matches production without inventing a second policy.
_GUARD_NAMES = (
    'flash_map', 'at_risk', 'plan_migration',
    '_user_loop_menu_ids', '_loop_menu_after_migration', 'migration_reorders_loop_menu',
    'guard_migration_loop_menu',
)
extract(ROOT / 'tools/x0x_slots.py',
        lambda n: isinstance(n, ast.FunctionDef) and n.name in _GUARD_NAMES,
        XSSCOPE)
XSSCOPE['zlib'] = __import__('zlib')
XSSCOPE['LOOP_MENU_GUARD'] = (
    "LOOP menu order would change; saved break pattern A/B indices would point at the wrong "
    "loops. Restore samples manually from the backup, then re-assign LOOP A/B in each pattern."
)

class MockXS:
    class SlotError(Exception):
        pass

    def __init__(self, layout, images):
        self.backup_layout, self.images = layout, images
        self.saved, self.raised, self.archive_images = False, False, None

    def backup(self, link):
        if self.raised:
            raise self.SlotError('mock SMP_READ failure')
        return self.backup_layout, self.images

    def flash_map(self, lay, board):
        return XSSCOPE['flash_map'](lay, board)

    def at_risk(self, lay, old_map, new_map):
        # The production function uses DATA_OFF from the actual source's constant value.
        old = XSSCOPE['at_risk'].__globals__
        old['DATA_OFF'] = 512
        return XSSCOPE['at_risk'](lay, old_map, new_map)

    def layout(self, link):
        return getattr(self, 'post_install_layout', self.backup_layout)

    def plan_migration(self, images, risk, old_map, new_lay, new_map):
        return XSSCOPE['plan_migration'](images, risk, old_map, new_lay, new_map)

    def guard_migration_loop_menu(self, old_lay, images, risk, old_map, new_lay, new_map, todo, skipped):
        return XSSCOPE['guard_migration_loop_menu'](
            old_lay, images, risk, old_map, new_lay, new_map, todo, skipped)

    def restore_mapped(self, link, todo, images):
        self.restored = list(todo)

    def make_archive(self, lay, images):
        self.archive_images = list(images)
        return b'verified-synthetic-backup'

    def save_backup_archive(self, path, data):
        if getattr(self, 'save_fails', False):
            raise OSError('mock disk failure')
        path.write_bytes(data)
        self.saved = True

    def read_archive(self, raw):
        assert raw == b'verified-synthetic-backup'
        return {}, self.archive_images

SCOPE = {'EditorLink': lambda link: link, 'Path': Path, 'xs': None}
extract(ROOT / 'tools/fm1_install.py', lambda n: isinstance(n, ast.ClassDef) and n.name == 'SlotMove', SCOPE)
SlotMove = SCOPE['SlotMove']

def layout(used=(1,0), offs=(0x1000,0x5000)):
    return {'count':2,'size':16*1024,'ext':True,
            'slots':[{'off':o,'size':16*1024,'nz':nz,'used':nz*3,'name':f'S{k}'}
                     for k,(o,nz) in enumerate(zip(offs,used))]}

class BackupSnapshotTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.path = Path(self.tmp.name) / 'saved.x0xslots'
        self.before = layout()
        self.new_map = [(0x9000,16*1024),(0xA000,16*1024)]
        self.old_map = XSSCOPE['flash_map'](self.before, 'smk')
        self.move = SlotMove(self.before,self.old_map,self.new_map,[0],'smk')
        self.mock = MockXS(layout(used=(1,1), offs=(0x1000,0x6000)), [b'sampleA',b'sampleB'])
        SCOPE['xs'] = self.mock

    def test_new_at_risk_slot_between_probe_and_backup_is_included(self):
        self.move.backup(object(), self.path, io.StringIO())
        self.assertEqual([0,1], self.move.risk)
        self.assertEqual([b'sampleA',b'sampleB'], self.move.images)
        self.assertTrue(self.mock.saved)

    def test_actual_backed_up_flash_map_overrides_stale_preflight_offsets(self):
        self.move.backup(object(), self.path, io.StringIO())
        self.assertEqual([(0x1000,16*1024),(0x6000,16*1024)],self.move.old_map)

    def test_unchanged_smp_info_retains_original_migration_plan(self):
        self.mock.backup_layout = self.before
        self.mock.images = [b'sampleA',None]
        self.move.backup(object(), self.path, io.StringIO())
        self.assertEqual([0], self.move.risk)
        self.assertEqual(self.old_map, self.move.old_map)

    def test_restore_uses_the_refreshed_risk_and_preserves_both_samples(self):
        self.mock.post_install_layout = layout(used=(0,0), offs=(0x9000,0xA000))
        self.move.backup(object(), self.path, io.StringIO())
        todo, skipped = self.move.restore(object(), io.StringIO())
        self.assertEqual([], skipped)
        self.assertEqual({0,1}, {old for old, _ in todo})
        self.assertEqual(2, len({new for _, new in todo}))
        self.assertEqual(todo, self.mock.restored)

    def test_skipped_middle_slot_never_reaches_restore_writes(self):
        self.mock.backup_layout = layout(used=(1, 1, 1), offs=(0x1000, 0x20000, 0x40000))
        self.mock.backup_layout['slots'][1]['nz'] = 2
        self.mock.images = [b'A' * 40000, b'B' * 100000, b'C' * 30000]
        self.mock.post_install_layout = layout(used=(1, 0), offs=(0x1000, 0x50000))
        for sl in self.mock.post_install_layout['slots']:
            sl['size'] = 80000
        self.move.new_map = [(0x1000, 80000), (0x50000, 80000)]
        self.move.backup(object(), self.path, io.StringIO())
        todo, skipped = self.move.restore(object(), io.StringIO())
        self.assertEqual([], todo)
        self.assertEqual([], self.mock.restored)
        self.assertTrue(self.path.exists())
        self.assertEqual(self.mock.images, self.mock.archive_images)
        self.assertTrue(any('LOOP menu' in message for message in skipped))

    def test_slot_emptied_before_backup_is_not_rewritten(self):
        self.mock.backup_layout = layout(used=(0,1))
        self.mock.images = [None,b'sampleB']
        self.move.backup(object(), self.path, io.StringIO())
        self.assertEqual([1], self.move.risk)

    def test_backup_read_failure_stops_before_archive(self):
        self.mock.raised = True
        with self.assertRaises(self.mock.SlotError):
            self.move.backup(object(), self.path, io.StringIO())
        self.assertFalse(self.path.exists())
        self.assertFalse(self.mock.saved)

    def test_archive_save_failure_is_not_claimed_successful(self):
        self.mock.save_fails = True
        with self.assertRaises(OSError):
            self.move.backup(object(), self.path, io.StringIO())
        self.assertFalse(self.path.exists())
        self.assertFalse(self.mock.saved)

if __name__=='__main__':
    unittest.main(verbosity=2)
