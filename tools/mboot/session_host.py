#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Inject-only read-only packet transport. No MIDI/USB discovery or device I/O.

Use with capture_host.retain_capture and an explicitly injected exchange. The
production ED_CAPTURE endpoint reports zero capabilities. No write opcode exists.
"""
import hashlib
import os
import struct
import capture_host as h

VERSION, COMMAND = 1, 49
CAPS, OPEN, READ, FINISH, REVALIDATE, CURRENT, CLOSE = range(7)


def pack7(data):
    if type(data) is not bytes:
        raise h.CaptureError('invalid packet bytes')
    out = bytearray()
    for i in range(0, len(data), 7):
        group = data[i:i+7]
        out.append(sum((x >> 7) << j for j, x in enumerate(group)))
        out.extend(x & 127 for x in group)
    return bytes(out)


def unpack7(data):
    if type(data) is not bytes or not data or len(data) % 8 == 1 or any(x > 127 for x in data):
        raise h.CaptureError('invalid packet encoding')
    out = bytearray()
    for i in range(0, len(data), 8):
        mask, group = data[i], data[i+1:i+8]
        if mask >> len(group):
            raise h.CaptureError('noncanonical packet encoding')
        out.extend(x | (((mask >> j) & 1) << 7) for j, x in enumerate(group))
    return bytes(out)


class WireTransport:
    """exchange(full_sysex)->full_sysex must be explicit, serialized and bounded.

    expected_unit is an externally reviewed identity digest, never guessed from
    product/JEDEC/MAC/key. This layer checks binding consistency, not physical
    authenticity. A missing OPEN reply leaves ownership unresolved until separately
    verified expiry/reconnect; it is never retried or treated as clean release.
    """
    def __init__(self, exchange=None, *, expected_unit=None, model=None, selection=None):
        self.exchange, self.supported = exchange, False
        self.binding = None
        self.pending = False
        self.sequence = 0
        self.deadline = None
        self.finished = False
        if exchange is None:
            return
        h.binary32(expected_unit); h.binary32(selection)
        if type(model) is not int or model not in (1, 2):
            raise h.CaptureError('unsupported model')
        self.expected_unit, self.model, self.selection = expected_unit, model, selection
        caps = self._call(CAPS, b'', sequence=0)
        if len(caps) != 14:
            raise h.CaptureError('invalid capability response')
        facts, size, head, chunk = struct.unpack('<IIIH', caps)
        self.supported = (facts == 15 and (size, head, chunk) == (h.SIZE, h.HEAD, h.CHUNK))

    def _call(self, op, payload, *, sequence=None):
        seq = self.sequence if sequence is None else sequence
        if not 0 <= seq < 0xffffffff:
            raise h.CaptureError('session sequence exhausted')
        header = struct.pack('<BBI', VERSION, op, seq)
        frame = b'\xf0\x7d\x46\x4c' + bytes([COMMAND]) + pack7(header+payload) + b'\xf7'
        try:
            response = self.exchange(frame)
        except Exception:
            raise h.CaptureError('packet exchange failed') from None
        if type(response) is not bytes or len(response) > 445 or response[:5] != frame[:5] or response[-1:] != b'\xf7':
            raise h.CaptureError('invalid packet response')
        raw = unpack7(response[5:-1])
        if len(raw) < 7 or raw[:6] != header or raw[6] != 0:
            raise h.CaptureError('session request refused')
        if sequence is None:
            self.sequence += 1
        return raw[7:]

    def _envelope(self, payload, extra):
        if len(payload) != 148+extra:
            raise h.CaptureError('invalid session response length')
        b = h.Binding(payload[:32],payload[32:64],payload[64:96],payload[96:128],
                      struct.unpack_from('<Q',payload,128)[0],struct.unpack_from('<I',payload,136)[0]).validate()
        now = struct.unpack_from('<Q',payload,140)[0]
        if self.binding is not None and b != self.binding:
            raise h.CaptureError('session binding changed')
        return b, now, payload[148:]

    def acquire(self):
        if not self.supported or self.pending or self.binding is not None:
            raise h.CaptureError('capture acquisition unavailable')
        self.challenge = os.urandom(32)
        self.pending = True  # even a lost response may have acquired a target lease
        response = self._call(OPEN,self.challenge+self.selection,sequence=0)
        b, now, tail = self._envelope(response,12)
        self.binding = b  # retain owner token before checking expected identity
        facts, self.deadline = struct.unpack('<IQ',tail)
        if b.unit != self.expected_unit or b.model != self.model or b.challenge != self.challenge or b.selection != self.selection:
            raise h.CaptureError('capture identity or selection mismatch')
        self.sequence = 1
        self.finished = False
        self.full, self.head = hashlib.sha256(), hashlib.sha256()
        return b, facts, now, self.deadline

    def chunks(self):
        if self.binding is None or self.finished:
            raise h.CaptureError('capture stream unavailable')
        for off in range(0,h.SIZE,h.CHUNK):
            n = min(h.CHUNK,h.SIZE-off)
            response = self._call(READ,self.binding.session+struct.pack('<IH',off,n))
            b, now, tail = self._envelope(response,4+n)
            if struct.unpack_from('<I',tail)[0] != off:
                raise h.CaptureError('capture offset mismatch')
            data = tail[4:]
            self.full.update(data)
            if off < h.HEAD: self.head.update(data[:h.HEAD-off])
            yield b, off, data, now

    def finish(self):
        expected = self.full.digest()+self.head.digest()
        b, now, tail = self._envelope(self._call(FINISH,self.binding.session+expected),64)
        if tail != expected:
            raise h.CaptureError('target digest mismatch')
        self.finished = True
        return b,tail[:32],tail[32:],now

    def revalidate(self):
        fresh = os.urandom(32)
        expected = fresh+self.full.digest()+self.head.digest()
        b,now,tail = self._envelope(self._call(REVALIDATE,self.binding.session+expected),96)
        if tail != expected:
            raise h.CaptureError('revalidation mismatch')
        return b,fresh,tail[32:64],tail[64:],now

    def current(self):
        b,now,_ = self._envelope(self._call(CURRENT,self.binding.session),0)
        return b,now

    def release(self,binding):
        if not self.pending:
            return
        if self.binding is None:
            raise h.CaptureError('acquisition outcome unresolved')
        if binding is not None and binding != self.binding:
            raise h.CaptureError('foreign release refused')
        if self._call(CLOSE,self.binding.session):
            raise h.CaptureError('invalid release response')
        self.pending = False
        self.binding = None
