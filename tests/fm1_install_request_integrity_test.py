#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Actual request parser offline; synthetic packet bytes, no backend/device."""
import sys
import unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import fm1_install as I

def raw(fl=0,addr=0,length=8):
    u=bytearray([0,0x59,0x30,8,0,0,fl])+addr.to_bytes(4,'little')+length.to_bytes(3,'little')
    return u+bytes([~sum(u[6:])&255])

def frame(u):
    return b'\xf0'+I.pack7(u)+b'\xf7'

class RequestIntegrity(unittest.TestCase):
    def test_valid_fields(self):
        for args in [(0,0,8),(255,0x12345678,0x123456),(0,0xe0000000,8),(0,0xf0000000,0)]:
            with self.subTest(args=args): self.assertEqual(I.parse_request(frame(raw(*args))),args)
    def test_malformed(self):
        bad=[]
        for n in (0,7,9,0x108,0x10008):
            u=raw();u[3:6]=n.to_bytes(3,'little');bad.append((f'length {n}',frame(u)))
        for i,label in [(14,'checksum'),(7,'address')]:
            u=raw();u[i]^=1;bad.append((label,frame(u)))
        u=raw();u[2]=0x11;bad.append(('command',frame(u)))
        p=frame(raw());bad += [('start',p[1:]),('end',p[:-1]),('truncated',p[:-2]+b'\xf7'),('extra',p[:-1]+b'\0\xf7')]
        u=bytearray(p);u[2]|=128;bad.append(('status',u))
        u=bytearray(p);u[-2]|=64;bad.append(('padding',u))
        for label,pkt in bad:
            with self.subTest(label=label):
                self.assertIsNone(I.parse_request(pkt))
                link = FakeLink([pkt, frame(raw(addr=I.FINISH_WRITE, length=0))])
                result = I.Updater(None).serve(link, bytes(range(16)), I.FINISH_WRITE, 1, lambda *_: None)
                self.assertEqual(result, (0, True, False))
                self.assertEqual(len(link.sent), 1, 'only canonical finish reply, no firmware bytes')

    def test_canonical_serve(self):
        link = FakeLink([frame(raw()), frame(raw(addr=I.FINISH_WRITE, length=0))])
        result = I.Updater(None).serve(link, bytes(range(16)), I.FINISH_WRITE, 1, lambda *_: None)
        self.assertEqual(result, (1, True, False))
        self.assertEqual(len(link.sent), 2)


class FakeLink:
    lost = False
    def __init__(self, packets): self.packets, self.sent = list(packets), []
    def read(self, _timeout): return self.packets.pop(0) if self.packets else None
    def send(self, packet): self.sent.append(packet); return True
if __name__=='__main__': unittest.main(verbosity=2)
