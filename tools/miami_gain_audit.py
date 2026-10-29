#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Read-only, fail-closed original Miami device/slot gain investigation.

Reads a revision-13 raw X0XP project or a revision-13 .x0xproject archive.
Does not write, normalize, change mixer controls, connect to hardware or load
private factory tables. Identifies exact stored mixer and macro edit bytes;
attenuation figures are diagnostics, not proof of the captured DSP state.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
from pathlib import Path
import struct
import sys
import zlib

MAX_SIZE = 4 * 1024 * 1024
SETTINGS_SIZE = 40
TARGETS = 7
VOICES = 12
PARAMS = 32
SOUND_SIZE = TARGETS * VOICES * PARAMS
SOUND_START = 32 + SETTINGS_SIZE
MIX = 5
MIAMI_PART = 2
CHICAGO_PART = 3
MIAMI_KIND = 1
CHICAGO_KIND = 0


class InvalidProject(ValueError):
    pass


def u16(data: bytes, at: int) -> int:
    return struct.unpack_from('<H', data, at)[0]


def u32(data: bytes, at: int) -> int:
    return struct.unpack_from('<I', data, at)[0]


def checked_project(raw: bytes) -> bytes:
    if len(raw) < SOUND_START + SOUND_SIZE or len(raw) > MAX_SIZE:
        raise InvalidProject('invalid raw project size')
    if raw[:4] != b'X0XP' or u32(raw, 4) != 1 or u32(raw, 16) != 1:
        raise InvalidProject('unsupported raw project header')
    if u32(raw, 8) != len(raw) - 32 or u32(raw, 20) != 3 or u32(raw, 24) != 13 or u32(raw, 28):
        raise InvalidProject('requires exact project stream 3 and sound revision 13')
    if zlib.crc32(raw[32:]) != u32(raw, 12):
        raise InvalidProject('raw project CRC mismatch')
    if u32(raw, 32) != 0x50305830 or u32(raw, 36) != 1:
        raise InvalidProject('invalid project settings magic/format')
    if raw[32+18] != 2 or raw[32+19] != 13:
        raise InvalidProject('settings are not stereo/revision 13')
    if raw[32+25] not in (0, 1) or raw[32+26] not in (0, 1):
        raise InvalidProject('invalid LOUD/USB FULL flags')
    return raw


def unpack(data: bytes) -> bytes:
    if data[:8] != b'X0XPROJ\0':
        if data[:4] == b'X0XP':
            return checked_project(data)
        raise InvalidProject('expected a raw X0XP or portable .x0xproject file')
    if len(data) < 20 or len(data) > MAX_SIZE:
        raise InvalidProject('invalid portable file size')
    if u16(data, 8) != 1 or u16(data, 10) not in (0, 1):
        raise InvalidProject('unsupported portable project version')
    count = u32(data, 12)
    if count > 16384 or 16 + count + 4 > len(data):
        raise InvalidProject('bad portable metadata length')
    if zlib.crc32(data[:-4]) != u32(data, len(data)-4):
        raise InvalidProject('portable archive CRC mismatch')
    try:
        meta = json.loads(data[16:16+count].decode('utf-8'))
    except (UnicodeError, json.JSONDecodeError) as exc:
        raise InvalidProject(f'bad portable metadata: {exc}') from exc
    if not isinstance(meta, dict) or meta.get('format') != 'x0xproject' or meta.get('version') != 1:
        raise InvalidProject('unsupported portable metadata')
    info = meta.get('project')
    if not isinstance(info, dict) or set(info) != {'offset', 'length', 'crc32'} or info['offset'] != 0:
        raise InvalidProject('missing project component')
    n = info['length']
    start = 16 + count
    if type(n) is not int or n < SOUND_START + SOUND_SIZE or start + n > len(data)-4:
        raise InvalidProject('truncated project component')
    project = data[start:start+n]
    if zlib.crc32(project) != info['crc32']:
        raise InvalidProject('project component CRC mismatch')
    if (u16(data, 10) == 0 and (meta.get('archive') is not None or start+n != len(data)-4)):
        raise InvalidProject('unexpected extra bytes in project-only file')
    if u16(data, 10) == 1:
        extra = meta.get('archive')
        if not isinstance(extra, dict) or set(extra) != {'offset','length','crc32'}:
            raise InvalidProject('missing sample archive descriptor')
        n_extra = extra['length']
        if type(n_extra) is not int or extra['offset'] != n or n_extra < 1 or start+n+n_extra != len(data)-4:
            raise InvalidProject('sample archive size mismatch')
        if zlib.crc32(data[start+n:start+n+n_extra]) != extra['crc32']:
            raise InvalidProject('sample archive CRC mismatch')
    return checked_project(project)


def cell(raw: bytes, target: int, voice: int) -> bytes:
    at = SOUND_START + (target * VOICES + voice) * PARAMS
    return raw[at:at+PARAMS]


def mixer_gain(level: int) -> float:
    return (level / 100.0) ** 2


