#!/usr/bin/env python3
"""SMK-only update runner regression with generated scratch inputs, no SDK or target build.

Run python3 tests/host/test_smk_update_contract.py. Exercises the real package maker,
OTA and loader host tests, installer parity, and gate expected-test admission.
The dummy SDK/app/loader bytes are fixtures, never installable firmware evidence.
"""
import os
from pathlib import Path
import random
import shutil
import subprocess
import sys
import tempfile
import unittest

TOP = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(TOP / 'tools'))
import build as builder
import fm1pkg_make as package


class SmkUpdateContract(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.runner = (TOP / 'tests/run_tests.sh').read_text()
        self.gate = (TOP / 'tools/gate.sh').read_text()
        self.env = os.environ.copy()

    def run_shell(self, source):
        return subprocess.run(['sh', '-c', source], cwd=self.root, env=self.env,
                              text=True, capture_output=True)

    def test_update_consumers_without_fm1_build(self):
        for name in ('firmware', 'tools', 'web'):
            (self.root / name).symlink_to(TOP / name, target_is_directory=True)
        (self.root / 'tests').mkdir()
        for name in ('ota_test.c', 'ldr_test.c', 'smk_pkg_test.py', 'install_test.py'):
            shutil.copy2(TOP / 'tests' / name, self.root / 'tests' / name)
        out = self.root / 'build/smk37'
        (out / 'loader').mkdir(parents=True)
        # Deliberately different old/new app contents after truncating at 200000 bytes.
        (out / 'x0x.bin').write_bytes(bytes(range(256)) * 1000)
        rng = random.Random(37)
        loader = b'FELUCCA-LOADER-1\0' + bytes(rng.randrange(256) for _ in range(2048))
        (out / 'loader/ota.bin').write_bytes(builder.ldr_wrap(loader))
        sdk = self.root / 'sdk'
        for name in ('uboot.boot', 'cfg_tool.bin', 'cfg/eq_cfg_hw.bin'):
            path = sdk / 'cpu/wl82/tools' / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(bytes(range(128)))
        self.env['AC79_SDK'] = str(sdk)
        # The built-package tests require both supported model identities.
        old_sdk, old_board = package.SDK, package.BOARD
        self.addCleanup(package.set_board, old_board)
        self.addCleanup(setattr, package, 'SDK', old_sdk)
        package.SDK = sdk
        package.set_board('smk37')
        flash = package.flash_image((out / 'x0x.bin').read_bytes(), package.KEY)
        ota = (out / 'loader/ota.bin').read_bytes()
        for suffix, model in (('', 'Pro'), ('-elite', 'Elite')):
            (self.root / f'build/x0x-smk37{suffix}.fwsc').write_bytes(
                package.ufw(flash, ota, f'SMK-37 {model}_900'))
        start = self.runner.rindex('\nif ', 0, self.runner.index('    run smk-package')) + 1
        end = self.runner.index('# SMK-37 Bluetooth MIDI:', start)
        prelude = '''set -u
CC="${CC:-cc}"
OUT=build/host
FAIL=0
mkdir -p "$OUT"
run() { name="$1"; shift; if "$@"; then echo "  ok   $name"; else FAIL=1; fi; }
'''
        result = self.run_shell(prelude + self.runner[start:end] +
                                '\nrun installer python3 tests/install_test.py\nexit "$FAIL"\n')
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        for name in ('smk-package', 'smk-ota-entry', 'smk-ota-entry-model1',
                     'smk-ota-entry-model2', 'smk-update-loader', 'smk-build-package',
                     'smk-build-package-elite', 'installer'):
            self.assertIn('  ok   ' + name + '\n', result.stdout)
        self.assertNotIn('skipped', result.stdout)
        self.assertIn('logical_image / product_of == fm1pkg.js (x0x-smk37.fwsc', result.stdout)
        self.assertIn('protected ranges untouched', result.stdout)
        self.assertFalse((self.root / 'build/x0x.fwsc').exists())
        self.assertFalse((self.root / 'build/x0x.bin').exists())

    def test_gate_checks_both_models_and_configuration_budgets(self):
        # Stub only the target compiler entry point; execute the actual gate loops.
        logs = self.root / 'logs'
        logs.mkdir()
        self.env.pop('MODEL', None)
        self.env['L'] = str(logs)
        stub = self.root / 'build.sh'
        stub.write_text("#!/bin/sh\n[ \"$*\" = '--board smk37' ] || exit 91\n"
                        "echo \"${MODEL:-pro} $X0X_DEV_PADCAL\" >> calls\n")
        stub.chmod(0o755)
        build_fn = self.gate.split('build_config() {\n', 1)[1].split('\n}\n', 1)[0]
        command = ('build_config() {\n' + build_fn + '\n}\n'
                   'build_config "$PWD" "$HOME" "$PWD" 1 ""\n'
                   'build_config "$PWD" "$HOME" "$PWD" 0 -rel\n')
        result = subprocess.run(['bash', '-c', command], cwd=self.root, env=self.env,
                                text=True, capture_output=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual((self.root / 'calls').read_text().splitlines(),
                         ['elite 1', 'pro 1', 'elite 0', 'pro 0'])
        builds = ('elite', 'pro', 'elite-rel', 'pro-rel')
        self.assertEqual({p.name for p in logs.glob('*.rc')},
                         {f'build-{name}.rc' for name in builds})
        status = self.gate.split('for c in "dev:" "rel:-rel"; do', 1)[1].split('\nPCT=', 1)[0]
        budget = self.gate.split('num() {', 1)[1].split('\n# X0X_RAM_CANARY:', 1)[0]
        checks = ('failed=0\nfail() { echo "$*"; failed=1; }\n'
                  'for c in "dev:" "rel:-rel"; do' + status + '\nnum() {' + budget +
                  '\nexit "$failed"\n')
        valid = 'app code area 1024 B, 10 B free\nRAM .data+.bss 20 B of 30\npool 100 B of 8292\n'
        for name in builds:
            (logs / f'build-{name}.log').write_text(valid)
        def check():
            return subprocess.run(['bash', '-c', checks], cwd=self.root, env=self.env,
                                  text=True, capture_output=True)
        result = check()
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        for name in builds:
            rc = logs / f'build-{name}.rc'
            rc.write_text('1\n')
            self.assertNotEqual(check().returncode, 0)
            rc.write_text('0\n')
            log = logs / f'build-{name}.log'
            for bad in (valid.replace('10 B free', 'over limit'),
                        valid.replace('20 B of 30', '31 B of 30'),
                        valid.replace('100 B of 8292', '101 B of 8292')):
                log.write_text(bad)
                self.assertNotEqual(check().returncode, 0, name + ': ' + bad)
            log.write_text(valid)

    def test_gate_expected_tests_require_smk_coverage(self):
        (self.root / 'tests').mkdir()
        (self.root / 'tests/run_tests.sh').write_text(self.runner +
            '\nrun quoted-comment echo "# literal" # run phantom not-a-test\n')
        # Execute the real expectation function without any gate setup or build.
        function = self.gate.split('expected_tests() {\n', 1)[1].split('\n}\n', 1)[0]
        log = self.root / 'tests.log'
        log.write_text('')
        expected, missing = self.root / 'expected', self.root / 'missing'
        command = 'expected_tests() {\n' + function + '\n}\nexpected_tests tests.log expected missing\n'
        result = self.run_shell(command)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        names = set(expected.read_text().splitlines())
        required = {'smk-package', 'smk-ota-entry', 'smk-ota-entry-model1',
                    'smk-ota-entry-model2', 'smk-update-loader', 'installer',
                    'smk-build-package', 'smk-build-package-elite', 'ble-package',
                    'smk-update-contract'}
        self.assertTrue(required <= names, required - names)
        self.assertTrue({'ota-entry', 'update-loader', 'storage', 'phantom'}.isdisjoint(names))
        self.assertIn('quoted-comment', names)
        log.write_text(''.join('  ok   ' + name + '\n' for name in sorted(names)))
        self.assertEqual(self.run_shell(command).returncode, 0)
        self.assertEqual(missing.read_text().strip(), '')
        for removed in sorted(required):
            log.write_text(''.join('  ok   ' + name + '\n' for name in sorted(names - {removed})))
            self.assertEqual(self.run_shell(command).returncode, 0)
            self.assertEqual(missing.read_text().strip(), removed)


if __name__ == '__main__':
    unittest.main()
