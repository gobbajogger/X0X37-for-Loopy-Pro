# SPDX-License-Identifier: GPL-3.0-only
"""Bounded local head capture; transport is injected, no device discovery/write.
exchange takes a 16-byte request and returns a decoded response. Integration
must first authenticate the exact staged exporter against immutable source.
No key is printed or returned. Output is private to the local unit owner.
"""
from pathlib import Path
import hashlib
import json
import os
import secrets
import struct
import tempfile
import zlib

MAGIC,MODEL,SIZE,CHUNK=0x31455848,0x894C0FA4,16384,224
INFO,READ,FINISH,DONE=1,2,3,4


class ExportError(ValueError):
    pass


def need(ok,msg):
    if not ok:raise ExportError(msg)


def response(exchange,cmd,nonce,off,n):
    raw=exchange(struct.pack('<4I',cmd,nonce,off,n))
    need(type(raw) is bytes and len(raw)>=28,'short export reply')
    magic,model,got_nonce,crc,got_off,size,status=struct.unpack_from('<7I',raw)
    need(magic==MAGIC and model==MODEL and nonce==got_nonce,'foreign model/session reply')
    need(len(raw)==28+size and size<=CHUNK,'reply extent')
    need(got_off==(SIZE if cmd==INFO else off),'reply offset')
    need(size==(n if cmd==READ else 0) and status==(DONE if cmd==FINISH else 0),'reply state/length')
    return crc,raw[28:]


def _persist(path,data):
    if path.exists():
        need(path.read_bytes()==data,'existing capture differs; never overwrite another unit')
        with path.open('rb') as f:os.fsync(f.fileno())
        d=os.open(path.parent,os.O_RDONLY)
        try:os.fsync(d)
        finally:os.close(d)
        return
    fd,tmp=tempfile.mkstemp(prefix=path.name+'.partial-',dir=path.parent)
    try:
        with os.fdopen(fd,'wb') as f:
            f.write(data);f.flush();os.fsync(f.fileno())
        # Link publishes without overwriting an existing file/symlink. Tempfile
        # mode0600 is retained. Both entries are on the same filesystem.
        os.link(tmp,path)
        d=os.open(path.parent,os.O_RDONLY)
        try:os.fsync(d)
        finally:os.close(d)
    finally:
        os.unlink(tmp)
    need(path.read_bytes()==data,'local capture readback failed')


def capture(exchange,output,*,validate_head,protected_paths=(),nonce=None,validation_provenance=None):
    """Capture+CRC+validate+persist before FINISH. A lost cleanup reply is explicit.

    validate_head is the reviewed head_key integrity parser (return ignored).
    protected_paths must include input package, staged-loader record and backups.
    No partial data is published. Re-entry rereads the entire head with a new
    nonce; an already identical complete file is retained, never replaced.
    """
    path=Path(output);receipt=path.with_name(path.name+'.json')
    outputs=[path,receipt]
    if validation_provenance is not None:
        need(type(validation_provenance) is dict,'validation provenance must be an object')
        validation_provenance=json.loads(json.dumps(validation_provenance,sort_keys=True))
        # Separate sidecar: neither original stage receipt nor established head
        # metadata is rewritten to pretend it came from the revised host code.
        outputs.append(path.with_name(path.name+'.validation.json'))
    paths=[*outputs,*map(Path,protected_paths)]
    for i,p in enumerate(outputs):
        for q in paths[i+1:]:
            need(p.resolve()!=q.resolve() and not(p.exists() and q.exists() and os.path.samefile(p,q)),'capture path aliases protected input')
    nonce=nonce if nonce is not None else secrets.randbelow(2**32-1)+1
    need(type(nonce) is int and 0<nonce<2**32,'nonce range')
    crc,_=response(exchange,INFO,nonce,0,0)
    data=bytearray()
    while len(data)<SIZE:
        off=len(data);n=min(CHUNK,SIZE-off)
        got,chunk=response(exchange,READ,nonce,off,n)
        need(got==crc,'head identity changed mid-transfer')
        data.extend(chunk)
    need(zlib.crc32(data)==crc,'whole-head CRC mismatch')
    validate_head(bytes(data))
    sha=hashlib.sha256(data).hexdigest()
    metadata=dict(schema=1,model='SMK-37 Elite',size=SIZE,crc32=crc,sha256=sha,scope='raw head only; no main/sample writes')
    _persist(path,bytes(data))
    _persist(receipt,(json.dumps(metadata,sort_keys=True,indent=2)+'\n').encode())
    if validation_provenance is not None:
        provenance=dict(schema=1,head_sha256=sha,validation=validation_provenance)
        _persist(outputs[2],(json.dumps(provenance,sort_keys=True,indent=2)+'\n').encode())
    last=None
    for _ in range(3):
        try:
            got,_=response(exchange,FINISH,nonce,crc,0)
            need(got==crc,'cleanup reply changed identity')
            return dict(metadata,cleanup='confirmed',saved=True)
        except (ExportError,TimeoutError,OSError) as e:
            last=e
    return dict(metadata,cleanup='uncertain',detail=str(last),saved=True)
