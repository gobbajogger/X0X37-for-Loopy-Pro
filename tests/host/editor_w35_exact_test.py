#!/usr/bin/env python3
"""Exact byte/length test of editor.c's bounded ED_DIAG / ED_BT_TEST writer.

Extracts and executes the actual firmware helper, not an independently
reimplemented version. Does not require the proprietary SDK or device.
"""
import os
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
src = (root / "firmware/src/app/editor.c").read_text()
start = "/* ED_W35_BEGIN */"
end = "/* ED_W35_END */"
assert src.count(start) == src.count(end) == 1, "missing / duplicate writer markers"
writer = src.split(start, 1)[1].split(end, 1)[0]

harness = r'''
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static uint8_t ed_out[732];
static uint32_t ed_n;
''' + writer + r'''
static uint8_t ref_out[732];
static uint32_t ref_n;
static void ref_b(uint32_t v)
{
    if (ref_n < sizeof ref_out - 1u)
        ref_out[ref_n++] = (uint8_t)(v & 0x7fu);
}
static void ref_w35(uint32_t v)
{
    ref_b(v);
    ref_b(v >> 7);
    ref_b(v >> 14);
    ref_b(v >> 21);
    ref_b(v >> 28);
}
static uint32_t random_word(uint32_t *s)
{
    *s ^= *s << 13;
    *s ^= *s >> 17;
    *s ^= *s << 5;
    return *s;
}
static uint32_t cases = 0, words = 0;
static int trial(unsigned initial, unsigned count, uint32_t seed)
{
    static const uint32_t edge[] = {
        0u, 1u, 0x7fu, 0x80u, 0x3fffu, 0x4000u,
        0x1fffffu, 0x200000u, 0x0fffffffu,
        0x10000000u, 0x80000000u, 0xffffffffu
    };
    unsigned i;
    if (initial + 5u * count > 731u)
        return 0;
    memset(ed_out, 0xa5, sizeof ed_out);
    memset(ref_out, 0xa5, sizeof ref_out);
    ed_n = ref_n = initial;
    for (i = 0; i < count; ++i) {
        uint32_t v = i < sizeof(edge) / sizeof(edge[0])
            ? edge[i] : random_word(&seed);
        ref_w35(v);
        ed_w35(v);
    }
    if (ed_n != ref_n || memcmp(ed_out, ref_out, sizeof ed_out)) {
        fprintf(stderr, "FAIL start=%u count=%u seed=%08x len=%u/%u\n",
                initial, count, seed, ed_n, ref_n);
        return 1;
    }
    ++cases;
    words += count;
    return 0;
}
int main(void)
{
    static const unsigned starts[] = {5u, 6u, 7u, 90u, 390u, 630u, 726u};
    static const unsigned counts[] = {0u, 1u, 2u, 5u, 12u, 64u, 77u, 127u};
    unsigned i, j, seed;
    for (i = 0; i < sizeof starts / sizeof starts[0]; ++i)
        for (j = 0; j < sizeof counts / sizeof counts[0]; ++j)
            for (seed = 1; seed <= 256u; ++seed)
                if (trial(starts[i], counts[j], seed * 0x9e3779b9u))
                    return 1;
    printf("PASS: %u bounded cases / %u words, byte-exact\n", cases, words);
    return 0;
}
''';
with tempfile.TemporaryDirectory(prefix="dnbox-ed-w35-") as tmp:
    test_c = Path(tmp) / "test.c"
    exe = Path(tmp) / "test"
    test_c.write_text(harness)
    subprocess.run([os.environ.get("CC", "cc"), "-std=c11", "-O2", "-Wall",
                    "-Wextra", "-Werror", str(test_c), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
