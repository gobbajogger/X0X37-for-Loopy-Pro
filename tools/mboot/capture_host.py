#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Inject-only capture/private-backup adapter. No live transport or installer.

CLI can report unsupported live capability or verify an existing local backup.
Every receipt is offline consistency evidence only, never device/write authority.
"""
import argparse
import ctypes as C
from dataclasses import dataclass
import hashlib
import json
import os
from pathlib import Path
import re
import stat
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent
SIZE, HEAD, CHUNK = 0x100000, 0x4000, 224
STATUS = 'PRIVATE_READONLY_BACKUP_NOT_INSTALLABLE'


class CaptureError(ValueError):
    """Sanitized errors deliberately exclude private payload, binding and digests."""


def integer(value, high=(1 << 64)-1):
    if type(value) is not int or not 0 <= value <= high:
        raise CaptureError('invalid integer field')
    return value


def binary32(value):
    if type(value) is not bytes or len(value) != 32 or not any(value):
        raise CaptureError('invalid binary binding field')
    return value


@dataclass(frozen=True)
class Binding:
    unit: bytes
    session: bytes
    challenge: bytes
    selection: bytes
    epoch: int
    model: int

    def validate(self):
        for field in ('unit', 'session', 'challenge', 'selection'):
            binary32(getattr(self, field))
        integer(self.epoch)
        if type(self.model) is not int or self.model not in (1, 2):
            raise CaptureError('unsupported model')
        return self


class CBinding(C.Structure):
    _fields_ = [(k, C.c_uint8 * 32) for k in ('unit', 'session', 'challenge', 'selection')] + [
        ('epoch', C.c_uint64), ('model', C.c_uint32)]


class CDigests(C.Structure):
    _fields_ = [('full', C.c_uint8 * 32), ('head', C.c_uint8 * 32)]


def cbinding(binding):
    if type(binding) is not Binding:
        raise CaptureError('invalid binding type')
    binding.validate()
    b = CBinding()
    for key in ('unit', 'session', 'challenge', 'selection'):
        getattr(b, key)[:] = getattr(binding, key)
    b.epoch, b.model = binding.epoch, binding.model
    return b


def cdigests(full, head):
    if any(type(x) is not bytes or len(x) != 32 for x in (full, head)):
        raise CaptureError('invalid digest field')
    d = CDigests(); d.full[:], d.head[:] = full, head
    return d


class Core:
    """Build exactly adjacent reviewed source, never load a caller-supplied library."""
    def __enter__(self):
        self.temp = tempfile.TemporaryDirectory(prefix='mboot-capture-core-')
        try:
            directory = Path(self.temp.name)
            shim = directory / 'layout.c'
            shim.write_text('#include "capture_core.h"\n'
                'size_t mb_host_context_size(void){return sizeof(struct mb_capture);}\n'
                'size_t mb_host_binding_size(void){return sizeof(struct mb_capture_binding);}\n')
            result = subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror',
                '-shared', '-fPIC', '-I'+str(HERE), str(HERE/'capture_core.c'), str(shim),
                '-o', str(directory/'core.so')], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            if result.returncode:
                raise CaptureError('capture core build failed')
            self.lib = C.CDLL(str(directory/'core.so'))
            self.lib.mb_capture_invalidate.argtypes = [C.c_void_p]
            self.lib.mb_capture_invalidate.restype = None
            for name in ('mb_host_context_size', 'mb_host_binding_size'):
                getattr(self.lib, name).restype = C.c_size_t
            if self.lib.mb_host_binding_size() != C.sizeof(CBinding):
                raise CaptureError('capture core ABI mismatch')
            n = self.lib.mb_host_context_size()
            if not 0 < n <= 4096:
                raise CaptureError('capture core size mismatch')
            self.ctx = (C.c_uint64 * ((n+7)//8))()  # zeroed and suitably aligned
            bp, dp, vp, u64 = C.POINTER(CBinding), C.POINTER(CDigests), C.c_void_p, C.c_uint64
            signatures = {
                'start': [vp,bp,C.c_uint32,u64,u64],
                'chunk': [vp,bp,C.c_uint32,vp,C.c_uint32,u64],
                'finish': [vp,bp,dp,u64,dp],
                'revalidate': [vp,bp,vp,dp,u64],
                'evidence_current': [vp,bp,u64],
                'cancel': [vp,bp],
            }
            for name, args in signatures.items():
                fn = getattr(self.lib, 'mb_capture_'+name)
                fn.argtypes, fn.restype = args, C.c_int
            return self
        except BaseException:
            self.temp.cleanup()
            raise

    def call(self, name, *args):
        if getattr(self.lib, 'mb_capture_'+name)(self.ctx, *args):
            raise CaptureError('capture core refused '+name)

    def __exit__(self, *unused):
        self.temp.cleanup()


class UnsupportedTransport:
    supported = False
    # Intentionally no open/read/release or device imports.


def transport_reply(transport, method, count):
    try:
        reply = getattr(transport, method)()
    except Exception:
        raise CaptureError('transport '+method+' failed') from None
    if type(reply) is not tuple or len(reply) != count:
        raise CaptureError('invalid transport '+method+' response')
    return reply


def transport_chunks(transport):
    try:
        stream = iter(transport.chunks())
    except Exception:
        raise CaptureError('transport chunks failed') from None
    while True:
        try:
            message = next(stream)
        except StopIteration:
            return
        except Exception:
            raise CaptureError('transport chunks failed') from None
        yield message


def private_directory(parent, name):
    """Use an existing owner-only parent and a new no-overwrite child via dirfds."""
    if type(name) is not str or not re.fullmatch(r'[a-z0-9][a-z0-9-]{0,63}', name):
        raise CaptureError('invalid backup name')
    parent = Path(parent)
    if parent.absolute() != parent.resolve(strict=True):
        raise CaptureError('symlinked backup parent refused')
    pfd = os.open(parent, os.O_RDONLY | os.O_DIRECTORY | os.O_NOFOLLOW)
    fd = None
    created = False
    try:
        info = os.fstat(pfd)
        if info.st_uid != os.getuid() or stat.S_IMODE(info.st_mode) & 0o077:
            raise CaptureError('backup parent must be private and owned')
        os.mkdir(name, 0o700, dir_fd=pfd)  # EEXIST refuses files/directories/symlinks
        created = True
        fd = os.open(name, os.O_RDONLY | os.O_DIRECTORY | os.O_NOFOLLOW, dir_fd=pfd)
        os.fsync(pfd)
        return fd, pfd
    except BaseException:
        if fd is not None: os.close(fd)
        if created:
            try: os.rmdir(name, dir_fd=pfd)
            except OSError: pass
        os.close(pfd)
        raise


def create_private(fd, name):
    return os.open(name, os.O_WRONLY | os.O_CREAT | os.O_EXCL | os.O_NOFOLLOW,
                   0o600, dir_fd=fd)


def write_all(fd, data):
    view = memoryview(data)
    while view:
        n = os.write(fd, view)
        if n <= 0:
            raise CaptureError('private backup write failed')
        view = view[n:]


def publish(fd, source, dest, owned):
    os.link(source, dest, src_dir_fd=fd, dst_dir_fd=fd, follow_symlinks=False)
    owned.add(dest)
    os.unlink(source, dir_fd=fd)
    owned.remove(source)


def retain_capture(transport, core, parent, name):
    """Consume an explicitly injected read-only session; no production transport.

    Adapter contract: acquire() either returns a held lease or safely releases its
    provisional resources before raising. All other calls are owner-bound and
    fail closed. release(binding) must be idempotent and release only that owner; None means
    release this transport instance's provisional acquisition, never another owner.
    No interface authorizes device acquisition merely by setting supported=True.
    """
    try:
        supported = getattr(transport, 'supported', False)
    except Exception:
        raise CaptureError('transport capability failed') from None
    if supported is not True:
        raise CaptureError('live capture unsupported')
    binding = None
    fd = pfd = imagefd = receiptfd = None
    published = False
    owned = set()
    acquire_attempted = False
    core_started = False
    try:
        # Allocate destination before acquisition; existing destinations are never touched.
        fd, pfd = private_directory(parent, name)
        acquire_attempted = True
        binding, facts, now, deadline = transport_reply(transport, 'acquire', 4)
        b = cbinding(binding)
        if integer(facts, 0xffffffff) != 15:
            raise CaptureError('unqualified capture capabilities')
        core.call('start', C.byref(b), facts, integer(now), integer(deadline))
        core_started = True
        imagefd = create_private(fd, 'image.partial')
        owned.add('image.partial')
        for message in transport_chunks(transport):
            if type(message) is not tuple or len(message) != 4:
                raise CaptureError('invalid chunk envelope')
            item, offset, data, now = message
            ib = cbinding(item)
            if type(data) is not bytes or not 0 < len(data) <= CHUNK:
                raise CaptureError('invalid chunk payload')
            raw = C.create_string_buffer(data)
            core.call('chunk', C.byref(ib), integer(offset,0xffffffff), raw, len(data), integer(now))
            write_all(imagefd, data)
        item, full, head, now = transport_reply(transport, 'finish', 4)
        ib, target, output = cbinding(item), cdigests(full,head), CDigests()
        core.call('finish', C.byref(ib), C.byref(target), integer(now), C.byref(output))
        os.fchmod(imagefd, 0o400); os.fsync(imagefd); os.close(imagefd); imagefd = None
        # Verify bytes actually retained, not merely bytes handed to os.write.
        stored = os.open('image.partial', os.O_RDONLY | os.O_NOFOLLOW, dir_fd=fd)
        try:
            sf, sh, count = hashlib.sha256(), hashlib.sha256(), 0
            while True:
                block = os.read(stored, 4096)
                if not block: break
                sf.update(block)
                if count < HEAD: sh.update(block[:HEAD-count])
                count += len(block)
                if count > SIZE: raise CaptureError('retained backup overrun')
            if count != SIZE or sf.digest() != bytes(output.full) or sh.digest() != bytes(output.head):
                raise CaptureError('retained backup consistency mismatch')
        finally:
            os.close(stored)
        # Backup fsync can take time: revalidate after it, then check once more at publication.
        item, challenge, full, head, now = transport_reply(transport, 'revalidate', 5)
        ib, target = cbinding(item), cdigests(full,head)
        nonce = C.create_string_buffer(binary32(challenge))
        core.call('revalidate', C.byref(ib), nonce, C.byref(target), integer(now))
        receipt = {'schema':1, 'status':STATUS, 'size':SIZE, 'model':binding.model,
                   'sha256':bytes(output.full).hex(), 'head_sha256':bytes(output.head).hex(),
                   'lease_authority':False, 'device_identity_verified_by_this_adapter':False}
        receiptfd = create_private(fd, 'receipt.partial')
        owned.add('receipt.partial')
        write_all(receiptfd, (json.dumps(receipt, sort_keys=True)+'\n').encode())
        os.fchmod(receiptfd,0o400); os.fsync(receiptfd); os.close(receiptfd); receiptfd=None
        item, now = transport_reply(transport, 'current', 2)
        ib = cbinding(item)
        core.call('evidence_current', C.byref(ib), integer(now))
        publish(fd, 'image.partial', 'image.bin', owned)
        publish(fd, 'receipt.partial', 'receipt.json', owned)  # completion marker last
        os.fsync(fd)
        published = True
        return receipt
    except CaptureError:
        raise
    except Exception:
        raise CaptureError('capture or private backup failed') from None
    finally:
        try:
            for handle in (imagefd,receiptfd):
                if handle is not None:
                    os.close(handle)
            if fd is not None:
                if not published:
                    for file in owned:
                        try: os.unlink(file,dir_fd=fd)
                        except FileNotFoundError: pass
                os.close(fd)
            if pfd is not None:
                if not published:
                    try: os.rmdir(name,dir_fd=pfd)
                    except OSError: pass  # preserve unknown material; never recursive cleanup
                os.close(pfd)
        finally:
            if core_started:
                core.lib.mb_capture_invalidate(core.ctx)
            if acquire_attempted:
                try:
                    transport.release(binding)
                except Exception:
                    # A retained backup may exist, but never claim a clean lease release.
                    raise CaptureError('capture lease release unresolved; no write or retry authorized') from None


def strict_json(raw):
    def pairs(items):
        out = {}
        for key,value in items:
            if key in out: raise CaptureError('duplicate receipt field')
            out[key]=value
        return out
    try: return json.loads(raw, object_pairs_hook=pairs)
    except (ValueError,UnicodeError): raise CaptureError('invalid receipt') from None


def verify_backup(directory):
    """Check owner-only files and offline hashes. Never grants freshness/identity."""
    fd = os.open(directory,os.O_RDONLY|os.O_DIRECTORY|os.O_NOFOLLOW)
    try:
        info=os.fstat(fd)
        if info.st_uid!=os.getuid() or stat.S_IMODE(info.st_mode)&0o077:
            raise CaptureError('backup directory not private')
        def read(name,limit):
            f=os.open(name,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK,dir_fd=fd)
            try:
                st=os.fstat(f)
                if not stat.S_ISREG(st.st_mode) or st.st_nlink!=1 or st.st_uid!=os.getuid() or stat.S_IMODE(st.st_mode)&0o077 or st.st_size>limit:
                    raise CaptureError('invalid private backup file')
                with os.fdopen(f,'rb',closefd=False) as stream:return stream.read(limit+1)
            finally:os.close(f)
        receipt=strict_json(read('receipt.json',4096))
        fields={'schema','status','size','model','sha256','head_sha256','lease_authority','device_identity_verified_by_this_adapter'}
        if (type(receipt) is not dict or set(receipt)!=fields or type(receipt['schema']) is not int or receipt['schema']!=1
            or receipt['status']!=STATUS or type(receipt['size']) is not int or receipt['size']!=SIZE
            or type(receipt['model']) is not int or receipt['model'] not in (1,2)
            or receipt['lease_authority'] is not False or receipt['device_identity_verified_by_this_adapter'] is not False):
            raise CaptureError('unsupported backup receipt')
        raw=read('image.bin',SIZE)
        if (len(raw)!=SIZE or hashlib.sha256(raw).hexdigest()!=receipt['sha256']
                or hashlib.sha256(raw[:HEAD]).hexdigest()!=receipt['head_sha256']):
            raise CaptureError('backup consistency mismatch')
        return {'status':'OFFLINE_BACKUP_CONSISTENT_NOT_AUTHORIZATION','size':SIZE,'model':receipt['model']}
    finally:os.close(fd)


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--verify-backup',type=Path)
    args=ap.parse_args()
    try:
        result=verify_backup(args.verify_backup) if args.verify_backup else {'live_capture':'UNSUPPORTED','write_capability':False}
    except (CaptureError,OSError):
        ap.exit(2,'backup verification refused\n')
    print(json.dumps(result,sort_keys=True))


if __name__=='__main__':main()
