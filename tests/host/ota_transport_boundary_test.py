#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Strict OTA boundary acceptance tests against the COMPLETE installer module.

Run normally and with -O. Current R9 intentionally FAILS the zero-foreign-write
contract. This is a separate diagnostic gate, not an expected-failure PASS in
the regular host suite. No hardware or firmware bytes are used.

The adversarial Mido implementation models contract possibilities, not evidence
that real OS handles retarget. In particular dynamic in-send rerouting is a
simulation. Open-time index churn and ALSA subscriber routing have separately
audited upstream source evidence; see the adjacent transport handoff.
"""
from pathlib import Path
import sys
from types import ModuleType, SimpleNamespace
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'tools'))
import fm1_install as I  # noqa: E402 -- full real module, no AST projections


class Input:
    def __init__(self, unit, callback):
        self.unit, self.callback = unit, callback

    def close(self):
        pass


class Output:
    def __init__(self, midi, unit):
        self.midi, self.unit = midi, unit

    def send(self, msg):
        # For stable-open-handle cases the captured unit never changes. A
        # deliberate test hook can instead model a weaker backend contract.
        target = self.midi.before_send(self) if self.midi.before_send else self.unit
        self.midi.sent[target].append(bytes([0xF0, *msg.data, 0xF7]))

    def close(self):
        pass


class MIDI(ModuleType):
    def __init__(self, name='SMK-37 Pro', output_name=None):
        super().__init__('mido')
        self.name, self.output_name = name, output_name or name
        self.input_unit = self.output_unit = self.token = 'A'
        self.before_input = self.before_output = self.before_send = None
        self.sent, self.inputs = {'A': [], 'B': []}, []

    def get_input_names(self):
        return [self.name] if self.token is not None else []

    def get_output_names(self):
        return [self.output_name]

    def open_input(self, name, callback):
        if self.before_input:
            self.before_input()
        inp = Input(self.input_unit, callback)
        self.inputs.append(inp)
        return inp

    def open_output(self, name):
        if self.before_output:
            self.before_output()
        return Output(self, self.output_unit)

    @staticmethod
    def Message(kind, data):
        return SimpleNamespace(type=kind, data=data)


class UnitBackend(I.MidoBackend):
    """Synthetic INPUT-only identity provider: not a production capability."""
    def physical_id(self, name):
        return self.mido.token if name == self.mido.name else None


class Boundary(unittest.TestCase):
    def setUp(self):
        self.links = []
        self.delay = patch.dict(I.DELAY, open=0)
        self.delay.start()

    def tearDown(self):
        for link in self.links:
            link.close()
        self.delay.stop()

    def opened(self, midi, native=False):
        with patch.dict(sys.modules, mido=midi):
            backend = I.MidoBackend() if native else UnitBackend()
            out = I.pair_output(midi.name, backend.output_names())
            self.assertIsNotNone(out)
            link = backend.open(midi.name, out)
        self.links.append(link)
        return backend, link

    def send_without_foreign_write(self, midi):
        _, link = self.opened(midi)
        result = link.send(I.UPGRADE)
        # This assertion intentionally ignores the return value: returning
        # False AFTER a foreign send is not the required prevention.
        self.assertEqual(midi.sent['B'], [], f'foreign B received UPGRADE; send returned {result}')
        return link

    def test_stable_open_handle_keeps_A_despite_name_replacement(self):
        midi = MIDI()
        _, link = self.opened(midi)
        midi.input_unit = midi.output_unit = 'B'
        # The captured output remains A; this simulated strong OS contract
        # must be distinguished from the weak retargeting cases below.
        self.assertTrue(link.send(I.UPGRADE))
        self.assertEqual(midi.sent, {'A': [I.UPGRADE], 'B': []})

    def test_input_A_output_B_equal_names(self):
        midi = MIDI()
        midi.output_unit = 'B'
        self.send_without_foreign_write(midi)

    def test_input_A_output_B_normalized_name_fallback(self):
        midi = MIDI('SMK-37 Pro 1', 'SMK-37 Pro 2')
        midi.output_unit = 'B'
        self.send_without_foreign_write(midi)

    def test_input_A_output_B_sole_output_fallback(self):
        midi = MIDI('SMK-37 Pro', 'Felucca Update')
        midi.output_unit = 'B'
        self.send_without_foreign_write(midi)

    def test_foreign_output_never_receives_package_response(self):
        midi = MIDI()
        midi.output_unit = 'B'
        _, link = self.opened(midi)
        # Synthetic response bytes only, not a firmware or controller payload.
        link.send(I.response(0x4000, b'synthetic'))
        self.assertEqual(midi.sent['B'], [], 'foreign output received package response')

    def test_foreign_output_never_receives_sample_erase(self):
        midi = MIDI()
        midi.output_unit = 'B'
        _, link = self.opened(midi)
        with self.assertRaises(TimeoutError):
            # Valid slot-zero erase request, entirely inside this simulator.
            I.EditorLink(link).req(I.xs.CMD_ERASE, [0], timeout=0)
        self.assertEqual(midi.sent['B'], [], 'foreign output received slot-zero erase command')

    def test_replacement_during_output_open(self):
        midi = MIDI()
        midi.before_output = lambda: setattr(midi, 'output_unit', 'B')
        self.send_without_foreign_write(midi)

    def test_replacement_during_input_open_observed_token_loss(self):
        midi = MIDI()
        def swap():
            midi.input_unit = midi.output_unit = midi.token = 'B'
        midi.before_input = swap
        link = self.send_without_foreign_write(midi)
        self.assertTrue(link.lost)

    def test_in_send_retarget_B_even_when_postcheck_fails(self):
        midi = MIDI()
        def retarget(_):
            midi.token = 'B'
            return 'B'
        midi.before_send = retarget
        self.send_without_foreign_write(midi)

    def test_in_send_transient_B_to_A_is_not_prevented(self):
        midi = MIDI()
        # Model routing B while the input lookup is A again. No polling can
        # prove this weak send implementation atomic.
        midi.before_send = lambda _: 'B'
        self.send_without_foreign_write(midi)

    def test_disconnect_before_send_never_writes_replacement(self):
        midi = MIDI()
        _, link = self.opened(midi)
        midi.token, midi.output_unit = None, 'B'
        self.assertFalse(link.send(I.UPGRADE))
        self.assertEqual(midi.sent['B'], [])

    def test_queued_callback_from_B_after_lookup_returns_to_A(self):
        midi = MIDI()
        midi.input_unit = 'B'  # captured foreign native source
        _, link = self.opened(midi)
        midi.input_unit = 'A'  # current name lookup is now A
        self.assertEqual(midi.inputs[0].unit, 'B')
        midi.inputs[0].callback(SimpleNamespace(type='sysex', data=[0x7e]))
        self.assertIsNone(link.read(0), 'callback has no captured native source/epoch binding')

    def test_fresh_synthetic_application_to_loader_rename(self):
        midi = MIDI('Felucca Update')
        self.send_without_foreign_write(midi)
        self.assertEqual(midi.sent['A'], [I.UPGRADE])

    def test_native_backend_without_identity_refuses_legacy_upgrade(self):
        midi = MIDI()
        backend, link = self.opened(midi, native=True)
        dev = I.Device(link, I.Identity('SMK-37 Pro_016', 'SMK-37 Pro', 16), midi.name)
        dev.physical_id = None
        with self.assertRaises(I.InstallError) as caught:
            I.Updater(backend).install(dev, b'', 'SMK-37 Pro_900')
        self.assertEqual(caught.exception.code, 'model')
        self.assertEqual(midi.sent, {'A': [], 'B': []})

    def test_wrong_family_and_wrong_destination_loader_refuse_before_send(self):
        for identity, model in [('FM-1_900', 'FM-1'), ('ota-SMK-37 Elite_900', 'ota-SMK-37 Elite')]:
            with self.subTest(identity=identity):
                midi = MIDI()
                backend, link = self.opened(midi)
                dev = I.Device(link, I.Identity(identity, model, 900), midi.name)
                dev.physical_id = 'A'
                with self.assertRaises(I.InstallError) as caught:
                    I.Updater(backend).install(dev, b'', 'SMK-37 Pro_900')
                self.assertEqual(caught.exception.code, 'model')
                self.assertEqual(midi.sent, {'A': [], 'B': []})


if __name__ == '__main__':
    unittest.main(verbosity=2)
