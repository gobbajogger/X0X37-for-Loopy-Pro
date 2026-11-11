#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Add a Bluetooth controller image (ctl.img) to an X0X package for the SMK-37, or take it out.

  tools/add_ctl.py X0X.fwsc ctl.img -o X0X-ble.fwsc     add (or replace) the image
  tools/add_ctl.py X0X.fwsc --remove -o X0X-plain.fwsc   an empty Bluetooth block again
  tools/add_ctl.py ctl.img --check                       check an image only

The image is built by ble/ctl/build_ctl.sh from the reviewed JieLi inputs. Qualified NIGHTLY
packages include their matching controller; tools/package_nightly.py checks and composes that
package without making a distribution-rights decision. It goes into the package's Bluetooth
block (flash 0x8B000..0x9AFFF, inside the app area,
encrypted like the rest of it); the app.bin and app area checksums and the package's own CRCs are
written again. Everything else in the package stays byte for byte. Every X0X update writes this
block too, so include the image in each new NIGHTLY package. Legacy unbundled packages still
need an explicit matching image (tools/fm1_install.py --ctl or the web installer file field).
web/fm1pkg.js insertCtl does the same composition in the browser
(tests/ble_pkg_test.py checks that both give the same bytes).
"""
import argparse
import os
import struct
import sys
import tempfile
import zlib
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import fm1pkg_make as pm  # noqa: E402

BLOCKS, BLK, KEEP = 20, 0x30, 0x2F
CODE_MAX = 0x86EE0                       # app.bin offset of the Bluetooth block (fm1pkg_make BOARDS smk37)
BLE_SIZE = 0x10000
SMK_SLOT = 0x97400
XIP = 0x02087000
HDR_MAGIC, HDR_VERSION, ABI, HDR_SIZE = 0x43423058, 2, 2, 68
RAM_LO, RAM_HI = 0x01C01800, 0x01C07F00
IRQ_TAKEN = {1, 11, 16, 20, 24, 37, 62, 63}    # X0X's own interrupts on core 0 (src/ble/ble_glue.h)


class PkgError(Exception):
    pass


def ctl_problem(img):
    """what is wrong with a controller image (as src/ble/ble_glue.h ble_img_check sees it), or None"""
    if len(img) < HDR_SIZE:
        return "too short for a controller image"
    if len(img) > BLE_SIZE:
        return f"{len(img)} B: larger than the 64 KiB Bluetooth block"
    (magic, ver, hsize, _fw, size, crc, abi, init, poll, slo, shi, rlo, rhi, ilo, ihi) = \
        struct.unpack_from("<IHHIIIIIIIIIIII", img, 0)
    if magic != HDR_MAGIC:
        return "not a controller image (no X0BC header)"
    if ver != HDR_VERSION or hsize != HDR_SIZE or abi != ABI:
        return f"header version {ver}, size {hsize}, ABI {abi}: this X0X takes version {HDR_VERSION}, ABI {ABI}"
    if not HDR_SIZE <= size <= len(img):
        return f"image size {size} does not fit the file ({len(img)} B)"
    if any(byte != 0xFF for byte in img[size:]):
        return "non-erased bytes after the controller image declared size"
    if zlib.crc32(img[:16] + b"\0\0\0\0" + img[20:size]) != crc:
        return "CRC mismatch (a damaged file)"
    for name, a in (("init", init), ("poll", poll)):
        if not XIP + HDR_SIZE <= a < XIP + size:
            return f"{name} entry {a:#x} is not inside the image"
    if not (RAM_LO <= rlo < rhi <= RAM_HI):
        return f"RAM {rlo:#x}..{rhi:#x} is not inside {RAM_LO:#x}..{RAM_HI:#x}"
    if not (rlo <= slo < shi <= rhi) or shi & 7:
        return "interrupt stack outside its RAM"
    taken = sorted(n for n in IRQ_TAKEN if (ilo if n < 32 else ihi) >> (n & 31) & 1)
    if taken:
        return f"it routes interrupts X0X uses: {taken}"
    return None


def _dec(b):
    x = bytearray(b)
    pm.enc(x, 0, len(x))
    return x


def logical_of(raw):
    """(product, logical image) of a 20-block .fwsc"""
    if len(raw) < BLOCKS * BLK:
        raise PkgError("not an X0X package (too short)")
    product = "".join(chr((m - i - 1) & 0xFF) for i in range(BLOCKS) if (m := raw[i * BLK + KEEP]) != 0x7D)
    out = bytearray()
    for i in range(BLOCKS):
        out += raw[i * BLK:i * BLK + KEEP]
    return product, out + raw[BLOCKS * BLK:]


def fwsc_of(logical, product):
    raw = bytearray()
    for i in range(BLOCKS):
        ch = product[i] if i < len(product) else None
        raw += logical[i * KEEP:(i + 1) * KEEP] + bytes([(ord(ch) + i + 1) & 0xFF if ch else 0x7D])
    return bytes(raw + logical[BLOCKS * KEEP:])


class NoBlock(PkgError):
    """the package has no free Bluetooth block: its app code runs into 0x8B000 (an X0X before 7054cc5)"""


def _open(logical):
    """the decrypted app area of a logical image, and what _close needs to put it back"""
    lg = bytearray(logical)
    hdr = _dec(lg[0:0x40])
    if pm.crc16(hdr[2:0x40]) != struct.unpack_from("<H", hdr, 0)[0]:
        raise PkgError("package header CRC")
    count = struct.unpack_from("<H", hdr, 8)[0]
    lst = bytearray(lg[0x40:0x40 + 0x50 * count])
    if pm.crc16(lst) != struct.unpack_from("<H", hdr, 2)[0]:
        raise PkgError("package file list CRC")
    for i in range(count):
        e = _dec(lst[i * 0x50:(i + 1) * 0x50])
        if struct.unpack_from("<H", e, 0)[0] == 0:
            break
    else:
        raise PkgError("no flash.bin in the package")
    off, size = struct.unpack_from("<II", e, 8)
    flash = bytearray(lg[off:off + size])
    if pm.crc16(flash) != struct.unpack_from("<H", e, 4)[0]:
        raise PkgError("flash.bin CRC")
    # the chip key: isd_config.ini, the second entry of the flash head
    isd = _dec(flash[64:96])
    if not isd[16:32].startswith(b"isd_config.ini"):
        raise PkgError("no isd_config.ini in the flash head")
    key = pm.chipkey_decode(bytes(flash[struct.unpack_from("<I", isd, 4)[0]:][:32]))
    head = bytearray(flash[0x4000:0x4020])
    pm.sfc(head, 0, 32, 0, key)
    xip, blk = struct.unpack_from("<II", head, 4)
    if xip != 0x02000120 or not head[16:32].startswith(b"app_area_head"):
        raise PkgError("no app area head")
    n = (blk + 31) & ~31
    area = bytearray(flash[0x4000:0x4000 + n])
    pm.sfc(area, 0, n, 0, key)
    ent = area[32:64]
    a_off, slot = struct.unpack_from("<II", ent, 4)
    if not ent[16:32].startswith(b"app.bin") or a_off != 0x120:
        raise PkgError("no app.bin in the app area")
    if slot != SMK_SLOT:
        raise PkgError("not an SMK-37 package (the Bluetooth block is on the SMK-37 only)")
    return dict(lg=lg, hdr=hdr, lst=lst, i=i, e=e, off=off, size=size, flash=flash, key=key, n=n, area=area,
                blk=blk, slot=slot)


def _close(st):
    """the logical image of an _open state: app area and package checksums written again"""
    area, flash, e, hdr, lst, lg = st["area"], st["flash"], st["e"], st["hdr"], st["lst"], st["lg"]
    slot, blk, n, key, i = st["slot"], st["blk"], st["n"], st["key"], st["i"]
    struct.pack_into("<H", area, 34, pm.crc16(area[0x120:0x120 + slot]))   # app.bin dcrc, then its CRC
    struct.pack_into("<H", area, 32, pm.crc16(area[34:64]))
    struct.pack_into("<H", area, 2, pm.crc16(area[32:blk]))                # app_area_head dcrc, CRC
    struct.pack_into("<H", area, 0, pm.crc16(area[2:32]))
    pm.sfc(area, 0, n, 0, key)
    flash[0x4000:0x4000 + n] = area
    lg[st["off"]:st["off"] + st["size"]] = flash
    struct.pack_into("<H", e, 4, pm.crc16(flash))
    pm.enc(e, 0, 0x50)
    lst[i * 0x50:(i + 1) * 0x50] = e
    struct.pack_into("<H", hdr, 2, pm.crc16(lst))
    struct.pack_into("<H", hdr, 0, pm.crc16(hdr[2:0x40]))
    pm.enc(hdr, 0, 0x40)
    lg[0:0x40] = hdr
    lg[0x40:0x40 + len(lst)] = lst
    return bytes(lg)


def block_free(st):
    """True if the Bluetooth block of an _open state belongs to the block: empty (0xFF), a controller image
    (X0BC header), or a package with the slot map record (7054cc5 and later: the build limits the app code)"""
    lo = 0x120 + CODE_MAX
    b = st["area"][lo:lo + BLE_SIZE]
    return (b == b"\xFF" * BLE_SIZE or struct.unpack_from("<I", b, 0)[0] == HDR_MAGIC
            or pm.slotmap_of(st["lg"]) is not None)


NO_BLOCK = ("the app code of this package goes into the Bluetooth block (flash 0x8B000-0x9AFFF). "
            "It is an X0X from before the block. It takes no controller image (install it without one)")


def check_block(logical):
    """raise NoBlock if the package cannot take a controller image (PkgError if it is not an SMK-37 package)"""
    if not block_free(_open(logical)):
        raise NoBlock(NO_BLOCK)


def controller_of(logical):
    """Return the valid controller embedded in a package, or None for an empty controller block.

    NoBlock still identifies old packages whose application owns this range.  Other non-empty
    contents are refused: a current package must not silently install an unknown controller block.
    """
    st = _open(logical)
    if not block_free(st):
        raise NoBlock(NO_BLOCK)
    lo = 0x120 + CODE_MAX
    block = bytes(st["area"][lo:lo + BLE_SIZE])
    if block == b"\xFF" * BLE_SIZE:
        return None
    if len(block) < HDR_SIZE or struct.unpack_from("<I", block, 0)[0] != HDR_MAGIC:
        raise PkgError("the Bluetooth block is neither empty nor a controller image")
    size = struct.unpack_from("<I", block, 12)[0]
    ctl = block[:size]
    if (problem := ctl_problem(ctl)):
        raise PkgError(f"embedded controller image: {problem}")
    if block[size:] != b"\xFF" * (BLE_SIZE - size):
        raise PkgError("embedded controller image has non-erased bytes after its declared size")
    return ctl


def insert(logical, ctl):
    """the logical image with ctl (bytes, or None for an empty block) in the Bluetooth block"""
    st = _open(logical)
    if not block_free(st):
        raise NoBlock(NO_BLOCK)       # writing the block would overwrite app code: the unit would not start
    lo = 0x120 + CODE_MAX
    block = bytearray(b"\xFF" * BLE_SIZE)
    if ctl is not None:
        block[:len(ctl)] = ctl
    st["area"][lo:lo + BLE_SIZE] = block
    return _close(st)


def add(raw, ctl):
    """a .fwsc with ctl (None: an empty block) -> the new .fwsc"""
    if ctl is not None and (p := ctl_problem(ctl)):
        raise PkgError(f"ctl.img: {p}")
    product, logical = logical_of(raw)
    return fwsc_of(insert(logical, ctl), product)


def write_package(path, data):
    """Durably publish a checked package without partially replacing an existing one.

    The output can intentionally equal the input path.  No existing package is
    truncated until the full new bytes have been written, synced and read back.
    """
    path = Path(path)
    fd, tmp = tempfile.mkstemp(prefix=f".{path.name}.", suffix=".tmp", dir=path.parent)
    try:
        with os.fdopen(fd, "wb") as f:
            fd = -1
            f.write(data)
            f.flush()
            os.fsync(f.fileno())
        if Path(tmp).read_bytes() != data:
            raise OSError("temporary controller package did not read back")
        os.replace(tmp, path)  # same directory / filesystem: atomic replacement
        try:
            dirfd = os.open(path.parent, os.O_RDONLY)
            try:
                os.fsync(dirfd)
            finally:
                os.close(dirfd)
        except OSError as exc:
            raise OSError(
                f"{path}: replacement completed, but directory finalization failed; "
                "durability is uncertain. Inspect the output before retrying: "
                f"{exc}"
            ) from exc
    finally:
        if fd >= 0:
            os.close(fd)
        if os.path.exists(tmp):
            os.unlink(tmp)


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("package", type=Path, help="X0X package for the SMK-37 (.fwsc), or ctl.img with --check")
    ap.add_argument("ctl", type=Path, nargs="?", help="the controller image (ble/ctl/out/ctl.img)")
    ap.add_argument("-o", "--out", type=Path, help="the new package")
    ap.add_argument("--remove", action="store_true", help="write an empty Bluetooth block instead")
    ap.add_argument("--check", action="store_true", help="only check a ctl.img")
    a = ap.parse_args(argv)
    try:
        if a.check:
            p = ctl_problem(a.package.read_bytes())
            print(f"{a.package}: {p or 'ok'}")
            return 1 if p else 0
        if bool(a.ctl) == bool(a.remove) or not a.out:
            ap.error("give PACKAGE CTL -o OUT, or PACKAGE --remove -o OUT")
        out = add(a.package.read_bytes(), None if a.remove else a.ctl.read_bytes())
        write_package(a.out, out)
        print(f"{a.out}: {'empty Bluetooth block' if a.remove else 'with ' + str(a.ctl)}")
        return 0
    except (OSError, PkgError) as e:
        print(f"add_ctl: {e}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
