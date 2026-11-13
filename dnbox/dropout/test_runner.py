"""Host input and ELF tests; synthetic files cannot qualify a firmware scene."""
import importlib.util
import json
from pathlib import Path
import struct
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest import mock


def elf32(symbols):
    strings = b"\0"
    table = bytes(16)
    for name, address, size in symbols:
        offset = len(strings); strings += name.encode() + b"\0"
        table += struct.pack("<IIIBBH", offset, address, size, 1, 0, 1)
    shoff, dataoff = 52, 52 + 3 * 40
    header = b"\x7fELF\x01\x01\x01" + bytes(9) + struct.pack("<HHIIIIIHHHHHH", 2, 0, 1, 0, 0, shoff, 0, 52, 0, 0, 40, 3, 0)
    sym = struct.pack("<10I", 0, 2, 0, 0, dataoff, len(table), 2, 0, 4, 16)
    string = struct.pack("<10I", 0, 3, 0, 0, dataoff + len(table), len(strings), 0, 0, 1, 0)
    return header + bytes(40) + sym + string + table + strings


class RunnerTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        sys.path.insert(0, str(Path(__file__).parent))
        path = Path(__file__).with_name("runner.py")
        cls.mod = None
        if path.exists():
            spec = importlib.util.spec_from_file_location("dropout_runner", path)
            cls.mod = importlib.util.module_from_spec(spec); spec.loader.exec_module(cls.mod)

    def setUp(self):
        self.assertIsNotNone(self.mod, "pinned Runner integration is missing")

    def symbols(self):
        # Distinct synthetic addresses; target addresses always come from the real ELF.
        address, values = 0x01C01000, []
        for name, size in self.mod.REQUIRED.items():
            values.append((name, address, size)); address += (size + 3) & ~3
        return values

    def test_map_uses_elf_symbols_and_checks_all_sizes(self):
        data = elf32(self.symbols())
        mapped = self.mod.observer_map(data)
        self.assertIn("x0x_dbg 0x01c01000 52\n", mapped)
        self.assertIn("eng_prof ", mapped)
        wrong = self.symbols(); wrong[0] = ("x0x_dbg", 0x01C01000, 48)
        with self.assertRaisesRegex(ValueError, "size"):
            self.mod.observer_map(elf32(wrong))

    def test_missing_ambiguous_outside_ram_and_truncated_symbols_rejected(self):
        original = self.symbols()
        cases = [elf32(original[1:]), elf32(original + [("x0x_dbg", 0x01C09000, 52)]),
                 elf32([("x0x_dbg", 0x02000000, 52)] + original[1:]), elf32(original)[:-7]]
        for data in cases:
            with self.subTest(size=len(data)), self.assertRaises(ValueError):
                self.mod.observer_map(data)

    def test_same_sized_unique_lto_symbol_resolves_without_guessed_address(self):
        symbols = self.symbols(); symbols[0] = ("x0x_dbg.lto_priv.0", 0x01C01200, 52)
        # Relocate every other symbol so the complete ranges remain disjoint.
        symbols[1:] = [(name, address + 0x1000, size) for name, address, size in symbols[1:]]
        self.assertIn("x0x_dbg 0x01c01200 52", self.mod.observer_map(elf32(symbols)))

    def test_canonical_symbols_expose_only_unique_object_addresses(self):
        symbols = self.symbols() + [("scene_value.lto_priv.3", 0x01C10000, 7),
                                    ("local", 0x01C20000, 4), ("local", 0x01C20004, 4)]
        resolved = self.mod.elf_symbols(elf32(symbols))
        self.assertEqual(resolved["scene_value"], (0x01C10000, 7))
        self.assertEqual(resolved["x0x_dbg"], (0x01C01000, 52))
        self.assertNotIn("local", resolved, "ambiguous local names must never yield a guessed address")

    def test_hash_mismatch_is_rejected_even_with_expected_filename(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "x0x.elf"; path.write_bytes(b"not the pinned ELF")
            with self.assertRaisesRegex(ValueError, "SHA256"):
                self.mod.verified_bytes(path, self.mod.PINS["elite"]["elf"])

    def test_environment_removes_measurement_and_cpu0_only_overrides(self):
        env = self.mod.run_environment({"PATH": "/bin", "FM1_NO_BLOCKS": "1", "FM1_TPROF": "old", "FM1_USB_SEND": "bad"})
        self.assertEqual(env["FM1_STRICT"], "1")
        self.assertEqual(env["FM1_BENCH_KEYS"], "")
        self.assertNotIn("FM1_NO_BLOCKS", env)
        self.assertNotIn("FM1_TPROF", env)
        self.assertNotIn("FM1_USB_SEND", env)

    def test_nonexistent_inputs_produce_blocked_inventory_without_execution(self):
        with tempfile.TemporaryDirectory() as tmp:
            missing = Path(tmp) / "missing"
            result = self.mod.inventory({"elf": (missing, self.mod.PINS["elite"]["elf"]), "pack": (missing, "0" * 64)})
            self.assertEqual(result["status"], "BLOCKED_INPUTS")
            self.assertEqual(set(result["missing"]), {"elf", "pack"})

    def test_scene_must_be_representative_and_keeps_files_inside_fixture(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); (root / "scene.keys").write_text("4000:PATCH,7800:PATCH^\n")
            (root / "scene.usb").write_text("19000:913064;319100:813000\n")
            data = dict(schema=1, firmware_source=self.mod.FIRMWARE_SOURCE, owner_scene_exact=False,
                        kind="representative", start_ms=20000, end_ms=320000, shot_ms=320500,
                        keys="scene.keys", usb="scene.usb", intended={}, owner_unknown=["tempo"])
            p = root / "scene.json"; p.write_text(json.dumps(data))
            self.assertEqual(self.mod.load_scene(p)["start_ms"], 20000)
            data["owner_scene_exact"] = True; p.write_text(json.dumps(data))
            with self.assertRaises(ValueError): self.mod.load_scene(p)
            data["owner_scene_exact"] = False; data["keys"] = "../secret"; p.write_text(json.dumps(data))
            with self.assertRaises(ValueError): self.mod.load_scene(p)

    def scene_files(self, root):
        root.mkdir(exist_ok=True)
        keys = b"4000:PATCH,7800:PATCH^\n"
        usb = b"19000:913064;320100:813000\n"
        (root / "scene.keys").write_bytes(keys)
        (root / "scene.usb").write_bytes(usb)
        data = dict(schema=1, firmware_source=self.mod.FIRMWARE_SOURCE, owner_scene_exact=False,
                    kind="representative", start_ms=20000, end_ms=320000, shot_ms=320500,
                    keys="scene.keys", usb="scene.usb", intended={}, owner_unknown=["tempo"])
        scene = root / "scene.json"
        scene.write_text(json.dumps(data))
        return scene, keys, usb

    def inputs(self, root):
        scene, keys, usb = self.scene_files(root / "source")
        inputs = {}
        for name, data in (("emulator", b"synthetic emulator; never executed"),
                           ("fwsc", b"synthetic package"), ("elf", elf32(self.symbols())),
                           ("pack", b"synthetic sample pack")):
            path = scene.parent / name
            path.write_bytes(data)
            inputs[name] = (path, self.mod.sha256(data))
        inputs["scene"] = (scene, self.mod.sha256(scene.read_bytes()))
        return inputs, keys, usb

    def test_scene_validation_and_hashing_use_the_same_single_byte_read(self):
        with tempfile.TemporaryDirectory() as tmp:
            scene, keys, usb = self.scene_files(Path(tmp))
            original = Path.read_bytes
            reads = []
            def read_bytes(path):
                reads.append(path.resolve())
                return original(path)
            with mock.patch.object(Path, "read_bytes", read_bytes), \
                 mock.patch.object(Path, "read_text", side_effect=AssertionError("separate text read")):
                loaded = self.mod.load_scene(scene)
            self.assertEqual(loaded["keys_sha256"], self.mod.sha256(keys))
            self.assertEqual(loaded["usb_sha256"], self.mod.sha256(usb))
            self.assertCountEqual(reads, [scene.resolve(), (scene.parent / "scene.keys").resolve(),
                                         (scene.parent / "scene.usb").resolve()])

    def test_captured_inputs_survive_original_mutation_and_bind_both_runs(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            inputs, keys, usb = self.inputs(root)
            scene, captured = self.mod.capture_execution_inputs(root / "execution-inputs", inputs, "synthetic map\n")
            for path, _ in inputs.values():
                path.write_bytes(b"changed after capture")
            (inputs["scene"][0].parent / "scene.keys").write_bytes(b"invalid new keys")
            (inputs["scene"][0].parent / "scene.usb").write_bytes(b"invalid new usb")
            initial = bytes((1 << 20))
            def fake_run(command, **kwargs):
                self.assertEqual(Path(command[0]).read_bytes(), b"synthetic emulator; never executed")
                self.assertEqual(Path(command[2]).read_bytes(), b"synthetic package")
                env = kwargs["env"]
                self.assertEqual(Path(env["FM1_BENCH_KEYS"][1:]).read_bytes(), keys)
                self.assertEqual(Path(env["FM1_USB_SEND"][1:]).read_bytes(), usb)
                flash = Path(command[command.index("--flash") + 1])
                temporary = flash.with_suffix(".tmp")
                temporary.write_bytes(initial)
                temporary.replace(flash)
                return SimpleNamespace(returncode=0)
            for name in ("control", "observed"):
                flash = root / (name + ".flash")
                flash.write_bytes(initial)
                env = self.mod.run_environment({"PATH": "/bin"})
                env.update(FM1_BENCH_KEYS="@" + scene["keys_path"], FM1_USB_SEND="@" + scene["usb_path"])
                if name == "observed":
                    env["FM1_DNBOX_MAP"] = captured["observer_map"]["path"]
                with mock.patch.object(self.mod.subprocess, "run", side_effect=fake_run) as process:
                    receipt = self.mod.execute(Path(captured["emulator"]["path"]), Path(captured["fwsc"]["path"]),
                                               root / name, flash, 320500, env, True, input_manifest=captured)
                process.assert_called_once()
                self.assertEqual(receipt["status"], "COMPLETED")
                self.assertEqual(receipt["fm1_environment"], {k: v for k, v in env.items() if k.startswith("FM1_")})
                self.assertEqual(receipt["inputs"]["fwsc"]["sha256"], inputs["fwsc"][1])
                self.assertEqual(receipt["inputs"]["keys"]["sha256"], self.mod.sha256(keys))
                self.assertEqual(receipt["final_flash"]["sha256"], self.mod.sha256(initial))
                self.assertTrue(receipt["final_flash"]["atomically_replaced"])

    def test_changed_captured_input_prevents_launch_and_records_failure(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            inputs, _, _ = self.inputs(root)
            _, captured = self.mod.capture_execution_inputs(root / "execution-inputs", inputs, "synthetic map\n")
            altered = Path(captured["fwsc"]["path"])
            altered.chmod(0o600)
            altered.write_bytes(b"changed captured input")
            flash = root / "run.flash"; flash.write_bytes(bytes((1 << 20)))
            with mock.patch.object(self.mod.subprocess, "run") as process:
                with self.assertRaisesRegex(ValueError, "SHA256"):
                    self.mod.execute(Path(captured["emulator"]["path"]), altered, root / "run", flash,
                                     320500, self.mod.run_environment({}), False, input_manifest=captured)
            process.assert_not_called()
            receipt = json.loads((root / "run" / "command.json").read_text())
            self.assertEqual(receipt["status"], "FAILED")
            self.assertIsNone(receipt["returncode"])
            self.assertNotIn("final_flash", receipt)

    def test_main_uses_captured_paths_for_bootstrap_control_and_observed(self):
        # Host orchestration only: every process and trace analysis is mocked.
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); inputs, keys, usb = self.inputs(root)
            output = root / "result"
            initial = bytes(1 << 20)
            launched = []
            def fake_run(command, **kwargs):
                directory = Path(kwargs["cwd"])
                launched.append(directory.name)
                self.assertEqual(Path(command[0]), output / "execution-inputs" / "fm1-live")
                self.assertEqual(Path(command[2]), output / "execution-inputs" / "firmware.fwsc")
                self.assertEqual(Path(command[0]).read_bytes(), b"synthetic emulator; never executed")
                self.assertEqual(Path(command[2]).read_bytes(), b"synthetic package")
                flash = Path(command[command.index("--flash") + 1])
                temporary = flash.with_suffix(".tmp")
                temporary.write_bytes(initial); temporary.replace(flash)
                if directory.name == "bootstrap":
                    for path, _ in inputs.values():
                        path.write_bytes(b"changed after bootstrap")
                    (inputs["scene"][0].parent / "scene.keys").write_bytes(b"changed keys")
                    (inputs["scene"][0].parent / "scene.usb").write_bytes(b"changed usb")
                else:
                    env = kwargs["env"]
                    if directory.name == "break-only":
                        self.assertEqual(Path(env["FM1_BENCH_KEYS"][1:]).read_bytes(), b"synthetic isolated BREAK input")
                        self.assertNotIn("FM1_USB_SEND", env)
                    else:
                        self.assertEqual(Path(env["FM1_BENCH_KEYS"][1:]).read_bytes(), keys)
                        self.assertEqual(Path(env["FM1_USB_SEND"][1:]).read_bytes(), usb)
                    Path(env["FM1_DNBOX_PCM"]).write_bytes(b"synthetic identical PCM")
                    Path(env["FM1_DUMPRAM"]).write_bytes(initial)
                    if directory.name == "observed":
                        self.assertEqual(Path(env["FM1_DNBOX_MAP"]), output / "execution-inputs" / "observer.map")
                return SimpleNamespace(returncode=0)
            argv = ["runner.py", "--out", str(output), "--execute"]
            for name in ("emulator", "elf", "fwsc", "pack", "scene"):
                argv.extend(["--" + name, str(inputs[name][0])])
            pins = {"elite": {"elf": inputs["elf"][1], "fwsc": inputs["fwsc"][1]}}
            good = self.complete_result()
            analyzer = mock.Mock(return_value=good["analysis"])
            with mock.patch.object(self.mod, "PINS", pins), \
                 mock.patch.object(self.mod, "FACTORY_PACK_SHA256", inputs["pack"][1]), \
                 mock.patch.object(self.mod, "make_fixture", return_value=(initial, {"synthetic_test_only": True})), \
                 mock.patch.object(self.mod.subprocess, "run", side_effect=fake_run), \
                 mock.patch.object(sys, "argv", argv), \
                 mock.patch.dict(sys.modules, {
                     "trace": SimpleNamespace(analyze_trace=analyzer),
                     "scene": SimpleNamespace(scene_map=lambda *a: "", check_window=lambda *a, **kw: good["scene_state"]),
                     "activity": SimpleNamespace(activity_map=lambda *a: "", check_808_inactive=lambda *a: good["808_activity"]),
                     "pcm": SimpleNamespace(analyze_pcm=lambda *a: good["pcm"]),
                     "isolation": SimpleNamespace(break_only_keys=lambda raw: b"synthetic isolated BREAK input",
                                                  check_break_only=lambda *a: good["isolated_break"]["state"])}), \
                 mock.patch("builtins.print"):
                self.assertEqual(self.mod.main(), 0)
            self.assertEqual(launched, ["bootstrap", "control", "observed", "break-only"])
            self.assertEqual(analyzer.call_count, 2)
            result = json.loads((output / "result.json").read_text())
            self.assertFalse(result["owner_scene_exact"])
            self.assertEqual(result["control_final_flash_sha256"], self.mod.sha256(initial))
            self.assertEqual(result["observed_final_flash_sha256"], self.mod.sha256(initial))

    @staticmethod
    def complete_result():
        return {"analysis": {"status": "OBSERVED", "trace_sha256": "a" * 64,
                    "deadline": {"completed_irq_deadline_misses": 1},
                    "break_evidence": {"loaded": "OBSERVED", "mix_contribution": "OBSERVED", "invalid_mix_halves": 0}},
                "scene_state": {"required_state_matches_at_all_observed_boundaries": True},
                "808_activity": {"status": "INACTIVE_OBSERVED", "all_observed_stretches_inactive": True,
                                 "mix_guard_excluded_at_all_observed_stretches": True, "coverage_complete": True},
                "pcm": {"status": "OBSERVED", "numeric_output": "NONZERO", "trace_sha256": "a" * 64},
                "isolated_break": {"state": {"status": "NUMERICAL_BREAK_OUTPUT_OBSERVED"}},
                "observer_pcm_bit_exact": True, "observer_ram_bit_exact": True, "observer_flash_bit_exact": True}

    def test_completed_observation_can_record_a_deadline_failure(self):
        self.assertTrue(self.mod.observation_complete(self.complete_result()))

    def test_missing_scene_break_activity_or_control_evidence_is_incomplete(self):
        import copy
        changes = [("analysis", "status", "INCOMPLETE"),
                   ("scene_state", "required_state_matches_at_all_observed_boundaries", False),
                   ("808_activity", "status", "UNVERIFIED"),
                   ("808_activity", "coverage_complete", False),
                   ("pcm", "numeric_output", "ZERO"),
                   ("pcm", "trace_sha256", "b" * 64)]
        for group, key, value in changes:
            result = copy.deepcopy(self.complete_result()); result[group][key] = value
            with self.subTest(group=group, key=key): self.assertFalse(self.mod.observation_complete(result))
        for flag in ("observer_pcm_bit_exact", "observer_ram_bit_exact", "observer_flash_bit_exact"):
            result = copy.deepcopy(self.complete_result()); result[flag] = False
            with self.subTest(flag=flag): self.assertFalse(self.mod.observation_complete(result))
        result = copy.deepcopy(self.complete_result())
        result["analysis"]["break_evidence"]["mix_contribution"] = "NOT_OBSERVED"
        self.assertFalse(self.mod.observation_complete(result))
        result = copy.deepcopy(self.complete_result())
        result["isolated_break"]["state"]["status"] = "UNVERIFIED"
        self.assertFalse(self.mod.observation_complete(result))

    def test_zero_exit_and_save_error_cannot_label_initial_fixture_final(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); inputs, _, _ = self.inputs(root)
            flash = root / "run.flash"; flash.write_bytes(bytes((1 << 20)))
            def fake_run(command, **kwargs):
                kwargs["stderr"].write(f"{flash}: Permission denied\n".encode())
                return SimpleNamespace(returncode=0)
            with mock.patch.object(self.mod.subprocess, "run", side_effect=fake_run):
                with self.assertRaisesRegex(RuntimeError, "flash.*replace"):
                    self.mod.execute(inputs["emulator"][0], inputs["fwsc"][0], root / "run", flash,
                                     320500, self.mod.run_environment({}), False)
            receipt = json.loads((root / "run" / "command.json").read_text())
            self.assertEqual(receipt["returncode"], 0)
            self.assertEqual(receipt["status"], "FAILED")
            self.assertTrue(receipt["flash_save"]["stderr_save_error"])
            self.assertNotIn("final_flash", receipt)

    def test_changed_bytes_without_atomic_replacement_are_not_a_final_save(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); inputs, _, _ = self.inputs(root)
            flash = root / "run.flash"; flash.write_bytes(bytes((1 << 20)))
            def fake_run(command, **kwargs):
                flash.write_bytes(bytes([1]) * (1 << 20))
                return SimpleNamespace(returncode=0)
            with mock.patch.object(self.mod.subprocess, "run", side_effect=fake_run):
                with self.assertRaisesRegex(RuntimeError, "flash.*replace"):
                    self.mod.execute(inputs["emulator"][0], inputs["fwsc"][0], root / "run", flash,
                                     320500, self.mod.run_environment({}), False)

    def test_atomic_save_requires_full_flash_size_and_bootstrap_creation_is_supported(self):
        for size in (31, 1 << 20):
            with self.subTest(bytes=size), tempfile.TemporaryDirectory() as tmp:
                root = Path(tmp); inputs, _, _ = self.inputs(root)
                flash = root / "base.flash"
                def fake_run(command, **kwargs):
                    temporary = flash.with_suffix(".tmp")
                    temporary.write_bytes(bytes(size)); temporary.replace(flash)
                    return SimpleNamespace(returncode=0)
                with mock.patch.object(self.mod.subprocess, "run", side_effect=fake_run):
                    if size != (1 << 20):
                        with self.assertRaisesRegex(RuntimeError, "flash.*size"):
                            self.mod.execute(inputs["emulator"][0], inputs["fwsc"][0], root / "run", flash,
                                             2000, self.mod.run_environment({}), False)
                    else:
                        receipt = self.mod.execute(inputs["emulator"][0], inputs["fwsc"][0], root / "run", flash,
                                                   2000, self.mod.run_environment({}), False)
                        self.assertEqual(receipt["status"], "COMPLETED")
                        self.assertEqual(receipt["initial_flash"]["state"], "ABSENT")
                        self.assertEqual(receipt["final_flash"]["bytes"], (1 << 20))


if __name__ == "__main__":
    unittest.main()
