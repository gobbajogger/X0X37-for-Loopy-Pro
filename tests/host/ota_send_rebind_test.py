#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Host-negative MIDI transport test: send must report observed mid-send unit loss.

Run: python3 -B tests/host/ota_send_rebind_test.py tools/fm1_install.py
Extracts the actual MidoLink AST from the provided source. Simulated transport
only; an output packet already accepted by a foreign unit cannot be recalled.
An authenticated atomic unit-bound transport is still REQUIRED for real OTA.
"""
import ast
import copy
import pathlib
import queue
import sys
import time
from types import ModuleType, SimpleNamespace


class Input:
    def close(self):
        pass


class Output:
    def __init__(self, mido):
        self.mido = mido
        self.sent = []
        self.fail = False

    def send(self, msg):
        if self.fail:
            raise OSError('transport output failed')
        hook = self.mido.send_hook
        if hook:
            hook()
        self.sent.append(bytes(msg.data))

    def close(self):
        pass


class FakeMido(ModuleType):
    def __init__(self):
        super().__init__('mido')
        self.send_hook = None
        self.outputs = []

    def open_input(self, name, callback):
        return Input()

    def open_output(self, name):
        out = Output(self)
        self.outputs.append(out)
        return out

    @staticmethod
    def Message(kind, data):
        return SimpleNamespace(data=data)


class Backend:
    def __init__(self, token='unit-A', name='SMK-37 Pro'):
        self.token, self.name = token, name

    def physical_id(self, name):
        return self.token if name == self.name else None

    def input_names(self):
        return [self.name]


def load(path):
    path = pathlib.Path(path)
    tree = ast.parse(path.read_text(encoding='utf-8'))
    cls = next(n for n in tree.body if isinstance(n, ast.ClassDef) and n.name == 'MidoLink')
    module = ast.fix_missing_locations(ast.Module(body=[copy.deepcopy(cls)], type_ignores=[]))
    fake = FakeMido()
    sys.modules['mido'] = fake
    ns = {'queue': queue, 'time': time, 'DELAY': {'open': 0}}
    exec(compile(module, str(path), 'exec'), ns)
    return ns['MidoLink'], fake


def run(path):
    Link, midi = load(path)
    checks = []

    def ck(condition, label):
        condition = bool(condition)
        checks.append(condition)
        print(('PASS' if condition else 'FAIL') + ' ' + label)

    p = b'\xf0\x04\x05\xf7'
    b = Backend()
    link = Link(b, b.name, b.name)
    ck(link.send(p), 'stable physical A permits outbound packet')
    ck(link.out.sent == [p[1:-1]] and not link.lost, 'stable A packet and link status retained')
    link.close()

    b = Backend()
    link = Link(b, b.name, b.name)
    midi.send_hook = lambda: setattr(b, 'token', 'unit-B')
    returned = link.send(p)
    ck(returned is False, 'post-send identity check reports an observed A-to-B in-send rebind')
    ck(link.out.sent == [p[1:-1]], 'host admits first packet was already handed to output; not atomic protection')
    b.token = 'unit-A'
    midi.send_hook = None
    ck(link.lost, 'observed in-send swap permanently taints the opened link after A returns')
    ck(link.send(p) is False and len(link.out.sent) == 1, 'tainted link never sends a second packet')
    link.close()

    b = Backend()
    link = Link(b, b.name, b.name)
    midi.send_hook = lambda: setattr(b, 'token', None)
    ck(link.send(p) is False, 'identity disappearing during output returns failure')
    midi.send_hook = None
    b.token = 'unit-A'
    ck(link.lost and link.send(p) is False, 'failed identity provider cannot revive the tainted link')
    link.close()

    b = Backend()
    b.name = 'Felucca Update'
    link = Link(b, b.name, b.name)
    ck(link.send(p) and not link.lost, 'fresh authenticated loader rename remains usable')
    link.close()

    b = Backend()
    link = Link(b, b.name, b.name)
    link.out.fail = True
    ck(link.send(p) is False, 'failed native output still reports False')
    link.close()

    print('SUMMARY', sum(checks), '/', len(checks), 'passed')
    return 0 if all(checks) else 1


if __name__ == '__main__':
    sys.exit(run(sys.argv[1] if len(sys.argv) > 1 else 'tools/fm1_install.py'))