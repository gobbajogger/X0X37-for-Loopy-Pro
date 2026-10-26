#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""ED_DIAG (editor.c) must not write past the end of its word buffer or of ed_out.

The reply is the crash record (fm1_crash_t), the 2 boot guard words, then ble_diag (ble.c):
the start record (ble_guard_t) and the live words. ble_diag gets max = the free words.
Each "if (n + K > max) return n;" in ble_diag must cover all the words up to the next check.
The words must fit w[] in editor.c, and the reply (5 x 7 bits for each word) must fit ed_out.
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
editor = (ROOT / "firmware/src/app/editor.c").read_text()
ble = (ROOT / "firmware/src/ble.c").read_text()
irq = (ROOT / "firmware/hal/fm1_irq.h").read_text()
glue = (ROOT / "firmware/src/ble/ble_glue.h").read_text()


def struct_words(src, name):
    body = re.search(r"typedef struct \{(.*?)\}\s*" + name + ";", src, re.S).group(1)
    body = re.sub(r"/\*.*?\*/", "", body, flags=re.S)
    n = 0
    for decl in re.findall(r"uint32_t([^;]*);", body):
        for v in decl.split(","):
            m = re.search(r"\[(\d+)\]", v)
            n += int(m.group(1)) if m else 1
    return n


fails = 0
crash = struct_words(irq, "fm1_crash_t")
guard = struct_words(glue, "ble_guard_t")
case = re.search(r"case ED_DIAG: \{(.*?)break;", editor, re.S).group(1)
size = int(re.search(r"uint32_t w\[(\d+)\]", case).group(1))
boot = len(re.findall(r"w\[nw\+\+\] = bootguard", case))
out = int(re.search(r"static uint8_t ed_out\[(\d+)\];", editor).group(1))

body = re.search(r"static uint32_t ble_diag\(uint32_t \*w, uint32_t max\)\n\{(.*?)\n\}", ble, re.S).group(1)
live, k, writes = 0, None, 0
for ln in body.split("\n"):
    m = re.search(r"if \(n \+ (\d+)u > max\)", ln)
    if m:
        if k is not None and writes > k:
            print(f"FAIL ble_diag: a check for {k} words, then {writes} words")
            fails += 1
        k, writes = int(m.group(1)), 0
    elif "w[n++] =" in ln and "g[i]" not in ln:
        writes += 1
        live += 1
if k is not None and writes > k:
    print(f"FAIL ble_diag: a check for {k} words, then {writes} words")
    fails += 1

total = crash + boot + guard + live
print(f"DIAG words: crash {crash} + boot {boot} + guard {guard} + live {live} = {total}, w[{size}]")
if total > size:
    print(f"FAIL ED_DIAG: {total} words, but w[] has {size}")
    fails += 1
reply = 5 + 1 + 5 * total + 1          # header with cmd, count, 5 bytes for each word, F7
if reply > out:
    print(f"FAIL ED_DIAG: the reply has {reply} bytes, but ed_out has {out}")
    fails += 1
inlog = (ROOT / "firmware/src/app/smk_inlog.h").read_text()
n_in = int(re.search(r"#define SMK_INLOG_N (\d+)u", inlog).group(1))
cells = int(re.search(r"#define SMK_INLOG_CELLS (\d+)u", inlog).group(1))
reply = 5 + 1 + 5 + 5 * 2 * n_in + 2 * cells + 1   # header with cmd, count, now, 2 words for each entry, cells, F7
print(f"INLOG reply: {n_in} entries, {reply} bytes, ed_out {out}")
if n_in > 127:
    print(f"FAIL ED_INLOG: {n_in} entries, but the count is one 7-bit byte")
    fails += 1
if reply > out:                         # ed_b keeps the last byte of ed_out for F7
    print(f"FAIL ED_INLOG: the reply has {reply} bytes, but ed_out has {out}")
    fails += 1
print(f"{'FAIL' if fails else 'PASS'} ED_DIAG and ED_INLOG writes stay in w[] and ed_out ({fails} fail)")
sys.exit(1 if fails else 0)