def db(value: float) -> float | None:
    return 20.0 * math.log10(value) if value > 0.0 else None


def inspect(raw: bytes) -> dict:
    raw = checked_project(raw)
    a = cell(raw, 2, 0)
    b = cell(raw, 2, 1)
    for name, c, kind, size in [('Miami', a, MIAMI_KIND, 30), ('Chicago', b, CHICAGO_KIND, 55)]:
        if c[26:28] != b'OS' or c[28] != 2 or c[29] != kind or c[30] >= size or c[31]:
            raise InvalidProject(f'{name} slot identity/schema invalid, requires rev13')
    parts = []
    for name, index, patch in [('Miami/303A', MIAMI_PART, a), ('Chicago/303B', CHICAGO_PART, b)]:
        c = cell(raw, MIX, index)
        level, pan = c[0], c[3]
        if level > 127 or pan > 127:
            raise InvalidProject(f'{name} mixer LEVEL/PAN outside 0..127')
        edit_mask = int.from_bytes(patch[22:26], 'little')
        if edit_mask & ~((1 << (18 if index == MIAMI_PART else 22)) - 1):
            raise InvalidProject(f'{name} invalid patch edit mask')
        if patch[14] > 127:
            raise InvalidProject(f'{name} output display byte out of range')
        gain = mixer_gain(level)
        parts.append({
            'slot': name, 'mixer_level_0_127': level,
            'mixer_gain_linear': gain, 'mixer_gain_db': db(gain),
            'mixer_pan_0_127': pan, 'factory_index': patch[30],
            'patch_edit_mask': f'0x{edit_mask:08x}',
            'display_output_byte_0_127': patch[14] if index == MIAMI_PART else None,
            'miami_output_explicitly_edited': bool(edit_mask & (1 << 14)) if index == MIAMI_PART else None,
        })
    return {
        'source': 'revision-13 persistent project (CRC validated)',
        'project_sha256': hashlib.sha256(raw).hexdigest(),
        'output_setting': 'LOUD' if raw[32+25] else 'NORMAL',
        'usb_audio_setting': 'FULL' if raw[32+26] else 'FOLLOWS KNOB',
        'midi_part_channel_2': raw[32+20+2] or 2,  # 0 means default 2
        'parts': parts,
    }


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--project', type=Path, help='raw X0XP or .x0xproject file exported from the device')
    p.add_argument('--golden-peak-dbfs', type=float, default=-9.6)
    p.add_argument('--device-peak-dbfs', type=float, default=-37.1)
    p.add_argument('--golden-rms-dbfs', type=float, default=-19.1)
    p.add_argument('--device-rms-dbfs', type=float, default=-49.0)
    p.add_argument('--golden-velocity', type=int, default=100,
                   help='reference MIDI note velocity; the internal p00 harness uses 100')
    p.add_argument('--json', type=Path, help='local report destination (no private bytes stored)')
    a = p.parse_args()
    if not 1 <= a.golden_velocity <= 127:
        p.error('--golden-velocity must be 1..127')
    output = {'reference_midi_velocity': a.golden_velocity}
    for label in ('peak', 'rms'):
        desired, measured = getattr(a, f'golden_{label}_dbfs'), getattr(a, f'device_{label}_dbfs')
        if not math.isfinite(desired) or not math.isfinite(measured):
            p.error('dB inputs must be finite')
        delta = measured - desired
        output[f'{label}_difference_db'] = round(delta, 4)
        output[f'{label}_device_over_golden_linear'] = 10**(delta/20)
        output[f'{label}_mixer_level_if_only_mixer_loss'] = 100 * (10**(delta/20))**0.5
        output[f'{label}_midi_velocity_if_only_squared_velocity_loss'] = (
            a.golden_velocity * (10**(delta/20))**0.5
        )
    output['warning'] = (
        'Mixer LEVEL and MIDI velocity equivalences are independent diagnostic '
        'hypotheses, not proof of a cause. The synth684 p00 golden uses a '
        'multi-note, parameter-change script, not a single ch2 MIDI note. '
        'Device and reference must use identical note/velocity/script, preset, '
        'mixer, master, LOUD, USB FULL and output tap before attributing dB loss.'
    )
    if a.project:
        output['project'] = inspect(unpack(a.project.read_bytes()))
        actual = output['project']['parts'][0]
        output['mixer_gain_matches_measured_peak_within_1db'] = (
            actual['mixer_gain_db'] is not None and
            abs(actual['mixer_gain_db'] - output['peak_difference_db']) <= 1.0
        )
    print(json.dumps(output, indent=2, sort_keys=True))
    if a.json:
        a.json.parent.mkdir(parents=True, exist_ok=True)
        a.json.write_text(json.dumps(output, indent=2, sort_keys=True) + '\n')
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except (InvalidProject, OSError) as error:
        print(f'Miami gain audit FAIL: {error}', file=sys.stderr)
        sys.exit(2)
