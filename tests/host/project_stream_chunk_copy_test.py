#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Differential test of the production st_put against its original byte loop.

Run in an authorized host-test environment with Python 3 and a C99 compiler:
    python3 tests/host/project_stream_chunk_copy_test.py
The mock flush captures complete object bytes and injects save failures. This
checks stream assembly, not the NOR driver or target memcpy implementation.
"""
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / 'firmware/src/app/project.c').read_text()
start = source.index('static int st_put(')
end = source.index('\n/* Check capacity', start)
actual = source[start:end]
plat = (ROOT / 'firmware/src/app/plat.h').read_text()
size = re.search(r'#define PLAT_STORE_MAX\s+(\d+)u', plat).group(1)
harness = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define PLAT_STORE_MAX @SIZE@u
#define NSTREAM 4
#define STREAM_CAP (NSTREAM * (PLAT_STORE_MAX - 4u) - 12u)
typedef struct { int measure, k, err; uint32_t fill, total; } st_out_t;
static uint8_t io_buf[PLAT_STORE_MAX];
static uint8_t objects[NSTREAM][PLAT_STORE_MAX];
static uint32_t lengths[NSTREAM];
static int fail_at, calls;
static int st_flush(st_out_t *o)
{
    assert(o->k < NSTREAM);
    memcpy(objects[o->k], io_buf, o->fill);
    lengths[o->k] = o->fill;
    int rc = o->k == fail_at ? -1 : 0;
    calls++;
    o->k++;
    o->fill = 4;
    io_buf[0] = 13; io_buf[1] = io_buf[2] = io_buf[3] = 0;
    return rc;
}
@ACTUAL@
static int reference(void *ctx, const uint8_t *p, uint32_t n)
{
    st_out_t *o = ctx;
    o->total += n;
    if (o->measure) return o->total > STREAM_CAP ? -1 : 0;
    while (n) {
        if (o->fill == PLAT_STORE_MAX) {
            if (o->k + 1 >= NSTREAM || st_flush(o)) {
                o->err = 1;
                return -1;
            }
        }
        io_buf[o->fill++] = *p++;
        n--;
    }
    return 0;
}
static uint8_t input[STREAM_CAP + 2], expected_buf[PLAT_STORE_MAX];
static uint8_t expected_objects[NSTREAM][PLAT_STORE_MAX];
static uint32_t expected_lengths[NSTREAM];
static void reset(void)
{
    memset(io_buf, 0xa5, sizeof io_buf);
    memset(objects, 0xcc, sizeof objects);
    memset(lengths, 0, sizeof lengths);
    calls = 0;
}
int main(void)
{
    const uint32_t fills[] = {4, 16, PLAT_STORE_MAX - 1, PLAT_STORE_MAX};
    const uint32_t sizes[] = {0, 1, 3, 128, PLAT_STORE_MAX - 16,
        PLAT_STORE_MAX - 4, PLAT_STORE_MAX, STREAM_CAP, STREAM_CAP + 1};
    const uint32_t chunks[] = {1, 3, 128, STREAM_CAP + 1};
    unsigned cases = 0;
    for (uint32_t i = 0; i < sizeof input; i++) input[i] = (uint8_t)(i * 37u + i / 251u);
    for (int measure = 0; measure <= 1; measure++)
    for (int k = 0; k < NSTREAM; k++)
    for (unsigned f = 0; f < sizeof fills / sizeof *fills; f++)
    for (unsigned s = 0; s < sizeof sizes / sizeof *sizes; s++)
    for (unsigned c = 0; c < sizeof chunks / sizeof *chunks; c++)
    for (fail_at = -1; fail_at < NSTREAM; fail_at++) {
        st_out_t result[2]; int rc[2], count[2];
        for (int variant = 0; variant < 2; variant++) {
            reset();
            st_out_t o = {measure, k, 0, fills[f], 0};
            uint32_t pos = 0;
            int r = 0;
            do {
                uint32_t n = sizes[s] - pos;
                if (n > chunks[c]) n = chunks[c];
                r = variant ? st_put(&o, input + pos, n) : reference(&o, input + pos, n);
                pos += n;
            } while (!r && pos < sizes[s]);
            result[variant] = o; rc[variant] = r; count[variant] = calls;
            if (!variant) {
                memcpy(expected_buf, io_buf, sizeof io_buf);
                memcpy(expected_objects, objects, sizeof objects);
                memcpy(expected_lengths, lengths, sizeof lengths);
            } else {
                assert(!memcmp(expected_buf, io_buf, sizeof io_buf));
                assert(!memcmp(expected_objects, objects, sizeof objects));
                assert(!memcmp(expected_lengths, lengths, sizeof lengths));
            }
        }
        assert(rc[0] == rc[1] && count[0] == count[1]);
        assert(result[0].measure == result[1].measure);
        assert(result[0].k == result[1].k && result[0].err == result[1].err);
        assert(result[0].fill == result[1].fill && result[0].total == result[1].total);
        cases++;
    }
    printf("project stream chunk copy: %u differential cases PASS\n", cases);
    return 0;
}
'''.replace('@SIZE@', size).replace('@ACTUAL@', actual)
with tempfile.TemporaryDirectory(prefix='project-stream-copy-') as tmp:
    c_file = Path(tmp) / 'test.c'
    exe = Path(tmp) / 'test'
    c_file.write_text(harness)
    subprocess.run(shlex.split(os.environ.get('CC', 'cc')) +
                   ['-std=c99', '-O2', '-Wall', '-Wextra', '-Werror',
                    str(c_file), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
