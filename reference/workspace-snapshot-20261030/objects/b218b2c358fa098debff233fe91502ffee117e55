#!/usr/bin/env python3
"""Central receive words must never be printed as peripheral interval/MTU."""
import sys
import os
import types
from pathlib import Path
sys.path.insert(0, str(Path(os.environ.get('DIAG_TOOL_ROOT', Path(__file__).resolve().parents[1])) / 'tools'))
# No device/API operations: only exercise the production text decoder.
sys.modules['mido'] = types.ModuleType('mido')
import ble_autotest as b
from collections import defaultdict
d = defaultdict(int)
d.update(h_step=0x8605000e, rx0=0x04030201, rx1=0x04070605)
s = b.summary(d, None)['link']
assert 'ACL 1, notifications 2, MIDI 3, rejected 4' in s
assert 'last receive rejection 6' in s
assert 'queue enqueued 5, dropped 6, dispatched 7, dispatch reason 4' in s
assert 'interval' not in s
d['h_step'] = 0
d['rx0'] = 24
d['rx1'] = 23 << 8
s = b.summary(d, None)['link']
assert 'interval 30 ms' in s and 'asked MTU 23' in s
print('PASS: central receive and unchanged peripheral diagnostic formats')

# Versioned extension decodes without shifting old live words or guard data.
for tail in ([], [0x534D4301, 113 | 114 << 16, 0x00080001, 0x00060004, 12 | 5 << 16 | 0x1B << 24, 113]):
    words = [0] * (len(b.CRASH) + len(b.BOOT) + len(b.GUARD)) + [0] * len(b.LIVE)
    words[-len(b.LIVE)+b.LIVE.index('h_step')] = 0x8605000e
    words += tail
    wire = [len(words)]
    for v in words: wire += [(v >> shift) & 127 for shift in (0,7,14,21,28)]
    b.ask = lambda cmd: wire
    parsed = b.diag()
    assert parsed['h_step'] == 0x8605000e
    if tail:
        text = b.summary(parsed, None)['smc_trace']
        assert 'value handle 113, CCCD handle 114' in text
        assert 'header 01 00 08 00 04 00 06 00, CID 0x0006' in text
        assert 'reason 5, bytes 12' in text
    else: assert 'smc_trace' not in b.summary(parsed, None)
print('PASS: old and extended DIAG decoding, handles and bounded rejected header')
