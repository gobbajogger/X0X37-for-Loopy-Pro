#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Write Bluetooth image piece 2 to the unit (ED_CTL2_WRITE / ED_CTL2_READ).

  tools/ctl2_write.py ctl.img [--port NAME]
  tools/ctl2_write.py ctl.img --check-only

Piece 1 is in the package (tools/add_ctl.py). Piece 2 is plain flash at 0xAB000 with a hole
at 0xAF000 for the store owner tag. The unit must already run a build that answers ED_CTL2.
"""
import argparse, struct, sys, time, zlib
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
import add_ctl, fm1_install as fi

ED_HDR = bytes((0x7D, 0x46, 0x4C))
ED_CTL2_WRITE, ED_CTL2_READ = 35, 36
BLE_SIZE, HOLE, HOLE_SZ = 0x10000, 0x4000, 0x1000

def pack7(b):
    out = bytearray()
    for i in range(0, len(b), 7):
        chunk = b[i:i+7]
        m = 0
        for j, x in enumerate(chunk):
            m |= (x >> 7) << j
        out.append(m)
        out.extend(x & 0x7F for x in chunk)
    return bytes(out)

def off7(off):
    return bytes((off & 0x7F, (off >> 7) & 0x7F, (off >> 14) & 0x7F))

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("ctl")
    ap.add_argument("--port")
    ap.add_argument("--check-only", action="store_true")
    a = ap.parse_args()
    img = Path(a.ctl).read_bytes()
    if (p := add_ctl.ctl_problem(img)):
        raise SystemExit(f"{a.ctl}: {p}")
    p2_size = struct.unpack_from("<I", img, 56)[0]
    if not p2_size:
        print("no piece 2")
        return 0
    p2 = bytearray(img[BLE_SIZE:BLE_SIZE + p2_size])
    if a.check_only:
        print(f"piece 2 {p2_size} B ok")
        return 0
    port = fi.open_port(a.port)
    try:
        for off in range(0, p2_size, 256):
            if HOLE <= off < HOLE + HOLE_SZ:
                continue
            chunk = bytes(p2[off:off+256])
            if off + 256 > HOLE and off < HOLE:
                chunk = bytes(p2[off:HOLE]) + b"\xff" * (off + 256 - HOLE)
            # skip writing the hole sector entirely
            if off == HOLE:
                continue
            body = off7(off) + pack7(chunk[:256] if len(chunk) >= 256 else chunk + b"\xff" * (256 - len(chunk)))
            # only write real length for the last partial
            n = min(256, p2_size - off)
            if HOLE <= off < HOLE + HOLE_SZ:
                continue
            if off < HOLE and off + n > HOLE:
                n = HOLE - off
            body = off7(off) + pack7(bytes(p2[off:off+n]))
            fi.ed_req(port, ED_CTL2_WRITE, body)
            time.sleep(0.01)
        # read back CRC of non-hole
        got = bytearray(p2_size)
        for off in range(0, p2_size, 224):
            n = min(224, p2_size - off)
            r = fi.ed_req(port, ED_CTL2_READ, off7(off) + bytes((n // 2,)))
            # reply: off3 + packed bytes; skip off
            data = fi.ed_unpack7(r[3:])
            got[off:off+len(data)] = data[:n]
        a_b, b_b = bytes(got[:HOLE]), bytes(got[HOLE+HOLE_SZ:p2_size])
        want = struct.unpack_from("<I", img, 60)[0]
        c = zlib.crc32(b_b, zlib.crc32(a_b)) & 0xFFFFFFFF
        if c != want:
            raise SystemExit(f"read-back CRC {c:#x} != {want:#x}")
        print(f"piece 2 {p2_size} B written, CRC ok")
    finally:
        port.close()
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
