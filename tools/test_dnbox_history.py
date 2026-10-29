#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
import hashlib
import tempfile
import unittest
from pathlib import Path
from dnbox_history import safe_local_name, verified_package, render_history, write_history


class HistoryTests(unittest.TestCase):
    def test_empty_site(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp)
            page = render_history(out, {'versions': []}, 'body{color:red}')
            self.assertIn('No verified firmware versions', page)
            self.assertIn('<style>body{color:red}</style>', page)
            self.assertNotIn('0.0.1', page)

    def test_exact_bytes_only(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp)
            (out / 'firmware').mkdir()
            data = b'only mock bytes, not a firmware package'
            fw = out / 'firmware/test.fwsc'
            fw.write_bytes(data)
            digest = hashlib.sha256(data).hexdigest()
            catalog = {'versions': [{'version': '0.0.7', 'label': '<script>alert(1)</script>',
              'models': {'pro': {'pkg': '../firmware/test.fwsc','size':len(data),'sha256':digest},
                         'elite': {'pkg': '../firmware/missing.fwsc','size':1,'sha256':digest}}}]}
            page = render_history(out, catalog)
            self.assertIn('Version 0.0.7', page)
            self.assertIn('href="../firmware/test.fwsc"',page)
            self.assertEqual(page.count('Package bytes verified'),1)
            self.assertIn('checksum mismatch', page)
            self.assertIn('&lt;script&gt;',page)
            self.assertNotIn('<script>alert(1)</script>',page)
            fw.write_bytes(data+b'X')
            self.assertNotIn('href="../firmware/test.fwsc"',render_history(out,catalog))
            self.assertFalse(verified_package(fw,digest,len(data)))

    def test_path_size_guard_and_file_write(self):
        self.assertIsNone(safe_local_name({'pkg':'../firmware/../test.fwsc'}))
        self.assertIsNone(safe_local_name({'pkg':'https://example.com/test.fwsc'}))
        self.assertIsNone(safe_local_name({'pkg':'../firmware/.hidden'}))
        self.assertEqual(safe_local_name({'pkg':'../firmware/good.fwsc'}),'good.fwsc')
        with tempfile.TemporaryDirectory() as tmp:
            out=Path(tmp); css=out/'style.css'; css.write_text('a{color:lime}')
            write_history(out, {'versions':[]},css)
            self.assertTrue((out/'history/index.html').exists())
            self.assertIn('a{color:lime}',(out/'history/index.html').read_text())


if __name__ == '__main__':
    unittest.main()