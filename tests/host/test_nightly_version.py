#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Version flags only: importing the build recipe never executes a build."""
import sys
import os
from pathlib import Path
import unittest
from unittest.mock import patch, Mock

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
import build


class NightlyVersionTest(unittest.TestCase):
    def test_nightly_cli_keeps_model_900_for_loader_and_package(self):
        class StopBeforeSDK(Exception):
            pass
        for model, product in (('pro', 'SMK-37 Pro_900'), ('elite', 'SMK-37 Elite_900')):
            with self.subTest(model=model), \
                 patch.multiple(build, BOARD='fm1', MODEL='pro', VERSION=None, GIT_SHA=None,
                                PRODUCT='', RELEASE_LABEL=None, OUT=ROOT / 'build', LDR=ROOT / 'build/loader',
                                APP_SLOT=build.APP_SLOT, APP_CODE_MAX=build.APP_CODE_MAX,
                                BOARD_DEFS={k: list(v) for k, v in build.BOARD_DEFS.items()},
                                BOARD_PREFIX=dict(build.BOARD_PREFIX)), \
                 patch.object(build, 'synth_build_config', return_value=(False, 0)), \
                 patch.object(build.subprocess, 'run', return_value=Mock(returncode=0)), \
                 patch.object(build.fm1pkg_make, 'sdk_file', side_effect=StopBeforeSDK), \
                 patch.object(build.fm1pkg_make, 'SDK', None), \
                 patch.object(build.sys, 'argv', ['build.py', '--board', 'smk37', '--model', model,
                                                '--nightly-version', '0.0.25', '--git-sha', 'a' * 40]):
                previous_board = build.fm1pkg_make.BOARD
                try:
                    with self.assertRaises(StopBeforeSDK):
                        build.main()
                    self.assertEqual(build.PRODUCT, product)
                    self.assertEqual(build.VERSION, '0.0.25')
                    self.assertEqual(build.GIT_SHA, 'aaaaaaaa')
                    self.assertIsNone(build.RELEASE_LABEL)
                    self.assertFalse(any('X0X_GIT_SHA' in flag or 'X0X_VERSION' in flag
                                         for flag in build.BOARD_DEFS['smk37']))
                finally:
                    build.fm1pkg_make.set_board(previous_board)

    def test_nightly_is_numeric_without_a_release_identity(self):
        self.assertEqual(build.nightly_identity('0.0.25', 'a' * 40),
                         {'version_macro': '0.0.25', 'git_sha': 'aaaaaaaa'})

    def test_empty_cli_version_refuses_before_build_work(self):
        with patch.object(build.sys, 'argv', ['build.py', '--nightly-version', '',
                                             '--git-sha', 'a' * 40]), \
             patch.object(build, 'synth_build_config', side_effect=AssertionError('build work started')):
            with self.assertRaises(SystemExit) as exc:
                build.main()
            self.assertEqual(exc.exception.code, 2)

    def test_invalid_version_or_sha_cannot_enter_c_defines(self):
        for version in ('DEV', '', '00.0.25', '0.0.25_openbeta', '0.0.25-nightly', '100.0.0'):
            with self.subTest(version=version), self.assertRaises(ValueError):
                build.nightly_identity(version, 'a' * 40)
        for sha in ('', 'abc1234', 'g' * 40, 'A' * 40, 'a"\\n' + '0' * 37):
            with self.subTest(sha=sha), self.assertRaises(ValueError):
                build.nightly_identity('0.0.25', sha)

    def test_app_defines_keep_sha_separate_from_display_version(self):
        self.assertEqual(build.version_defines('0.0.25', None, 'bb190749'),
                         ['-DX0X_VERSION="0.0.25"', '-DX0X_GIT_SHA="bb190749"'])
        self.assertEqual(build.version_defines(None, None, None), [])
        self.assertEqual(build.version_defines('0.1.3', '0.1.3_openbeta', None),
                         ['-DX0X_VERSION="0.1.3"', '-DX0X_RELEASE_LABEL="0.1.3_openbeta"'])

    def test_app_compiler_receives_version_and_sha_without_changing_identity(self):
        class StopBeforeCompile(Exception):
            pass

        for model, product in (('pro', 'SMK-37 Pro_900'),
                               ('elite', 'SMK-37 Elite_900')):
            captured = []

            def capture(flags, original_synth, wobble):
                captured.extend(flags)
                raise StopBeforeCompile

            with self.subTest(model=model), patch.dict(os.environ, {}, clear=True), \
                 patch.multiple(build, BOARD='smk37', MODEL=model, PRODUCT=product,
                                VERSION='0.0.25', GIT_SHA='bb190749', RELEASE_LABEL=None), \
                 patch.object(build, 'synth_build_config', return_value=(False, 0)), \
                 patch.object(build, 'clang_std_inc', return_value=[]), \
                 patch.object(build, 'app_compile_commands', side_effect=capture):
                with self.assertRaises(StopBeforeCompile):
                    build.build_app()
            self.assertIn('-DX0X_VERSION="0.0.25"', captured)
            self.assertIn('-DX0X_GIT_SHA="bb190749"', captured)
            self.assertIn('-DFELUCCA_ID="' + product + '"', captured)
            self.assertFalse(any(flag.startswith('-DX0X_RELEASE_LABEL=') for flag in captured))


if __name__ == '__main__':
    unittest.main()
