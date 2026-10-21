#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Metadata-only diagnosis on the SAME pending exporter; no new staging.
Original head_key remains the admission decision. No raw bytes, key or head hash
are printed. On validation failure no head persistence or FINISH occurs.
"""
import argparse
import json
from pathlib import Path
import struct
import sys
import diagnostic_acquire_core as acquire
from diagnostic_acquire_core import bundle, fi


def diagnose(head, emit=print):
    """Only bounded entry CRC metadata, never dump failed entry bytes."""
    bundle.need(len(head)==0x4000,'exact head extent required')
    for pos,name in ((32,'uboot.boot'),(64,'isd_config.ini')):
        e=bundle._entry(head[pos:pos+32])
        bundle.need(e[16:32].rstrip(b'\0\xff')==name.encode(),'unexpected head entry')
        off,size=struct.unpack_from('<II',e,4)
        bundle.need(0xA0<=off<=0x4000 and 0<size<=0x4000-off,'head entry outside head')
        row=dict(entry=name,offset=off,size=size,expected_crc16=struct.unpack_from('<H',e,2)[0],actual_crc16=bundle.pm.crc16(head[off:off+size]))
        if name=='isd_config.ini':
            row['key_blob_crc_valid']=size>=34 and bundle.pm.crc16(head[off:off+32])==struct.unpack_from('<H',head,off+32)[0]
        emit(json.dumps(row,sort_keys=True))
    bundle.head_key(head)  # unchanged authoritative rejection; no bypass
    raise ValueError('diagnostic-only stop after successful validation; no persistence or FINISH')


def loader_only(*args):
    raise ValueError('diagnostic resume requires the existing exporter loader; no staging or reboot performed')


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--dev-head-export-diagnostic-resume',action='store_true',required=True)
    for n in ('source','source-sha256','ota-sha256','output','receipt'):p.add_argument('--'+n,required=True)
    p.add_argument('--port');a=p.parse_args()
    receipt=Path(a.receipt)
    bundle.need(receipt.is_file(),'original saved exporter receipt is required; no new transaction')
    raw=Path(a.source).read_bytes();acquire.source(raw,a.source_sha256,a.ota_sha256)
    expected=dict(schema=1,profile='elite-head-export-dev',source_sha256=a.source_sha256,ota_sha256=a.ota_sha256,output=str(Path(a.output).resolve()))
    bundle.need(json.loads(receipt.read_text())==expected,'original receipt/source/output identity mismatch')
    if fi.foreground_guard() or fi.hardware_lock():return 10
    def ack():
        return fi.ask_tty('Resume ONLY the existing pending head exporter, without power cycling or staging another loader? Disconnect amplified outputs/headphones. Print entry CRC metadata only; unchanged validation still applies. [y/N] ')
    try:
        result=acquire.run(fi.MidoBackend(),raw,a.source_sha256,a.ota_sha256,a.output,a.receipt,
            acknowledge=ack,cold_check=loader_only,port=a.port,protected_paths=[a.source],validate_head=diagnose)
    except ValueError as e:
        print('Diagnostic stopped: '+str(e)+'. No fallback, new staging or record cleanup is attempted after this error.',file=sys.stderr)
        return 2
    # Never print capture result: it contains a private head hash.
    print('Head admission passed; local capture '+('saved and cleanup confirmed' if result['cleanup']=='confirmed' else 'saved; cleanup uncertain'))
    return 0 if result['cleanup']=='confirmed' else 4
if __name__=='__main__':sys.exit(main())
