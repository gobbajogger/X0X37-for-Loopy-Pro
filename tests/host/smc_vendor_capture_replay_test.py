#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Exact owner Mac notification payloads -> new vendor-host app parser, no HCI/device.

Not a donor capture. No connection, vendor transport or physical-control claim.
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
assert hashlib.sha256(raw).hexdigest() == 'a92f72f54c6aba7004e2c3a29759fa38818bb72a976486fb272b38c61d9d245a'
packets = [bytes.fromhex(row['hex']) for row in json.loads(raw)]
assert len(packets) == 1271 and max(map(len, packets)) == 67
expected = []
for p in packets:
    # Independent corpus-specific oracle: this capture only has channel status
    # plus pairs of running-status data, no extra timestamps or system messages.
    assert p[0] & 0xc0 == 0x80 and p[1] & 0x80 and p[2] >> 4 in (9, 11, 14)
    assert len(p) >= 5 and (len(p) - 3) % 2 == 0 and all(b < 128 for b in p[3:])
    expected.extend(bytes((p[2], *p[i:i+2])).hex() for i in range(3, len(p), 2))
assert len(expected) == 2711
HARNESS = r'''
#include <assert.h>
#include <stdio.h>
#include "smc_host_ingress.h"
static int got(void *ctx,const uint8_t *p,uint8_t n) {
    (void)ctx;for(unsigned i=0;i<n;i++)printf("%02x",p[i]);puts("");return 1;
}
int main(void) {
    smc_host_ingress_t s={0};uint8_t b[128],h[2];smc_hi_link(&s,1);
    while(fread(h,1,2,stdin)==2) {
        unsigned n=h[0]|(unsigned)h[1]<<8;
        assert(n<=sizeof b&&fread(b,1,n,stdin)==n);
        assert(smc_hi_notification(&s,b,n,got,0)==SMC_HI_OK);
    }
    assert(feof(stdin)&&s.notifications==1271&&s.messages==2711&&!s.rejected&&!s.dropped);
    return 0;
}
'''
with tempfile.TemporaryDirectory() as scratch:
    source = Path(scratch) / 'replay.c'
    binary = Path(scratch) / 'replay'
    source.write_text(HARNESS)
    subprocess.run([os.environ.get('CC', 'cc'), '-std=c11', '-O1', '-g', '-Wall', '-Wextra',
                    '-Werror', '-fsanitize=address,undefined',
                    '-I' + str(ROOT / 'firmware/src/ble'), str(source), '-o', str(binary)], check=True)
    env = dict(os.environ, ASAN_OPTIONS='detect_leaks=0')  # LSan unavailable under executor ptrace
    payload = b''.join(struct.pack('<H', len(p)) + p for p in packets)
    run = subprocess.run([str(binary)], input=payload, capture_output=True, env=env, check=True)
    assert run.stdout.decode().splitlines() == expected
print('PASS: exact Mac capture, 1271 notifications -> 2711 matching messages; 0 rejects/drops')
print('Capture SHA-256:', hashlib.sha256(raw).hexdigest())
print('Vendor transport / physical device behavior: NOT TESTED')
