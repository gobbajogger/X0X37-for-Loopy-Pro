"""Verify saved install evidence and write the ledger. No device access or writes."""
from pathlib import Path
import hashlib
import json
import subprocess
import sys
import zlib

out = Path(__file__).resolve().parent
lane = out.parents[1]
sys.path.insert(0, str(lane / 'tools'))
import add_ctl
import fm1_install as f
import x0x_slots as xs


def read(path):
    return json.loads(path.read_text())


def fp(data):
    return dict(bytes=len(data), sha256=hashlib.sha256(data).hexdigest(),
                crc32=f'{zlib.crc32(data):08x}')


pre = read(out / 'preflight.json')
rec = read(out / 'install-record.json')
assert rec['status'] == 'VERIFIED DEVELOPMENT INSTALL'
assert rec['installer_exit'] == 0 and all(v == 0 for v in rec['guards'].values())
assert rec['all_complete_slots_identical'] and rec['all_loop_images_identical']
assert rec['full_controller_bytes_identical']
before, after = rec['before'], rec['after']
assert before['identity'] == after['identity'] == 'SMK-37 Elite_900'
assert len(before['slots']) == len(after['slots']) == 3
slot_rows = []
for a, b in zip(before['slots'], after['slots']):
    old, new = Path(a['path']).read_bytes(), Path(b['path']).read_bytes()
    assert old == new and fp(old) == a['complete_slot'] == b['complete_slot']
    assert a['flash'] == b['flash']
    slot_rows.append(dict(slot=a['slot'], flash=a['flash'], before=fp(old),
                          after=fp(new), byte_equal=True))
archives = [out / 'before/loops.x0xslots', Path(rec['arguments']['backup']),
            out / 'after/loops.x0xslots']
images = [xs.read_archive(p.read_bytes())[1] for p in archives]
assert images[0] == images[1] == images[2]
ctl = Path(rec['arguments']['ctl']).read_bytes()
expected_ctl = ctl + b'\xff' * (65536 - len(ctl))
assert (out / 'before/controller-64KiB.bin').read_bytes() == expected_ctl
assert (out / 'after/controller-64KiB.bin').read_bytes() == expected_ctl
package = Path(rec['arguments']['package']).read_bytes()
assembled = add_ctl.add(package, ctl)
assert fp(assembled) == pre['artifacts']['assembled_install_image']
logical, _ = f.deinterleave(assembled)
flash = bytes(add_ctl._open(logical)['flash'])
assert len(flash[0x4000:0x9d000]) // 4096 == 153
log = (out / 'install.log').read_text()
assert 'done: the FM-1 runs SMK-37 Elite_900' in log
assert 'unit CRC32 4a400899, expected block CRC32 4a400899' in log
assert 'VERIFIED DEVELOPMENT INSTALL' in log
assert not subprocess.check_output(['git', '-C', str(lane), 'diff',
    pre['validated_revision'], 'HEAD', '--', *pre['checked_paths']]).strip()
ledger = dict(stage=6, marker='smc-complete-20261021-install-after-combined-dev',
    result='INSTALL AND DATA CHECKS PASS', install_chat='01a15ad9-c772-7660-8a69-64685904cbfa',
    validation_owner='01a15ac6-2f01-7e93-93b8-cf1910a41ea1',
    integration_owner='01a15ac1-6dac-7ca2-a2fc-877c4671ab33',
    validated_revision=rec['commit'], evidence_revision=rec['evidence_revision'],
    completed_merge=pre['completed_merge'], model=rec['model'], config=rec['config'],
    X0X_DEV_PADCAL=1, dev_record=rec['dev_record'], arguments=rec['arguments'],
    artifacts=pre['artifacts'], started=rec['started'], completed=rec['completed'],
    guards=rec['guards'], installer_exit=0, protected_head=rec['bootloader'],
    image_readback=rec['image_readback'], application_write_range=[0x4000, 0x9d000],
    expected_flash_image=fp(flash), expected_application_write_range=fp(flash[0x4000:0x9d000]),
    controller_readback=fp(expected_ctl), slots=slot_rows, all_loop_data_preserved=True,
    archives=[dict(path=str(p), identity=fp(p.read_bytes())) for p in archives],
    imported_loops=sum(s['nz'] for s in before['layout']['slots']),
    physical_behavior='NOT TESTED', original_input_failure='UNRESOLVED',
    limits=['The native loader verifies each application sector before FINISH_WRITE. No independent host application dump is available.',
            'No independent unit bootloader-head reference is available. The package head matches the previous installed package; loader writes exclude the bootloader.',
            'Installation and readback do not establish a physical-input repair.'],
    source_repairs=False, release=False, full_gate=False, external_push=False,
    publication=False, next_stage=None,
    next_stage_reason='No external destination, ref and scope are approved. The assigned installation is complete.',
    sandbox_attempt='attempt-1.json')
(out / 'install-ledger.json').write_text(json.dumps(ledger, indent=2) + '\n')
(out / 'RESULT.txt').write_text(
    'INSTALL AND DATA CHECKS PASS.\n'
    'Combined Elite DEV revision 1165396199210889fa460d26cbdd0b66183267e1 installed.\n'
    'The loader verified all 153 application sectors; full controller readback passed.\n'
    'All three complete loop slots are byte-identical. All five imported loops are preserved.\n'
    'Physical behavior is NOT TESTED. The original all-input failure remains UNRESOLVED.\n'
    'No independent host app dump or unit bootloader-head reference is available.\n'
    'No release, full gate, external push or publication occurred.\n'
    'Exact identities, CRCs, limits and local backup paths are in install-ledger.json.\n')
print(json.dumps(dict(result=ledger['result'], slots=[s['after']['crc32'] for s in slot_rows],
                      imported_loops=ledger['imported_loops'], controller=fp(expected_ctl)), indent=2))
