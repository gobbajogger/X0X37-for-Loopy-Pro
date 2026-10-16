#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Write Bluetooth image piece 2 to a running SMK-37 X0X over its MIDI port (ED_CTL2_WRITE),
then read each block back (ED_CTL2_READ) and compare. The whole CRC is checked at the end.

  tools/ctl2_midi.py ctl.img            write, read back, check
  tools/ctl2_midi.py ctl.img --verify   read back and check only (no write)

Piece 2 is plain flash 0xAB000..0xB6FFF. The hole at 0xAF000 (the store owner tag) is never
written. The installer link cuts long messages, so this tool uses the MIDI port (mido)."""
import argparse
import struct
import sys
import time
import zlib

import mido

HDR = [0x7D, 0x46, 0x4C]
ED_CTL2_WRITE, ED_CTL2_READ, ED_BT_TEST = 35, 36, 34
BLE_SIZE, HOLE, HOLE_END = 0x10000, 0x4000, 0x5000
BLOCK, READ = 256, 112


class Ctl2Error(Exception):
    pass


def pack7(b):
    out = []
    for i in range(0, len(b), 7):
        c = b[i:i + 7]
        out.append(sum(((x >> 7) & 1) << j for j, x in enumerate(c)))
        out.extend(x & 0x7F for x in c)
    return out


def unpack7(v):
    out = bytearray()
    for i in range(0, len(v), 8):
        m = v[i]
        out.extend(x | (((m >> j) & 1) << 7) for j, x in enumerate(v[i + 1:i + 8]))
    return bytes(out)


def o7(off):
    return [off & 0x7F, (off >> 7) & 0x7F, (off >> 14) & 0x7F]


def ports():
    import ble_autotest                       # the same port choice as the autotest
    p = ble_autotest.ports()
    if not p:
        raise Ctl2Error("no X0X MIDI port")
    return p


class Port:
    def __init__(self):
        a, b = ports()
        self.i, self.o = mido.open_input(a), mido.open_output(b)
        time.sleep(0.3)                       # macOS drops a message sent at once after the port opens

    def close(self):
        self.i.close()
        self.o.close()

    def req(self, cmd, args, want_off=None, timeout=1.0, tries=4):
        for _ in range(tries):
            for _m in self.i.iter_pending():
                pass
            self.o.send(mido.Message("sysex", data=HDR + [cmd] + list(args)))
            t = time.monotonic()
            while time.monotonic() - t < timeout:
                for m in self.i.iter_pending():
                    d = list(m.data) if m.type == "sysex" else []
                    if d[:4] == HDR + [cmd]:
                        r = d[4:]
                        if want_off is None or (len(r) >= 3 and (r[0] | r[1] << 7 | r[2] << 14) == want_off):
                            return r
                time.sleep(0.002)
        raise Ctl2Error(f"no reply to cmd {cmd}" + (f" at {want_off:#x}" if want_off is not None else ""))


def read(port, off, n):
    out = bytearray()
    while n:
        k = min(READ, n)
        r = port.req(ED_CTL2_READ, o7(off) + [(k + 1) // 2], want_off=off)
        d = unpack7(bytes(r[3:]))[:k]
        if len(d) != k:
            raise Ctl2Error(f"short read at {off:#x}")
        out += d
        off += k
        n -= k
    return bytes(out)


def piece2(img):
    p2_size, p2_crc = struct.unpack_from("<II", img, 56)
    return img[BLE_SIZE:BLE_SIZE + p2_size], p2_crc


def verify(port, img, out=sys.stdout):
    p2, want = piece2(img)
    a = read(port, 0, HOLE)
    b = read(port, HOLE_END, len(p2) - HOLE_END)
    crc = zlib.crc32(b, zlib.crc32(a)) & 0xFFFFFFFF
    print(f"piece 2 read back: CRC {crc:08x}, want {want:08x}", file=out)
    return crc == want


def write(port, img, out=sys.stdout):
    p2, _ = piece2(img)
    for off in range(0, len(p2), BLOCK):
        if HOLE <= off < HOLE_END:
            continue
        n = min(BLOCK, len(p2) - off)
        data = p2[off:off + n]
        r = port.req(ED_CTL2_WRITE, o7(off) + pack7(data), want_off=off, timeout=3.0, tries=2)
        if len(r) < 4 or r[3] != 0:
            raise Ctl2Error(f"write at {off:#x}: result {r[3] if len(r) > 3 else '-'}")
        if read(port, off, n) != data:
            raise Ctl2Error(f"block at {off:#x} does not read back")
    print(f"piece 2: {len(p2)} B written, every block read back ok", file=out)


def main(argv=None):
    ap = argparse.ArgumentParser()
    ap.add_argument("ctl")
    ap.add_argument("--verify", action="store_true")
    ap.add_argument("--no-restart", action="store_true")
    a = ap.parse_args(argv)
    img = open(a.ctl, "rb").read()
    port = Port()
    try:
        if not a.verify:
            write(port, img)
        ok = verify(port, img)
        if ok and not a.verify and not a.no_restart:
            port.o.send(mido.Message("sysex", data=HDR + [ED_BT_TEST, 2]))
            print("restart asked (ED_BT_TEST 2)")
        return 0 if ok else 1
    except Ctl2Error as e:
        print(f"ctl2: {e}", file=sys.stderr)
        return 2
    finally:
        port.close()


if __name__ == "__main__":
    sys.exit(main())
