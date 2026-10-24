#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Protect native OTA/model reconnects and post-install user-sample restore.

This is a no-device native host test. It compiles the actual Python source's
Updater.check()/verify() ASTs rather than duplicating their implementations,
and evaluates run()'s actual post-install wait_for predicate. Usage:
python3 tests/install_ota_reconnect_ast_test.py [tools/fm1_install.py]
"""
import ast
import copy
import pathlib
import re
import sys
from types import SimpleNamespace

class InstallError(Exception):
    def __init__(self, code, message):
        super().__init__(message)
        self.code = code

class Link:
    def __init__(self):
        self.sent, self.closed = [], False
    def send(self, packet):
        self.sent.append(packet)
        return True
    def close(self):
        self.closed = True

class Identity:
    def __init__(self, text):
        self.text = text
        self.loader = text.startswith('ota-')

class Device:
    def __init__(self, text):
        self.id, self.link = Identity(text), Link()

def load_checked_methods(source_path):
    root = ast.parse(pathlib.Path(source_path).read_text())
    updater = next(n for n in root.body if isinstance(n, ast.ClassDef) and n.name == 'Updater')
    selected = [copy.deepcopy(n) for n in updater.body if isinstance(n, ast.FunctionDef) and n.name in ('check', 'verify')]
    if len(selected) != 2:
        raise AssertionError('Updater.check and Updater.verify are required')
    # Compile the repository's real keyboard-family predicate too: false
    # model equivalence would defeat both preflash and destructive restore tests.
    helpers = [copy.deepcopy(n) for n in root.body if isinstance(n, ast.FunctionDef)
               and n.name in ('model_of', 'same_keyboard')]
    families = [copy.deepcopy(n) for n in root.body if isinstance(n, ast.Assign)
                and any(isinstance(t, ast.Name) and t.id == 'SMK_FAMILY' for t in n.targets)]
    if len(helpers) != 2 or len(families) != 1:
        raise AssertionError('exact model helper definitions are required')
    declaration = ast.ClassDef(name='SourceMethods', bases=[], keywords=[], body=selected, decorator_list=[])
    module = ast.fix_missing_locations(ast.Module(body=helpers + families + [declaration], type_ignores=[]))
    import time
    ns = {'time': time, 're': re, 'DELAY': dict(start=0, loader=0, wait_loader=0, reboot=0, wait_reboot=0, idle_check=0),
          'FINISH_CHECK': 0xE0000000, 'UPGRADE': b'UPGRADE',
          'InstallError': InstallError, 'product': 'SMK-37 Pro_900'}
    real_sleep = time.sleep
    time.sleep = lambda _n: None
    try:
        exec(compile(module, str(source_path), 'exec'), ns)
        # Return a wrapped class; scoped callbacks use the current code's actual methods.
        class FakeUpdater(ns['SourceMethods']):
            def __init__(self, devices): self.devices = devices
            def serve(self, *args): return 1, True, False
            def wait_for(self, want, duration): return next((d for d in self.devices if want(d.id)), None)

        run = next(n for n in root.body if isinstance(n, ast.FunctionDef) and n.name == 'run')
        calls = [n for n in ast.walk(run) if isinstance(n, ast.Assign)
                 and any(isinstance(t, ast.Name) and t.id == 'back' for t in n.targets)
                 and isinstance(n.value, ast.Call) and isinstance(n.value.func, ast.Attribute)
                 and n.value.func.attr == 'wait_for']
        if len(calls) != 2:
            # run() has the sample-restore and controller-CRC wait_for call sites.
            raise AssertionError(f'expected two run() post-install wait_for sites, saw {len(calls)}')
        lambdas = [n.value.args[0] for n in calls]
        checkers = [eval(compile(ast.fix_missing_locations(ast.Expression(body=copy.deepcopy(n))),
                                 str(source_path), 'eval'), ns) for n in lambdas]
        return FakeUpdater, checkers
    finally:
        time.sleep = real_sleep

def main(path):
    FakeUpdater, postselectors = load_checked_methods(path)
    errors = 0
    def check(cond, label):
        nonlocal errors
        print(f"{'PASS' if cond else 'FAIL'} {label}")
        errors += not cond
    foreign_loader, target_loader = Device('ota-FM-1_900'), Device('ota-SMK-37 Pro_900')
    correct_running, foreign_running = Device('SMK-37 Pro_900'), Device('FM-1_900')
    stale_running = Device('SMK-37 Pro_901')
    start = Device('SMK-37 Pro_016')
    no_step = lambda *args: None
    check(FakeUpdater([foreign_loader, target_loader]).check(start,b'data',no_step) is target_loader,
          'do not select unrelated FM-1 loader before target SMK loader')
    refused = False
    try: FakeUpdater([foreign_loader]).check(Device('SMK-37 Pro_016'),b'data',no_step)
    except InstallError as e: refused = e.code == 'noloader'
    check(refused, 'refuse foreign-family-only loader before any firmware write')
    accepted = False
    try: accepted = FakeUpdater([foreign_running, correct_running]).verify('SMK-37 Pro_900',no_step) == 'SMK-37 Pro_900'
    except InstallError: pass
    check(accepted,'ignore foreign-family first running device at final verify')
    accepted = False
    try: accepted = FakeUpdater([stale_running, correct_running]).verify('SMK-37 Pro_900',no_step) == 'SMK-37 Pro_900'
    except InstallError: pass
    check(accepted,'ignore stale same-family firmware identity while waiting for verified version')
    for j, selector in enumerate(postselectors):
        check(FakeUpdater([foreign_running, correct_running]).wait_for(selector,0) is correct_running,
              f'postinstall operation {j+1}: select matching product')
        check(FakeUpdater([foreign_running]).wait_for(selector,0) is None,
              f'postinstall operation {j+1}: refuse foreign-only product')
        check(FakeUpdater([stale_running]).wait_for(selector,0) is None,
              f'postinstall operation {j+1}: refuse wrong firmware version on same family')
    check(FakeUpdater([target_loader]).check(Device('SMK-37 Pro_016'),b'data',no_step) is target_loader,
          'retain normal same-family OTA resume')
    mismatch = False
    try: FakeUpdater([Device('SMK-37 Pro_901')]).verify('SMK-37 Pro_900',no_step)
    except InstallError as e: mismatch = e.code == 'mismatch'
    check(mismatch,'retain exact-version mismatch diagnostics for same family')
    print(f'CHECKS={12-errors}/12 FAILURES={errors}')
    return int(errors != 0)

if __name__ == '__main__':
    src = pathlib.Path(sys.argv[1]) if len(sys.argv) > 1 else pathlib.Path(__file__).resolve().parents[1]/'tools/fm1_install.py'
    sys.exit(main(src))