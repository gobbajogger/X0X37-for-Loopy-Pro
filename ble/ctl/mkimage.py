#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
# Copyright (C) 2026 M. Geisler (genkigenki/makkuro)
"""Make ctl.img from the image ELF (firmware/hal/x0x_ble_abi.h, ABI 3, xb_header_t).

ctl.img = piece 1 (the Bluetooth block, XIP 0x02087000) padded with 0xFF to 64 KiB, then piece 2
(XIP 0x020A7000, flash 0xAB000..0xB6FFF; the app writes it there on the installer's request), with
0xFF in its hole (0xAF000..0xAFFFF, the store owner tag: never written). The header in piece 1 holds
image_size and crc32 (piece 1) and p2_size and p2_crc (piece 2, CRC without the hole). It also checks
what the app checks before it starts the image.

    mkimage.py ctl.elf ctl.img"""
import struct
import sys
import zlib

XB_HDR_MAGIC, XB_ABI, XB_HDR_VERSION = 0x43423058, 3, 3
XB_FLASH_XIP, XB_FLASH_SIZE = 0x02087000, 0x10000
XB_FLASH2_XIP, XB_FLASH2_SIZE = 0x020A7000, 0xC000
XB_FLASH2_HOLE, XB_FLASH2_HOLE_SIZE = 0x4000, 0x1000
XB_RAM_LO, XB_RAM_HI = 0x01C01480, 0x01C07F00
XB_RAM2_LO, XB_RAM2_HI = 0x01C7CDC0, 0x01C7D400
HDR = struct.Struct("<IHHIIIIIIIIIIIIIIII")   # 72 bytes


def load_image(elf):
    """-> (piece 1, piece 2) bytes"""
    if elf[:4] != b"\x7fELF" or elf[4] != 1 or elf[5] != 1:
        raise SystemExit("mkimage: need a 32-bit little-endian ELF")
    phoff, = struct.unpack_from("<I", elf, 28)
    phentsize, phnum = struct.unpack_from("<HH", elf, 42)
    shoff, = struct.unpack_from("<I", elf, 32)
    shentsize, shnum, _ = struct.unpack_from("<HHH", elf, 46)
    phs = [struct.unpack_from("<8I", elf, phoff + i * phentsize) for i in range(phnum)]

    def lma(off):
        for p in phs:
            if p[0] == 1 and p[1] <= off < p[1] + p[4]:
                return p[3] + off - p[1]
        raise SystemExit(f"mkimage: file offset {off:#x} is in no load segment")
    pieces = {1: [], 2: []}
    for i in range(shnum):
        _, typ, flags, addr, off, size, *_ = struct.unpack_from("<10I", elf, shoff + i * shentsize)
        if typ == 1 and flags & 2 and size:            # PROGBITS, ALLOC
            a = lma(off)
            if XB_FLASH_XIP <= a < XB_FLASH_XIP + XB_FLASH_SIZE:
                pieces[1].append((a, elf[off:off + size], addr))
            elif XB_FLASH2_XIP <= a < XB_FLASH2_XIP + XB_FLASH2_SIZE and not \
                    XB_FLASH2_XIP + XB_FLASH2_HOLE <= a < XB_FLASH2_XIP + XB_FLASH2_HOLE + XB_FLASH2_HOLE_SIZE:
                pieces[2].append((a, elf[off:off + size], addr))
            else:
                raise SystemExit(f"mkimage: a section loads at {a:#x}, outside both pieces")
    out = []
    for k, base in ((1, XB_FLASH_XIP), (2, XB_FLASH2_XIP)):
        segs = sorted(pieces[k])
        if not segs:
            out.append(bytearray())
            continue
        if k == 1 and segs[0][0] != base:
            raise SystemExit(f"mkimage: piece 1 must start at {base:#x} (the Bluetooth block)")
        end = max(a + len(d) for a, d, _ in segs)
        img = bytearray(b"\xff" * (end - base))
        for a, d, v in segs:
            img[a - base:a - base + len(d)] = d
            print(f"  piece {k} load {a:08x}  vma {v:08x}  {len(d):6d} B")
        img += b"\xff" * (-len(img) % 4)
        out.append(img)
    return out[0], out[1]


def p2_crc(p2):
    """CRC-32 of piece 2 without its hole (as the app computes it, ble_glue.h)"""
    a = bytes(p2[:XB_FLASH2_HOLE])
    b = bytes(p2[XB_FLASH2_HOLE + XB_FLASH2_HOLE_SIZE:])
    return zlib.crc32(b, zlib.crc32(a)) & 0xFFFFFFFF


