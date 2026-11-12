#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Host regression for the actual firmware plat_slot conversion and slot admission.

Extracts the exact plat_slot C function from the source tree, compiles it
alongside the original slots.c validator with GCC/Clang, and checks real sampleio
serialized slot headers at all supported Q16 rates. The rest of the target
unity build and the physical device are NOT tested by this standalone probe.
"""
from __future__ import annotations
import argparse
import re
import struct
import subprocess
import sys
import tempfile
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
import sampleio  # noqa: E402


def function_source(text: str) -> str:
    pattern = r'^const uint8_t \*plat_slot\(int k, int z, uint32_t \*nsamples, uint32_t \*rate, char name\[9\]\)\n\{'
    m = re.search(pattern, text, re.M)
    if not m:
        raise RuntimeError("plat_slot function signature not found in source; NOT TESTED")
    brace_pos = text.index('{', m.start())
    depth = 0
    for i in range(brace_pos, len(text)):
        if text[i] == '{':
            depth += 1
        elif text[i] == '}':
            depth -= 1
            if depth == 0:
                return text[m.start():i + 1]
    raise RuntimeError("unterminated plat_slot function; NOT TESTED")


def run(cc: str, flags: list[str]):
    text = (ROOT / "firmware/src/app/plat_fm1.c").read_text()
    fn = function_source(text)
    with tempfile.TemporaryDirectory(prefix="x0x37-slot-rate-") as td:
        tmp = Path(td)
        cases = [22050, 44100, 65536, 88200, 96000, 176400]
        for rate in cases:
            header, data = sampleio.user_slot("RATE", [([0] * 64, 60, None, None)], rate=rate, slot_size=0x14000)
            if len(header) != 480 or zlib.crc32(data) != struct.unpack_from("<I", header, 20)[0]:
                raise RuntimeError(f"invalid serialized fixture for {rate} Hz; NOT TESTED")
            (tmp / f"slot-{rate}.bin").write_bytes(header.ljust(512, b'\0') + data)
        src = tmp / "slot_rate.c"
        src.write_text('''#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
/* Real validator from production source; target virtual flash represented by RAM. */
static uint32_t mem[3][0x1E000/4];
#define BOARD_SMK37 1
#define SMP_USER_XIP(k) ((const uint8_t *)mem[(k)])
#include "firmware/src/app/slots.c"
/* Exact production plat_slot function inserted below (not rewritten in the test). */
#line 189 "firmware/src/app/plat_fm1.c"
''' + fn + '''
int main(int argc, char **argv)
{
    int failures = 0;
    for (int a = 1; a < argc; a++) {
        memset(mem, 0, sizeof mem);
        FILE *f = fopen(argv[a], "rb");
        if (!f) return 90;
        if (!fread(mem[0], 1, 544, f)) { fclose(f); return 91; }
        fclose(f);
        smp_user_scan(0);
        if (usr_nz[0] != 1) { fprintf(stderr,"FAIL validator rejected sample %s\\n",argv[a]); return 92; }
        const smp_user_hdr_t *h = (const smp_user_hdr_t *)mem[0];
        uint32_t n=0, actual=0;
        char name[9];
        const uint8_t *adpcm = plat_slot(0, 0, &n, &actual, name);
        uint32_t expected = (uint32_t)(((uint64_t)h->zone[0].rate * 44100u + 32768u) >> 16);
        fprintf(stderr,"ADMITTED q16=%u expected=%u actual=%u n=%u data=%s\\n",
                h->zone[0].rate,expected,actual,n,adpcm ? "yes":"no");
        if (!adpcm || n != 64 || actual != expected) failures++;
    }
    /* Check every Q16 value the real firmware slot validator admits. */
    const smp_user_hdr_t *h = (const smp_user_hdr_t *)mem[0];
    smp_user_hdr_t *w = (smp_user_hdr_t *)mem[0];
    int sweep_bad = 0;
    for (uint32_t q = 1; q <= (4u << 16); q++) {
        w->zone[0].rate = q;
        smp_user_scan(0);
        if (usr_nz[0] != 1) return 94;
        uint32_t ns=0, hz=0;
        char nm[9];
        const uint8_t *adpcm = plat_slot(0, 0, &ns, &hz, nm);
        uint32_t expected = (uint32_t)(((uint64_t)h->zone[0].rate * 44100u + 32768u) >> 16);
        if (!adpcm || ns != 64 || hz != expected) {
            if (sweep_bad < 4) fprintf(stderr,"SWEEP FAIL q16=%u expected=%u observed=%u\\n",q,expected,hz);
            sweep_bad++;
        }
    }
    fprintf(stderr,"TOTAL supported-Q16 checks=%u failures=%d; file cases=%d\\n",
            4u << 16, sweep_bad, failures);
    return failures || sweep_bad ? 1 : 0;
}
''')
        binary = tmp / 'rate_test'
        cmd = [cc, '-std=c11', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
               '-Wno-unused-function', '-Wno-unused-parameter', '-fsanitize=address,undefined,bounds',
               '-fno-sanitize-recover=all', '-I', str(ROOT), str(src), '-o', str(binary)] + flags
        print('COMPILE', *cmd, flush=True)
        subprocess.run(cmd, check=True)
        args = [str(binary)] + [str(tmp / f"slot-{rate}.bin") for rate in cases]
        print('EXEC', *args, flush=True)
        subprocess.run(args, check=True)
        print('PASS', len(cases), 'valid source-serialized sample rates, source-extracted plat_slot, slot_scan', flush=True)


if __name__ == '__main__':
    p = argparse.ArgumentParser()
    p.add_argument('--cc', default='gcc')
    a = p.parse_args()
    run(a.cc, [])
