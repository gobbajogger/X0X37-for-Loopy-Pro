#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Build dedicated read-only exporter source, never a main-firmware installer."""
import argparse
import json
from pathlib import Path
import sys
import zipfile
sys.path.insert(0,str(Path(__file__).resolve().parents[2]))
import smc_vendor_bundle as b
from acquire import source
from host import _persist

def build(kit,ota):
    b.need(b.sha(kit)=='c9ef5aa79d50189646703a046000c0b3e4e517c00f7df5ecaf4fc13f36a63907','approved baseline kit identity')
    import io
    with zipfile.ZipFile(io.BytesIO(kit)) as z:
        names=[n for n in z.namelist() if n.endswith('/firmware/x0x-smk37-elite-with-bt.fwsc')]
        b.need(len(names)==1,'baseline member identity');raw=z.read(names[0])
    b.need(b.sha(raw)==b.BASELINE_FWSC_SHA,'baseline payload identity')
    flash,_=b.unpack(raw)
    raw=b.pm.ufw(flash,ota,b.PRODUCT)
    source(raw,b.sha(raw),b.sha(ota))
    return raw,dict(schema=1,profile='elite-head-export-dev',model=b.MODEL,source_sha256=b.sha(raw),ota_sha256=b.sha(ota),ota_size=len(ota),scope='Read-only raw head export; only owned OTA staging/record writes. NOT a firmware installer.')
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--baseline-kit',required=True);p.add_argument('--ota',required=True);p.add_argument('--output',required=True);a=p.parse_args()
    raw,m=build(Path(a.baseline_kit).read_bytes(),Path(a.ota).read_bytes())
    out=Path(a.output);_persist(out,raw);_persist(Path(str(out)+'.json'),(json.dumps(m,indent=2,sort_keys=True)+'\n').encode())
    print(json.dumps(m,sort_keys=True))
