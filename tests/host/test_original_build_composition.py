#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Check replacement/legacy source and flag selection without invoking a compiler."""
import importlib.util
import os
from pathlib import Path
import sys
import tempfile
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location("x0x_build", ROOT / "tools/build.py")
build = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(build)


def compile_plan(env):
    original, wobble = build.synth_build_config(env)
    flags = [*build.CFLAGS, *build.FPU, "-DBOARD_SMK37=1", "-DSMK_MODEL=1",
             f"-DX0X_WOBBLE={wobble}", f"-DX0X_DEV_WOBBLE={wobble}"]
    commands, objects = build.app_compile_commands(flags, original, wobble)
    sources = {}
    for command in commands:
        path = Path(command[command.index("-c") + 1])
        if path.is_absolute():
            path = path.relative_to(build.SRC)
        sources[str(path)] = command
    return commands, objects, sources


class OriginalBuildCompositionTests(unittest.TestCase):
    def test_default_is_the_only_profile_and_zero_is_refused(self):
        commands, objects, sources = compile_plan({})
        self.assertEqual((commands, objects), compile_plan({"X0X_ORIGINAL_SYNTH": "1"})[:2])
        self.assertNotIn("firmware/src/dsp/bass303.c", sources)
        self.assertNotIn("experiments/wobble/wobble.c", sources)
        with self.assertRaisesRegex(SystemExit, "X0X_ORIGINAL_SYNTH: only 1"):
            compile_plan({"X0X_ORIGINAL_SYNTH": "0"})

    def test_replacement_is_exclusive_and_preserves_independent_sources(self):
        commands, objects, sources = compile_plan({"X0X_ORIGINAL_SYNTH": "1"})
        self.assertNotIn("firmware/src/dsp/bass303.c", sources)
        self.assertNotIn("experiments/wobble/wobble.c", sources)
        self.assertNotIn("experiments/original_synth/master_unmuted.c", sources)
        for name in ("drum909", "drum808", "breaks", "fxbus", "master", "x0x_param"):
            self.assertIn(f"firmware/src/dsp/{name}.c", sources)
        for name in ("sequencer", "tb3po", "pattern", "motion"):
            self.assertIn(f"firmware/src/seq/{name}.c", sources)
        for name in ("original_chicago_backend", "original_miami_backend"):
            self.assertIn(f"firmware/src/dsp/{name}.c", sources)
        for name in ("miami_fx_conversion", "miami_decimator_adapter", "miami_decimator_component"):
            self.assertIn(f"experiments/wobble/{name}.c", sources)
        self.assertIn("experiments/original_synth/chicago_program.c", sources)
        self.assertIn("experiments/original_synth/miami_program.c", sources)
        self.assertIn("experiments/original_synth/chicago_fx2_compact.c", sources)
        self.assertNotIn("experiments/original_synth/chicago_fx_history.c", sources)
        self.assertNotIn("experiments/original_synth/chicago_fx2_bank.c", sources)
        self.assertEqual(len(objects), len(set(objects)), "object names must not collide")
        self.assertEqual(len(commands), len(sources), "each unit must be compiled once")
        self.assertIn(build.OUT / "o2/dsp_master.o", objects)
        self.assertIn(build.OUT / "original/master.o", objects)

    def test_reference_arithmetic_flags_and_existing_board_flags(self):
        _, _, sources = compile_plan({"X0X_ORIGINAL_SYNTH": "1"})
        for path, command in sources.items():
            if path.endswith(".S"):
                continue
            self.assertIn("-DBOARD_SMK37=1", command)
            self.assertIn("-DSMK_MODEL=1", command)
            self.assertIn("-DX0X_ORIGINAL_SYNTH=1", command)
            self.assertIn("-DX0X_WOBBLE=0", command)
            self.assertIn("-DX0X_DEV_WOBBLE=0", command)
            self.assertIn("-ffp-contract=off", command)
            self.assertNotIn("-ffast-math", command)
            if path.startswith("experiments/") or path.endswith("_backend.c"):
                for flag in ("-Os", "-fno-fast-math", "-DNDEBUG", "-DORIGINAL_USE_SOFT_FMA",
                             "-DMIAMI_FX_NO_HOST_FENV", "-Iexperiments/original_synth",
                             "-Iexperiments/wobble"):
                    self.assertIn(flag, command)
                self.assertNotIn("-O2", command)
                self.assertNotIn("-Oz", command)
        self.assertIn("-O2", sources["firmware/src/dsp/drum909.c"])
        self.assertIn("-Oz", sources["firmware/src/x0x.c"])

    def test_conflicting_or_invalid_selectors_fail(self):
        for flag in ("X0X_WOBBLE", "X0X_DEV_WOBBLE"):
            for value in ("1", "invalid"):
                with self.subTest(flag=flag, value=value):
                    with self.assertRaisesRegex(SystemExit, flag):
                        compile_plan({"X0X_ORIGINAL_SYNTH": "1", flag: value})
        with self.assertRaisesRegex(SystemExit, "X0X_ORIGINAL_SYNTH: only 1"):
            compile_plan({"X0X_ORIGINAL_SYNTH": "invalid"})
        self.assertEqual(build.synth_build_config({"X0X_ORIGINAL_SYNTH": "1",
                                                  "X0X_WOBBLE": "0",
                                                  "X0X_DEV_WOBBLE": "0"}), (True, "0"))

    def test_explicit_wobble_zero_is_accepted(self):
        for env in ({"X0X_WOBBLE": "0"}, {"X0X_DEV_WOBBLE": "0"}):
            _, _, sources = compile_plan({"X0X_ORIGINAL_SYNTH": "1", **env})
            self.assertNotIn("experiments/wobble/wobble.c", sources)

    def test_missing_replacement_fails_before_any_tool_is_called(self):
        with tempfile.TemporaryDirectory() as directory:
            with mock.patch.object(build, "SRC", Path(directory)), \
                 mock.patch.dict(os.environ, {"X0X_ORIGINAL_SYNTH": "1"}, clear=True), \
                 mock.patch.object(sys, "argv", ["build.py"]), \
                 mock.patch.object(build, "tc") as tool, \
                 mock.patch.object(build, "tc_all") as tools, \
                 mock.patch.object(build, "clang_std_inc") as includes, \
                 mock.patch.object(build.subprocess, "run") as command:
                for entry in (build.build_app, build.main):
                    with self.assertRaisesRegex(SystemExit, "missing replacement sources") as failure:
                        entry()
                    self.assertIn("original_chicago_backend.c", str(failure.exception))
                    self.assertIn("original_miami_backend.c", str(failure.exception))
                tool.assert_not_called()
                tools.assert_not_called()
                includes.assert_not_called()
                command.assert_not_called()

    def test_invalid_external_inputs_fail_before_any_tool_is_called(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            required = [*build.ORIGINAL_SYNTH_UNITS, *build.ORIGINAL_SYNTH_BACKENDS,
                        *(str(Path(p).with_suffix(".h")) for p in build.ORIGINAL_SYNTH_BACKENDS)]
            for name in required:
                path = root / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.touch()
            with mock.patch.object(build, "SRC", root), \
                 mock.patch.dict(os.environ, {"X0X_ORIGINAL_SYNTH": "1"}, clear=True), \
                 mock.patch.object(sys, "argv", ["build.py"]), \
                 mock.patch.object(build, "tc") as tool, \
                 mock.patch.object(build, "tc_all") as tools, \
                 mock.patch.object(build, "clang_std_inc") as includes, \
                 mock.patch.object(build.subprocess, "run") as command:
                for entry in (build.build_app, build.main):
                    with self.assertRaisesRegex(SystemExit, "invalid external inputs"):
                        entry()
                tool.assert_not_called()
                tools.assert_not_called()
                includes.assert_not_called()
                command.assert_not_called()


if __name__ == "__main__":
    unittest.main(verbosity=2)
