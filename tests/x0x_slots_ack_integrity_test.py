#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Fail-before/pass-after native slot-restore ACK identity/corruption negatives.

Run: python3 -B slot_ack_integrity_test.py /path/to/source-root
Only fake command replies; no device, no flash, no external dependencies.
"""
import os
import sys
import unittest
from pathlib import Path

src = Path(sys.argv.pop(1)) if len(sys.argv) > 1 else Path(__file__).resolve().parents[1]
sys.path.insert(0, str(src / 'tools'))
import x0x_slots as xs

# Only the actual write_slot/req/pack7 source functions are exercised.
class FakeLink:
    def __init__(self, mutate=None, fail_command=None):
        self.mutate=mutate
        self.fail_command=fail_command
        self.events=[]

    def req(self,cmd,args,timeout):
        self.events.append((cmd, list(args)))
        k=args[0]
        if cmd == xs.CMD_WRITE:
            reply=[k,args[1],args[2],args[3],0]
        else:
            reply=[k,0]
        if self.fail_command == cmd:
            reply[-1]=3
        if self.mutate:
            reply=self.mutate(cmd, list(reply), len(self.events))
        return reply


IMAGE=(b'HEADER' * 80)[:xs.HDR_LEN] + b'\xFF' * (xs.DATA_OFF-xs.HDR_LEN) + bytes(range(256))*2


class AckTests(unittest.TestCase):
    def error(self, mutate, *, image=IMAGE, prefix=None, expect_last=None):
        link=FakeLink(mutate)
        with self.assertRaises(xs.SlotError):
            xs.write_slot(link,0,image)
        if prefix is not None:
            self.assertEqual([cmd for cmd,_ in link.events],prefix)
        if expect_last is not None:
            self.assertEqual(link.events[-1][0],expect_last)
        return link

    def test_valid_two_frame_write_and_end(self):
        link=FakeLink()
        xs.write_slot(link,0,IMAGE)
        self.assertEqual([c for c,a in link.events],[xs.CMD_BEGIN,xs.CMD_WRITE,xs.CMD_WRITE,xs.CMD_END])
        self.assertEqual([a[1]|a[2]<<7|a[3]<<14 for c,a in link.events if c==xs.CMD_WRITE],[512,768])

    def test_valid_erase(self):
        link=FakeLink()
        xs.write_slot(link,1,None)
        self.assertEqual([c for c,a in link.events],[xs.CMD_ERASE])

    def test_begin_wrong_slot_fails_before_any_payload(self):
        self.error(lambda c,r,n: [r[0]+1,r[1]] if c==xs.CMD_BEGIN else r,prefix=[xs.CMD_BEGIN])

    def test_begin_truncated_fails_before_payload(self):
        self.error(lambda c,r,n: [r[0]] if c==xs.CMD_BEGIN else r,prefix=[xs.CMD_BEGIN])

    def test_begin_extra_ack_bytes_refused(self):
        self.error(lambda c,r,n: r+[0] if c==xs.CMD_BEGIN else r,prefix=[xs.CMD_BEGIN])

    def test_erase_wrong_slot_never_reports_success(self):
        self.error(lambda c,r,n: [r[0]+1,r[1]] if c==xs.CMD_ERASE else r,image=None,prefix=[xs.CMD_ERASE])

    def test_erase_short_ack_refused(self):
        self.error(lambda c,r,n: [] if c==xs.CMD_ERASE else r,image=None,prefix=[xs.CMD_ERASE])

    def test_write_wrong_slot_fails_before_end(self):
        self.error(lambda c,r,n: [r[0]+1]+r[1:] if c==xs.CMD_WRITE else r,
                   prefix=[xs.CMD_BEGIN,xs.CMD_WRITE])

    def test_write_wrong_offset_fails_before_end(self):
        self.error(lambda c,r,n: [r[0],r[1]^1]+r[2:] if c==xs.CMD_WRITE else r,
                   prefix=[xs.CMD_BEGIN,xs.CMD_WRITE])

    def test_second_write_replayed_prior_ack_fails_before_end(self):
        self.error(lambda c,r,n: [r[0],0,4,0,r[4]] if c==xs.CMD_WRITE and n==3 else r,
                   prefix=[xs.CMD_BEGIN,xs.CMD_WRITE,xs.CMD_WRITE])

    def test_write_truncated_ack_is_sloterror_not_indexerror(self):
        self.error(lambda c,r,n: r[:4] if c==xs.CMD_WRITE else r,
                   prefix=[xs.CMD_BEGIN,xs.CMD_WRITE])

    def test_end_wrong_slot_is_not_success(self):
        self.error(lambda c,r,n: [r[0]+1,r[1]] if c==xs.CMD_END else r,
                   prefix=[xs.CMD_BEGIN,xs.CMD_WRITE,xs.CMD_WRITE,xs.CMD_END])

    def test_end_empty_ack_is_sloterror(self):
        self.error(lambda c,r,n: [] if c==xs.CMD_END else r,
                   prefix=[xs.CMD_BEGIN,xs.CMD_WRITE,xs.CMD_WRITE,xs.CMD_END])

    def test_device_write_error_still_aborts(self):
        link=FakeLink(fail_command=xs.CMD_WRITE)
        with self.assertRaises(xs.SlotError):
            xs.write_slot(link,0,IMAGE)
        self.assertEqual([c for c,a in link.events],[xs.CMD_BEGIN,xs.CMD_WRITE])

    def test_device_end_error_still_aborts(self):
        link=FakeLink(fail_command=xs.CMD_END)
        with self.assertRaises(xs.SlotError):
            xs.write_slot(link,0,IMAGE)
        self.assertEqual(link.events[-1][0],xs.CMD_END)

    def test_device_erase_error_still_aborts(self):
        link=FakeLink(fail_command=xs.CMD_ERASE)
        with self.assertRaises(xs.SlotError):
            xs.write_slot(link,0,None)
        self.assertEqual([c for c,a in link.events],[xs.CMD_ERASE])

if __name__=='__main__':
    unittest.main(verbosity=2)
