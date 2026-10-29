#!/usr/bin/env python3
"""Replay preserved SMC notification payloads through simulated HCI, no device.

Exact payloads retain their bytes; HCI headers and link/handle are simulated.
The separate MTU-23 case resegments messages and is not exact-wire evidence.
Run from any directory with Python 3 and a host C compiler (CC).
"""
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
CAPTURE = ROOT / 'cloud-handoff/2026-10-20/smc_filter_fix/evidence/smc-raw-ble-retry.json'
raw = CAPTURE.read_bytes()
packets = [bytes.fromhex(row['hex']) for row in json.loads(raw)]
assert len(packets) == 1271


def messages(payload):
    # This recorded corpus has one explicit channel status followed by pairs of
    # running-status data bytes, with no further timestamps or system messages.
    # Assert that premise before using this independent, corpus-specific oracle.
    assert payload[0] & 0xc0 == 0x80 and payload[1] & 0x80
    assert payload[2] >> 4 in (9, 11, 14)
    assert len(payload) >= 5 and (len(payload) - 3) % 2 == 0
    assert all(value < 128 for value in payload[3:])
    return [bytes((payload[2], *payload[i:i + 2])) for i in range(3, len(payload), 2)]


decoded = [messages(payload) for payload in packets]
assert sum(map(len, decoded)) == 2711
HARNESS = r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "ble_midi_central.h"
static int send_h4(uint8_t t, const uint8_t *p, uint16_t n, void *x)
{ (void)t; (void)p; (void)n; (void)x; return 0; }
static void got(const uint8_t *p, uint8_t n, void *x)
{ (void)x; for (unsigned i=0;i<n;i++) printf("%02x",p[i]); puts(""); }
int main(void) {
    uint8_t addr[6]={0}, b[512], header[2]; blec_t c;
    blec_init(&c,send_h4,got,0,addr);
    c.state=BLEC_CONNECTED; c.conn=1;c.linked=1; c.midi_handle=113;
    unsigned rejected=0;
    while(fread(header,1,2,stdin)==2) {
        unsigned n=header[0] | (unsigned)header[1]<<8;
        assert(n<=sizeof b && fread(b,1,n,stdin)==n);
        unsigned before=c.rx_reject;
        blec_rx(&c,2,b,n,1);
        if(c.rx_reject!=before) { assert(c.rx_reason==BLEC_RX_SIZE); rejected++; }
    }
    assert(feof(stdin)); fprintf(stderr,"%u\n",rejected); return 0;
}
'''


def frames(payloads, fragmented):
    result = bytearray()
    for index, payload in enumerate(payloads):
        att = b'\x1b\x71\0' + payload
        body = struct.pack('<HH', len(att), 4) + att
        tail = index % 9 if fragmented and len(att) <= 64 else 0
        parts = [(1, body)] if not tail else [(1, body[:-tail]), (0x1001, body[-tail:])]
        for handle, part in parts:
            frame = struct.pack('<HH', handle, len(part)) + part
            result += struct.pack('<H', len(frame)) + frame
    return result


with tempfile.TemporaryDirectory() as scratch:
    source = Path(scratch) / 'replay.c'
    binary = Path(scratch) / 'replay'
    source.write_text(HARNESS)
    subprocess.run([os.environ.get('CC', 'cc'), '-std=c99', '-O1', '-g',
                    '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined',
                    '-I' + str(ROOT / 'firmware/src/ble'), str(source),
                    str(ROOT / 'firmware/src/ble/ble_midi_central.c'), '-o', str(binary)], check=True)

    def check(label, payloads, expected, rejects, fragmented=False):
        run = subprocess.run([str(binary)], input=frames(payloads, fragmented), capture_output=True, check=True)
        assert run.stdout.decode().splitlines() == [message.hex() for message in expected], label
        assert run.stderr.decode().strip() == str(rejects), run.stderr.decode()
        print(f'PASS {label}: {len(payloads)} payloads, {len(expected)} MIDI, {rejects} size rejections')

    supported = [message for payload, group in zip(packets, decoded) if len(payload) + 3 <= 64 for message in group]
    assert len(supported) == 2616
    check('exact recorded payloads / simulated HCI', packets, supported, 3)
    check('exact recorded payloads / simulated fragmented HCI tails 1-8', packets, supported, 3, True)
    # Explicitly preserve header/timestamp and status when starting each new
    # notification, then retain running status within each <=20-byte payload.
    mtu23 = [payload[:3] + payload[offset:offset + 16]
             for payload in packets for offset in range(3, len(payload), 16)]
    assert all(len(payload) + 3 <= 23 for payload in mtu23)
    check('resegmented MTU-23 / simulated fragmented HCI', mtu23,
          [message for group in decoded for message in group], 0, True)
print('Capture SHA-256:', hashlib.sha256(raw).hexdigest())
print('Physical device behavior: NOT TESTED')
