#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Host-only regression: raw duplicate MIDI endpoint names must fail closed.

Extracts actual MidoBackend enumeration and Updater discovery methods from
installer source; never opens a real MIDI endpoint, writes firmware or data.
Run: python3 -B tests/host/ota_raw_midi_names_test.py tools/fm1_install.py
"""
import ast
import copy
from pathlib import Path
import re
import sys
from types import SimpleNamespace


class InstallError(Exception):
    def __init__(self, code, message):
        super().__init__(message)
        self.code = code


class Link:
    def __init__(self):
        self.closed = False

    def close(self):
        self.closed = True


class Device:
    def __init__(self, link, ident, name):
        self.link, self.id, self.name = link, ident, name


def extracted_classes(source):
    tree = ast.parse(Path(source).read_text(encoding='utf-8'))
    targets = {'MidoBackend': ('input_names', 'output_names'),
               'Updater': ('__init__', '_physical', 'candidates', 'find')}
    classes = []
    for name, methods in targets.items():
        old = next(n for n in tree.body if isinstance(n, ast.ClassDef) and n.name == name)
        selected = [copy.deepcopy(n) for n in old.body
                    if isinstance(n, ast.FunctionDef) and n.name in methods]
        expected = tuple(m for m in methods if m != '_physical' or any(
            isinstance(n, ast.FunctionDef) and n.name == '_physical' for n in old.body))
        assert tuple(n.name for n in selected) == expected, (name, [n.name for n in selected])
        classes.append(ast.ClassDef(name=name, bases=[], keywords=[], body=selected,
                                    decorator_list=[]))
    port_expr = next(n for n in tree.body if isinstance(n, ast.Assign)
                     and any(isinstance(t, ast.Name) and t.id == 'PORT_RE' for t in n.targets))
    module = ast.fix_missing_locations(ast.Module(body=[copy.deepcopy(port_expr)] + classes,
                                           type_ignores=[]))
    ns = {'re': re, 'InstallError': InstallError, 'Device': Device,
          'pair_output': lambda name, outs, backend=None: name if name in outs else None,
          'handshake': lambda _link, _tries: SimpleNamespace(text='SMK-37 Pro_900')}
    exec(compile(module, str(source), 'exec'), ns)
    return ns['MidoBackend'], ns['Updater']


class FakeMidi:
    def __init__(self, inputs, outputs):
        self.inputs, self.outputs = list(inputs), list(outputs)

    def get_input_names(self):
        return list(self.inputs)

    def get_output_names(self):
        return list(self.outputs)


def probe(backend_class, updater_class, inputs, outputs, selected_port=None):
    backend = backend_class.__new__(backend_class)
    backend.mido = FakeMidi(inputs, outputs)
    attempts = []

    def never_real_open(inp, out):
        attempts.append((inp, out))
        return Link()

    backend.open = never_real_open
    updater = updater_class(backend, selected_port)
    try:
        result = updater.find()
    except InstallError as exc:
        return exc.code, attempts
    if result:
        result.link.close()
    return ('found' if result else 'notfound'), attempts


def main(source):
    backend, updater = extracted_classes(source)
    tests = [
        ('unique input/output permits one endpoint', ['SMK-37 Pro'], ['SMK-37 Pro'], 'found', 1, None),
        ('two distinct matching keyboards rejected', ['SMK-37 Pro 1', 'SMK-37 Pro 2'],
         ['SMK-37 Pro 1', 'SMK-37 Pro 2'], 'model', 2, None),
        ('duplicate raw input refused before opening any MIDI endpoint',
         ['SMK-37 Pro', 'SMK-37 Pro'], ['SMK-37 Pro'], 'model', 0, None),
        ('duplicate raw output refused before opening any MIDI endpoint',
         ['SMK-37 Pro'], ['SMK-37 Pro', 'SMK-37 Pro'], 'model', 0, None),
        ('duplicate loader input refused before opening any MIDI endpoint',
         ['Felucca Update', 'Felucca Update'], ['Felucca Update'], 'model', 0, None),
        ('duplicate loader output refused before opening any MIDI endpoint',
         ['Felucca Update'], ['Felucca Update', 'Felucca Update'], 'model', 0, None),
        ('explicit --port cannot override raw duplicate inputs',
         ['SMK-37 Pro', 'SMK-37 Pro'], ['SMK-37 Pro'], 'model', 0, 'SMK-37 Pro'),
        ('unrelated duplicate names fail closed without pretending device identity',
         ['SMK-37 Pro', 'Generic Port', 'Generic Port'], ['SMK-37 Pro'], 'model', 0, None),
    ]
    passed = 0
    for message, inputs, outputs, expected, opens, chosen_port in tests:
        got, attempts = probe(backend, updater, inputs, outputs, chosen_port)
        safe = got == expected and len(attempts) == opens
        print(('PASS' if safe else 'FAIL') + f' {message}: result={got}, opens={len(attempts)}')
        passed += bool(safe)
    print(f'SUMMARY {passed}/{len(tests)} PASS')
    return int(passed != len(tests))


if __name__ == '__main__':
    sys.exit(main(Path(sys.argv[1]) if len(sys.argv) > 1 else Path('tools/fm1_install.py')))
