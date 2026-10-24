#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Host-only physical MIDI endpoint negative tests against installer AST source.

The fake backend has independent physical MIDI ports and identity transitions.
Only the actual Updater methods are compiled; no USB, flash or user data exists.
Run: python3 -B ota_port_binding_test.py tools/fm1_install.py
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


class Clock:
    def __init__(self):
        self.tick = 0

    def monotonic(self):
        self.tick += 0.01
        return self.tick

    def sleep(self, seconds):
        self.tick += max(seconds, 0.01)


class Link:
    def __init__(self, backend, name):
        self.backend, self.name, self.closed = backend, name, False

    @property
    def ident(self):
        identity = self.backend.ports.get(self.name)
        return SimpleNamespace(text=identity, loader=identity.startswith('ota-')) if identity else None

    def send(self, packet):
        return True

    def close(self):
        self.closed = True


class Device:
    def __init__(self, link, ident, name):
        self.link, self.id, self.name = link, ident, name


class Backend:
    def __init__(self, ports):
        self.ports = dict(ports)
        self.written_to = []

    def input_names(self):
        return list(self.ports)

    def output_names(self):
        return list(self.ports)

    def open(self, name, out_name):
        assert name == out_name
        return Link(self, name)


def load_source(path):
    tree = ast.parse(pathlib.Path(path).read_text())
    src = next(n for n in tree.body if isinstance(n, ast.ClassDef) and n.name == 'Updater')
    names = ('__init__', 'candidates', 'find', 'wait_for', 'check', 'write', 'verify', 'install')
    methods = [copy.deepcopy(n) for n in src.body if isinstance(n, ast.FunctionDef) and n.name in names]
    assert [n.name for n in methods] == list(names), [n.name for n in methods]
    helper = [copy.deepcopy(n) for n in tree.body if isinstance(n, ast.FunctionDef)
              and n.name in ('model_of', 'same_keyboard')]
    matchers = [copy.deepcopy(n) for n in tree.body if isinstance(n, ast.Assign)
                and any(isinstance(t, ast.Name) and t.id in ('SMK_FAMILY', 'PORT_RE') for t in n.targets)]
    assert len(helper) == 2 and len(matchers) == 2
    klass = ast.ClassDef(name='RealUpdater', bases=[], keywords=[], body=methods, decorator_list=[])
    compiled = ast.fix_missing_locations(ast.Module(body=helper + matchers + [klass], type_ignores=[]))
    clock = Clock()
    delay = {'start': 0, 'loader': 0, 'wait_loader': .4, 'reboot': 0,
             'wait_reboot': .4, 'idle_check': 0, 'idle_write': 0, 'retry': .02}
    ns = {'re': re, 'time': clock, 'DELAY': delay, 'Device': Device,
          'UPGRADE': b'upgrade', 'FINISH_CHECK': 111, 'FINISH_WRITE': 222,
          'InstallError': InstallError, 'pair_output': lambda n, outs: n if n in outs else None,
          'handshake': lambda link, tries=2: link.ident}
    exec(compile(compiled, str(path), 'exec'), ns)

    class Updater(ns['RealUpdater']):
        def __init__(self, backend, port=None, loader=None, installed=None,
                     abort_write=False, rename_to=None):
            super().__init__(backend, port)
            self.loader_text, self.installed_text = loader, installed
            self.abort_write, self.rename_to = abort_write, rename_to

        def serve(self, link, image, finish, idle, progress):
            if finish == 111:
                if self.rename_to:
                    self.backend.ports.pop(link.name, None)
                    self.backend.ports[self.rename_to] = self.loader_text
                elif self.loader_text:
                    self.backend.ports[link.name] = self.loader_text
                return 1, True, False
            self.backend.written_to.append(link.name)
            if self.abort_write:
                return 0, False, True
            if self.installed_text:
                self.backend.ports[link.name] = self.installed_text
            return 1, True, False

    root_run = next(n for n in tree.body if isinstance(n, ast.FunctionDef) and n.name == 'run')
    selectors = [n.value.args[0] for n in ast.walk(root_run)
                 if isinstance(n, ast.Assign) and any(isinstance(t, ast.Name) and t.id == 'back'
                    for t in n.targets) and isinstance(n.value, ast.Call)
                 and isinstance(n.value.func, ast.Attribute) and n.value.func.attr == 'wait_for']
    assert len(selectors) == 2
    predicates = [eval(compile(ast.fix_missing_locations(ast.Expression(body=copy.deepcopy(s))),
                                str(path), 'eval'), {**ns, 'product': 'SMK-37 Pro_900'}) for s in selectors]
    return Updater, predicates


