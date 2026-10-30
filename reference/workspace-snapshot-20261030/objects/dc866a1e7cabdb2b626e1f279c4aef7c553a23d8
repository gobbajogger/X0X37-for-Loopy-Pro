#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""tools/add_ctl.py and web/fm1pkg.js insertCtl on an SMK-37 package (build/x0x-smk37.fwsc; skipped
without a build): the image lands in the Bluetooth block, every checksum is right again, nothing else
changes, --remove gives the package back byte for byte, Python and JavaScript give the same bytes,
and a damaged or unsuitable ctl.img is refused. Also fm1_install.py's explicit-controller guard."""
import argparse
import io
import json
import os
import shutil
import struct
import subprocess
import sys
import tempfile
import zlib
from pathlib import Path

SRC = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SRC / "tools"))
import add_ctl  # noqa: E402
import fm1_install as fi  # noqa: E402
import fm1pkg_make as pm  # noqa: E402

fails = 0


def ok(cond, what):
    global fails
    if not cond:
        fails += 1
        print(f"FAIL {what}")


def make_ctl(size=40000, **over):
    h = dict(magic=0x43423058, ver=2, hsize=68, fw=0x20000, size=size, crc=0, abi=2,
             init=0x02087000 + 0xD8, poll=0x02087000 + 0xF0, slo=0x01C06E80, shi=0x01C07680,
             rlo=0x01C01800, rhi=0x01C07680, ilo=(1 << 29) | (1 << 31), ihi=(1 << 8) | (1 << 13))
    h.update(over)
    img = bytearray(size)
    for i in range(68, size):
        img[i] = (i * 13) & 0xFF
    struct.pack_into("<IHHIIIIIIIIIIII", img, 0, h["magic"], h["ver"], h["hsize"], h["fw"], h["size"], 0, h["abi"],
                     h["init"], h["poll"], h["slo"], h["shi"], h["rlo"], h["rhi"], h["ilo"], h["ihi"])
    struct.pack_into("<I", img, 16, zlib.crc32(img[:16] + b"\0" * 4 + img[20:h["size"]]) if not over.get("badcrc") else 1)
    return bytes(img)


def plain_area(logical):
    """the decrypted app area of a logical image, and its block size"""
    hdr = add_ctl._dec(logical[0:0x40])
    count = struct.unpack_from("<H", hdr, 8)[0]
    for i in range(count):
        e = add_ctl._dec(logical[0x40 + i * 0x50:0x40 + (i + 1) * 0x50])
        if struct.unpack_from("<H", e, 0)[0] == 0:
            break
    off, size = struct.unpack_from("<II", e, 8)
    flash = logical[off:off + size]
    isd = add_ctl._dec(flash[64:96])
    key = pm.chipkey_decode(bytes(flash[struct.unpack_from("<I", isd, 4)[0]:][:32]))
    head = bytearray(flash[0x4000:0x4020])
    pm.sfc(head, 0, 32, 0, key)
    blk = struct.unpack_from("<I", head, 8)[0]
    n = (blk + 31) & ~31
    area = bytearray(flash[0x4000:0x4000 + n])
    pm.sfc(area, 0, n, 0, key)
    return area, blk, flash


