#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Guard sample-slot discovery against malformed SMP_INFO before firmware install.

Use in the complete checkout: python3 tests/x0x_slots_info_preflight_test.py
Optional explicit source-window fixtures: python3 tests/x0x_slots_info_preflight_test.py SLOTS_SOURCE INSTALL_FIXTURE
No device is contacted; only the MIDI link is stubbed.
"""
import importlib.util,sys,types
from pathlib import Path
from unittest import TestCase,TestSuite,TextTestRunner
if len(sys.argv) == 1:
    project_root = Path(__file__).resolve().parents[1]
    slot_path = project_root / "tools" / "x0x_slots.py"
    install_path = project_root / "tools" / "fm1_install.py"
else:
    slot_path, install_path = map(Path, sys.argv[1:3])
    sys.argv = sys.argv[:1]
sys.path.insert(0,str(slot_path.resolve().parent))

def load(path,name):
    spec=importlib.util.spec_from_file_location(name,path)
    module=importlib.util.module_from_spec(spec);sys.modules[name]=module;spec.loader.exec_module(module)
    return module
xs=load(slot_path,'x0x_slots')
fm=load(install_path,'fm1_install_probe')
# Replace ONLY real hardware link behavior. Tested production plan_slots and parser stay intact.
class NoHardwareEditorLink:
    def __init__(self, link):self.link=link
    def req(self, cmd, args, timeout=3.0):return self.link.req(cmd,args,timeout)
fm.EditorLink = NoHardwareEditorLink
fm.package_slot_map = lambda image, product: []

def enc(v):return [v&0x7f,(v>>7)&0x7f]
def reply(slots,ext=None,tail=None,size_kib=80):
    r=[len(slots),size_kib]
    for nz,name,used in slots:r.extend([nz,*name.encode('ascii'),0,used])
    if ext is not None:
        r.extend([ord('X'),1]);
        for off_blocks,sz_blocks,used in ext:
            for v in (off_blocks,sz_blocks,used):r.extend(enc(v))
    if tail is not None:r.extend(tail)
    return r

ONE=[(1,'A',2)]
GOOD_EXT=reply(ONE,[(0xC2,20,2)])
TWO=[(1,'A',2),(1,'B',3)]
GOOD_TWO=reply(TWO,[(0xC2,20,2),(0xD6,20,3)])

class Link:
    def __init__(self,answer):self.answer=answer;self.commands=[]
    def req(self,cmd,args,timeout):
        self.commands.append(cmd)
        if cmd==xs.CMD_INFO:
            if self.answer is None:raise TimeoutError('SMP_INFO timeout')
            return self.answer
        raise AssertionError(f'Unexpected destructive device cmd {cmd}')
class Device:
    def __init__(self,link):self.link=link;self.id=types.SimpleNamespace(text='SMK-37 Elite_900')

class Guard(TestCase):
    def negative(self,r):
        with self.assertRaises(xs.SlotError):xs.parse_info(r)
        l=Link(r)
        with self.assertRaises(xs.SlotError):fm.plan_slots(Device(l),b'image','SMK-37 Elite_900')
        self.assertEqual(l.commands,[xs.CMD_INFO], 'No BEGIN/WRITE/END/ERASE when malformed layout')
    def test_valid_legacy(self):
        a=xs.parse_info(reply(ONE));self.assertFalse(a['ext']);self.assertEqual(a['count'],1)
        self.assertEqual(xs.flash_map(a,'smk')[0],(xs.LEGACY_BASE['smk'],80*1024))
    def test_valid_extended_offsets(self):
        a=xs.parse_info(GOOD_TWO);self.assertTrue(a['ext']);self.assertEqual(xs.flash_map(a,'smk'),[(0xC2000,0x14000),(0xD6000,0x14000)])
        l=Link(GOOD_TWO);m=fm.plan_slots(Device(l),b'image','SMK-37 Elite_900');self.assertEqual(m.risk,[0,1])
        self.assertEqual(l.commands,[xs.CMD_INFO])
    def test_valid_extension_larger_than_legacy_base(self):
        # Layout 2 SMK: INFO legacy base 44 KiB; slot 2 truly 120 KiB.
        r = reply([(1,'A',30),(1,'B',100),(1,'C',43)],
                  [(0xC2,20,30),(0xD6,30,100),(0xF4,11,43)],size_kib=44)
        lay=xs.parse_info(r)
        self.assertTrue(lay['ext'])
        self.assertEqual([s['size'] for s in lay['slots']],[80*1024,120*1024,44*1024])
    def test_oversized_slot_name(self):self.negative(reply([(1,'TOO-LONG-NAME',2)]))
    def test_unknown_extension_marker(self):self.negative(reply(ONE,tail=[ord('Y'),1]))
    def test_timeout_is_unknown_not_stock(self):
        l=Link(None)
        with self.assertRaisesRegex(xs.SlotError, 'SMP_INFO'):
            fm.plan_slots(Device(l), b'image', 'SMK-37 Elite_900')
        self.assertTrue(l.commands)
        self.assertTrue(all(cmd==xs.CMD_INFO for cmd in l.commands), 'no destructive cmd after timeout')
    def test_timeout_explicit_backup_also_refuses(self):
        l=Link(None)
        with self.assertRaises(xs.SlotError):
            fm.plan_slots(Device(l), b'image', 'SMK-37 Elite_900', require_backup=True)
        self.assertTrue(all(cmd==xs.CMD_INFO for cmd in l.commands))
    def test_stock_looking_id_cannot_justify_unknown_flash(self):
        l=Link(None);d=Device(l);d.id.text='SMK-37 Pro_016'
        with self.assertRaises(xs.SlotError):
            fm.plan_slots(d, b'image', 'SMK-37 Pro_900')
        self.assertTrue(all(cmd==xs.CMD_INFO for cmd in l.commands))
    def test_none_parser_layout_must_not_skip_backup(self):
        l=Link(GOOD_EXT);old=fm.xs.parse_info
        try:
            fm.xs.parse_info=lambda _raw:None
            with self.assertRaises(xs.SlotError):
                fm.plan_slots(Device(l), b'image', 'SMK-37 Elite_900')
        finally:
            fm.xs.parse_info=old
        self.assertEqual(l.commands,[xs.CMD_INFO])
    def test_known_empty_slot_is_safe_without_backup(self):
        l=Link(reply([(0,'',0)]))
        self.assertIsNone(fm.plan_slots(Device(l), b'image', 'SMK-37 Elite_900'))
        self.assertEqual(l.commands,[xs.CMD_INFO])
    def test_truncated_slot_descriptor(self):self.negative([1,80,1,65,0])
    def test_missing_slot_name_nul(self):self.negative([1,80,1,65,70,80])
    def test_slot_zero_count(self):self.negative([0,80])
    def test_zero_slot_base_size(self):self.negative([1,0,1,65,0,2])
    def test_legacy_used_size_exceeded(self):self.negative(reply([(1,'A',81)]))
    def test_truncated_flash_marker(self):self.negative(reply(ONE,tail=[ord('X')]))
    def test_unknown_flash_ext_version(self):self.negative(reply(ONE,tail=[ord('X'),2,*enc(0xC2),*enc(20),*enc(2)]))
    def test_truncated_flash_descriptor(self):self.negative(GOOD_EXT[:-2])
    def test_zero_extended_size(self):self.negative(reply(ONE,[(0xC2,0,2)]))
    def test_extended_used_size_exceeded(self):self.negative(reply(ONE,[(0xC2,20,100)]))
    def test_overlapping_flash_ranges(self):self.negative(reply(TWO,[(0xC2,20,2),(0xD0,20,3)]))
    def test_identical_flash_ranges(self):self.negative(reply(TWO,[(0xC2,20,2),(0xC2,20,3)]))
    def test_zone_count_beyond_header(self):self.negative(reply([(17,'A',2)]))
    def test_zero_zone_legacy_nonzero_used(self):
        self.negative(reply([(0,'',2)]))
    def test_zero_zone_extended_nonzero_used(self):
        self.negative(reply([(0,'',0)],[(0xC2,20,2)]))
    def test_legacy_extended_used_disagreement(self):
        self.negative(reply([(1,'A',0)],[(0xC2,20,2)]))
    def test_nonzero_used_disagreement(self):
        self.negative(reply([(1,'A',3)],[(0xC2,20,2)]))
    def test_zero_zone_phantom_name(self):
        self.negative(reply([(0,'phantom',0)],[(0xC2,20,0)]))
    def test_valid_7bit_legacy_clamp_at_127_kib(self):
        # Firmware editor.c clamps legacy used KiB to 127; X0X reports actual 14-bit size.
        a=xs.parse_info(reply([(1,'A',127)],[(0xC2,40,128)],size_kib=127))
        self.assertEqual(a['slots'][0]['used'],128)
        self.assertEqual(a['slots'][0]['kb'],127)
    def test_invalid_7bit_clamp_disagreement(self):
        self.negative(reply([(1,'A',126)],[(0xC2,40,128)],size_kib=127))
    def test_zone_legacy_extended_consistency_matrix(self):
        # All positive and negative combinations: malformed slots must refuse prewrite.
        for nz in (0,1,2):
            for legacy in (0,1,2):
                for ext_used in (0,1,2):
                    frame=reply([(nz,'' if nz == 0 else 'A',legacy)],
                                [(0xC2,20,ext_used)])
                    with self.subTest(nz=nz,legacy=legacy,ext_used=ext_used):
                        if (nz != 0 or legacy == 0) and legacy == ext_used:
                            a=xs.parse_info(frame)
                            self.assertEqual(a['slots'][0]['used'],ext_used)
                        else:
                            self.negative(frame)

suite=__import__('unittest').defaultTestLoader.loadTestsFromTestCase(Guard)
result=TextTestRunner(verbosity=2).run(suite)
print(f'ASSERTIONS={result.testsRun} FAILURES={len(result.failures)} ERRORS={len(result.errors)}')
sys.exit(0 if result.wasSuccessful() else 1)
