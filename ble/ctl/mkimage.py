#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
# Copyright (C) 2026 M. Geisler (genkigenki/makkuro)
"""Make ctl.img from the controller ELF: the load bytes from the header at XB_FLASH_XIP on, then
image_size and crc32 in the header (firmware/hal/x0x_ble_abi.h, xb_header_t). It also checks
what the app checks before it starts the image.

    mkimage.py ctl.elf ctl.img"""
import struct
import sys
import zlib

XB_HDR_MAGIC, XB_ABI, XB_HDR_VERSION = 0x43423058, 2, 2
XB_FLASH_XIP, XB_FLASH_SIZE = 0x02087000, 0x10000
XB_RAM_LO, XB_RAM_HI = 0x01C01800, 0x01C07F00
HDR = struct.Struct("<IHHIIIIIIIIIIII3I")


def load_image(elf):
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
    segs = []
    for i in range(shnum):
        _, typ, flags, addr, off, size, *_ = struct.unpack_from("<10I", elf, shoff + i * shentsize)
        if typ == 1 and flags & 2 and size:            # PROGBITS, ALLOC
            segs.append((lma(off), elf[off:off + size], addr))
    segs.sort()
    if not segs or segs[0][0] != XB_FLASH_XIP:
        raise SystemExit(f"mkimage: the image must start at {XB_FLASH_XIP:#x} (the Bluetooth block)")
    end = max(a + len(d) for a, d, _ in segs)
    img = bytearray(b"\xff" * (end - XB_FLASH_XIP))
    for a, d, v in segs:
        img[a - XB_FLASH_XIP:a - XB_FLASH_XIP + len(d)] = d
        print(f"  load {a:08x}  vma {v:08x}  {len(d):6d} B")
    img += b"\xff" * (-len(img) % 4)
    return img


def check(img):
    """the app's checks (firmware/src/ble.c ble_image_check), with a reason"""
    if len(img) < HDR.size:
        return "shorter than the header"
    h = HDR.unpack_from(img, 0)
    magic, ver, hsize, _fw, size, crc, abi, init, poll, slo, shi, rlo, rhi, ilo, ihi = h[:15]
    if magic != XB_HDR_MAGIC or ver != XB_HDR_VERSION or hsize != HDR.size or abi != XB_ABI:
        return "not an X0X controller image of this interface"
    if size != len(img) or size > XB_FLASH_SIZE:
        return f"image_size {size} is wrong (file {len(img)} B, block {XB_FLASH_SIZE} B)"
    z = bytearray(img)
    struct.pack_into("<I", z, 16, 0)
    if zlib.crc32(bytes(z)) & 0xFFFFFFFF != crc:
        return "CRC-32 mismatch"
    for name, a in (("init", init), ("poll", poll)):
        if not XB_FLASH_XIP + HDR.size <= a < XB_FLASH_XIP + size:
            return f"{name} {a:#x} is outside the image"
    if not XB_RAM_LO <= rlo <= slo < shi <= rhi <= XB_RAM_HI:
        return f"RAM {rlo:#x}..{rhi:#x} or stack {slo:#x}..{shi:#x} is outside {XB_RAM_LO:#x}..{XB_RAM_HI:#x}"
    if (ihi >> 30) & 1 or (ihi >> 31) & 1:
        return "the image asks for TIMER4 or TIMER5 (the app's)"
    return None


def main():
    if len(sys.argv) != 3:
        raise SystemExit(__doc__)
    img = load_image(open(sys.argv[1], "rb").read())
    if len(img) > XB_FLASH_SIZE:
        raise SystemExit(f"mkimage: {len(img)} B is larger than the 64 KiB Bluetooth block")
    struct.pack_into("<I", img, 12, len(img))
    struct.pack_into("<I", img, 16, 0)
    struct.pack_into("<I", img, 16, zlib.crc32(bytes(img)) & 0xFFFFFFFF)
    why = check(img)
    if why:
        raise SystemExit(f"mkimage: {why}")
    open(sys.argv[2], "wb").write(img)
    h = HDR.unpack_from(img, 0)
    print(f"{sys.argv[2]}: {len(img)} B of {XB_FLASH_SIZE}, crc32 {h[5]:08x}, "
          f"RAM {h[11]:#x}..{h[12]:#x} ({h[12] - h[11]} B of {XB_RAM_HI - XB_RAM_LO})")


if __name__ == "__main__":
    main()
