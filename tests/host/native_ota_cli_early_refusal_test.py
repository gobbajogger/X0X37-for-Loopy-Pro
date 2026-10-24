#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Native RtMidi OTA preflight against the actual installer main()/run() AST.

Usage: python3 -B native_ota_cli_early_refusal_test.py tools/fm1_install.py
Synthetic backends; no device, package, or firmware bytes are needed.
"""
import argparse
import ast
import contextlib
import io
from pathlib import Path
import sys
from types import SimpleNamespace


class InstallError(Exception):
    def __init__(self, code, message):
        self.code = code
        super().__init__(message)


class ProbeReached(Exception):
    pass


def source_nodes(path):
    tree = ast.parse(path.read_text(encoding='utf-8'), filename=str(path))
    functions = [node for node in tree.body if isinstance(node, ast.FunctionDef)
                 and node.name in ('main', 'run')]
    if {node.name for node in functions} != {'main', 'run'} or len(functions) != 2:
        raise ValueError('expected exactly one production main() and run()')
    # Constant is part of actual source rather than a synthetic test override.
    const = [node for node in tree.body if isinstance(node, (ast.Assign, ast.AnnAssign))
             and any(isinstance(name, ast.Name) and name.id == 'NATIVE_RTMIDI_UNSUPPORTED'
                     for name in (node.targets if isinstance(node, ast.Assign) else [node.target]))]
    if len(const) > 1:
        raise ValueError('ambiguous native OTA diagnostic constant')
    return ast.fix_missing_locations(ast.Module(body=const + functions, type_ignores=[])), tree


def environment(source):
    events = []

    class MidoBackend:
        def __init__(self):
            events.append('mido-backend')

    class FakeBackend:
        pass

    class Device:
        id = SimpleNamespace(text='SMK-37 Pro_900', loader=False)
        name = 'SMK-37 Pro'
        link = SimpleNamespace(close=lambda: events.append('info-close'))

    class Updater:
        def __init__(self, backend, port):
            events.append('updater-init')
            self.backend = backend
            self.port = port
        def find(self, *args, **kwargs):
            events.append('info-find')
            return Device()

    def probe_package(*args, **kwargs):
        events.append('package-load')
        raise ProbeReached('package was read despite forbidden native OTA' if not isinstance(args[0], Path) else 'package input')

    def stop(name):
        def guarded(*args, **kwargs):
            events.append(name)
            return 0
        return guarded

    env = dict(
        __doc__='native OTA test', sys=sys, argparse=argparse, out=io.StringIO(),
        InstallError=InstallError, MidoBackend=MidoBackend, Updater=Updater,
        Progress=object, EXIT={'model': 6, 'head': 11},
        head_ref_path=lambda a: 'trusted-head', head_guard=stop('head-guard'),
        gate_guard=stop('gate-guard'), foreground_guard=stop('foreground-guard'),
        hardware_lock=stop('hardware-lock'), load_package=probe_package,
        not_found=lambda up: InstallError('notfound', 'not found'),
        ask_tty=lambda prompt: True, with_ctl=lambda a, product, image, out: image,
        **{'time': SimpleNamespace(strftime=lambda f: '20261025'), 'SameFake': FakeBackend}
    )
    exec(compile(source, 'tools/fm1_install.py', 'exec'), env)
    return events, env, MidoBackend, FakeBackend


def capture(func):
    out, err = io.StringIO(), io.StringIO()
    with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
        try:
            value, exception = func(), None
        except BaseException as exc:
            value, exception = None, exc
    return value, exception, out.getvalue(), err.getvalue()


def main():
    source_file = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).resolve().parents[2] / 'tools/fm1_install.py'
    source, syntax = source_nodes(source_file)
    backend_node = next((node for node in syntax.body if isinstance(node, ast.ClassDef)
                         and node.name == 'MidoBackend'), None)
    production_methods = {node.name for node in backend_node.body if isinstance(node, ast.FunctionDef)} if backend_node else set()
    # A future authenticated, atomic backend requires re-evaluating this specific refusal.
    outcomes = []
    ok = backend_node is not None and 'physical_id' not in production_methods and 'output_physical_id' not in production_methods
    print(f"{'PASS' if ok else 'FAIL'} exact current MidoBackend lacks per-unit identity providers")
    outcomes.append(ok)

    events, env, MidoBackend, FakeBackend = environment(source)
    code, exc, stdout, stderr = capture(lambda: env['main'](['fictional.fwsc', '--yes'], backend=None))
    ok = (code == 6 and exc is None and events == [] and not stdout
          and 'unsupported' in stderr.lower() and 'physical' in stderr.lower()
          and 'atomic' in stderr.lower() and 'no midi' in stderr.lower())
    print(f"{'PASS' if ok else 'FAIL'} default real CLI rejects before package/gate/port/backup/prompt: code={code}, events={events}, error={exc}, stderr={stderr.strip()!r}")
    outcomes.append(ok)

    events, env, MidoBackend, FakeBackend = environment(source)
    opts = SimpleNamespace(info=False, port=None, package='fictional.fwsc', force=False,
                           backup='my-samples.x0xslots', no_slots=False, ctl=None, yes=True)
    code, exc, stdout, stderr = capture(lambda: env['run'](opts, MidoBackend(), io.StringIO(), lambda prompt: (_ for _ in ()).throw(ProbeReached('prompt reached'))))
    ok = isinstance(exc, InstallError) and exc.code == 'model' and events == ['mido-backend'] and not stdout
    print(f"{'PASS' if ok else 'FAIL'} direct run(MidoBackend) rejects before Updater/find/backup/confirmation: events={events}, error={exc}")
    outcomes.append(ok)

    events, env, MidoBackend, FakeBackend = environment(source)
    code, exc, stdout, stderr = capture(lambda: env['main'](['fictional.fwsc'], backend=FakeBackend(), out=io.StringIO()))
    ok = isinstance(exc, ProbeReached) and events == ['updater-init', 'package-load']
    print(f"{'PASS' if ok else 'FAIL'} explicitly injected fake backend still reaches normal package/simulator path: events={events}, error={exc}")
    outcomes.append(ok)

    events, env, MidoBackend, FakeBackend = environment(source)
    code, exc, stdout, stderr = capture(lambda: env['main'](['--info'], backend=None, out=io.StringIO()))
    ok = code == 0 and exc is None and events == ['hardware-lock', 'mido-backend', 'updater-init', 'info-find', 'info-close']
    print(f"{'PASS' if ok else 'FAIL'} read-only --info still queries and closes its link: code={code}, events={events}, error={exc}")
    outcomes.append(ok)

    events, env, MidoBackend, FakeBackend = environment(source)
    code, exc, stdout, stderr = capture(lambda: env['main'](['fictional.fwsc', '--head-check', '--head-ref', 'trusted-head'],
                                                         backend=None, out=io.StringIO()))
    ok = code == 0 and exc is None and events == ['head-guard']
    print(f"{'PASS' if ok else 'FAIL'} offline --head-check remains supported: code={code}, events={events}, error={exc}")
    outcomes.append(ok)

    passed = sum(outcomes)
    print(f'SUMMARY {passed}/{len(outcomes)}')
    return 0 if passed == len(outcomes) else 1


if __name__ == '__main__':
    raise SystemExit(main())
