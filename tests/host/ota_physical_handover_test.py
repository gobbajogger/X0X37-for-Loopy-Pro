#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Fail-closed OTA handover regressions, without USB, firmware or sample access.

Execute from the repository root:
    python3 -B tests/host/ota_physical_handover_test.py

These scenarios deliberately distinguish an OS MIDI port *label* from an
underlying physical keyboard. The temporary test backend exposes a trusted
opaque per-unit token solely to support a future physical-continuity check.
A package identity, OS port name or unique match is not a physical identifier.
Both genuine loader rename and swapped-device cases must be distinguished;
without physical identity the updater must refuse before entering update mode.
"""
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
sys.path.insert(0, str(HERE))
from ota_port_binding_test import Backend, InstallError, load_source  # noqa: E402

Updater, _ = load_source(ROOT / 'tools' / 'fm1_install.py')
PORT = 'smk-port-A'
LOADER = 'Felucca Update'
OLD = 'SMK-37 Pro_016'
PACKAGE = 'SMK-37 Pro_900'


class PhysicalBackend(Backend):
    def __init__(self):
        super().__init__({PORT: OLD})
        # Deliberately independent of OS port *names* and model/version text.
        self.physical = {PORT: 'unit-A'}
        self.writes = []
        self.verifications = []

    def physical_id(self, name):
        """Test-only opaque non-port hardware identifier."""
        return self.physical.get(name)

    def change(self, old, new, ident, *, replace=False):
        unit = self.physical.pop(old)
        self.ports.pop(old)
        self.ports[new] = ident
        self.physical[new] = 'unit-B' if replace else unit


class TraceUpdater(Updater):
    def __init__(self, backend, phase):
        super().__init__(backend, PORT, loader='ota-' + PACKAGE, installed=PACKAGE)
        self.phase = phase
        self.stages = []

    def serve(self, link, image, finish, idle, progress):
        self.stages.append(finish)
        if finish == 111:   # application finished preflight; loader re-enumerates
            rename = self.phase == 'rename-same-unit'
            replace = self.phase == 'swap-before-write'
            self.backend.change(link.name, LOADER if rename else link.name,
                                'ota-' + PACKAGE, replace=replace)
            return 1, True, False
        if finish == 222:   # loader would now program flash
            self.backend.writes.append(self.backend.physical[link.name])
            self.backend.change(link.name, PORT if self.phase == 'rename-same-unit' else link.name,
                                PACKAGE, replace=self.phase == 'swap-before-verify')
            return 1, True, False
        raise AssertionError('unexpected OTA stage')

    def verify(self, product, step):
        result = super().verify(product, step)
        # Detect deceptive success on physical B with identical model/version.
        dev = self.find(lambda i: not i.loader and i.text == product, only_port=False)
        if dev:
            self.backend.verifications.append(self.backend.physical[dev.name])
            dev.link.close()
        return result


def scenario(phase):
    backend = PhysicalBackend()
    if phase == 'no-hardware-id':
        backend.physical_id = None  # backend without a hardware binding capability
    updater = TraceUpdater(backend, phase)
    error, result = None, None
    try:
        dev = updater.find()
        assert dev is not None
        if phase == 'swap-before-start':
            backend.physical[PORT] = 'unit-B'
        result = updater.install(dev, b'synthetic-package', PACKAGE)
    except InstallError as exc:
        error = exc.code
    if phase == 'no-hardware-id':
        safe = result is None and error == 'model' and not backend.writes and not updater.stages
    elif phase == 'swap-before-start':
        safe = result is None and error == 'model' and not backend.writes and not updater.stages
    elif phase == 'stable':
        safe = (result == PACKAGE and backend.writes == ['unit-A']
                and backend.verifications == ['unit-A'])
    elif phase == 'rename-same-unit':
        safe = (result == PACKAGE and backend.writes == ['unit-A']
                and backend.verifications == ['unit-A'])
    elif phase == 'swap-before-write':
        safe = 'unit-B' not in backend.writes
    elif phase == 'swap-before-verify':
        safe = 'unit-B' not in backend.verifications and result != PACKAGE
    else:
        raise AssertionError(phase)
    print(f'{"PASS" if safe else "FAIL"} {phase}: result={result!r} error={error!r} '
          f'writes={backend.writes} verified={backend.verifications}')
    return safe


if __name__ == '__main__':
    modes = ('stable', 'rename-same-unit', 'swap-before-start',
             'swap-before-write', 'swap-before-verify', 'no-hardware-id')
    results = [scenario(mode) for mode in modes]
    print(f'SUMMARY {sum(results)}/{len(results)} safe; '
          f'{len(results) - sum(results)} failing physical-continuity regressions')
    sys.exit(not all(results))