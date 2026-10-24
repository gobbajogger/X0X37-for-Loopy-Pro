#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""CoreMIDI provider and native-helper boundary tests; no actual MIDI/USB I/O.

Native SDK compilation and actual CoreMIDI calls require a separately validated
Mac. Mocked subprocess results here exercise the Python/native JSON boundary,
not native handle lifetime or hardware continuity.
"""
import copy
import hashlib
import io
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
import fm1_install as I  # noqa: E402
import ota_coremidi as C  # noqa: E402


def row(endpoint, entity=10, device=20, **values):
    result = dict(endpoint=endpoint, entity=entity, device=device, device_offline=0,
                  entity_offline=None, endpoint_offline=None,
                  usb_vendor_product=0x12090001, usb_location=0x14300000,
                  serial_property_present=False)
    result.update(values)
    return result


def snapshot(sources=None, destinations=None):
    return dict(schema='x0x37-coremidi-active-inventory-v1',
                sources=[row(1)] if sources is None else sources,
                destinations=[row(2)] if destinations is None else destinations,
                skipped_sources=0, skipped_destinations=0)


class Reader:
    def __init__(self, *snapshots):
        self.snapshots = iter(snapshots)

    def read(self):
        return copy.deepcopy(next(self.snapshots))

    def saved_devices(self):
        raise AssertionError('cached device enumeration is forbidden')


class Pairing(unittest.TestCase):
    def discover(self, data):
        return C.CoreMIDIProvider(Reader(data)).discover()

    def test_same_active_native_entity_and_device_pairs(self):
        result = self.discover(snapshot())
        self.assertEqual(len(result['pairs']), 1)
        self.assertEqual(result['pairs'][0]['source_ref'], 1)
        self.assertEqual(result['pairs'][0]['destination_ref'], 2)
        self.assertTrue(result['capabilities']['live_pair_observation'])
        self.assertTrue(all(not v for k, v in result['capabilities'].items()
                            if k != 'live_pair_observation'))

    def test_input_A_output_B_never_pair_by_usb_metadata_or_serial_presence(self):
        for source, destination in [(row(1), row(2, device=21)),
                                    (row(1), row(2, entity=11)),
                                    (row(1, serial_property_present=True),
                                     row(2, device=21, serial_property_present=True))]:
            with self.subTest(destination=destination):
                self.assertEqual(self.discover(snapshot([source], [destination]))['pairs'], [])

    def test_half_pairs_and_ambiguous_cables_have_no_sole_fallback(self):
        for data in (snapshot([], [row(2)]), snapshot([row(1)], []),
                     snapshot([row(1), row(3)], [row(2)]),
                     snapshot([row(1)], [row(2), row(3)])):
            with self.subTest(data=data):
                self.assertEqual(self.discover(data)['pairs'], [])

    def test_cached_devices_never_become_live_endpoints(self):
        result = self.discover(snapshot([], []))
        self.assertEqual(result['pairs'], [])
        self.assertFalse(result['capabilities']['live_pair_observation'])
        data = snapshot()
        data['saved_devices'] = [dict(name='SMK-37 Elite', SerialNumber='private')]
        with self.assertRaises(C.CapabilityUnavailable):
            self.discover(data)

    def test_offline_unknown_and_malformed_records_refused(self):
        for key, value in [('device_offline', 1), ('device_offline', None),
                           ('device_offline', False), ('endpoint_offline', 1),
                           ('entity_offline', 1), ('entity_offline', '0'),
                           ('usb_location', None), ('usb_vendor_product', 0),
                           ('serial_property_present', '16-char-is-not-proof'),
                           ('endpoint', True), ('entity', 0), ('device', -1)]:
            with self.subTest(key=key, value=value):
                self.assertEqual(self.discover(snapshot([dict(row(1), **{key: value})], [row(2)]))['pairs'], [])

    def test_inconsistent_parent_usb_metadata_refused(self):
        for delta in (dict(usb_location=0x14400000), dict(usb_vendor_product=0x12090002),
                      dict(serial_property_present=True)):
            self.assertEqual(self.discover(snapshot([row(1)], [row(2, **delta)]))['pairs'], [])

    def test_duplicate_endpoint_refs_refused(self):
        for data in (snapshot([row(1), row(1)], [row(2)]), snapshot([row(1)], [row(1)])):
            with self.assertRaises(C.CapabilityUnavailable):
                self.discover(data)

    def test_serial_presence_and_location_never_create_physical_id(self):
        result = self.discover(snapshot([row(1, serial_property_present=True)],
                                        [row(2, serial_property_present=True)]))
        self.assertTrue(result['pairs'][0]['serial_property_observed'])
        self.assertFalse(result['capabilities']['persistent_physical_id'])
        self.assertFalse(result['capabilities']['cross_reboot_continuity'])
        provider = C.CoreMIDIProvider(Reader(snapshot()))
        for attr in ('physical_id', 'open', 'send'):
            self.assertFalse(hasattr(provider, attr))

    def test_unknown_name_uniqueid_and_raw_serial_fields_are_rejected(self):
        for key in ('name', 'uniqueID', 'SerialNumber'):
            data = snapshot([dict(row(1), **{key: 'private-unit-data'})], [row(2)])
            result = self.discover(data)
            self.assertEqual(result['pairs'], [])
            self.assertNotIn('private-unit-data', json.dumps(result))

    def test_disappearance_and_replacement_do_not_reuse_previous_pairs(self):
        provider = C.CoreMIDIProvider(Reader(snapshot(), snapshot([], []),
            snapshot([row(101, entity=110, device=120)], [row(102, entity=110, device=120)])))
        self.assertEqual(provider.discover()['pairs'][0]['device_ref'], 20)
        self.assertEqual(provider.discover()['pairs'], [])
        replaced = provider.discover()
        self.assertEqual(replaced['pairs'][0]['device_ref'], 120)
        self.assertFalse(replaced['capabilities']['persistent_physical_id'])

    def test_collection_schema_and_bounds_refused(self):
        for delta in (dict(schema='cached-devices'), dict(sources={}),
                      dict(sources=[row(1)] * 513), dict(skipped_sources=-1)):
            with self.assertRaises(C.CapabilityUnavailable):
                self.discover(dict(snapshot(), **delta))


class NativeBoundary(unittest.TestCase):
    def test_helper_source_and_binary_hashes_must_match(self):
        with tempfile.TemporaryDirectory() as tmp:
            reader = C.NativeCoreMIDIReader()
            reader.source, reader.helper = Path(tmp) / 'source.m', Path(tmp) / 'helper'
            reader.source.write_bytes(b'synthetic source')
            reader.helper.write_bytes(b'synthetic helper')
            manifest = {'schema': 'x0x37-coremidi-helper-build-v1',
                        'source_sha256': hashlib.sha256(reader.source.read_bytes()).hexdigest(),
                        'helper_sha256': hashlib.sha256(reader.helper.read_bytes()).hexdigest()}
            with self.assertRaises(C.CapabilityUnavailable):
                reader._verify_helper()
            reader.helper.with_suffix('.json').write_text(json.dumps(manifest))
            reader._verify_helper()
            reader.helper.write_bytes(b'other helper')
            with self.assertRaises(C.CapabilityUnavailable):
                reader._verify_helper()
            reader.helper.write_bytes(b'synthetic helper')
            reader.source.write_bytes(b'other source')
            with self.assertRaises(C.CapabilityUnavailable):
                reader._verify_helper()

    def test_non_mac_platform_refuses_before_running_anything(self):
        with patch.object(C.sys, 'platform', 'linux'), patch.object(C.subprocess, 'run') as run:
            with self.assertRaises(C.CapabilityUnavailable):
                C.NativeCoreMIDIReader().read()
            run.assert_not_called()

    def test_missing_helper_refuses_without_fallback(self):
        with patch.object(C.sys, 'platform', 'darwin'), patch.object(C.Path, 'is_file', return_value=False), \
                patch.object(C.subprocess, 'run') as run:
            with self.assertRaises(C.CapabilityUnavailable):
                C.NativeCoreMIDIReader().read()
            run.assert_not_called()

    def test_native_json_boundary_uses_fixed_helper_no_shell(self):
        with patch.object(C.sys, 'platform', 'darwin'), patch.object(C.Path, 'is_file', return_value=True), \
                patch.object(C.NativeCoreMIDIReader, '_verify_helper'), \
                patch.object(C.subprocess, 'run', return_value=SimpleNamespace(
                    returncode=0, stdout=json.dumps(snapshot()).encode())) as run:
            self.assertEqual(C.NativeCoreMIDIReader().read(), snapshot())
            args, kwargs = run.call_args
            self.assertEqual(len(args[0]), 1)
            self.assertTrue(args[0][0].endswith('/build/native/ota_coremidi_inventory'))
            self.assertNotIn('shell', kwargs)
            self.assertEqual(kwargs['timeout'], 5)
            self.assertEqual(kwargs['stderr'], subprocess.DEVNULL)

    def test_bad_native_output_exit_and_timeout_fail_closed(self):
        outputs = [SimpleNamespace(returncode=2, stdout=b'private-error'),
                   SimpleNamespace(returncode=0, stdout=b'not json'),
                   SimpleNamespace(returncode=0, stdout=b'{"sources":[],"sources":[]}'),
                   SimpleNamespace(returncode=0, stdout=b'x' * (1024 * 1024 + 1))]
        for output in outputs:
            with patch.object(C.sys, 'platform', 'darwin'), patch.object(C.Path, 'is_file', return_value=True), \
                    patch.object(C.NativeCoreMIDIReader, '_verify_helper'), \
                    patch.object(C.subprocess, 'run', return_value=output):
                with self.assertRaises(C.CapabilityUnavailable):
                    C.NativeCoreMIDIReader().read()
        with patch.object(C.sys, 'platform', 'darwin'), patch.object(C.Path, 'is_file', return_value=True), \
                patch.object(C.NativeCoreMIDIReader, '_verify_helper'), \
                patch.object(C.subprocess, 'run', side_effect=subprocess.TimeoutExpired('helper', 5)):
            with self.assertRaises(C.CapabilityUnavailable):
                C.NativeCoreMIDIReader().read()

    def test_native_source_has_only_read_only_coremidi_calls(self):
        source = (ROOT / 'tools/native/ota_coremidi_inventory.m').read_text()
        calls = set(re.findall(r'\b(MIDI\w+)\s*\(', source))
        self.assertEqual(calls, {'MIDIObjectGetIntegerProperty', 'MIDIObjectGetProperties',
            'MIDIEndpointGetEntity', 'MIDIEntityGetDevice', 'MIDIGetNumberOfSources',
            'MIDIGetNumberOfDestinations', 'MIDIGetSource', 'MIDIGetDestination'})
        self.assertNotIn('kMIDIPropertyUniqueID', source)


class InstallerIntegration(unittest.TestCase):
    def test_live_pair_observation_does_not_enable_installer(self):
        observed = C.CoreMIDIProvider(Reader(snapshot())).discover()
        sent = []
        link = SimpleNamespace(close=lambda: None, send=lambda pkt: sent.append(pkt))
        dev = I.Device(link, I.Identity('SMK-37 Pro_016', 'SMK-37 Pro', 16), 'synthetic-port')
        dev.physical_id = None
        backend = SimpleNamespace(native_observations=observed)
        with self.assertRaises(I.InstallError) as caught:
            I.Updater(backend).install(dev, b'', 'SMK-37 Pro_900')
        self.assertEqual(caught.exception.code, 'model')
        self.assertEqual(sent, [])

    def test_report_never_opens_mido_or_runs_install_guards(self):
        output = io.StringIO()
        with patch.object(C.NativeCoreMIDIReader, 'read', return_value=snapshot()), \
                patch.object(I, 'MidoBackend', side_effect=AssertionError('Mido opened')), \
                patch.object(I, 'gate_guard', side_effect=AssertionError('install guard entered')), \
                patch.object(I, 'hardware_lock', side_effect=AssertionError('hardware lock entered')):
            self.assertEqual(I.main(['--native-endpoints'], out=output), 0)
        report = json.loads(output.getvalue())
        self.assertTrue(report['capabilities']['live_pair_observation'])
        self.assertFalse(report['capabilities']['ota_install'])

    def test_no_pair_or_no_capability_returns_refusal_without_mido(self):
        for data in (snapshot([], []), snapshot([row(1, device_offline=1)], [row(2)])):
            with patch.object(C.NativeCoreMIDIReader, 'read', return_value=data), \
                    patch.object(I, 'MidoBackend', side_effect=AssertionError('Mido opened')):
                self.assertEqual(I.main(['--native-endpoints'], out=io.StringIO()), 3)
        with patch.object(C.NativeCoreMIDIReader, 'read', side_effect=C.CapabilityUnavailable('missing')), \
                patch.object(I, 'MidoBackend', side_effect=AssertionError('Mido opened')):
            self.assertEqual(I.main(['--native-endpoints'], out=io.StringIO()), 3)

    def test_install_options_cannot_use_read_only_report_to_bypass_guards(self):
        for extra in (['fake.fwsc'], ['--info'], ['--yes'], ['--port', 'SMK'],
                      ['--no-slots'], ['--ctl', 'fake.img'], ['--config', 'rel'], ['--force']):
            with patch.object(sys, 'stderr', io.StringIO()), \
                    patch.object(C.NativeCoreMIDIReader, 'read') as read:
                with self.assertRaises(SystemExit) as caught:
                    I.main(['--native-endpoints', *extra], out=io.StringIO())
                self.assertEqual(caught.exception.code, 2)
                read.assert_not_called()


if __name__ == '__main__':
    unittest.main(verbosity=2)
