#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""SMP_BEGIN must not wipe the last good sample when the import is cancelled.

Cancel = BEGIN with no WRITE/END. The previous header and zones stay valid.
A full BEGIN→WRITE→END rewrite still replaces the slot.
"""
import struct
import sys
import zlib
from pathlib import Path

SRC = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SRC / "tools"))
sys.path.insert(0, str(SRC / "tests"))
import x0x_slots as xs  # noqa: E402
from slot_mock import MockDevice  # noqa: E402

FAILS = 0


def check(cond, what):
    global FAILS
    print(f"  {'ok  ' if cond else 'FAIL'} {what}")
    FAILS += not cond


def img(name=b"GOODLOOP", nsamp=128, rate=22050):
    data_len = (nsamp + 1) // 2
    data = bytes((i * 17) & 0xFF for i in range(data_len))
    hdr = bytearray(xs.HDR_LEN)
    struct.pack_into("<IHBB8sII", hdr, 0, xs.MAGIC, 1, 1, 0, name, data_len,
                     zlib.crc32(data) & 0xFFFFFFFF)
    struct.pack_into("<5I", hdr, 32, 0, nsamp, 0, 0, int(round(rate * 65536 / 44100)))
    return bytes(hdr) + bytes(xs.DATA_OFF - xs.HDR_LEN) + data


def header_name(dev, k):
    o = dev.slots[k][0]
    h = xs.parse_header(bytes(dev.fl.mem[o:o + xs.HDR_LEN]))
    return None if not h else h["name"]


def main():
    b = img()
    d = MockDevice(count=2, size=0x14000)
    k = 0
    assert d.req(xs.CMD_BEGIN, [k])[1] == 0
    for off in range(xs.DATA_OFF, len(b), 256):
        a = [k, off & 0x7F, (off >> 7) & 0x7F, (off >> 14) & 0x7F] + xs.pack7(b[off:off + 256])
        assert d.req(xs.CMD_WRITE, a)[-1] == 0
    assert d.req(xs.CMD_END, [k] + xs.pack7(b[:xs.HDR_LEN]))[1] == 0
    check(d.nz[k] == 1 and header_name(d, k) == "GOODLOOP", "setup: last-good sample present")

    before = bytes(d.fl.mem[d.slots[k][0]:d.slots[k][0] + len(b)])
    assert d.req(xs.CMD_BEGIN, [k])[1] == 0
    check(d.nz[k] == 1 and header_name(d, k) == "GOODLOOP",
          "BEGIN alone keeps nz and name (cancel / abandon)")
    check(bytes(d.fl.mem[d.slots[k][0]:d.slots[k][0] + len(b)]) == before,
          "BEGIN alone leaves flash bytes unchanged")

    # Without BEGIN, a populated slot still refuses WRITE.
    d2 = MockDevice(count=2, size=0x14000)
    d2.load(0, b[:xs.HDR_LEN], b[xs.DATA_OFF:])
    off = xs.DATA_OFF
    a = [0, off & 0x7F, (off >> 7) & 0x7F, (off >> 14) & 0x7F] + xs.pack7(b[off:off + 256])
    check(d2.req(xs.CMD_WRITE, a)[-1] == 4, "WRITE without BEGIN on occupied slot -> rc 4")

    # Full rewrite after BEGIN still works and replaces the sample.
    b2 = img(name=b"NEWLOOP1", nsamp=64)
    assert d.req(xs.CMD_BEGIN, [k])[1] == 0
    for off in range(xs.DATA_OFF, len(b2), 256):
        a = [k, off & 0x7F, (off >> 7) & 0x7F, (off >> 14) & 0x7F] + xs.pack7(b2[off:off + 256])
        assert d.req(xs.CMD_WRITE, a)[-1] == 0
    assert d.req(xs.CMD_END, [k] + xs.pack7(b2[:xs.HDR_LEN]))[1] == 0
    check(d.nz[k] == 1 and header_name(d, k) == "NEWLOOP1", "BEGIN+WRITE+END replaces sample")

    print("x0x slots begin-cancel: all ok" if not FAILS else "X0X SLOTS BEGIN-CANCEL FAILED")
    return 1 if FAILS else 0


if __name__ == "__main__":
    sys.exit(main())
