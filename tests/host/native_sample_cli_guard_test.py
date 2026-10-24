#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Source-AST CLI guard replay for native sample operations (host-only synthetic).

Usage: python3 -B tests/host/native_sample_cli_guard_test.py [sample-source] [slots-source]
This test executes source-exact main() AST nodes, not complete import/runtime/hardware.
"""
import ast
import contextlib
import io
from pathlib import Path
import sys
import tempfile
import types

BASE = Path(__file__).resolve().parents[2]
SAMPLE = Path(sys.argv[1]) if len(sys.argv) > 1 else BASE / 'tools' / 'fm1_sample_upload.py'
SLOTS = Path(sys.argv[2]) if len(sys.argv) > 2 else BASE / 'tools' / 'x0x_slots.py'


def source_main(path):
    parsed = ast.parse(path.read_text(encoding='utf-8'), filename=str(path))
    matches = [node for node in parsed.body if isinstance(node, ast.FunctionDef) and node.name == 'main']
    if len(matches) != 1:
        raise RuntimeError(f'{path}: missing/ambiguous main()')
    return ast.fix_missing_locations(ast.Module(body=matches, type_ignores=[]))


class SlotError(Exception):
    pass


class ReachedDevice(Exception):
    pass


def invoke_sample(main_ast, args, root):
    events = []
    class Link:
        def __init__(self):
            events.append(('open',))
        def req(self, cmd, args, *rest):
            events.append(('req', cmd))
            if cmd == 15:
                return [1, 80, 0, 0, 0]
            raise ReachedDevice(f'a destructive command was reached: {cmd}')
    def build(name, specs, trim):
        events.append(('build', name, bool(trim)))
        if name == 'BAD':
            raise OSError('simulated unreadable file')
        return bytes([0] * 6 + [2] + [0] * 20), b'abc'
    ns = {'sys': sys, 'Path': Path, 'Link': Link, 'build': build, '__doc__': 'usage',
          'time': types.SimpleNamespace(time=lambda: 42), 'sio': types.SimpleNamespace(SLOT_DATA_OFF=512),
          'pack7': lambda b: list(b), 'RC': {}}
    exec(compile(main_ast, str(SAMPLE), 'exec'), ns)
    prev = sys.argv
    sys.argv = ['fm1_sample_upload.py', *args]
    try:
        with contextlib.redirect_stdout(io.StringIO()) as buf:
            try:
                ret = ns['main']()
                error = None
            except BaseException as exc:
                ret, error = None, exc
        return events, error, buf.getvalue(), ret
    finally:
        sys.argv = prev


def invoke_slots(main_ast, args, root):
    events = []
    class Link:
        def __init__(self):
            events.append(('open',))
            if args[0] == 'restore':
                raise ReachedDevice('restore opened a MIDI port before denial')
        def req(self, cmd, args, *rest):
            events.append(('req', cmd))
            if cmd not in (1, 15, 16):
                raise ReachedDevice(f'destructive request {cmd}')
            return []
    module = types.ModuleType('fm1_sample_upload')
    module.Link = Link
    old = sys.modules.get('fm1_sample_upload')
    sys.modules['fm1_sample_upload'] = module
    def read_archive(data):
        events.append(('read-archive', len(data)))
        return {'count': 1, 'created': '2026', 'slots': []}, [b'x'*512 + b'data']
    def backup(link, progress):
        events.append(('backup',))
        return {'count': 1, 'slots': []}, [None]
    def save_backup_archive(path, payload, **kw):
        events.append(('save-backup', kw))
        path.write_bytes(payload)
    ns = {'sys': sys, 'Path': Path, 'SlotError': SlotError, '__doc__': 'usage',
          'read_archive': read_archive, 'describe': lambda meta: 'fake safe archive',
          'HDR_LEN': 480, 'DATA_OFF': 512, '_bar': lambda *a: None,
          'backup': backup, 'make_archive': lambda lay, images: b'archive',
          'save_backup_archive': save_backup_archive,
          'layout': lambda link: {'count': 1},
          'plan_restore': lambda lay, meta, images, only: ([(0, images[0])], []),
          'restore': lambda *a: events.append(('restore-write',)), 'input': lambda q: 'yes'}
    exec(compile(main_ast, str(SLOTS), 'exec'), ns)
    try:
        with contextlib.redirect_stdout(io.StringIO()) as buf:
            try:
                ret = ns['main'](args)
                error = None
            except BaseException as exc:
                ret, error = None, exc
        return events, error, buf.getvalue(), ret
    finally:
        if old is None:
            sys.modules.pop('fm1_sample_upload', None)
        else:
            sys.modules['fm1_sample_upload'] = old


def main():
    sample_ast, slots_ast = source_main(SAMPLE), source_main(SLOTS)
    results = []
    with tempfile.TemporaryDirectory(prefix='x0x-sample-cli-') as temp:
        p = Path(temp)
        archive = p / 'archive.x0xslots'
        archive.write_bytes(b'archive')
        missing = p / 'missing.x0xslots'
        outprefix = p / 'offline'
        for title, args in [
            ('load valid early denial', ['load', '1', 'VALID', 'file.wav']),
            ('load invalid source before parsing WAV', ['load', '1', 'BAD', 'file.wav']),
            ('load missing WAV file before input access', ['load', '1', 'VALID']),
            ('load with --no-trim before MIDI', ['--no-trim', 'load', '1', 'VALID', 'file.wav']),
            ('erase exact existing slot', ['erase', '1']),
            ('erase invalid slot without connection', ['erase', 'not-a-slot']),
        ]:
            events, exc, output, _ = invoke_sample(sample_ast, args, p)
            ok = (isinstance(exc, SystemExit) and 'disabled' in str(exc).lower()
                  and not events and not output)
            print(f"{'PASS' if ok else 'FAIL'} {title}: events={events}, error={exc}")
            results.append(ok)
        events, exc, output, ret = invoke_sample(sample_ast, ['build', 'VALID', str(outprefix), 'fake.wav'], p)
        ok = (exc is None and ret is None and events == [('build', 'VALID', True)]
              and (p / 'offline.hdr').read_bytes()[6] == 2
              and (p / 'offline.bin').read_bytes() == b'abc')
        print(f"{'PASS' if ok else 'FAIL'} offline WAV->ADPCM build retained: events={events}, error={exc}")
        results.append(ok)
        events, exc, output, ret = invoke_sample(sample_ast, ['info'], p)
        ok = exc is None and events == [('open',), ('req', 15)] and 'USR1: empty' in output
        print(f"{'PASS' if ok else 'FAIL'} read-only info retained: events={events}, error={exc}")
        results.append(ok)
        for title, args in [
            ('restore missing archive denies before read', ['restore', str(missing)]),
            ('restore existing archive denies before open', ['restore', str(archive)]),
            ('restore --yes denies', ['restore', str(archive), '--yes']),
            ('restore --no-verify denies', ['restore', str(archive), '--no-verify']),
            ('restore --slots subset denies', ['restore', str(archive), '--slots', '1']),
            ('restore all options deny', ['restore', str(archive), '--slots', '1', '--yes', '--no-verify']),
        ]:
            events, exc, output, _ = invoke_slots(slots_ast, args, p)
            ok = (isinstance(exc, SlotError) and 'disabled' in str(exc).lower()
                  and not events and not output)
            print(f"{'PASS' if ok else 'FAIL'} {title}: events={events}, error={exc}")
            results.append(ok)
        events, exc, output, ret = invoke_slots(slots_ast, ['show', str(archive)], p)
        ok = exc is None and ret is None and events == [('read-archive', 7)] and 'fake safe archive' in output
        print(f"{'PASS' if ok else 'FAIL'} offline show retained: events={events}, error={exc}")
        results.append(ok)
        outdir = p / 'out'
        events, exc, output, ret = invoke_slots(slots_ast, ['extract', str(archive), str(outdir)], p)
        ok = (exc is None and ret is None and events == [('read-archive', 7)]
              and (outdir / 'slot1.hdr').read_bytes() == b'x'*480
              and (outdir / 'slot1.bin').read_bytes() == b'data')
        print(f"{'PASS' if ok else 'FAIL'} offline extract retained: events={events}, error={exc}")
        results.append(ok)
        backup_path = p / 'newbackup.x0xslots'
        events, exc, output, ret = invoke_slots(slots_ast, ['backup', str(backup_path)], p)
        ok = (exc is None and ret is None and events[0] == ('open',)
              and events[1] == ('backup',) and events[2][0] == 'save-backup'
              and backup_path.read_bytes() == b'archive')
        print(f"{'PASS' if ok else 'FAIL'} read-only backup retained: events={events}, error={exc}")
        results.append(ok)
        events, exc, output, ret = invoke_slots(slots_ast, ['backup', str(backup_path)], p)
        ok = isinstance(exc, SystemExit) and not events and 'exists' in str(exc).lower()
        print(f"{'PASS' if ok else 'FAIL'} backup refuses overwrite by default: events={events}, error={exc}")
        results.append(ok)
    n = sum(results)
    print(f'SUMMARY {n}/{len(results)}')
    return 0 if n == len(results) else 1


if __name__ == '__main__':
    raise SystemExit(main())
