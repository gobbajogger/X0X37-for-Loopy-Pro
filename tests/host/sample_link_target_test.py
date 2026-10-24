#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Source-AST native regression of standalone Felucca/X0X sample upload/backup/restore
MIDI pair selection. Test-only simulated endpoints; no physical USB or device write.

Usage: python3 -B tests/host/sample_link_target_test.py tools/fm1_sample_upload.py
"""
import ast
from pathlib import Path
import sys
from types import ModuleType, SimpleNamespace


class Clock:
    def __init__(self):
        self.now = 0.0

    def time(self):
        self.now += 0.7
        return self.now

    def sleep(self, _seconds):
        pass


class FakeMidi(ModuleType):
    def __init__(self, inputs, outputs, *, reply=True, input_open_error=False):
        super().__init__('mido')
        self.inputs, self.outputs = list(inputs), list(outputs)
        self.reply, self.input_open_error = reply, input_open_error
        self.opened, self.sent, self.closed = [], [], []
        self.pending = []

    def get_input_names(self):
        return list(self.inputs)

    def get_output_names(self):
        return list(self.outputs)

    def open_output(self, name):
        self.opened.append(('output', name))
        outer = self

        class Output:
            def send(self, msg):
                outer.sent.append((name, tuple(msg.data)))
                if outer.reply:
                    outer.pending.append(SimpleNamespace(type='sysex', data=tuple([0x7D, 0x46, 0x4C, msg.data[3], 0])))

            def close(self):
                outer.closed.append(('output', name))
        return Output()

    def open_input(self, name):
        self.opened.append(('input', name))
        if self.input_open_error:
            raise OSError('fake input open failed')
        outer = self

        class Input:
            def iter_pending(self):
                while outer.pending:
                    yield outer.pending.pop(0)

            def close(self):
                outer.closed.append(('input', name))
        return Input()

    @staticmethod
    def Message(kind, data):
        return SimpleNamespace(type=kind, data=data)


def link_type(filename):
    src = Path(filename).read_text(encoding='utf-8')
    nodes = [n for n in ast.parse(src).body if isinstance(n, ast.ClassDef) and n.name == 'Link']
    if len(nodes) != 1 or not {'__init__', 'req'}.issubset(
            {n.name for n in nodes[0].body if isinstance(n, (ast.FunctionDef, ast.AsyncFunctionDef))}):
        raise RuntimeError('expected exact production sample Link class/methods missing')
    block = ast.fix_missing_locations(ast.Module(body=[nodes[0]], type_ignores=[]))
    ns = {'sys': sys, 'time': Clock(), 'HDR': [0x7D, 0x46, 0x4C]}
    exec(compile(block, str(filename), 'exec'), ns)
    return ns['Link']


def run_case(Link, name, ins, outs, *, allowed=False, reply=True, input_open_error=False):
    fake = FakeMidi(ins, outs, reply=reply, input_open_error=input_open_error)
    old = sys.modules.get('mido')
    sys.modules['mido'] = fake
    raised, link = None, None
    try:
        link = Link()
    except (SystemExit, OSError, RuntimeError) as exc:
        raised = exc
    finally:
        if old is None:
            sys.modules.pop('mido', None)
        else:
            sys.modules['mido'] = old
    if allowed:
        good = (raised is None and link is not None and
                len(fake.opened) == 2 and fake.opened[0][0] == 'output' and
                fake.opened[1][0] == 'input' and
                fake.opened[0][1] == fake.opened[1][1] and
                len(fake.sent) == 1 and fake.sent[0][1][3] == 1)
    else:
        good = (raised is not None and link is None and
                all(msg[1][3] not in (11, 12, 13, 14) for msg in fake.sent) and
                (not reply or input_open_error or not fake.opened) and
                (not input_open_error or ('output', outs[0]) in fake.closed))
    print(('PASS' if good else 'FAIL') + ' ' + name +
          f' | opened={fake.opened} sends={len(fake.sent)} closed={fake.closed} error={type(raised).__name__ if raised else None}')
    return good


def main(source):
    Link = link_type(source)
    cases = [
        ('single matched Felucca', ['Felucca MIDI'], ['Felucca MIDI'], True, True, False),
        ('single matched X0X', ['X0X MIDI'], ['X0X MIDI'], True, True, False),
        ('unrelated MIDI alongside', ['Other MIDI', 'Felucca A'], ['Other MIDI', 'Felucca A'], True, True, False),
        ('foreign output singleton', ['Felucca A'], ['X0X B'], False, True, False),
        ('foreign input singleton', ['X0X B'], ['Felucca A'], False, True, False),
        ('two matching keyboards', ['Felucca A', 'Felucca B'], ['Felucca A', 'Felucca B'], False, True, False),
        ('two raw-identical port labels', ['Felucca A', 'Felucca A'], ['Felucca A', 'Felucca A'], False, True, False),
        ('one input two outputs', ['Felucca A'], ['Felucca A', 'Felucca B'], False, True, False),
        ('two inputs one output', ['Felucca A', 'Felucca B'], ['Felucca A'], False, True, False),
        ('no eligible output', ['Felucca A'], ['Generic B'], False, True, False),
        ('no eligible input', ['Generic B'], ['Felucca A'], False, True, False),
        ('all 3 probes unanswered', ['Felucca A'], ['Felucca A'], False, False, False),
        ('input open error closes output', ['Felucca A'], ['Felucca A'], False, True, True),
    ]
    got = [run_case(Link, n, i, o, allowed=a, reply=r, input_open_error=e) for n, i, o, a, r, e in cases]
    print(f'SUMMARY {sum(got)}/{len(got)}')
    return 0 if all(got) else 1


if __name__ == '__main__':
    raise SystemExit(main(sys.argv[1] if len(sys.argv) > 1 else 'tools/fm1_sample_upload.py'))
