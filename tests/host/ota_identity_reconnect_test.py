#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Native host regression: OTA reconnects must not select another model or stale version.

Usage: python3 -B tests/host/ota_identity_reconnect_test.py tools/fm1_install.py
Compiles exact source AST for Updater.check/verify, identity helpers and the
actual sample-restore/controller-readback selectors. No physical device or
full firmware build is exercised.
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


class Device:
    def __init__(self, identity):
        self.id = SimpleNamespace(text=identity, loader=identity.startswith("ota-"))
        self.link = Link()


def exact_source_methods(source_path):
    root = ast.parse(pathlib.Path(source_path).read_text())
    updater = next(n for n in root.body if isinstance(n, ast.ClassDef) and n.name == "Updater")
    names = ("check", "verify")
    methods = [copy.deepcopy(n) for n in updater.body if isinstance(n, ast.FunctionDef) and n.name in names]
    helpers = [copy.deepcopy(n) for n in root.body if isinstance(n, ast.FunctionDef)
               and n.name in ("model_of", "same_keyboard")]
    families = [copy.deepcopy(n) for n in root.body if isinstance(n, ast.Assign)
                and any(isinstance(t, ast.Name) and t.id == "SMK_FAMILY" for t in n.targets)]
    if len(methods) != 2 or len(helpers) != 2 or len(families) != 1:
        raise AssertionError("exact update methods and model helpers are required")
    cls = ast.ClassDef(name="SourceMethods", bases=[], keywords=[], body=methods, decorator_list=[])
    mod = ast.fix_missing_locations(ast.Module(body=helpers + families + [cls], type_ignores=[]))
    fake_time = SimpleNamespace(sleep=lambda _: None)
    ns = {"re": re, "time": fake_time,
          "DELAY": {k: 0 for k in ("start", "loader", "wait_loader", "reboot", "wait_reboot", "idle_check")},
          "UPGRADE": b"UPGRADE", "FINISH_CHECK": 0x70000000, "InstallError": InstallError}
    exec(compile(mod, str(source_path), "exec"), ns)

    class MockUpdater(ns["SourceMethods"]):
        def __init__(self, devices):
            self.devices = devices

        def serve(self, *args):
            return 1, True, False

        def wait_for(self, pred, seconds):
            return next((d for d in self.devices if pred(d.id)), None)

    run = next(n for n in root.body if isinstance(n, ast.FunctionDef) and n.name == "run")
    calls = [n for n in ast.walk(run) if isinstance(n, ast.Assign)
             and any(isinstance(t, ast.Name) and t.id == "back" for t in n.targets)
             and isinstance(n.value, ast.Call) and isinstance(n.value.func, ast.Attribute)
             and n.value.func.attr == "wait_for"]
    if len(calls) != 2:
        raise AssertionError(f"expected two postinstall wait_for sites, got {len(calls)}")
    selectors = []
    for n in calls:
        # Capture the source's *real* lambda, including its captured product name.
        expr = ast.fix_missing_locations(ast.Expression(body=copy.deepcopy(n.value.args[0])))
        selectors.append(eval(compile(expr, str(source_path), "eval"), {**ns, "product": "SMK-37 Pro_900"}))
    return MockUpdater, selectors


def main(source_path):
    MockUpdater, selectors = exact_source_methods(source_path)
    total, failed = 0, 0

    def check(condition, why):
        nonlocal total, failed
        total += 1
        failed += not condition
        print(f"{'PASS' if condition else 'FAIL'} {why}")

    target = Device("SMK-37 Pro_016")
    target_loader = Device("ota-SMK-37 Pro_016")
    foreign_loader = Device("ota-FM-1_016")
    variant_loader = Device("ota-SMK-37 Elite_016")
    correct = Device("SMK-37 Pro_900")
    foreign = Device("FM-1_900")
    variant = Device("SMK-37 Elite_900")
    stale_version = Device("SMK-37 Pro_899")
    callback = lambda *a: None

    chosen = MockUpdater([foreign_loader, variant_loader, target_loader]).check(target, b"image", callback)
    check(chosen is target_loader, "initial OTA handover selects exactly the originating model")
    for other, msg in ((foreign_loader, "foreign family"), (variant_loader, "different SMK model")):
        blocked = False
        try:
            MockUpdater([other]).check(Device("SMK-37 Pro_016"), b"image", callback)
        except InstallError as e:
            blocked = e.code == "noloader"
        check(blocked, f"initial OTA handover refuses {msg} loader")

    try:
        result = MockUpdater([foreign, variant, correct]).verify("SMK-37 Pro_900", callback)
    except InstallError:
        result = None
    check(result == "SMK-37 Pro_900", "final verify ignores foreign and different-model devices")

    for index, predicate in enumerate(selectors, start=1):
        action = "restore" if index == 1 else "controller readback"
        check(MockUpdater([foreign, variant, stale_version, correct]).wait_for(predicate, 0) is correct,
              f"{action}: select exact installed model and version")
        check(MockUpdater([foreign]).wait_for(predicate, 0) is None,
              f"{action}: do not run on foreign-only device")
        check(MockUpdater([variant]).wait_for(predicate, 0) is None,
              f"{action}: do not run on different SMK model")
        check(MockUpdater([stale_version]).wait_for(predicate, 0) is None,
              f"{action}: do not run on stale firmware version")

    check(MockUpdater([target_loader]).check(Device("SMK-37 Pro_016"), b"image", callback)
          is target_loader, "retain normal model-matched loader recovery")
    wrong_version = False
    try:
        MockUpdater([stale_version]).verify("SMK-37 Pro_900", callback)
    except InstallError as e:
        wrong_version = e.code == "mismatch"
    check(wrong_version, "retain meaningful same-model wrong-version diagnostic")
    check(MockUpdater([correct]).verify("SMK-37 Pro_900", callback) == "SMK-37 Pro_900",
          "retain successful exact-version OTA verification")
    print(f"CHECKS={total-failed}/{total} FAILURES={failed}")
    return int(failed != 0)


if __name__ == "__main__":
    src = pathlib.Path(sys.argv[1]) if len(sys.argv) > 1 else pathlib.Path("tools/fm1_install.py")
    sys.exit(main(src))
