#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Host OTA output-endpoint identity regressions; synthetic, no device writes.

Usage: python3 -B tests/host/ota_output_endpoint_identity_test.py tools/fm1_install.py
This extracts the actual MidoLink and pair_output AST from the source argument.
A trusted test-only backend supplies independently queried physical input and
output tokens. Real MidoBackend has no such authority: OTA stays unsupported.
A host MIDI output handle can still retarget DURING send; no atomicity claimed.
"""
import ast
import copy
import inspect
from pathlib import Path
import queue
import re
import sys
import time
from types import ModuleType, SimpleNamespace


class FakeMidi(ModuleType):
    def __init__(self, outputs):
        super().__init__('mido')
        self.outputs = dict(outputs)
        self.deliveries = []
        self.send_hook = None

    def open_input(self, name, callback):
        return SimpleNamespace(close=lambda: None)

    def open_output(self, name):
        if name not in self.outputs:
            raise OSError('unknown output port')
        midi = self

        class Output:
            def send(self, message):
                if midi.send_hook:
                    midi.send_hook()
                midi.deliveries.append((midi.outputs[name], bytes(message.data)))

            def close(self):
                pass

        return Output()

    @staticmethod
    def Message(kind, data):
        return SimpleNamespace(type=kind, data=data)


class AuthBackend:
    """Mock of a FUTURE trusted per-direction physical identity provider."""
    def __init__(self, input_name, output_name, input_unit='unit-A', output_unit='unit-A'):
        self.input_name = input_name
        self.output_name = output_name
        self.input_unit = input_unit
        self.output_unit = output_unit
        self.queries = []

    def physical_id(self, name):
        self.queries.append(('input', name))
        return self.input_unit if name == self.input_name else None

    def output_physical_id(self, name):
        self.queries.append(('output', name))
        return self.output_unit if name == self.output_name else None

    def input_names(self):
        return [self.input_name]


class InputOnlyBackend(AuthBackend):
    output_physical_id = None


class NoIdentityBackend:
    def __init__(self, name):
        self.name = name

    def input_names(self):
        return [self.name]


def load(path):
    tree = ast.parse(Path(path).read_text(encoding='utf-8'))
    selected = [node for node in tree.body if (
        (isinstance(node, ast.ClassDef) and node.name == 'MidoLink') or
        (isinstance(node, ast.FunctionDef) and node.name == 'pair_output'))]
    if len(selected) != 2:
        raise RuntimeError('required MidoLink/pair_output source nodes missing or duplicated')
    ns = {'queue': queue, 'time': time, 're': re, 'DELAY': {'open': 0}}
    source = ast.fix_missing_locations(ast.Module(body=copy.deepcopy(selected), type_ignores=[]))
    exec(compile(source, str(path), 'exec'), ns)
    return ns['MidoLink'], ns['pair_output']


def run(path):
    Link, pair = load(path)
    checks = []

    # The predecessor has a two-argument function: exercise its real behavior
    # rather than masking the intended expected-RED results with TypeError.
    def choose(name, outputs, backend=None):
        if len(inspect.signature(pair).parameters) == 2:
            return pair(name, outputs)
        return pair(name, outputs, backend)

    def check(ok, label):
        value = bool(ok)
        checks.append(value)
        print(('PASS ' if value else 'FAIL ') + label)

    original_mido = sys.modules.get('mido')
    packet = b'\xf0\x44\xf7'
    try:
        name = 'SMK-37 Pro 1'
        midi = FakeMidi({name: 'unit-A'})
        sys.modules['mido'] = midi
        backend = AuthBackend(name, name)
        link = Link(backend, name, name)
        check(choose(name, [name]) == name and link.send(packet) and not link.lost,
              'exact paired authenticated A allows normal outbound data')
        check(midi.deliveries == [('unit-A', b'\x44')]
              and ('output', name) in backend.queries,
              'outbound physical identity queried independently from input')
        link.close()

        wrong = 'SMK-37 Pro 2'
        backend = AuthBackend(name, wrong, 'unit-A', 'unit-B')
        midi = FakeMidi({wrong: 'unit-B'})
        sys.modules['mido'] = midi
        check(choose(name, [wrong], backend) is None,
              'numbered Windows name cannot substitute for physical continuity')
        link = Link(backend, name, wrong)
        check(not link.send(packet) and link.lost and not midi.deliveries,
              'even manually opened A-input/B-output link never sends to B')
        link.close()

        unrelated = 'Unrelated USB Synth'
        backend = AuthBackend(name, unrelated, 'unit-A', 'unit-B')
        check(choose(name, [unrelated], backend) is None,
              'single unrelated output is never an automatic fallback')

        backend = AuthBackend(name, wrong)
        midi = FakeMidi({wrong: 'unit-A'})
        sys.modules['mido'] = midi
        paired = choose(name, [wrong], backend)
        check(paired == wrong,
              'different numbered input/output names are allowed ONLY with identical authenticated unit')
        if paired:
            link = Link(backend, name, paired)
            check(link.send(packet) and midi.deliveries == [('unit-A', b'\x44')],
                  'authenticated Windows port-pair positive delivers to A')
            link.close()
        else:
            check(False, 'authenticated Windows port-pair positive delivers to A')

        backend = InputOnlyBackend(name, name)
        midi = FakeMidi({name: 'unit-B'})
        sys.modules['mido'] = midi
        link = Link(backend, name, name)
        check(not link.send(packet) and link.lost and not midi.deliveries,
              'input-only identity provider cannot authorize output packets')
        link.close()

        backend = AuthBackend(name, name)
        midi = FakeMidi({name: 'unit-A'})
        sys.modules['mido'] = midi
        link = Link(backend, name, name)
        backend.output_unit = 'unit-B'
        midi.outputs[name] = 'unit-B'
        check(not link.send(packet) and link.lost and not midi.deliveries,
              'output retarget BEFORE send fails without foreign delivery')
        backend.output_unit = 'unit-A'
        midi.outputs[name] = 'unit-A'
        check(link.lost and not link.send(packet),
              'observed output bounce never revives the old link')
        link.close()

        backend = AuthBackend(name, name)
        midi = FakeMidi({name: 'unit-A'})
        sys.modules['mido'] = midi
        link = Link(backend, name, name)
        midi.send_hook = lambda: (setattr(backend, 'output_unit', 'unit-B'),
                                  midi.outputs.__setitem__(name, 'unit-B'))
        returned = link.send(packet)
        check(not returned and link.lost,
              'in-send retarget is observed and permanently reported as failure')
        check(midi.deliveries == [('unit-B', b'\x44')],
              'UNRESOLVED atomicity: first packet can reach B before post-send guard')
        link.close()

        loader = 'Felucca Update'
        backend = AuthBackend(loader, loader)
        midi = FakeMidi({loader: 'unit-A'})
        sys.modules['mido'] = midi
        link = Link(backend, loader, loader)
        check(link.send(packet) and not link.lost,
              'fresh physically authenticated loader-name re-enumeration works')
        link.close()

        midi = FakeMidi({name: 'unknown-unit'})
        sys.modules['mido'] = midi
        backend = NoIdentityBackend(name)
        link = Link(backend, name, name)
        check(link.send(packet) and not link.lost,
              'ordinary Mido name-only link remains discovery-capable, NOT OTA-authorized')
        link.close()

        print(f'SUMMARY {sum(checks)}/{len(checks)} PASS; {len(checks)-sum(checks)} FAIL')
        print('LIMIT: fake authenticated tokens; no USB or atomic output guarantee')
        return 0 if all(checks) else 1
    finally:
        if original_mido is None:
            sys.modules.pop('mido', None)
        else:
            sys.modules['mido'] = original_mido


if __name__ == '__main__':
    sys.exit(run(sys.argv[1] if len(sys.argv) > 1 else 'tools/fm1_install.py'))