def check(img):
    """the app's checks (firmware/src/ble/ble_glue.h ble_img_check), with a reason; img = ctl.img"""
    if len(img) < HDR.size:
        return "shorter than the header"
    h = HDR.unpack_from(img, 0)
    magic, ver, hsize, _fw, size, crc, abi, init, poll, slo, shi, rlo, rhi, ilo, ihi, p2n, p2c, r2lo, r2hi = h
    if magic != XB_HDR_MAGIC or ver != XB_HDR_VERSION or hsize != HDR.size or abi != XB_ABI:
        return "not an X0X Bluetooth image of this interface (ABI 3)"
    if size > XB_FLASH_SIZE or size > len(img):
        return f"image_size {size} is wrong (file {len(img)} B, block {XB_FLASH_SIZE} B)"
    z = bytearray(img[:size])
    struct.pack_into("<I", z, 16, 0)
    if zlib.crc32(bytes(z)) & 0xFFFFFFFF != crc:
        return "CRC-32 mismatch (piece 1)"
    if p2n:
        if p2n > XB_FLASH2_SIZE or len(img) != XB_FLASH_SIZE + p2n:
            return f"piece 2: {p2n} B does not fit (file {len(img)} B)"
        if p2n <= XB_FLASH2_HOLE + XB_FLASH2_HOLE_SIZE:
            return "piece 2 must reach past its hole"
        if p2_crc(img[XB_FLASH_SIZE:]) != p2c:
            return "CRC-32 mismatch (piece 2)"
        if any(b != 0xFF for b in img[XB_FLASH_SIZE + XB_FLASH2_HOLE:XB_FLASH_SIZE + XB_FLASH2_HOLE + XB_FLASH2_HOLE_SIZE]):
            return "piece 2 has bytes in its hole (the store owner tag)"
    elif len(img) != size:
        return "a file without piece 2 must be piece 1 only"
    for name, a in (("init", init), ("poll", poll)):
        if not XB_FLASH_XIP + HDR.size <= a < XB_FLASH_XIP + size:
            return f"{name} {a:#x} is outside piece 1"
    if not XB_RAM_LO <= rlo <= slo < shi <= rhi <= XB_RAM_HI:
        return f"RAM {rlo:#x}..{rhi:#x} or stack {slo:#x}..{shi:#x} is outside {XB_RAM_LO:#x}..{XB_RAM_HI:#x}"
    if not (r2lo == r2hi or XB_RAM2_LO <= r2lo < r2hi <= XB_RAM2_HI):
        return f"RAM 2 {r2lo:#x}..{r2hi:#x} is outside {XB_RAM2_LO:#x}..{XB_RAM2_HI:#x}"
    if (ihi >> 30) & 1 or (ihi >> 31) & 1:
        return "the image asks for TIMER4 or TIMER5 (the app's)"
    # the update loader drops a sector of the VM whose last 256 bytes hold an update record
    # (firmware/loader/ldr_core.c): piece 2 must never look like one
    p2 = img[XB_FLASH_SIZE:]
    for s in range(0x1000, ((len(p2) + 0xFFF) // 0x1000) * 0x1000 + 1, 0x1000):
        r = p2[s - 0x100:s - 0x100 + 8] if s - 0x100 + 8 <= len(p2) else b""
        if len(r) == 8 and r[6] == 0x41 and r[7] == 0x54:
            return f"piece 2 sector {s // 0x1000 - 1}: looks like an update record (0x5441 at its end)"
    return None


def main():
    if len(sys.argv) != 3:
        raise SystemExit(__doc__)
    p1, p2 = load_image(open(sys.argv[1], "rb").read())
    if len(p1) > XB_FLASH_SIZE:
        raise SystemExit(f"mkimage: piece 1 {len(p1)} B is larger than the 64 KiB Bluetooth block")
    if len(p2) > XB_FLASH2_SIZE:
        raise SystemExit(f"mkimage: piece 2 {len(p2)} B is larger than {XB_FLASH2_SIZE} B")
    struct.pack_into("<I", p1, 12, len(p1))
    struct.pack_into("<II", p1, 56, len(p2), p2_crc(p2) if p2 else 0)
    struct.pack_into("<I", p1, 16, 0)
    struct.pack_into("<I", p1, 16, zlib.crc32(bytes(p1)) & 0xFFFFFFFF)
    img = bytes(p1) + (b"\xff" * (XB_FLASH_SIZE - len(p1)) + bytes(p2) if p2 else b"")
    why = check(img)
    if why:
        raise SystemExit(f"mkimage: {why}")
    open(sys.argv[2], "wb").write(img)
    h = HDR.unpack_from(img, 0)
    print(f"{sys.argv[2]}: piece 1 {len(p1)} B of {XB_FLASH_SIZE}, crc32 {h[5]:08x}; piece 2 {len(p2)} B of "
          f"{XB_FLASH2_SIZE}, crc32 {h[16]:08x}; RAM {h[11]:#x}..{h[12]:#x} ({h[12] - h[11]} B of "
          f"{XB_RAM_HI - XB_RAM_LO}); RAM 2 {h[17]:#x}..{h[18]:#x}")


if __name__ == "__main__":
    main()
