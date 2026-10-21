# SPDX-License-Identifier: GPL-3.0-only
import json
from pathlib import Path
import struct
import tempfile
import unittest
from unittest.mock import patch
import diagnose_resume as d
import host
class Test(unittest.TestCase):
    def test_diagnostics_do_not_change_admission(self):
        h=bytearray(b'\xff'*0x4000)
        for pos,name,off,data in ((32,'uboot.boot',0xA0,b'example-spl'),(64,'isd_config.ini',0x200,d.bundle.pm.key_blob(0x1234)+b'config')):
            e=bytearray(d.bundle.pm.entry(d.bundle.pm.crc16(data),off,len(data),0,0,0,name));d.bundle.pm.enc(e,0,32);h[pos:pos+32]=e;h[off:off+len(data)]=data
        # Deliberately wrong payload dCRC while preserving header/key CRC.
        h[0xA0]^=1;lines=[]
        with patch.object(d.bundle,'head_key',side_effect=ValueError('head data CRC')) as original:
            with self.assertRaisesRegex(ValueError,'head data CRC'):d.diagnose(bytes(h),lines.append)
            original.assert_called_once_with(bytes(h))
        rows=list(map(json.loads,lines));self.assertEqual(len(rows),2)
        self.assertNotEqual(rows[0]['expected_crc16'],rows[0]['actual_crc16']);self.assertTrue(rows[1]['key_blob_crc_valid'])
        self.assertEqual(set(rows[0]),{'entry','offset','size','expected_crc16','actual_crc16'})
        self.assertEqual(set(rows[1]),set(rows[0])|{'key_blob_crc_valid'})
    def test_no_new_stage(self):
        with self.assertRaisesRegex(ValueError,'existing exporter'):d.loader_only(None,None)
if __name__=='__main__':unittest.main()