def evaluate(path):
    Updater, selectors = load_source(path)
    success, failure = 0, 0

    def check(ok, msg):
        nonlocal success, failure
        if ok:
            success += 1
        else:
            failure += 1
        print(('PASS ' if ok else 'FAIL ') + msg)

    def attempt(up, initial=None, product='SMK-37 Pro_900'):
        try:
            dev = initial or up.find()
            if not dev:
                return None, 'notfound'
            return up.install(dev, b'image', product, lambda *args: None), None
        except InstallError as exc:
            return None, exc.code

    # Explicit port and same physical endpoint: both supported cross-model moves.
    for initial, product in [('SMK-37 Elite_016', 'SMK-37 Pro_900'),
                             ('SMK-37 Pro_016', 'SMK-37 Elite_900')]:
        b = Backend({'smk-port-A': initial, 'smk-port-B': 'FM-1_900'})
        up = Updater(b, 'smk-port-A', loader='ota-'+product, installed=product)
        result, err = attempt(up, product=product)
        check(result == product and not err and b.written_to == ['smk-port-A'],
              initial+' -> '+product+' keeps SAME physical MIDI port')

    # Without explicit port the user must disambiguate two identical devices.
    b = Backend({'smk-port-A': 'SMK-37 Pro_016', 'smk-port-B': 'SMK-37 Pro_016'})
    up = Updater(b)
    _, err = attempt(up)
    check(err == 'model' and not b.written_to, 'two same-model initial keyboards: refuse before UPGRADE')

    # A too-broad --port substring must not bypass the ambiguous-device guard.
    b = Backend({'smk-port-A-1': 'SMK-37 Pro_016', 'smk-port-A-2': 'SMK-37 Pro_016'})
    up = Updater(b, 'smk-port-A')
    _, err = attempt(up)
    check(err == 'model' and not b.written_to, '--port with two matching devices refuses selection')

    # One device enters update mode on port A, but another on B has the right loader.
    b = Backend({'smk-port-A': 'SMK-37 Pro_016', 'smk-port-B': 'ota-SMK-37 Pro_900'})
    up = Updater(b, 'smk-port-A', loader='ota-SMK-37 Elite_900', installed='SMK-37 Pro_900')
    _, err = attempt(up)
    check(err == 'noloader' and not b.written_to,
          'never flash foreign Pro loader at another MIDI port')

    # If the loader is physically re-enumerated under an unproven name, stop.
    b = Backend({'smk-port-A': 'SMK-37 Pro_016', 'smk-port-B': 'FM-1_900'})
    up = Updater(b, 'smk-port-A', loader='ota-SMK-37 Pro_900',
                 installed='SMK-37 Pro_900', rename_to='smk-port-C')
    _, err = attempt(up)
    check(err == 'noloader' and not b.written_to,
          'unverifiable changed MIDI port at handover must fail closed')

    # Correct loader on original port, foreign identical version present after
    # writing: verification must never falsely succeed through the foreign unit.
    b = Backend({'smk-port-A': 'SMK-37 Pro_016', 'smk-port-B': 'SMK-37 Pro_900'})
    up = Updater(b, 'smk-port-A', loader='ota-SMK-37 Pro_900', installed=None)
    _, err = attempt(up)
    check(err == 'noreturn' and b.written_to == ['smk-port-A'],
          'verify cannot count an identical foreign running unit after target remains loader')

    # Stale version on a second device cannot preempt correct unit; wrong
    # version on the pinned port should still yield the useful mismatch code.
    b = Backend({'smk-port-B': 'SMK-37 Pro_899', 'smk-port-A': 'SMK-37 Pro_900'})
    up = Updater(b)
    up._bound_port = 'smk-port-A'
    try:
        ok = up.verify('SMK-37 Pro_900', lambda *a: None) == 'SMK-37 Pro_900'
    except InstallError:
        ok = False
    check(ok, 'verify skips stale same-model sibling before correct unit')
    b = Backend({'smk-port-A': 'SMK-37 Pro_899', 'smk-port-B': 'SMK-37 Pro_900'})
    up = Updater(b)
    up._bound_port = 'smk-port-A'
    try:
        up.verify('SMK-37 Pro_900', lambda *a: None)
        err = None
    except InstallError as exc:
        err = exc.code
    check(err == 'mismatch', 'stale firmware on originating port retains mismatch diagnostic')

    # Two post-install write/readback operations must never use unrelated port B.
    for ix, selector in enumerate(selectors):
        b = Backend({'smk-port-B': 'SMK-37 Pro_900', 'smk-port-A': 'SMK-37 Pro_900'})
        up = Updater(b)
        up._bound_port = 'smk-port-A'
        try:
            dev = up.wait_for(selector, .4)
        except InstallError:
            dev = None
        check(dev is not None and dev.name == 'smk-port-A',
              ('sample restore' if ix == 0 else 'controller CRC readback')+' binds original physical port')
        if dev:
            dev.link.close()
        b = Backend({'smk-port-B': 'SMK-37 Pro_900'})
        up = Updater(b)
        up._bound_port = 'smk-port-A'
        try:
            dev = up.wait_for(selector, .4)
        except InstallError:
            dev = None
        check(dev is None,
              ('sample restore' if ix == 0 else 'controller CRC readback')+' refuses wrong sole unit')

    # No physical port was established; textual identity alone cannot verify.
    b = Backend({'smk-port-B': 'SMK-37 Pro_900'})
    up = Updater(b)
    try:
        got = up.verify('SMK-37 Pro_900', lambda *a: None)
        err = None
    except InstallError as exc:
        got, err = None, exc.code
    check(got is None and err == 'model', 'unbound verification fails closed')

    # An already-in-loader resume must check package destination model.
    b = Backend({'smk-port-A': 'ota-SMK-37 Elite_900'})
    up = Updater(b, 'smk-port-A')
    _, err = attempt(up, product='SMK-37 Pro_900')
    check(err == 'model' and not b.written_to, 'already-in-loader wrong-model resume refused')

    # Correct resume may write, but cannot verify via an unbound foreign unit.
    b = Backend({'smk-port-A': 'ota-SMK-37 Pro_900', 'smk-port-B': 'SMK-37 Pro_900'})
    up = Updater(b, 'smk-port-A', installed='SMK-37 Pro_900')
    result, err = attempt(up)
    check(result == 'SMK-37 Pro_900' and b.written_to == ['smk-port-A'],
          'correct-model interrupted resume stays on selected physical port')

    b = Backend({'smk-port-A': 'SMK-37 Pro_016'})
    up = Updater(b, 'smk-port-A', loader='ota-SMK-37 Pro_900', abort_write=True)
    result, err = attempt(up)
    check(result is None and err == 'lost' and b.written_to == ['smk-port-A'],
          'failed write does not verify or run destructive restore')

    print(f'SUMMARY {success}/{success+failure} PASS; {failure} failures')
    return int(failure != 0)


if __name__ == '__main__':
    source = pathlib.Path(sys.argv[1]) if len(sys.argv) > 1 else pathlib.Path('tools/fm1_install.py')
    sys.exit(evaluate(source))
