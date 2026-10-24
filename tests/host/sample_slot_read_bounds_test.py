#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Native source-AST slot-read safety regression; no device or dependency packages.

Usage: python3 -B tests/host/sample_slot_read_bounds_test.py [tools/x0x_slots.py]
"""
import ast
import inspect
from pathlib import Path
import struct
import sys
import zlib

ROOT = Path(__file__).resolve().parents[2]
source = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "tools" / "x0x_slots.py"
tree = ast.parse(source.read_text(encoding="utf-8"), filename=str(source))
functions = [node for node in tree.body
             if isinstance(node, ast.FunctionDef) and node.name in ("parse_header", "read_slot")]
assert len(functions) == 2, "both source functions must be extracted"

class SlotError(Exception):
    pass

MAGIC, HDR_LEN, DATA_OFF = 0x504D5346, 480, 512

def ascii_name(b):
    return "".join(chr(c) if 32 <= c < 127 else "?" for c in b)

def read_bytes(link, k, off, n, progress=None):
    link.reads.append((k, off, n))
    if off == 0:
        return link.header[:n]
    if off == DATA_OFF:
        return link.data[:n]
    raise RuntimeError(f"unexpected offset {off}")

ns = {"SlotError": SlotError, "MAGIC": MAGIC, "HDR_LEN": HDR_LEN, "DATA_OFF": DATA_OFF,
      "ascii_name": ascii_name, "read_bytes": read_bytes, "struct": struct, "zlib": zlib}
exec(compile(ast.Module(body=functions, type_ignores=[]), str(source), "exec"), ns)
reader = ns["read_slot"]

class Link:
    def __init__(self, data, nz=1, corrupt_crc=False, valid_magic=True):
        self.data = data
        self.reads = []
        self.header = bytearray(DATA_OFF)
        struct.pack_into("<IHBB8sII", self.header, 0,
                         MAGIC if valid_magic else 0, 1, nz, 0, b"DRUM\0\0\0\0",
                         len(data), zlib.crc32(data) ^ int(corrupt_crc))
        for i in range(nz):
            struct.pack_into("<5I", self.header, 32 + 28 * i,
                             0, 2 * len(data), 0, 0, 1 << 16)

def run_read(link, declared_nz=1, capacity=4096):
    # Compatibility adapter is solely to reproduce pre-fix semantics; the
    # final implementation must take the declared SMP_INFO size explicitly.
    opts = {"slot_size": capacity} if "slot_size" in inspect.signature(reader).parameters else {}
    return reader(link, 0, declared_nz, **opts)

def check_case(desc, link, declared_nz, capacity, expected, require_no_data=False):
    try:
        result = run_read(link, declared_nz, capacity)
        status = "ok"
    except SlotError:
        status, result = "refused", None
    if expected == "ok":
        ok = status == "ok" and result == bytes(link.header) + link.data
    elif expected == "empty":
        ok = status == "ok" and result is None and not link.reads
    else:
        ok = status == "refused" and (not require_no_data or all(off == 0 for _, off, _ in link.reads))
    print(f"{'PASS' if ok else 'FAIL'} {desc}: {status}, requests={link.reads}")
    return ok

outcomes = []
outcomes.append(check_case("canonical exact-bound", Link(b"a" * 512), 1, 1024, "ok"))
outcomes.append(check_case("canonical room", Link(b"b" * 8), 1, 2048, "ok"))
outcomes.append(check_case("oversized CRC-valid header: no out-of-slot read", Link(b"z" * 513),
                           1, 1024, "refused", True))
outcomes.append(check_case("mismatched SMP_INFO zone count: no data read", Link(b"x" * 16, nz=2),
                           1, 2048, "refused", True))
outcomes.append(check_case("missing physical slot size must refuse", Link(b"x" * 16),
                           1, None, "refused", True))
outcomes.append(check_case("empty slot is read-only noop", Link(b""), 0, 2048, "empty"))
outcomes.append(check_case("damaged data CRC", Link(b"abc", corrupt_crc=True), 1, 2048, "refused"))
outcomes.append(check_case("corrupted header magic", Link(b"abc", valid_magic=False), 1, 2048, "refused", True))
count = sum(outcomes)
print(f"summary {count}/{len(outcomes)}")
raise SystemExit(0 if count == len(outcomes) else 1)
