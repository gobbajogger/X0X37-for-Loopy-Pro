#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""tools/fm1_install.py: the Bluetooth controller image read-back after an SMK-37 install (no hardware).
The fake unit answers editor command 37 CTL_READ as firmware/src/app/editor.c does: only the 64 KiB
Bluetooth block, an offset outside it gets no reply, the count stops at the block end. The test checks
that the tool reads the image back, prints the unit CRC32, and stops with exit 8 if the CRC32 is not
the file CRC32 or the unit does not answer. Run from the repo root:
  python3 tests/install_ctl_test.py"""
import io
import sys
import zlib
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import install_test as T  # noqa: E402  (FakeSMK, package, cli; its tests run only as __main__)
import x0x_slots as xs  # noqa: E402

I = T.I
ok = T.ok
I.CTL_READ.update(timeout=0.2, tries=2)
BLOCK = 0x10000                                   # x0x_ble_abi.h XB_FLASH_SIZE = FL_BLE_HI - FL_BLE_LO
ELITE = "SMK-37 Elite_900"


def ctl_read(block, args):
    """editor.c ED_CTL_READ -> the reply args, or None (no reply)"""
    if len(args) < 4:
        return None
    off = args[0] | args[1] << 7 | args[2] << 14
    n = 224 if args[3] > 112 else args[3] * 2
    if off >= BLOCK:
        return None
    n = min(n, BLOCK - off)
    return [off & 0x7F, (off >> 7) & 0x7F, (off >> 14) & 0x7F] + xs.pack7(block[off:off + n])


class FakeBT(T.FakeSMK):
    """an SMK-37 with X0X: the Bluetooth block after the install is `block` (None: no CTL_READ, an older X0X)"""

    def __init__(self, image, block, **kw):
        self.block, self.reads = block, []
        super().__init__(image, identity=ELITE, name="SMK-37 Elite", after_write=ELITE, **kw)

    def rx(self, d):
        if d[:4] == I.ED_HDR and d[-1] == 0xF7:
            if d[4] == I.ED_CTL_READ and self.block is not None and not self.identity.startswith("ota-"):
                r = ctl_read(self.block, list(d[5:-1]))
                self.reads.append(list(d[5:-1]))
                if r is not None:
                    self.tx(I.ED_HDR + bytes([d[4]]) + bytes(r) + b"\xF7")
            return
        super().rx(d)


def make_ctl(n=64984):
    return bytes((i * 131 + (i >> 8) * 7 + 0x3C) & 0xFF for i in range(n))


def block_of(ctl):
    return ctl + b"\xFF" * (BLOCK - len(ctl))


def bounds():
    blk = bytes(range(256)) * (BLOCK // 256)
    ok(ctl_read(blk, [0x00, 0x00, 0x04, 112]) is None, "mock CTL_READ: offset 0x10000 (outside the block): no reply")
    r = ctl_read(blk, [0x7E, 0x7F, 0x03, 112])          # offset 0xFFFE: 2 bytes left
    ok(xs.unpack7(r[3:]) == blk[0xFFFE:], "mock CTL_READ: the count stops at the block end")


def unit_side():
    ctl = make_ctl()
    want = zlib.crc32(block_of(ctl))
    dev = FakeBT(b"", block_of(ctl))
    link = dev.open("SMK-37 Elite", None)
    out = io.StringIO()
    got = I.check_ctl(link, ctl, out)
    ok(got == want and f"unit CRC32 {want:08x}" in out.getvalue() and len(dev.reads) == (BLOCK + 223) // 224,
       f"read-back: full {BLOCK} B block in {len(dev.reads)} reads, unit CRC32 = expected CRC32 {want:08x}")
    ok(all(r[3] <= 112 and (r[0] | r[1] << 7 | r[2] << 14) + 2 * r[3] <= BLOCK for r in dev.reads),
       "read-back: every request stays inside the Bluetooth block")
    bad = bytearray(block_of(ctl))
    bad[40000] ^= 0x80
    dev = FakeBT(b"", bytes(bad))
    out = io.StringIO()
    try:
        I.check_ctl(dev.open("SMK-37 Elite", None), ctl, out)
        ok(False, "one changed bit: the read-back stops with an error")
    except I.InstallError as e:
        unit = zlib.crc32(bytes(bad))
        ok(e.code == "ctlcrc" and f"{unit:08x}" in str(e) and f"{want:08x}" in str(e)
           and f"unit CRC32 {unit:08x}" in out.getvalue(),
           f"one changed bit: error, unit CRC32 {unit:08x} printed, expected {want:08x}")
    bad_padding = bytearray(block_of(ctl))
    bad_padding[-1] = 0
    try:
        I.check_ctl(FakeBT(b"", bytes(bad_padding)).open("SMK-37 Elite", None), ctl, io.StringIO())
        ok(False, "changed erased padding byte: the full-block read-back stops with an error")
    except I.InstallError as e:
        ok(e.code == "ctlcrc", "changed erased padding byte is detected")
    dev = FakeBT(b"", None)
    try:
        I.check_ctl(dev.open("SMK-37 Elite", None), ctl, io.StringIO())
        ok(False, "no CTL_READ on the unit: the read-back stops with an error")
    except I.InstallError as e:
        ok(e.code == "ctlcrc" and "not read back" in str(e), "no CTL_READ on the unit: error, no CRC claimed")


def install():
    """the whole install: with_ctl gives the image (as --ctl), then the read-back after the restart"""
    ctl = make_ctl()
    raw = T.package(ELITE)
    image = I.logical_image(raw)
    p = T.pkgfile("elite.fwsc", raw)
    real = I.with_ctl

    def fake_with_ctl(a, product, img, out):
        a.ctl_bytes = ctl
        return img
    I.with_ctl = fake_with_ctl
    try:
        dev = FakeBT(image, block_of(ctl))
        rc, out, err = T.cli([p, "--yes", "--no-slots"], dev)
        ok(rc == 0 and "done:" in out and f"unit CRC32 {zlib.crc32(block_of(ctl)):08x}" in out,
           "install with a ctl.img: installed, read back, CRC32 ok, exit 0")
        bad = bytearray(block_of(ctl))
        bad[0] ^= 1
        dev = FakeBT(image, bytes(bad))
        rc, out, err = T.cli([p, "--yes", "--no-slots"], dev)
        ok(rc == 8 and "done:" in out and "is not the expected" in err, "install, the unit block differs: exit 8, error")
        dev = FakeBT(image, None)
        rc, out, err = T.cli([p, "--yes", "--no-slots"], dev)
        ok(rc == 8 and "not read back" in err, "install, the unit has no CTL_READ: exit 8")
    finally:
        I.with_ctl = real
    dev = FakeBT(image, None)
    rc, out, err = T.cli([p, "--yes", "--no-slots", "--no-ctl"], dev)
    ok(rc == 0 and not dev.reads, "install without a ctl.img: no read-back")


bounds()
unit_side()
install()
print(f"INSTALL CTL TESTS FAILED ({T.failed})" if T.failed else "install ctl tests passed")
sys.exit(1 if T.failed else 0)
