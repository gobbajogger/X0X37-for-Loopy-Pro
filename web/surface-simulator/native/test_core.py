#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Fresh native bridge tests; no device claims."""
import hashlib, pathlib, sys, unittest
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parent))
from server import Core,HERE
class Tests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):cls.c=Core(HERE/'build/core.so')
    def test_01_frame_and_transport(self):
        c=self.c;self.assertEqual(c.state()['playing'],0)
        p=c.png();self.assertTrue(p.startswith(b'\x89PNG'));self.assertGreater(len(p),1000)
        c.command({'op':'input','args':[0,10,1]});c.command({'op':'input','args':[0,10,0]})
        self.assertEqual(c.state()['playing'],1)
        c.command({'op':'advance','args':[200]})
        self.assertNotEqual(hashlib.sha256(p).digest(),hashlib.sha256(c.png()).digest())
        c.command({'op':'input','args':[0,10,1]});c.command({'op':'input','args':[0,10,0]})
        self.assertEqual(c.state()['playing'],0)
    def test_02_release_and_invalid(self):
        c=self.c;c.command({'op':'input','args':[1,5,100]});self.assertTrue(c.state()['heldKeys']&(1<<5))
        c.command({'op':'input','args':[5,0,0]});self.assertEqual(c.state()['heldKeys'],0)
        for data in [{'op':'input','args':[1,37,100]},{'op':'input','args':[2,0,100000]},{'op':'advance','args':[-1]},{'op':'advance','args':[1001]},{'op':'notes','args':[60,24,0,0,-1,0]},{'op':'pad','args':[8,2,0,0,0,16]}, {'op':'input','args':[True,0,1]}]:
            with self.assertRaises(ValueError):c.command(data)
    def test_03_actual_pure_header_calls(self):
        c=self.c
        self.assertEqual(c.command({'op':'notes','args':[60,0,0,0,0,0]})['notes'],[60,64,67])
        self.assertEqual(c.command({'op':'notes','args':[61,0,0,1,-1,0]})['notes'],[60])
        self.assertEqual(c.command({'op':'notes','args':[61,0,0,2,-1,0]})['notes'],[])
        self.assertEqual(c.command({'op':'pad','args':[8,2,0,0,0,0]})['value'],8)
        self.assertEqual(c.command({'op':'pad','args':[8,2,0,0,0,8]})['value'],0)
        self.assertEqual(c.command({'op':'relative','args':[65]})['value'],-1)
        self.assertEqual(c.command({'op':'relative','args':[1]})['value'],1)
    def test_04_actual_smc_queue_and_pickup(self):
        c=self.c
        c.command({'op':'smc-link','args':[1,0]})
        self.assertEqual(c.state()['smc']['up'],1)
        c.command({'op':'smc-midi','args':[0x90,0x5e,127]})
        self.assertEqual(c.state()['playing'],1)
        c.command({'op':'smc-midi','args':[0x90,0x5d,127]})
        self.assertEqual(c.state()['playing'],0)
        before=c.state()['smc']['values'][0]
        c.command({'op':'smc-midi','args':[0xb0,16,1]})
        target=c.state()['smc']['values'][0]
        self.assertGreaterEqual(target,before)
        c.command({'op':'smc-midi','args':[0xe0,0,target]})
        self.assertTrue(c.state()['smc']['picked']&1)
        c.command({'op':'smc-midi','args':[0xe0,0,0]})
        self.assertEqual(c.state()['smc']['values'][0],0)
        self.assertEqual(c.state()['smc']['rxDiag']>>24,0)
        c.command({'op':'smc-midi','args':[0x90,8,127]})
        self.assertEqual(c.state()['smc']['solo'],1)
        c.command({'op':'input','args':[1,5,100]})
        c.command({'op':'input','args':[5,0,0]})
        self.assertEqual(c.state()['smc']['solo'],0)
        self.assertEqual(c.state()['smc']['up'],1)
        self.assertEqual(c.state()['heldKeys'],0)
        # Connection/preset still accept a new real DAW hold after releaseAll.
        c.command({'op':'smc-midi','args':[0x90,8,127]})
        self.assertEqual(c.state()['smc']['solo'],1)
        c.command({'op':'smc-link','args':[0,0]})
        self.assertEqual(c.state()['smc']['solo'],0)
        self.assertEqual(c.state()['smc']['picked'],0)
        c.command({'op':'smc-midi','args':[0x90,0x5e,127]})
        self.assertEqual(c.state()['playing'],0)
        self.assertEqual(c.state()['smc']['rxDiag']>>24,2)
        c.command({'op':'smc-link','args':[1,1]})
        c.command({'op':'smc-mmc','args':[33]})
        self.assertEqual(c.state()['playing'],1)
        c.command({'op':'smc-mmc','args':[34]})
        self.assertEqual(c.state()['playing'],0)
if __name__=='__main__':unittest.main(verbosity=2)
