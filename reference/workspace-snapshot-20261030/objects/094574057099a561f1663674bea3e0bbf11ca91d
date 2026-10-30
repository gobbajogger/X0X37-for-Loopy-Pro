#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Independent source-function protocol boundary/fault-matrix checks for PR #110.

Usage: python3 -B tests/host/ctl_slot_wire_exhaustive_test.py [source-root]
No device writes, flash, mido, SDK, network, or proprietary sources.
Uses actual function ASTs from source-root/tools, not rewritten implementations.
"""
import ast
from pathlib import Path
import sys
import types
import unittest

ROOT = Path(sys.argv.pop(1)) if len(sys.argv) > 1 else Path(__file__).resolve().parents[2]


def pack7(data):
    out = []
    for i in range(0, len(data), 7):
        group = data[i:i + 7]
        out.append(sum(((b >> 7) & 1) << j for j, b in enumerate(group)))
        out.extend(b & 127 for b in group)
    return out


def unpack7(data):
    output = bytearray()
    for i in range(0, len(data), 8):
        chunk = data[i:i + 8]
        if len(chunk) > 1:
            output.extend(b | (((chunk[0] >> j) & 1) << 7) for j, b in enumerate(chunk[1:]))
    return bytes(output)


class SlotError(Exception):
    pass


def native_function(path, name, env):
    """Compile the actual AST function in the source root, without side effects of CLI imports."""
    tree = ast.parse(Path(path).read_text())
    matches = [n for n in tree.body if isinstance(n, (ast.FunctionDef, ast.AsyncFunctionDef)) and n.name == name]
    assert len(matches) == 1, (path, name, len(matches))
    unit = ast.fix_missing_locations(ast.Module(body=matches, type_ignores=[]))
    exec(compile(unit, str(path), 'exec'), env)
    return env[name]


def req(link, cmd, args, *unused):
    return link.req(cmd, args, 3.0)


xs = types.SimpleNamespace(req=req, pack7=pack7, unpack7=unpack7)
ctl_globals = dict(xs=xs, CTL_READ_N=224, ED_CTL_READ=37, CTL_READ={'timeout': 3.0, 'tries': 3})
slot_globals = dict(req=req, pack7=pack7, unpack7=unpack7, SlotError=SlotError,
                    READ_MAX=224, CMD_READ=16)
read_ctl = native_function(ROOT / 'tools/fm1_install.py', 'read_ctl', ctl_globals)
read_bytes = native_function(ROOT / 'tools/x0x_slots.py', 'read_bytes', slot_globals)
slot_ack = native_function(ROOT / 'tools/x0x_slots.py', '_slot_ack', slot_globals)


class Link:
    """Fault-injects one readback frame at an exact address; no actual USB operations."""
    def __init__(self, raw, target=None, position=0, value=128):
        self.raw = raw
        self.target = target
        self.position = position
        self.value = value
        self.calls = []

    def req(self, cmd, args, timeout):
        is_ctl = cmd == 37
        if is_ctl:
            addr = args[0] | args[1] << 7 | args[2] << 14
            length = args[3] * 2
            wire = [addr & 127, (addr >> 7) & 127, (addr >> 14) & 127]
        else:
            slot, a, b, c, pairs = args
            addr = a | b << 7 | c << 14
            length = pairs * 2
            wire = [slot, a, b, c]
        self.calls.append(addr)
        wire.extend(pack7(self.raw[addr:addr + length]))
        if self.target == addr:
            wire[self.position] = self.value
        return wire


class WireMatrix(unittest.TestCase):
    def test_controller_7bit_echo_exhaustive(self):
        for target in (0, 224, 16576):
            raw = bytes((i * 29 + 5) & 255 for i in range(target + 224))
            for position in range(3):
                for value in range(128, 256):
                    with self.subTest(addr=target, byte=position, value=value):
                        link = Link(raw, target, position, value)
                        with self.assertRaises(TimeoutError):
                            read_ctl(link, target + 2)
                        self.assertEqual(link.calls[-1], target)
                        self.assertEqual(len(link.calls), target // 224 + 1)

    def test_controller_negative_address_bytes_rejected(self):
        raw = b'\xaa\x55'
        for position in range(3):
            for value in (-1, -2, -128, -129):
                with self.subTest(position=position, value=value):
                    with self.assertRaises(TimeoutError):
                        read_ctl(Link(raw, 0, position, value), 2)

    def test_sample_slot_7bit_echo_exhaustive(self):
        raw = b'\x00' * 0x4000 + b'\x80\xfe'
        for addr in (0, 0x80, 0x4000):
            for position in range(4):
                for value in range(128, 256):
                    with self.subTest(addr=addr, position=position, value=value):
                        link = Link(raw, addr, position, value)
                        updates = []
                        with self.assertRaises(SlotError):
                            read_bytes(link, 0, addr, 2, lambda *args: updates.append(args))
                        self.assertEqual(link.calls, [addr])
                        self.assertFalse(updates)

    def test_write_ack_highbit_matrix(self):
        for position in range(5):
            for value in range(128, 256):
                with self.subTest(position=position, value=value):
                    reply = [0, 0, 0, 0, 0]
                    reply[position] = value
                    with self.assertRaises(SlotError):
                        slot_ack(reply, 0, 'write', 0)

    def test_legitimate_readback_and_write_ack(self):
        raw = bytes(range(256)) * 65
        for n in (1, 2, 3, 223, 224, 225, 448, 65536):
            data = (raw * ((n // len(raw)) + 2))[:n + (n % 2)]
            link = Link(data)
            with self.subTest(n=n):
                self.assertEqual(read_ctl(link, n), data[:n])
        slot_data = b'\x00' * 0x4000 + b'\x81\xfe'
        self.assertEqual(read_bytes(Link(slot_data), 0, 0x4000, 2), b'\x81\xfe')
        self.assertEqual(slot_ack([0, 0, 0, 1, 0], 0, 'write', 0x4000), 0)


if __name__ == '__main__':
    unittest.main(verbosity=2)
