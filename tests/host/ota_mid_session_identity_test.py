#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Host-only MIDI link physical-swap checks; no device I/O or flash writes.

Run from repository root:
    python3 -B tests/host/ota_mid_session_identity_test.py tools/fm1_install.py

AST loads the actual MidoLink class while a fake mido module controls input,
output and the authenticated backend's per-unit token. A real backend must
supply an independently verified non-reusable token. An OS name/USB path alone
cannot satisfy that contract. The production MidoBackend does not provide it.
"""
import ast
import copy
import pathlib
import queue
import sys
import time
from types import ModuleType, SimpleNamespace


class PhysicalBackend:
    def __init__(self, unit='unit-A', name='SMK-37 Pro'):
        self.name, self.unit, self.error = name, unit, False

    def input_names(self):
        return [self.name] if self.name else []

    def output_physical_id(self, name):
        return self.physical_id(name)

    def physical_id(self, name):
        if self.error:
            raise OSError('USB identity unavailable')
        return self.unit if name == self.name else None


class MidiInput:
    def __init__(self, callback):
        self.callback, self.closed = callback, False

    def close(self):
        self.closed = True

    def inject(self, data):
        self.callback(SimpleNamespace(type='sysex', data=data))


class MidiOutput:
    def __init__(self):
        self.sent, self.closed = [], False

    def send(self, msg):
        self.sent.append(msg)

    def close(self):
        self.closed = True


class FakeMido(ModuleType):
    def __init__(self):
        super().__init__('mido')
        self.inputs, self.outputs = [], []

    def open_input(self, _name, callback):
        obj = MidiInput(callback)
        self.inputs.append(obj)
        return obj

    def open_output(self, _name):
        obj = MidiOutput()
        self.outputs.append(obj)
        return obj

    def Message(self, msg_type, *, data):
        return SimpleNamespace(type=msg_type, data=data)


def load_original_class(source):
    tree = ast.parse(pathlib.Path(source).read_text(encoding='utf-8'))
    matching = [part for part in tree.body if isinstance(part, ast.ClassDef)
                and part.name == 'MidoLink']
    assert len(matching) == 1, 'MidoLink source missing or ambiguous'
    module = ast.fix_missing_locations(ast.Module(body=[copy.deepcopy(matching[0])], type_ignores=[]))
    fake = FakeMido()
    old_mido = sys.modules.get('mido')
    sys.modules['mido'] = fake
    ns = {'queue': queue, 'time': time, 'DELAY': {'open': 0}}
    exec(compile(module, str(source), 'exec'), ns)
    return ns['MidoLink'], fake, old_mido


def main(source):
    Link, midi, old_mido = load_original_class(source)
    passes, fails = 0, 0

    def check(test, description):
        nonlocal passes, fails
        if test:
            passes += 1
            print('PASS ' + description)
        else:
            fails += 1
            print('FAIL ' + description)

    def open_link(backend):
        return Link(backend, backend.name, backend.name)

    try:
        b = PhysicalBackend()
        link = open_link(b)
        check(link.send(b'\xf0\x01\x02\xf7') and len(link.out.sent) == 1,
              'authenticated unchanged unit accepts MIDI send')
        link.inp.inject([0x01, 0x02])
        check(link.read(0) == b'\xf0\x01\x02\xf7' and not link.lost,
              'unchanged physical unit accepts MIDI receive')
        link.close()
        check(link.inp.closed and link.out.closed, 'both transport handles close')

        b = PhysicalBackend()
        link = open_link(b)
        link.inp.inject([0x10])
        b.unit = 'unit-B'        # exact SAME OS MIDI name, different physical unit
        check(link.lost, 'sequential identical-name physical replacement is lost')
        check(not link.send(b'\xf0\x11\xf7') and not link.out.sent,
              'replacement cannot receive destructive outgoing MIDI')
        check(link.read(0) is None,
              'queued foreign/stale packet is not accepted after replacement')
        link.close()

        b = PhysicalBackend()
        link = open_link(b)
        check(link.send(b'\xf0\x01\xf7'), 'initial authenticated send succeeds')
        b.unit = 'unit-B'
        check(not link.send(b'\xf0\x02\xf7') and len(link.out.sent) == 1,
              'replacement between consecutive OTA request replies is rejected')
        link.close()

        b = PhysicalBackend()
        link = open_link(b)
        b.unit = None
        check(link.lost and not link.send(b'\xf0\x03\xf7'),
              'physical identity disappearing midtransfer fails closed')
        link.close()

        b = PhysicalBackend()
        link = open_link(b)
        b.error = True
        try:
            refused = not link.send(b'\xf0\x03\xf7') and link.lost
        except OSError:
            refused = False
        check(refused, 'physical identity provider OSError fails closed')
        link.close()

        b = PhysicalBackend(name='SMK-37 Pro')
        first = open_link(b)
        first.close()
        b.name = 'Felucca Update'  # actual normal loader OS name change
        second = open_link(b)
        check(second.send(b'\xf0\x01\xf7') and not second.lost,
              'genuine unit loader rename works on freshly bound port')
        b.unit = 'unit-B'
        check(second.lost and not second.send(b'\xf0\x02\xf7'),
              'a foreign unit at the loader name is rejected')
        second.close()

        b = PhysicalBackend(unit=None)
        link = open_link(b)
        check(link.lost and not link.send(b'\xf0\x00\xf7'),
              'present physical-ID provider with unavailable ID fails closed')
        link.close()

        class ReadOnlyNameBackend:
            # Real production MidoBackend currently has NO physical-ID API;
            # the Updater disallows flash writes despite this discovery link.
            name = 'SMK-37 Pro'

            def input_names(self):
                return [self.name]

        b = ReadOnlyNameBackend()
        link = open_link(b)
        check(link.send(b'\xf0\x00\xf7') and link.read(0) is None,
              'unverified name-only backend retains read-only discovery')
        link.close()
    finally:
        if old_mido is None:
            sys.modules.pop('mido', None)
        else:
            sys.modules['mido'] = old_mido

    print(f'SUMMARY {passes}/{passes+fails} PASS; {fails} failures')
    return int(fails != 0)


if __name__ == '__main__':
    src = pathlib.Path(sys.argv[1]) if len(sys.argv) > 1 else pathlib.Path('tools/fm1_install.py')
    sys.exit(main(src))
