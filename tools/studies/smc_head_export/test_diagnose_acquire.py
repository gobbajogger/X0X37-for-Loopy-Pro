# SPDX-License-Identifier: GPL-3.0-only
import contextlib
import io
import unittest
from unittest.mock import patch
import diagnose_acquire as d


def frame(w):
    return [len(w)] + [(x >> (7*j)) & 127 for x in w for j in range(5)]


class Test(unittest.TestCase):
    def test_exact_decoded_fields_and_unchanged_gate(self):
        for n in (71,77):
            w=[0]*n;w[48]=1;w[51]=1
            if n==77:w[-6]=0x534D4301
            self.assertIn('boot_on=0 up=0 dead=0 want=0 host_on=0 state=1',d.describe(frame(w)))
            d.acquire.install.cold_state(frame(w))
            for at,value,label in ((48,257,'boot_on=1'),(48,65537,'up=1'),(48,16777217,'dead=1'),(49,1,'want=1'),(49,256,'host_on=1'),(51,2,'state=2')):
                bad=w.copy();bad[at]=value
                self.assertIn(label,d.describe(frame(bad)))
                with self.assertRaises(d.acquire.bundle.BundleError):d.acquire.install.cold_state(frame(bad))
        self.assertIn('malformed',d.describe([128]))
        self.assertIn('invalid framing',d.describe([71]))
        self.assertIn('unsupported',d.describe(frame([0]*70)))

    def test_instructions_before_device_access(self):
        with patch.object(d.acquire.fi,'ask_tty',return_value=False) as ask,patch.object(d.acquire,'main') as run:
            self.assertEqual(d.main(),2)
            run.assert_not_called()
            self.assertIn('wait for SAVED',ask.call_args.args[0])

    def test_no_traceback_and_restore(self):
        original=d.acquire.install.check_cold
        with patch.object(d.acquire.fi,'ask_tty',return_value=True),patch.object(d.acquire,'main',side_effect=d.acquire.bundle.BundleError('cold refused')),contextlib.redirect_stderr(io.StringIO()) as out:
            self.assertEqual(d.main(),2)
        self.assertIn('Head acquisition refused',out.getvalue())
        self.assertNotIn('Traceback',out.getvalue())
        self.assertIs(d.acquire.install.check_cold,original)

if __name__=='__main__':unittest.main()