def main():
    pkg_path = Path(os.environ.get("X0X_SMK_PKG", SRC / "build" / "x0x-smk37.fwsc"))
    if not pkg_path.is_file():
        print("ble_pkg_test: skipped (no build/x0x-smk37.fwsc)")
        return 0
    raw = pkg_path.read_bytes()
    ctl = make_ctl()
    # the image checks
    ok(add_ctl.ctl_problem(ctl) is None, "a good image passes")
    ok(add_ctl.ctl_problem(ctl + b"\xff" * 44) is None, "erased controller tail is permitted")
    for suffix in (b"\x00", b"\x80", b"\xff\x00", b"\x00" * 44):
        ok(add_ctl.ctl_problem(ctl + suffix) is not None, "non-erased controller tail refused")
    for what, bad in (("CRC", make_ctl(badcrc=True)), ("ABI", make_ctl(abi=1)), ("init", make_ctl(init=0x02000000)),
                      ("RAM", make_ctl(rhi=0x01C08000)), ("TIMER5", make_ctl(ihi=1 << 31)), ("ALNK0", make_ctl(ilo=1 << 11)),
                      ("magic", b"\xFF" * 100), ("big", make_ctl(size=0x10001)), ("short", b"X0BC")):
        ok(add_ctl.ctl_problem(bad) is not None, f"refused: {what}")
    # in, and out again
    out = add_ctl.add(raw, ctl)
    ok(add_ctl.add(raw, ctl + b"\xff" * 44) == out, "erased padding does not change package bytes")
    try:
        add_ctl.add(raw, ctl + b"\x00")
        ok(False, "non-erased controller tail rejected before package write")
    except add_ctl.PkgError:
        pass
    ok(len(out) == len(raw), "same length")
    product, logical = add_ctl.logical_of(out)
    ok(product.startswith("SMK-37 "), "identity kept")
    ok(fi.package_damage(logical) is None, f"package CRCs: {fi.package_damage(logical)}")
    area, blk, flash = plain_area(logical)
    lo = 0x120 + add_ctl.CODE_MAX
    ok(area[lo:lo + len(ctl)] == ctl, "the image in the Bluetooth block")
    ok(area[lo + len(ctl):lo + 0x10000] == b"\xFF" * (0x10000 - len(ctl)), "the rest of the block erased")
    ok(struct.unpack_from("<H", area, 34)[0] == pm.crc16(area[0x120:0x120 + add_ctl.SMK_SLOT]), "app.bin CRC")
    ok(struct.unpack_from("<H", area, 32)[0] == pm.crc16(area[34:64]), "app.bin entry CRC")
    ok(struct.unpack_from("<H", area, 2)[0] == pm.crc16(area[32:blk]), "app area CRC")
    ok(struct.unpack_from("<H", area, 0)[0] == pm.crc16(area[2:32]), "app area head CRC")
    _, logical0 = add_ctl.logical_of(raw)
    area0, _, flash0 = plain_area(logical0)
    ok(area0[0x120:lo] == area[0x120:lo], "the app before the block unchanged")
    ok(area0[lo + 0x10000:] == area[lo + 0x10000:], "after the block unchanged (cfg_tool, the cfg)")
    ok(area0[64:0x120] == area[64:0x120], "the other entries unchanged")
    ok(flash0[:0x4000] == flash[:0x4000] and flash0[0x4000 + len(area):] == flash[0x4000 + len(area):],
       "flash outside the app area unchanged (the slot map too)")
    ok(add_ctl.add(out, None) == raw, "--remove gives the package back byte for byte")
    ok(add_ctl.add(raw, None) == raw, "an empty block on a package without one: no change")
    ok(add_ctl.add(add_ctl.add(out, make_ctl(60000)), ctl) == out, "replace, and back")
    try:
        add_ctl.add(raw, make_ctl(badcrc=True))
        ok(False, "a damaged image is refused")
    except add_ctl.PkgError:
        pass
    fm1 = SRC / "build" / "x0x.fwsc"
    if fm1.is_file():
        try:
            add_ctl.add(fm1.read_bytes(), ctl)
            ok(False, "an FM-1 package is refused")
        except add_ctl.PkgError:
            pass
    # a package with app code in the Bluetooth block (an X0X before 7054cc5, as 1.8-apc40): refused,
    # also --remove. The same package with the slot map record (the build limits the code since then): accepted.
    lo_blk = 0x120 + add_ctl.CODE_MAX
    st = add_ctl._open(logical0)
    st["area"][lo_blk:lo_blk + 38011] = bytes((i * 7 + 3) & 0xFF for i in range(38011))
    with_map = add_ctl._close(st)
    st = add_ctl._open(with_map)
    st["flash"][pm.SLOTMAP_OFF:pm.SLOTMAP_OFF + 16] = b"\xFF" * 16
    old_code = add_ctl._close(st)
    ok(pm.slotmap_of(old_code) is None and fi.package_damage(old_code) is None, "test package: no slot map, CRCs right")
    for what, c in (("ctl.img", ctl), ("--remove", None)):
        try:
            add_ctl.insert(old_code, c)
            ok(False, f"code in the Bluetooth block: {what} refused")
        except add_ctl.NoBlock as e:
            ok("Bluetooth block" in str(e), f"code in the Bluetooth block: {what} refused with a clear message")
    old_fwsc = add_ctl.fwsc_of(old_code, product)
    try:
        add_ctl.add(old_fwsc, ctl)
        ok(False, "add_ctl.add refuses an old package")
    except add_ctl.NoBlock:
        pass
    add_ctl.insert(with_map, ctl)            # code in the block but the slot map record: accepted (no raise)
    add_ctl.check_block(logical0)
    # the user's 1.8-apc40 package from the main tree, when it is there (X0X_OLD_PKG overrides the path)
    real_old = Path(__import__("os").environ.get("X0X_OLD_PKG", SRC.parent / "x0x37" / "build" / "x0x-smk37-elite-1.8-apc40.fwsc"))
    if real_old.is_file():
        try:
            add_ctl.add(real_old.read_bytes(), ctl)
            ok(False, f"{real_old.name} refused")
        except add_ctl.NoBlock:
            pass
    # JavaScript: the same bytes, the same verdicts
    node = shutil.which("node")
    if node:
        with tempfile.TemporaryDirectory() as td:
            td = Path(td)
            (td / "pkg.fwsc").write_bytes(raw)
            (td / "old.fwsc").write_bytes(old_fwsc)
            (td / "map.fwsc").write_bytes(add_ctl.fwsc_of(with_map, product))
            (td / "ctl.img").write_bytes(ctl)
            bads = {"crc": make_ctl(badcrc=True), "irq": make_ctl(ihi=1 << 31), "ram": make_ctl(rlo=0x01C00000),
                    "tail": ctl + b"\x00", "tail_after_erase": ctl + b"\xff\x00"}
            for k, v in bads.items():
                (td / f"bad_{k}.img").write_bytes(v)
            js = f"""
import {{ readFileSync, writeFileSync }} from "fs";
import {{ logicalImage, insertCtl, ctlProblem, CTL_NO_BLOCK }} from "{(SRC / 'web' / 'fm1pkg.js').as_posix()}";
const d = "{td.as_posix()}";
const pkg = new Uint8Array(readFileSync(d + "/pkg.fwsc")), ctl = new Uint8Array(readFileSync(d + "/ctl.img"));
writeFileSync(d + "/js_with.bin", insertCtl(logicalImage(pkg), ctl));
writeFileSync(d + "/js_without.bin", insertCtl(insertCtl(logicalImage(pkg), ctl), null));
const v = {{ good: ctlProblem(ctl) }};
for (const k of ["crc", "irq", "ram", "tail", "tail_after_erase"]) v[k] = ctlProblem(new Uint8Array(readFileSync(d + "/bad_" + k + ".img")));
for (const k of ["old", "map"]) {{
  try {{ writeFileSync(d + "/js_" + k + ".bin", insertCtl(logicalImage(new Uint8Array(readFileSync(d + "/" + k + ".fwsc"))), ctl)); v[k] = null; }}
  catch (e) {{ v[k] = e.message; }}
}}
v.noblock = CTL_NO_BLOCK;
writeFileSync(d + "/js.json", JSON.stringify(v));
"""
            (td / "t.mjs").write_text(js)
            r = subprocess.run([node, str(td / "t.mjs")], capture_output=True, text=True)
            ok(r.returncode == 0, f"node: {r.stderr.strip()}")
            if r.returncode == 0:
                ok((td / "js_with.bin").read_bytes() == add_ctl.insert(logical0, ctl), "JS insertCtl = Python insert")
                ok((td / "js_without.bin").read_bytes() == bytes(logical0), "JS remove = the original")
                v = json.loads((td / "js.json").read_text())
                ok(v["good"] is None and all(v[k] for k in ("crc", "irq", "ram", "tail", "tail_after_erase")), f"JS ctlProblem verdicts {v}")
                ok(v["old"] == v["noblock"] == add_ctl.NO_BLOCK, f"JS insertCtl refuses code in the block: {v['old']}")
                ok(v["map"] is None and (td / "js_map.bin").read_bytes() == add_ctl.insert(with_map, ctl),
                   "JS insertCtl with the slot map record = Python insert")
    else:
        print("ble_pkg_test: node not found, JavaScript parity skipped")
    # fm1_install.py: current SMK packages need an explicit or embedded controller.
    with tempfile.TemporaryDirectory() as td:
        cache = Path(td) / "ctl.img"
        buf = io.StringIO()
        a = argparse.Namespace(ctl=None, no_ctl=False)
        cache.write_bytes(ctl)
        try:
            fi.with_ctl(a, "SMK-37 Pro_900", logical0, buf)
            ok(False, "empty SMK controller block is refused")
        except fi.InstallError as e:
            ok(e.code == "badpkg" and "empty" in str(e) and "--ctl" in str(e),
               "empty SMK controller block: clear refusal before device access")
        a.no_ctl = True
        try:
            fi.with_ctl(a, "SMK-37 Pro_900", logical0, buf)
            ok(False, "--no-ctl with an empty SMK controller block is refused")
        except fi.InstallError as e:
            ok(e.code == "badpkg" and "does not preserve" in str(e), "--no-ctl cannot erase the controller")
        class NoDeviceAccess:
            def input_names(self):
                raise AssertionError("controller guard reached the device")
        try:
            fi.run(argparse.Namespace(info=False, package=str(pkg_path), force=False, port=None, ctl=None,
                                      no_ctl=True, no_slots=False, backup=None, yes=True),
                   NoDeviceAccess(), io.StringIO(), lambda _p: True)
            ok(False, "empty SMK package run is refused")
        except fi.InstallError as e:
            ok(e.code == "badpkg" and "empty" in str(e), "empty --no-ctl run is refused before device access")
        a.no_ctl = False
        a.ctl = str(cache)
        ok(fi.with_ctl(a, "SMK-37 Pro_900", logical0, buf) == add_ctl.insert(logical0, ctl)
           and a.ctl_bytes == ctl, "explicit valid --ctl is added and selected for read-back")
        embedded = add_ctl.insert(logical0, ctl)
        b = argparse.Namespace(ctl=None, no_ctl=True)
        ok(fi.with_ctl(b, "SMK-37 Pro_900", embedded, buf) == embedded and b.ctl_bytes == ctl,
           "a valid embedded controller is preserved and selected for read-back")
        buf = io.StringIO()
        ok(fi.with_ctl(argparse.Namespace(ctl=None, no_ctl=True), "SMK-37 Elite_9010800", old_code, buf) == old_code
           and "not applicable" in buf.getvalue(),
           "an old package whose app owns the range remains installable without a controller")
        cache.write_bytes(ctl)
        try:
            fi.with_ctl(argparse.Namespace(ctl=str(cache), no_ctl=False), "SMK-37 Elite_9010800", old_code, buf)
            ok(False, "--ctl on an old package is refused")
        except fi.InstallError as e:
            ok(e.code == "badpkg" and "Bluetooth block" in str(e), "--ctl on an old package: badpkg, clear message")
        ok(fi.with_ctl(argparse.Namespace(ctl=None, no_ctl=False), "FM-1_900", b"x", buf) == b"x",
           "FM-1: the cache is not used")
        try:
            fi.with_ctl(argparse.Namespace(ctl=str(cache), no_ctl=False), "FM-1_900", b"x", buf)
            ok(False, "--ctl on an FM-1 package is refused")
        except fi.InstallError:
            pass
        (Path(td) / "bad.img").write_bytes(make_ctl(badcrc=True))
        try:
            fi.with_ctl(argparse.Namespace(ctl=str(Path(td) / "bad.img"), no_ctl=False), "SMK-37 Pro_900", logical0, buf)
            ok(False, "a damaged --ctl is refused")
        except fi.InstallError as e:
            ok(e.code == "badpkg", "badpkg")
    print(f"ble_pkg_test: {'ok' if not fails else str(fails) + ' failed'}")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
