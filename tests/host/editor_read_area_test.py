#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Compile the production read dispatch against an independent wire encoder.

Only unrelated editor commands are excluded from this focused translation unit.
The selected cases, encoder and (when present) common helper are used verbatim.
"""
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "firmware/src/app/editor.c").read_text()


def function(name):
    import re
    match = re.search(r"^static[^\n]*\b" + name + r"\([^\n]*\)\n\{", source, re.M)
    if not match:
        return ""
    start = match.start()
    opening = source.index("{", match.start())
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


def case(name, following):
    start = source.index("    case " + name + ":", source.index("static void ed_handle("))
    end = source.index(following, start)
    return source[start:end]


unit = r'''
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#define ED_READ_MAX 224u
#define ED_SMP_READ 16
#define ED_CTL_READ 37
#define SMP_USER_SLOTS 2
#define SMP_USER_SIZE(k) ((k) == 0 ? 81920u : 122880u)
#define XB_FLASH_SIZE 65536u
#define XB_FLASH_OFF 0x10000u
#define FL_BLE_LO XB_FLASH_OFF
#define FL_BLE_HI (XB_FLASH_OFF + XB_FLASH_SIZE)
static uint8_t area[122880], ed_out[732], actual[732], expected[732];
static uint32_t ed_n, sent, sent_n;
static const uint8_t *smp_user_xip(unsigned k) { (void)k; return area; }
static const uint8_t *fm1_xip_ptr(unsigned off) { assert(off == XB_FLASH_OFF); return area; }
static int ed_send(void) {
    ed_out[ed_n++] = 0xf7;
    memcpy(actual, ed_out, ed_n); sent_n = ed_n; sent++; return 0;
}
'''
# The frame header is shared production code too.
unit += "\n#define ED_HDR0 0x7d\n#define ED_HDR1 0x46\n#define ED_HDR2 0x4c\n"
for name in ("ed_begin", "ed_b", "ed_put7", "ed_read_area"):
    unit += function(name) + "\n"
unit += "static void dispatch(unsigned cmd, const uint8_t *a, unsigned na) {\ned_begin(cmd);\nswitch(cmd) {\n"
unit += case("ED_SMP_READ", "    case ED_SMP_INFO:")
unit += case("ED_CTL_READ", "\n#endif\n    default:")
unit += "default: return;\n}\ned_send();\n}\n"
unit += r'''
/* Independent encoder: group by source-byte index, with explicit low/high bits. */
static unsigned reference(unsigned cmd, unsigned slot, unsigned off, unsigned count, unsigned size) {
    unsigned pos = 0;
    if (off >= size) return 0;
    unsigned n = count * 2;
    if (n > 224) n = 224;
    if (n > size - off) n = size - off;
    expected[pos++] = 0xf0; expected[pos++] = 0x7d;
    expected[pos++] = 0x46; expected[pos++] = 0x4c; expected[pos++] = cmd;
    if (cmd == ED_SMP_READ) expected[pos++] = slot;
    for (unsigned shift = 0; shift < 21; shift += 7) expected[pos++] = (off >> shift) & 127;
    for (unsigned i = 0; i < n;) {
        unsigned mask = pos++;
        expected[mask] = 0;
        for (unsigned bit = 0; bit < 7 && i < n; bit++, i++) {
            unsigned value = area[off+i];
            expected[mask] |= (value / 128) << bit;
            expected[pos++] = value % 128;
        }
    }
    expected[pos++] = 0xf7;
    return pos;
}
int main(void) {
    unsigned checks = 0;
    for (unsigned i = 0; i < sizeof area; i++) area[i] = (uint8_t)(i * 73 + 129);
    for (unsigned kind = 0; kind < 3; kind++) {
        unsigned cmd = kind == 2 ? ED_CTL_READ : ED_SMP_READ;
        unsigned size = kind == 2 ? XB_FLASH_SIZE : SMP_USER_SIZE(kind);
        unsigned offsets[] = {0, 1, 127, 128, 16384, size-7, size-1, size, size+1, 0x1fffff};
        unsigned counts[] = {0, 1, 3, 4, 7, 111, 112, 113, 127};
        for (unsigned o = 0; o < sizeof offsets/sizeof *offsets; o++)
        for (unsigned c = 0; c < sizeof counts/sizeof *counts; c++) {
            unsigned off = offsets[o];
            uint8_t a[5] = {off & 127, (off >> 7) & 127, (off >> 14) & 127, counts[c], 0};
            if (kind != 2) { memmove(a+1,a,4); a[0] = kind; }
            unsigned want = reference(cmd,kind,off,counts[c],size);
            sent = sent_n = 0;
            dispatch(cmd,a,kind == 2 ? 4 : 5);
            assert(sent == (want != 0)); assert(sent_n == want);
            assert(!want || memcmp(actual,expected,want) == 0);
            checks++;
        }
        uint8_t a[5] = {0};
        for (unsigned n = 0; n < (kind == 2 ? 4u : 5u); n++) {
            sent = 0; dispatch(cmd,a,n); assert(!sent); checks++;
        }
    }
    uint8_t bad_slot[5] = {2,0,0,0,1};
    sent = 0; dispatch(ED_SMP_READ,bad_slot,5); assert(!sent); checks++;
    printf("editor read replies: %u cases PASS\n", checks);
    return 0;
}
'''
with tempfile.TemporaryDirectory(prefix="editor-read-") as temporary:
    path = Path(temporary)
    (path / "test.c").write_text(unit)
    subprocess.run([os.environ.get("CC", "cc"), "-std=c11", "-O2", "-Wall", "-Wextra",
                    "-Wshadow", "-Wcast-align", "-Werror", str(path / "test.c"),
                    "-o", str(path / "test")], check=True)
    subprocess.run([str(path / "test")], check=True)
