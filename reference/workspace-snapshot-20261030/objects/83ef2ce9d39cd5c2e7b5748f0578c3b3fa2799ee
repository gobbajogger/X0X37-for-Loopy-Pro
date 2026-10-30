#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Host-only regression: incomplete post-OTA sample restore must never exit as success.

Run in the complete repository: python3 tests/fm1_install_restore_status_test.py
No MIDI port is opened and no firmware is flashed. The exact current run() and
plan_migration() function bodies are compiled from the repository's actual source.
Optional SOURCE env vars are for fail-before testing against an older checkout.
"""
import ast
import io
import os
from pathlib import Path
from types import SimpleNamespace
import time
import unittest

ROOT = Path(__file__).resolve().parents[1]
INSTALL = Path(os.environ.get("X0X_FM1_INSTALL_SOURCE", ROOT / "tools" / "fm1_install.py"))
SLOTS = Path(os.environ.get("X0X_X0X_SLOTS_SOURCE", ROOT / "tools" / "x0x_slots.py"))


def actual_function(path, name, namespace):
    tree = ast.parse(path.read_text(encoding="utf-8"), filename=str(path))
    node = next(n for n in tree.body if isinstance(n, ast.FunctionDef) and n.name == name)
    exec(compile(ast.Module(body=[node], type_ignores=[]), str(path), "exec"), namespace)
    return namespace[name]


class SlotError(Exception):
    pass


class InstallError(Exception):
    def __init__(self, code, message):
        super().__init__(message)
        self.code = code


plan_migration = actual_function(SLOTS, "plan_migration", {})


class Fixture:
    def __init__(self, sizes=(82000,), new_sizes=(4000,), at_risk=True, require_backup=False,
                 fail_backup=False, fail_restore=False, no_slots=False, loader=False):
        self.events = []
        self.skipped = []
        self.sizes = sizes
        self.new_sizes = new_sizes
        self.at_risk = at_risk
        self.fail_backup = fail_backup
        self.fail_restore = fail_restore
        self.loader = loader
        self.out = io.StringIO()
        self.args = SimpleNamespace(info=False, package="mock.fwsc", force=False, port=None,
            backup="qa-preserved-samples.x0xslots" if require_backup else None,
            no_slots=no_slots, yes=True, ctl_bytes=None)
        outer = self

        class XS:
            SlotError = SlotError

        class Link:
            def close(self):
                outer.events.append("close")

        class Updater:
            def __init__(self, backend, port):
                pass

            def find(self):
                return SimpleNamespace(id=SimpleNamespace(text="SMK-37 Elite_900", loader=outer.loader),
                                       name="virtual-no-device", link=Link())

            def install(self, *args):
                outer.events.append("flash")

            def wait_for(self, *args):
                return SimpleNamespace(id=SimpleNamespace(text="SMK-37 Elite_900", loader=False),
                                       name="virtual-no-device", link=Link())

            def reconnect_after(self, want, secs):
                # Match tools/fm1_install.py Updater.reconnect_after after F02 bind.
                return self.wait_for(want, secs)

        class Progress:
            def __init__(self, *args):
                pass

            def end(self):
                pass

        class SlotMove:
            def __init__(self):
                self.risk = list(range(len(sizes))) if at_risk else []
                self.new_map = [(0xC2000 + sum(new_sizes[:j]), n) for j, n in enumerate(new_sizes)]
                self.path = "qa-preserved-samples.x0xslots"

            def describe(self):
                return "existing sample slots"

            def backup(self, link, path, out):
                outer.events.append("backup")
                if fail_backup:
                    raise SlotError("injected archive write/readback corruption")

            def restore(self, link, out):
                outer.events.append("restore")
                if fail_restore:
                    raise SlotError("injected slot CRC readback mismatch")
                images = [bytes(n) for n in sizes]
                old_map = [(0xB0000 + k * 0x10000, n) for k, n in enumerate(sizes)]
                lay = {"slots": [{"nz": 0} for _ in new_sizes]}
                todo, skipped = plan_migration(images, self.risk, old_map, lay, self.new_map)
                for k, j in todo:
                    outer.events.append(f"readback:slot{k+1}->slot{j+1}")
                for s in skipped:
                    print("not written back:", s, file=out)
                outer.skipped = skipped
                return todo, skipped

            def help(self):
                return f"Backup retained at {self.path}; restore with tools/x0x_slots.py"

        def plan_slots(dev, image, product, require_backup=False):
            outer.events.append("plan")
            return SlotMove() if at_risk or require_backup else None

        namespace = {
            "Updater": Updater, "XS": XS, "xs": XS, "Progress": Progress,
            "InstallError": InstallError, "not_found": lambda up: InstallError("notfound", "no device"),
            "load_package": lambda path, force: ("SMK-37 Elite_900", b"mock firmware"),
            "with_ctl": lambda a, product, image, out: image,
            "same_keyboard": lambda old, new: old == new,
            "plan_slots": plan_slots, "DELAY": {"wait_reboot": 0},
            "time": time, "check_ctl": lambda *args: None,
        }
        fn = actual_function(INSTALL, "run", namespace)
        self.run = lambda ask=lambda prompt: True: fn(self.args, object(), self.out, ask)


class RestoreStatus(unittest.TestCase):
    def assert_incomplete(self, f, expected_skips):
        with self.assertRaises(InstallError) as cm:
            f.run()
        self.assertEqual(cm.exception.code, "slots", "installed-but-unrestored requires exit code 7")
        self.assertIn("qa-preserved-samples.x0xslots", str(cm.exception), "retain recovery location")
        self.assertEqual(len(f.skipped), expected_skips)
        self.assertLess(f.events.index("backup"), f.events.index("flash"))

    def test_partial_restore_is_incomplete(self):
        f = Fixture(sizes=(2000, 10000), new_sizes=(3000, 4000))
        self.assert_incomplete(f, 1)
        self.assertIn("readback:slot1->slot1", f.events)
        self.assertIn("not written back:", f.out.getvalue())

    def test_all_slots_unrestorable_is_incomplete(self):
        self.assert_incomplete(Fixture(sizes=(82000,), new_sizes=(4000,)), 1)

    def test_no_new_sample_map_is_incomplete(self):
        f = Fixture(sizes=(2000,), new_sizes=())
        with self.assertRaises(InstallError) as cm:
            f.run()
        self.assertEqual(cm.exception.code, "slots")
        self.assertIn("qa-preserved-samples.x0xslots", str(cm.exception))
        self.assertIn("backup", f.events)
        self.assertIn("flash", f.events)
        self.assertNotIn("restore", f.events)

    def test_all_restored_is_success(self):
        f = Fixture(sizes=(2000, 3000), new_sizes=(4000, 4000))
        self.assertEqual(f.run(), 0)
        self.assertFalse(f.skipped)
        self.assertEqual(f.events.count("readback:slot1->slot1"), 1)
        self.assertEqual(f.events.count("readback:slot2->slot2"), 1)

    def test_known_no_risk_is_success(self):
        f = Fixture(at_risk=False)
        self.assertEqual(f.run(), 0)
        self.assertNotIn("backup", f.events)

    def test_explicit_backup_without_risk_is_success(self):
        f = Fixture(at_risk=False, require_backup=True)
        self.assertEqual(f.run(), 0)
        self.assertLess(f.events.index("backup"), f.events.index("flash"))

    def test_backup_failure_refuses_preflash(self):
        f = Fixture(fail_backup=True)
        with self.assertRaises(InstallError) as cm:
            f.run()
        self.assertEqual(cm.exception.code, "slots")
        self.assertNotIn("flash", f.events)

    def test_restore_readback_mismatch_is_incomplete(self):
        f = Fixture(fail_restore=True)
        with self.assertRaises(InstallError) as cm:
            f.run()
        self.assertEqual(cm.exception.code, "slots")
        self.assertIn("flash", f.events)
        self.assertIn("qa-preserved-samples.x0xslots", str(cm.exception))

    def test_cancellation_refuses_preflash(self):
        f = Fixture()
        f.args.yes = False
        self.assertEqual(f.run(lambda prompt: False), 1)
        self.assertNotIn("flash", f.events)

    def test_explicit_no_slots_optout_is_unchanged(self):
        f = Fixture(no_slots=True)
        self.assertEqual(f.run(), 0)
        self.assertNotIn("backup", f.events)
        self.assertNotIn("plan", f.events)

    def test_update_loader_requires_explicit_preservation_decision(self):
        f = Fixture(loader=True)
        with self.assertRaises(InstallError) as cm:
            f.run()
        self.assertEqual(cm.exception.code, "slots")
        self.assertIn("--no-slots", str(cm.exception))
        self.assertIn("backup", str(cm.exception))
        self.assertNotIn("flash", f.events)
        self.assertNotIn("plan", f.events)
        self.assertIn("close", f.events)

    def test_loader_yes_does_not_override_sample_guard(self):
        f = Fixture(loader=True)
        f.args.yes = True
        with self.assertRaises(InstallError) as cm:
            f.run()
        self.assertEqual(cm.exception.code, "slots")
        self.assertNotIn("flash", f.events)

    def test_loader_with_explicit_no_slots_can_resume(self):
        f = Fixture(loader=True, no_slots=True)
        self.assertEqual(f.run(), 0)
        self.assertEqual(f.events.count("flash"), 1)
        self.assertNotIn("plan", f.events)
        self.assertNotIn("backup", f.events)

    def test_loader_explicit_backup_cannot_be_created_in_update_mode(self):
        f = Fixture(loader=True, require_backup=True)
        with self.assertRaises(InstallError) as cm:
            f.run()
        self.assertEqual(cm.exception.code, "slots")
        self.assertNotIn("flash", f.events)

    def test_loader_explicit_optout_still_honors_cancellation(self):
        f = Fixture(loader=True, no_slots=True)
        f.args.yes = False
        self.assertEqual(f.run(lambda prompt: False), 1)
        self.assertNotIn("flash", f.events)


if __name__ == "__main__":
    unittest.main(verbosity=2)
