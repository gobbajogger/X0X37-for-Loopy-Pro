#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Read-only native macOS endpoint pairing; NOT an OTA transport or unit ID.

The native helper starts from active sources/destinations, never saved devices.
Pairs use exact endpoint -> entity -> device relationships, never display names,
serial strings, USB locations, uniqueIDs or sole-output fallbacks. Observations
do not lease endpoints, establish atomic sends, or survive re-enumeration.
"""
from collections import defaultdict
import hashlib
import json
from pathlib import Path
import subprocess
import sys


class CapabilityUnavailable(RuntimeError):
    pass


def _unique_object(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise CapabilityUnavailable('duplicate native inventory field')
        result[key] = value
    return result


class NativeCoreMIDIReader:
    def __init__(self):
        self.helper = Path(__file__).resolve().parents[1] / 'build/native/ota_coremidi_inventory'
        self.source = Path(__file__).resolve().parent / 'native/ota_coremidi_inventory.m'

    def _verify_helper(self):
        try:
            manifest = json.loads(self.helper.with_suffix('.json').read_bytes(),
                                  object_pairs_hook=_unique_object)
            expected = {'schema': 'x0x37-coremidi-helper-build-v1',
                        'source_sha256': hashlib.sha256(self.source.read_bytes()).hexdigest(),
                        'helper_sha256': hashlib.sha256(self.helper.read_bytes()).hexdigest()}
        except (OSError, ValueError, UnicodeError) as exc:
            raise CapabilityUnavailable('native helper build provenance is unavailable') from exc
        if manifest != expected:
            raise CapabilityUnavailable('native helper does not match its source/build hashes')

    def read(self):
        if sys.platform != 'darwin':
            raise CapabilityUnavailable('native CoreMIDI discovery requires macOS')
        if not self.helper.is_file():
            raise CapabilityUnavailable('native CoreMIDI inventory helper is not built')
        self._verify_helper()
        try:
            result = subprocess.run([str(self.helper)], stdout=subprocess.PIPE,
                                    stderr=subprocess.DEVNULL, timeout=5, check=False)
        except (OSError, subprocess.TimeoutExpired) as exc:
            raise CapabilityUnavailable('native CoreMIDI inventory could not run') from exc
        if result.returncode or len(result.stdout) > 1024 * 1024:
            raise CapabilityUnavailable('native CoreMIDI inventory failed or exceeded its bound')
        try:
            return json.loads(result.stdout, object_pairs_hook=_unique_object)
        except (ValueError, UnicodeError) as exc:
            raise CapabilityUnavailable('native CoreMIDI inventory is not valid JSON') from exc


def _ref(value):
    return type(value) is int and 0 < value <= 0xffffffff


ROW_FIELDS = frozenset(('endpoint', 'entity', 'device', 'device_offline',
                        'entity_offline', 'endpoint_offline', 'usb_vendor_product',
                        'usb_location', 'serial_property_present'))


def _eligible(row):
    if not isinstance(row, dict) or set(row) != ROW_FIELDS:
        return False
    if not all(_ref(row[k]) for k in ('endpoint', 'entity', 'device')):
        return False
    # Require a successful online-device observation. Optional per-entity and
    # endpoint offline properties may be unsupported, but never accept a true
    # or malformed offline value. Membership came from active endpoint APIs.
    if type(row['device_offline']) is not int or row['device_offline'] != 0:
        return False
    for key in ('entity_offline', 'endpoint_offline'):
        if row[key] is not None and (type(row[key]) is not int or row[key] != 0):
            return False
    return (all(_ref(row[k]) for k in ('usb_vendor_product', 'usb_location'))
            and type(row['serial_property_present']) is bool)


class CoreMIDIProvider:
    """Produces current read-only pairing observations, never persistent IDs."""
    def __init__(self, reader=None):
        self.reader = reader if reader is not None else NativeCoreMIDIReader()

    def discover(self):
        snapshot = self.reader.read()  # no cache, including after failed reads
        keys = {'schema', 'sources', 'destinations', 'skipped_sources', 'skipped_destinations'}
        if not isinstance(snapshot, dict) or set(snapshot) != keys:
            raise CapabilityUnavailable('native inventory schema fields are unsupported')
        if snapshot['schema'] != 'x0x37-coremidi-active-inventory-v1':
            raise CapabilityUnavailable('native inventory version is unsupported')
        for key in ('skipped_sources', 'skipped_destinations'):
            if type(snapshot[key]) is not int or not 0 <= snapshot[key] <= 512:
                raise CapabilityUnavailable('invalid skipped-endpoint count')
        rows = {}
        for direction in ('sources', 'destinations'):
            values = snapshot[direction]
            if not isinstance(values, list) or len(values) > 512:
                raise CapabilityUnavailable('native endpoint collection is invalid')
            rows[direction] = values
        # A repeated ref is ambiguous even if its two rows look identical.
        refs = [r.get('endpoint') for values in rows.values() for r in values
                if isinstance(r, dict) and _ref(r.get('endpoint'))]
        if len(refs) != len(set(refs)):
            raise CapabilityUnavailable('duplicate live endpoint reference')
        grouped = defaultdict(lambda: {'sources': [], 'destinations': []})
        rejected = snapshot['skipped_sources'] + snapshot['skipped_destinations']
        for direction, values in rows.items():
            for row in values:
                if not _eligible(row):
                    rejected += 1
                    continue
                grouped[(row['device'], row['entity'])][direction].append(row)
        observations = []
        for (device, entity), directions in sorted(grouped.items()):
            sources, destinations = directions['sources'], directions['destinations']
            if len(sources) != 1 or len(destinations) != 1:
                rejected += len(sources) + len(destinations)
                continue
            source, destination = sources[0], destinations[0]
            metadata = ('usb_vendor_product', 'usb_location', 'serial_property_present')
            if any(source[k] != destination[k] for k in metadata):
                rejected += 2
                continue
            observations.append({
                'source_ref': source['endpoint'], 'destination_ref': destination['endpoint'],
                'entity_ref': entity, 'device_ref': device,
                'usb_vendor_product': source['usb_vendor_product'],
                'serial_property_observed': source['serial_property_present'],
                'scope': 'one active CoreMIDI inventory observation; session-local object refs',
            })
        return {
            'schema': 'x0x37-native-endpoint-capabilities-v1',
            'platform': 'macOS/CoreMIDI', 'pairs': observations,
            'rejected_endpoints': rejected,
            'capabilities': {
                'live_pair_observation': bool(observations),
                'native_usb_handle_binding': False, 'disconnect_invalidation': False,
                'atomic_send': False, 'persistent_physical_id': False,
                'cross_reboot_continuity': False, 'ota_install': False,
                'sample_restore': False,
            },
            'status': 'READ-ONLY DISCOVERY / NO DESTRUCTIVE TRANSPORT / NOT INSTALLABLE',
        }


def report(out=sys.stdout):
    try:
        result = CoreMIDIProvider().discover()
    except CapabilityUnavailable as exc:
        print(json.dumps({'status': 'NATIVE DISCOVERY UNAVAILABLE', 'reason': str(exc),
                          'ota_install': False}), file=out)
        return 3
    print(json.dumps(result, indent=2, sort_keys=True), file=out)
    return 0 if result['pairs'] else 3


if __name__ == '__main__':
    sys.exit(report())
