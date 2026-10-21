#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Explicit DEV first-acquisition only. Existing fm1_install transport and lock."""
import argparse
import json
from pathlib import Path
import secrets
import struct
import sys
import time
sys.path.insert(0,str(Path(__file__).resolve().parents[2]))
import fm1_install as fi
import smc_vendor_bundle as bundle
import smc_vendor_install as install
from host import capture, _persist


def exchange(link, request):
    raw=bytes([0,0x59,0x40,len(request),0,0])+request+bytes([(~sum(request))&255])
    if not link.send(bytes([0xF0])+fi.pack7(raw)+bytes([0xF7])):raise ValueError('head request send failed')
    end=time.monotonic()+5
    while time.monotonic()<end:
        pkt=link.read(.2)
        if pkt is None:continue
        if pkt[0]!=0xF0 or pkt[-1]!=0xF7:continue
        r=fi.unpack7(pkt[1:-1])
        if len(r)<7 or r[:3]!=b'\0\x59\x40':continue
        n=int.from_bytes(r[3:5],'little')
        if r[5] or len(r)!=n+7 or fi.pack7(r)!=pkt[1:-1] or r[-1]!=(~sum(r[6:-1]))&255:
            raise ValueError('malformed head response')
        return r[6:-1]
    raise TimeoutError('head exporter capability/response timeout; no writer fallback')


def source(raw, expected_sha, ota_sha):
    bundle.need(bundle.sha(raw)==expected_sha,'head-export source identity changed')
    _,ota=bundle.unpack(raw)
    bundle.need(bundle.sha(ota)==ota_sha and b'X0X-HEAD-EXPORT\0' in ota[0x48:0x88], 'dedicated exporter identity/marker required')
    bundle.need(544<=len(ota)<0x4000,'exporter must fit below record sector')
    # ota_stage replaces the first32B header; body is unchanged. Check all
    # staged sector-end record positions that intersect the exact body.
    for rel in (0xF00,0x1F00,0x2F00,0x3F00):
        if rel+80<=len(ota):
            r=ota[rel:rel+80]
            bundle.need(not(r[2:8]==b'\x0d\x5a\x01\x5aAT' and bundle.pm.crc16(r[2:])==int.from_bytes(r[:2],'little')), 'accidental staged update record')
    product,image=bundle.add_ctl.logical_of(raw)
    return bytes(image)


def run(backend, raw, expected_sha, ota_sha, output, receipt, *, acknowledge, cold_check, port=None, protected_paths=(), validate_head=None, validation_provenance=None):
    image=source(raw,expected_sha,ota_sha)
    output=Path(output);receipt=Path(receipt)
    paths=[output,Path(str(output)+'.json'),receipt,*map(Path,protected_paths)]
    if validation_provenance is not None:
        paths.append(Path(str(output)+'.validation.json'))
    for i,p in enumerate(paths):
        for q in paths[:i]:
            bundle.need(p.resolve()!=q.resolve() and not(p.exists() and q.exists() and p.samefile(q)), 'head-export paths must not alias')
    identity=dict(schema=1,profile='elite-head-export-dev',source_sha256=expected_sha,ota_sha256=ota_sha,output=str(output.resolve()))
    resume=receipt.exists()
    if resume:bundle.need(json.loads(receipt.read_text())==identity,'resume source/output identity mismatch')
    class Elite(fi.Updater):
        def find(self,want=None,only_port=True):
            d=super().find(want,only_port)
            if d and fi.model_of(d.id.text)!=bundle.MODEL:
                d.link.close();raise ValueError('exact SMK-37 Elite required')
            return d
    up=Elite(backend,port);up.read_policy=bundle.response_bytes
    dev=up.find()
    if not dev:raise fi.not_found(up)
    up.bind(dev)
    try:
        bundle.need(not dev.id.loader or resume,'unknown loader requires original exact source receipt')
        if not dev.id.loader:cold_check(dev.link,fi)
        # Caller must present local action-time audio/physical cold instructions.
        if not acknowledge():raise ValueError('head acquisition cancelled before staging')
        if not resume:_persist(receipt,(json.dumps(identity,sort_keys=True,indent=2)+'\n').encode())
        if not dev.id.loader:dev=up.check(dev,image,lambda *a:None)
        nonce=secrets.randbits(32) or 1
        hello=exchange(dev.link,struct.pack('<4I',0,nonce,0,0))
        bundle.need(hello==struct.pack('<7I',0x31455848,bundle.MODEL_ID,nonce,0,0,0,5),'not the dedicated head exporter; second UPGRADE refused')
        bundle.need(dev.link.send(fi.UPGRADE),'exporter admission request failed')
        _,done,lost=up.serve(dev.link,image,fi.FINISH_CHECK,fi.DELAY['idle_write'],lambda *a:None)
        bundle.need(done and not lost,'exporter source/owned record admission incomplete')
        return capture(lambda r:exchange(dev.link,r),output,validate_head=validate_head or bundle.head_key,
                       protected_paths=[receipt,*protected_paths],nonce=nonce,
                       validation_provenance=validation_provenance)
    finally:dev.link.close()


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--dev-head-export',action='store_true',required=True)
    for n in ('source','source-sha256','ota-sha256','output','receipt'):ap.add_argument('--'+n,required=True)
    ap.add_argument('--port');a=ap.parse_args()
    raw=Path(a.source).read_bytes();source(raw,a.source_sha256,a.ota_sha256)
    if fi.foreground_guard() or fi.hardware_lock():return 10
    if not hasattr(install,'check_cold'):raise ValueError('requires reviewed strict cold DIAG adapter')
    def ack():
        return fi.ask_tty('DEV head export only. Save Bluetooth OFF, then physically power-cycle before this step. Disconnect headphones and amplified outputs; onboard audio may make a transient. Keep only this Elite connected. Continue now? [y/N] ')
    result=run(fi.MidoBackend(),raw,a.source_sha256,a.ota_sha256,a.output,a.receipt,
               acknowledge=ack,cold_check=install.check_cold,port=a.port,protected_paths=[a.source])
    print(json.dumps(result,sort_keys=True))
    return 0 if result['cleanup']=='confirmed' else 4
if __name__=='__main__':sys.exit(main())
