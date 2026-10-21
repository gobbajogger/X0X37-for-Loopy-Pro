#!/usr/bin/env python3
"""Central receive words must never be printed as peripheral interval/MTU."""
import sys
import os
import types
import subprocess
import tempfile
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

# Exercise the actual passive C model and decode its wire words. Timestamp zero,
# retries, ring replacement, recovery and saturating counters are deliberate.
fixture = r'''
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include "ble_central_trace.h"
static void dump(const blec_trace_t *t) {
    uint32_t w[BLEC_TRACE_WORDS + 2u]; unsigned i;
    w[0] = 0xdeadbeefu; w[BLEC_TRACE_WORDS + 1u] = 0xc0ffeeu;
    blec_trace_words(t, w + 1);
    assert(w[0] == 0xdeadbeefu && w[BLEC_TRACE_WORDS + 1u] == 0xc0ffeeu);
    for (i = 1; i <= BLEC_TRACE_WORDS; i++) printf("%" PRIu32 " ", w[i]);
    puts("");
}
int main(void) {
    blec_trace_t t = {0};
    assert(sizeof t == 96u && BLEC_TRACE_WORDS == 26u);
    dump(&t);
    blec_trace_note(&t, 0, 4, 8, BLEC_TRACE_HCI_ERROR, 0x0c, 0x200d, 0xffff, 0, 1);
    dump(&t);
    blec_trace_note(&t, 10, 2, 0, BLEC_TRACE_START, 0, 0x0c03, 0xffff, 0, 0);
    blec_trace_note(&t, 20, 4, 8, BLEC_TRACE_CREATE, 0, 0x200d, 0xffff, 0, 0);
    blec_trace_note(&t, 30, 4, 8, BLEC_TRACE_CONNECT_FAIL, 0x3e, 0x200d, 0xffff, 0, 1);
    dump(&t);
    blec_trace_note(&t, 40, 5, 10, BLEC_TRACE_LINK, 0, 0, 0x50, 0, 0);
    blec_trace_note(&t, 50, 5, 13, BLEC_TRACE_ATT_ERROR, 3, 0x12, 0x73, 0x50, 1);
    dump(&t);
    blec_trace_note(&t, 60, 5, 13, BLEC_TRACE_TIMEOUT, BLEC_TRACE_WAIT_ATT, 0x12, 0x50, 1u << 8, 1);
    dump(&t);
    blec_trace_note(&t, 70, 3, 6, BLEC_TRACE_NO_PEER, 0, 0x200c, 0xffff, 0, 1);
    dump(&t);
    blec_trace_note(&t, 80, 6, 14, BLEC_TRACE_READY, 0, 0x12, 0x73, 0, 0);
    dump(&t);
    blec_trace_note(&t, 90, 6, 14, BLEC_TRACE_DISCONNECT, 8, 0x0406, 0x50, 0, 1);
    dump(&t);
    assert(t.first.ms == 0 && (t.first.context & 255u) == BLEC_TRACE_HCI_ERROR);
    assert(t.latest.ms == 90 && t.attempts == 1 && t.ready == 1 && t.failures == 6 && t.disconnects == 1);
    t.attempts = t.ready = t.failures = t.disconnects = 0xffffu;
    blec_trace_note(&t, 100, 4, 8, BLEC_TRACE_CREATE, 0, 0x200d, 0xffff, 0, 0);
    blec_trace_note(&t, 110, 6, 14, BLEC_TRACE_READY, 0, 0, 0x50, 0, 0);
    blec_trace_note(&t, 120, 6, 14, BLEC_TRACE_DISCONNECT, 0x13, 0x0406, 0x50, 0, 1);
    assert(t.attempts == 0xffffu && t.ready == 0xffffu && t.failures == 0xffffu && t.disconnects == 0xffffu);
    assert(t.first.ms == 0 && t.latest.ms == 120);
    return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='smc-trace-test-') as directory:
    folder = Path(directory)
    (folder / 'trace.c').write_text(fixture)
    subprocess.run([os.environ.get('CC', 'cc'), '-std=c99', '-Wall', '-Wextra', '-Werror',
                    '-I' + str(Path(__file__).resolve().parents[1] / 'firmware/src/ble'),
                    str(folder / 'trace.c'), '-o', str(folder / 'trace')], check=True)
    blocks = [[int(value) for value in line.split()] for line in
              subprocess.check_output([str(folder / 'trace')], text=True).splitlines()]


def with_trace(block, state):
    guard = [0] * len(b.GUARD)
    guard[b.GUARD.index('blocked')] = 41
    live = [0] * len(b.LIVE)
    live[b.LIVE.index('h_step')] = 0x80050000 | (14 if state == 6 else 13)
    live[b.LIVE.index('h_state')] = state
    live[b.LIVE.index('plat_state')] = 5 if state == 6 else 4
    live[b.LIVE.index('polls')] = 123456
    words = [0] * (len(b.CRASH) + len(b.BOOT)) + guard + block + live + [0x534D4301, 114 | 115 << 16, 0, 0, 0x13 << 24, 0]
    wire = [len(words)]
    for word in words:
        wire += [(word >> shift) & 127 for shift in (0, 7, 14, 21, 28)]
    parsed = b.decode_diag(wire)
    assert parsed['blocked'] == 41 and parsed['polls'] == 123456 and parsed['h_state'] == state
    assert parsed['smc_handles'] == 114 | 115 << 16
    # The legacy decoder peels the unchanged SMC1 suffix and takes the last LIVE
    # words. Its first GUARD words also remain exactly where they were.
    legacy_rest = words[len(b.CRASH) + len(b.BOOT):-6]
    assert dict(zip(b.LIVE, legacy_rest[-len(b.LIVE):]))['polls'] == 123456
    assert dict(zip(b.GUARD, legacy_rest[:-len(b.LIVE)]))['blocked'] == 41
    return parsed


assert len(blocks) == 8 and all(len(block) == 26 for block in blocks)
assert with_trace(blocks[0], 3)['smc_connection_trace']['first_failure'] is None
assert 'No connection failure recorded' in b.summary(with_trace(blocks[0], 3), None)['smc_failure']
expected = [(1, '0x0C (command disallowed)'), (2, '0x3E'),
            (3, 'ATT error 0x03 (write not permitted)'), (4, 'waiting for ATT response'),
            (5, 'No matching SMC advertisement'), (7, '0x08 (connection timeout)')]
for index, text in expected:
    parsed = with_trace(blocks[index], 3 if index in (5, 7) else 5)
    trace = parsed['smc_connection_trace']
    assert text in b.summary(parsed, None)['smc_failure']
    assert trace['first_failure']['ms'] == 0 and trace['first_failure']['opcode'] == 0x200d
    assert trace['first_failure']['kind_name'] == 'HCI_ERROR'
assert 'attribute 0x0073' in b.summary(with_trace(blocks[3], 5), None)['smc_failure']
recovered = with_trace(blocks[6], 6)
assert b.summary(recovered, None)['smc_failure'].startswith('Currently CONNECTED; retained earlier failure:')
latest = with_trace(blocks[7], 3)['smc_connection_trace']
assert [record['ms'] for record in latest['recent_events']] == [70, 80, 90]
assert latest['attempts'] == latest['ready_count'] == latest['disconnect_count'] == 1
assert latest['failure_count'] == 6
assert 'underlying radio/peer cause is not identified' in b.summary(with_trace(blocks[7], 3), None)['smc_failure']
bad = blocks[1].copy()
bad[-1] = 25
try:
    with_trace(bad, 4)
except ValueError:
    pass
else:
    raise AssertionError('malformed trace length accepted')
print('PASS: 96-byte passive model, sticky first/latest failures, retry retention, ring order, saturation, old/new DIAG alignment and precise failure text')

# A DISCOVERING deadline is also used while waiting for send credit. Do not
# attribute silence to the peer if no ATT request reached the transport.
def timeout(status, detail):
    return b.smc_trace_record([123, 5 | 5 << 8 | 13 << 16 | status << 24, 0x12 | 0x50 << 16, detail])


not_sent = timeout(4, 0)
assert not_sent['timeout_detail'] == {'hci_waiting': 0, 'att_waiting': 0, 'acl_credits': 0, 'command_retry': 0}
text = b.smc_failure_text(not_sent)
assert 'ATT request was not submitted (ACL credits 0)' in text and 'waiting for ATT response' not in text
assert 'opcode 0x0012' in text and 'DISCOVERING/SUBSCRIBE_NOTIFICATIONS' in text
assert 'ATT request was not submitted (ACL credits 5)' in b.smc_failure_text(timeout(4, 5 << 16))
assert 'waiting for ATT response' in b.smc_failure_text(timeout(4, 1 << 8))
for status, command, completed in ((3, 'cancellation', 'connection cancellation outcome'),
                                   (6, 'disconnection', 'disconnection completion'),
                                   (7, 'HCI', 'transport acceptance')):
    text = b.smc_failure_text(timeout(status, 1 << 24))
    assert f'awaiting {command} command submission' in text
    assert 'controller transport refused the command' in text
    assert 'waiting for ' + completed not in text
    assert 'waiting for ' + completed in b.smc_failure_text(timeout(status, 0))
print('PASS: ATT unsent/awaiting-response deadlines, ACL credits, and refused cancel/disconnect/HCI submission stay distinct')

# SMP trace metadata must not be mislabelled as HCI errors or raw key material.
for opcode in range(256):
    for length in (1, 2, 17, 56):
        safe = 0xA5 if length == 2 and opcode in (5, 0x0B) else 0
        r = b.smc_trace_record([123, 21 | 6 << 8 | 14 << 16 | safe << 24,
                                opcode | 0x50 << 16, length])
        assert r["kind_name"] == "SMP" and r["smp_length"] == length
        assert ("smp_security_request_auth_flags" in r) == (length == 2 and opcode == 0x0B)
        assert ("smp_pairing_failed_reason" in r) == (length == 2 and opcode == 5)
print("PASS: SMP trace decoder exposes only permitted metadata")
