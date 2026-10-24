"""SMP_READ framing, backup integrity and cancellation-negative host regressions.

Run from the repository root: python3 -B tests/x0x_slots_read_reply_test.py
No device flash or hardware access; fake link returns only deliberately scripted frames.
"""
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import x0x_slots as xs


class Link:
    def __init__(self, data, *, mode='valid'):
        self.data, self.mode, self.calls = bytes(data), mode, []

    def req(self, command, args, timeout):
        assert command == xs.CMD_READ
        slot = args[0]
        address = args[1] | (args[2] << 7) | (args[3] << 14)
        requested = args[4] * 2
        self.calls.append((slot, address, requested))
        raw = self.data[address:address + requested]
        if self.mode == 'overlong':
            raw += b'\xff\x99'
        elif self.mode == 'truncated':
            return [slot, args[1], args[2]]
        elif self.mode == 'wrong-address':
            return [slot, args[1] ^ 1, args[2], args[3]] + xs.pack7(raw)
        elif self.mode == 'wrong-slot':
            return [slot + 1, args[1], args[2], args[3]] + xs.pack7(raw)
        elif self.mode == 'empty':
            raw = b''
        elif self.mode == 'fragment':
            raw = raw[:2]
        return [slot, args[1], args[2], args[3]] + xs.pack7(raw)


class ReadReply(unittest.TestCase):
    def test_exact_even_readback(self):
        data = bytes(range(20))
        link = Link(data)
        self.assertEqual(xs.read_bytes(link, 0, 0, len(data)), data)
        self.assertEqual(link.calls, [(0, 0, 20)])

    def test_odd_pair_roundup(self):
        link = Link(bytes([0x80, 0xfe, 0x39, 0xa5]))
        self.assertEqual(xs.read_bytes(link, 1, 0, 3), bytes([0x80, 0xfe, 0x39]))
        self.assertEqual(link.calls, [(1, 0, 4)])

    def test_read_over_two_224_byte_frames(self):
        data = bytes(range(256)) * 2
        link = Link(data)
        self.assertEqual(xs.read_bytes(link, 0, 0, 450), data[:450])
        self.assertEqual([a for _, a, _ in link.calls], [0, 224, 448])

    def test_partial_valid_replies_advance_address(self):
        data = b'ABCDEF'
        link = Link(data, mode='fragment')
        self.assertEqual(xs.read_bytes(link, 0, 0, 6), data)
        self.assertEqual([a for _, a, _ in link.calls], [0, 2, 4])

    def test_oversized_payload_refused_before_return(self):
        link = Link(b'ABCD', mode='overlong')
        with self.assertRaises(xs.SlotError):
            xs.read_bytes(link, 0, 0, 4)
        self.assertEqual(link.calls, [(0, 0, 4)])

    def test_oversized_single_byte_request_refused(self):
        link = Link(b'AB', mode='overlong')
        with self.assertRaises(xs.SlotError):
            xs.read_bytes(link, 0, 0, 1)

    def test_truncated_frame_refused(self):
        link = Link(b'AB', mode='truncated')
        with self.assertRaises(xs.SlotError):
            xs.read_bytes(link, 0, 0, 2)

    def test_empty_reply_refused(self):
        with self.assertRaises(xs.SlotError):
            xs.read_bytes(Link(b'AB', mode='empty'), 0, 0, 2)

    def test_wrong_slot_refused(self):
        with self.assertRaises(xs.SlotError):
            xs.read_bytes(Link(b'AB', mode='wrong-slot'), 0, 0, 2)

    def test_wrong_address_refused(self):
        with self.assertRaises(xs.SlotError):
            xs.read_bytes(Link(b'AB', mode='wrong-address'), 0, 0, 2)


if __name__ == '__main__':
    unittest.main(verbosity=2)
