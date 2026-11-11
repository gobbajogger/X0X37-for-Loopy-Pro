#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Fail-before/pass-after 7-bit echo identity guards for SMP_READ/slot ACK.

Run: python3 -B tests/x0x_slots_echo_7bit_test.py
No device access. Actual source function windows from PR#103.
"""
import sys
import unittest
from pathlib import Path

SRC = Path(sys.argv.pop(1)) if len(sys.argv) > 1 else Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SRC / 'tools'))
import x0x_slots as xs


class Link:
    def __init__(self, data, mode='valid'):
        self.data, self.mode, self.calls = bytes(data), mode, []

    def req(self, command, args, timeout):
        assert command == xs.CMD_READ
        k, a, b, c, pairs = args
        address = a | b << 7 | c << 14
        self.calls.append(address)
        raw = self.data[address:address + 2 * pairs]
        echo = [k, a, b, c]
        if self.mode == 'aliased-low' and address == 0x80:
            # bit7 set in low address byte aliases bit0 of the middle byte.
            echo[1] = 0x80
        elif self.mode == 'aliased-middle' and address == 0x4000:
            # bit7 set in middle byte aliases bit0 of the high address byte.
            echo[2] = 0x80
        elif self.mode == 'negative-low' and address == 0x80:
            echo[1] = -0x80
        elif self.mode == 'fragment':
            raw = raw[:1]
        elif self.mode == 'wrong-slot':
            echo[0] += 1
        return echo + xs.pack7(raw)


class Echo7Bit(unittest.TestCase):
    def test_read_valid_even_odd_roundup_and_partial_replies(self):
        data = bytes((x * 17 + 3) & 255 for x in range(10))
        for count in (3, 4, 9, 10):
            with self.subTest(count=count):
                self.assertEqual(xs.read_bytes(Link(data), 0, 0, count), data[:count])
        fragment = Link(data, mode='fragment')
        self.assertEqual(xs.read_bytes(fragment, 0, 0, 4), data[:4])
        self.assertEqual(fragment.calls, [0, 1, 2, 3])

    def test_read_valid_canonical_high_addresses(self):
        data = b'\x00' * 0x4000 + b'\x81\xfe'
        for addr in (0x80, 0x4000):
            with self.subTest(addr=addr):
                link = Link(data)
                self.assertEqual(xs.read_bytes(link, 0, addr, 2), data[addr:addr + 2])
                self.assertEqual(link.calls, [addr])

    def test_read_reject_noncanonical_low_echo_even_when_address_matches(self):
        data = bytes(range(256))
        with self.assertRaises(xs.SlotError):
            xs.read_bytes(Link(data, 'aliased-low'), 0, 0x80, 2)

    def test_read_reject_noncanonical_middle_echo_even_when_address_matches(self):
        data = b'\x00' * 0x4000 + b'\xab\xcd'
        with self.assertRaises(xs.SlotError):
            xs.read_bytes(Link(data, 'aliased-middle'), 0, 0x4000, 2)

    def test_read_reject_negative_echo_even_when_address_matches(self):
        data = bytes(range(256))
        with self.assertRaises(xs.SlotError):
            xs.read_bytes(Link(data, 'negative-low'), 0, 0x80, 2)

    def test_read_wrong_slot_still_rejected(self):
        with self.assertRaises(xs.SlotError):
            xs.read_bytes(Link(b'AB', 'wrong-slot'), 0, 0, 2)

    def test_ack_good_canonical_offsets_and_no_address(self):
        self.assertEqual(xs._slot_ack([0, 0, 0, 1, 0], 0, 'write', 0x4000), 0)
        self.assertEqual(xs._slot_ack([0, 0, 1, 0, 0], 0, 'write', 0x80), 0)
        self.assertEqual(xs._slot_ack([1, 0], 1, 'begin'), 0)

    def test_ack_reject_noncanonical_low_echo_alias(self):
        with self.assertRaises(xs.SlotError):
            xs._slot_ack([0, 0x80, 1, 0, 0], 0, 'write', 0x80)

    def test_ack_reject_noncanonical_middle_echo_alias(self):
        with self.assertRaises(xs.SlotError):
            xs._slot_ack([0, 0, 0x80, 1, 0], 0, 'write', 0x4000)

    def test_ack_reject_status_outside_midi_data_byte(self):
        with self.assertRaises(xs.SlotError):
            xs._slot_ack([1, 0x80], 1, 'erase')

    def test_ack_wrong_offset_and_length_still_rejected(self):
        for reply in ([0, 0, 0, 0, 0], [0, 0, 0, 1], [1, 0, 0, 1, 0]):
            with self.subTest(reply=reply):
                with self.assertRaises(xs.SlotError):
                    xs._slot_ack(reply, 0, 'write', 0x4000)


if __name__ == '__main__':
    unittest.main(verbosity=2)
