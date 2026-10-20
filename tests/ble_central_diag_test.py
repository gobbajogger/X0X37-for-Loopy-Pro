#!/usr/bin/env python3
"""Central receive words must never be printed as peripheral interval/MTU."""
import sys
import types
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
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
