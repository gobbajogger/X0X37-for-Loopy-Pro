# SPDX-License-Identifier: GPL-3.0-only
from pathlib import Path
from types import SimpleNamespace as NS
from unittest.mock import patch
import json
import struct
import tempfile
import unittest
import acquire as a
class Link:
    def __init__(self,events):self.events=events
    def send(self,p):self.events.append('upgrade');return True
    def close(self):pass
class Test(unittest.TestCase):
    def test_order_and_unknown_loader(self):
        for mode in ('normal','resume','unknown','no-cap','cancel'):
            with self.subTest(mode=mode), tempfile.TemporaryDirectory() as td:
                events=[];p=Path(td);link=Link(events)
                dev=NS(link=link,id=NS(loader=mode in ('resume','unknown'),text=a.bundle.PRODUCT),name='Elite')
                class Up:
                    def __init__(self,*args):pass
                    def find(self,*args):return dev
                    def bind(self,d):events.append('bind')
                    def check(self,d,image,step):events.append('first-upgrade');return d
                    def serve(self,*args):events.append('record-admit');return 1,True,False
                ident=dict(schema=1,profile='elite-head-export-dev',source_sha256='source',ota_sha256='ota',output=str((p/'head').resolve()))
                if mode=='resume':(p/'receipt').write_text(json.dumps(ident))
                def hello(link,req):
                    events.append('hello');nonce=struct.unpack('<4I',req)[1]
                    if mode=='no-cap':raise TimeoutError()
                    return struct.pack('<7I',0x31455848,a.bundle.MODEL_ID,nonce,0,0,0,5)
                with patch.object(a,'source',return_value=b'image'),patch.object(a.fi,'Updater',Up),patch.object(a,'exchange',side_effect=hello),patch.object(a,'capture',side_effect=lambda *x,**k:events.append('capture')):
                    fn=lambda:a.run(None,b'raw','source','ota',p/'head',p/'receipt',acknowledge=lambda:mode!='cancel',cold_check=lambda *x:events.append('cold'))
                    if mode in ('unknown','no-cap','cancel'):
                        with self.assertRaises((ValueError,TimeoutError)):fn()
                        self.assertNotIn('upgrade',events);self.assertNotIn('capture',events)
                    else:
                        fn();self.assertLess(events.index('hello'),events.index('upgrade'));self.assertLess(events.index('record-admit'),events.index('capture'))
                        self.assertTrue((p/'receipt').exists())
                        if mode=='normal':self.assertLess(events.index('cold'),events.index('first-upgrade'))
if __name__=='__main__':unittest.main()
