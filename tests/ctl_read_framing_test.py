#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Controller ED_CTL_READ framing fail-before/pass-after; no device or flash.

Usage: python3 -B controller_read_framing_test.py before|after
The exercised read_ctl() function matches current Git blob's exact source
window FNV1a32=97c89af2 in the baseline, independently checked via GitHub.
"""
import sys
import unittest
from pathlib import Path

source_root = Path(__file__).resolve().parents[1]  # complete checkout default
if len(sys.argv)>1 and sys.argv[1] in ('before','after'):
    source_root = Path(__file__).resolve().parent / sys.argv.pop(1)
sys.path.insert(0, str(source_root / 'tools'))
import fm1_install as install

class FakeLink:
    def __init__(self, data, mode='valid', corrupt_at=0, corrupt_offset=None):
        self.data=bytes(data);self.mode=mode;self.calls=[];self.corrupt_at=corrupt_at
        self.pending=None
        self.corrupt_offset=corrupt_offset

    # Real installer wraps its link with EditorLink. Exercise that framing path,
    # not just read_ctl() directly, before treating readback as installed CRC proof.
    def drain(self):
        self.pending=None

    def send(self,packet):
        hdr=install.ED_HDR
        assert packet[:len(hdr)]==hdr and packet[-1]==0xf7
        self.pending=(packet[len(hdr)],list(packet[len(hdr)+1:-1]))
        return True

    def read(self,timeout):
        if self.pending is None: return None
        cmd,args=self.pending;self.pending=None
        response=self.req(cmd,args,timeout)
        return install.ED_HDR+bytes([cmd])+bytes(response)+b'\xF7'

    def req(self,cmd,args,timeout):
        assert cmd == install.ED_CTL_READ
        off=args[0] | args[1]<<7 | args[2]<<14
        n=args[3]*2
        self.calls.append((off,n))
        raw=self.data[off:off+n]
        wire=install.xs.pack7(raw)
        i=len(self.calls)
        if i>self.corrupt_at:
            if self.mode=='overlong':
                wire=install.xs.pack7(raw+b'\x51\xa9')
            elif self.mode=='orphan-mask':
                wire+=[0]
            elif self.mode=='unused-mask':
                # Exactly two bytes: bit 6 is unused and must be zero.
                wire[0] |= 0x40
            elif self.mode=='nonsevenbit':
                wire[1] |= 0x80
            elif self.mode=='short':
                wire=wire[:-1]
        addr=[off & 0x7f,(off>>7)&0x7f,(off>>14)&0x7f]
        if self.mode == 'wrong-address' and i>self.corrupt_at:
            addr[0]^=1
        if self.mode == 'truncated-address' and i>self.corrupt_at:
            addr=addr[:2]
        if i>self.corrupt_at and (self.corrupt_offset is None or off == self.corrupt_offset):
            if self.mode == 'nonsevenbit-address-low':
                addr[0] |= 0x80
            elif self.mode == 'nonsevenbit-address-middle':
                addr[1] |= 0x80
            elif self.mode == 'nonsevenbit-address-high':
                addr[2] |= 0x80
        return addr+wire

class ControllerReadTests(unittest.TestCase):
    def test_valid_highbit_and_multiframe(self):
        data=bytes(range(256))*3
        f=FakeLink(data)
        self.assertEqual(install.read_ctl(f,len(data)),data)
        self.assertEqual([o for o,_ in f.calls],[0,224,448,672])

    def test_valid_odd_final_pair_rounding(self):
        data=b'\x80\x7F\xFE\x55'
        f=FakeLink(data)
        self.assertEqual(install.read_ctl(f,3),data[:3])
        self.assertEqual(f.calls,[(0,4)])

    def test_valid_two_byte_reply(self):
        f=FakeLink(b'AB')
        self.assertEqual(install.read_ctl(f,2),b'AB')

    def test_zero_read_sends_no_commands(self):
        f=FakeLink(b'AB')
        self.assertEqual(install.read_ctl(f,0),b'')
        self.assertFalse(f.calls)

    def test_reject_overlong_canonical_frame(self):
        f=FakeLink(bytes(range(224)),mode='overlong')
        with self.assertRaisesRegex(TimeoutError,'payload'):
            install.read_ctl(f,224)
        self.assertEqual(len(f.calls),1)

    def test_reject_orphan_pack7_mask(self):
        f=FakeLink(bytes(range(224)),mode='orphan-mask')
        with self.assertRaisesRegex(TimeoutError,'payload'):
            install.read_ctl(f,224)
        self.assertEqual(len(f.calls),1)

    def test_reject_unused_mask_bits(self):
        f=FakeLink(b'AB',mode='unused-mask')
        with self.assertRaisesRegex(TimeoutError,'payload'):
            install.read_ctl(f,2)

    def test_reject_nonsevenbit_wire_data(self):
        f=FakeLink(b'AB',mode='nonsevenbit')
        with self.assertRaisesRegex(TimeoutError,'payload'):
            install.read_ctl(f,2)

    def test_reject_short_data(self):
        f=FakeLink(b'ABCD',mode='short')
        with self.assertRaises(TimeoutError):
            install.read_ctl(f,4)

    def test_reject_wrong_address(self):
        f=FakeLink(b'ABCD',mode='wrong-address')
        with self.assertRaises(TimeoutError):
            install.read_ctl(f,4)

    def test_reject_truncated_address(self):
        f=FakeLink(b'ABCD',mode='truncated-address')
        with self.assertRaises(TimeoutError):
            install.read_ctl(f,4)

    def test_reject_nonsevenbit_low_address_alias_second_frame(self):
        # off=224: high bit in low byte aliases valid bit 7 in the next byte.
        f=FakeLink(bytes(range(256))*3,mode='nonsevenbit-address-low',corrupt_at=1)
        with self.assertRaisesRegex(TimeoutError,'bad ED_CTL_READ reply'):
            install.read_ctl(f,768)
        self.assertEqual([o for o,_ in f.calls],[0,224])

    def test_reject_nonsevenbit_middle_address_alias_high_offset(self):
        # off=16576: high bit in middle byte aliases valid address bit 14.
        f=FakeLink(bytes(range(256))*65,mode='nonsevenbit-address-middle',corrupt_offset=16576)
        with self.assertRaisesRegex(TimeoutError,'bad ED_CTL_READ reply'):
            install.read_ctl(f,16640)
        self.assertEqual(f.calls[-1][0],16576)

    def test_reject_nonsevenbit_high_address_byte(self):
        f=FakeLink(b'ABCD',mode='nonsevenbit-address-high')
        with self.assertRaisesRegex(TimeoutError,'bad ED_CTL_READ reply'):
            install.read_ctl(f,4)
        self.assertEqual(len(f.calls),1)

    def test_check_ctl_rejects_aliased_address_even_with_matching_crc(self):
        ctl=b'ABC123XYZ'
        expected=ctl+b'\xFF'*(install.add_ctl.BLE_SIZE-len(ctl))
        f=FakeLink(expected,mode='nonsevenbit-address-low',corrupt_at=1)
        with self.assertRaises(install.InstallError) as cm:
            install.check_ctl(f,ctl,sys.stdout)
        self.assertEqual(cm.exception.code,'ctlcrc')
        self.assertEqual([o for o,_ in f.calls],[0,224])

    def test_malformed_second_frame_stops_before_third(self):
        f=FakeLink(bytes(range(256))*2,mode='overlong',corrupt_at=1)
        with self.assertRaisesRegex(TimeoutError,'payload'):
            install.read_ctl(f,512)
        self.assertEqual(len(f.calls),2)

    def test_install_gate_rejects_oversize_response_even_with_matching_crc(self):
        ctl=b'ABC123XYZ'
        expected=ctl+b'\xFF'*(install.add_ctl.BLE_SIZE-len(ctl))
        f=FakeLink(expected,mode='overlong')
        with self.assertRaises(install.InstallError) as cm:
            install.check_ctl(f,ctl,sys.stdout)
        self.assertEqual(cm.exception.code,'ctlcrc')
        self.assertEqual(len(f.calls),1)

    def test_install_gate_accepts_valid_complete_readback(self):
        ctl=b'ABC123XYZ'
        expected=ctl+b'\xFF'*(install.add_ctl.BLE_SIZE-len(ctl))
        f=FakeLink(expected)
        self.assertEqual(install.check_ctl(f,ctl,sys.stdout), __import__('zlib').crc32(expected)&0xffffffff)
        self.assertEqual(len(f.calls),(len(expected)+223)//224)

if __name__=='__main__':
    unittest.main(verbosity=2)
