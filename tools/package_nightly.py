#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Compose or inspect a model-specific NIGHTLY package with its matching controller.

  package_nightly.py APP.fwsc ctl.img --model elite --controller-sha256 HASH --controller-size SIZE -o COMPLETE.fwsc
  package_nightly.py COMPLETE.fwsc --check --model elite --controller-sha256 HASH --controller-size SIZE

The reviewed controller pin must be supplied explicitly. No cached image, empty-controller
fallback, hardware access, license approval or publication is implied. JSON on stdout records
the actual checked output bytes. Composition and durable output reuse tools/add_ctl.py.
"""
import argparse
import hashlib
import json
import re
import struct
import sys
import zlib
from pathlib import Path

import add_ctl
import fm1_install
import fm1pkg_make as pm
import release_alloc
from firmware_build_labels import canonical_version, release_display, version_of_product

MODELS = {"elite": "SMK-37 Elite", "pro": "SMK-37 Pro"}


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def _pin(expected_sha256, expected_size):
    if not isinstance(expected_sha256, str) or not re.fullmatch(r"[0-9a-f]{64}", expected_sha256):
        raise add_ctl.PkgError("controller SHA256 must be 64 lowercase hexadecimal characters")
    if type(expected_size) is not int or not add_ctl.HDR_SIZE <= expected_size <= add_ctl.BLE_SIZE:
        raise add_ctl.PkgError("controller size must be an exact image size between 68 and 65536 bytes")


def _embedded_version(product, model):
    """Legacy model_900 is unversioned. Any other identity must be the 9XXYYZZ codec."""
    if model not in MODELS:
        raise add_ctl.PkgError("NIGHTLY model must be elite or pro")
    if product == MODELS[model] + "_900":
        return None
    try:
        found = version_of_product(product, model)
    except (ValueError, TypeError, AttributeError) as exc:
        raise add_ctl.PkgError(f"package model {product!r} does not match {model}") from exc
    if found is None:
        raise add_ctl.PkgError(f"package model {product!r} does not match {model}")
    return found


def _require_allocated_version(version, ledger):
    """A numeric identity must already exist in the ledger. Legacy model_900 stays unversioned."""
    if version is None:
        return
    path = Path(ledger) if ledger is not None else Path("releases/openbeta/ledger.json")
    try:
        data = release_alloc._load(path)
    except release_alloc.ReleaseError as exc:
        raise add_ctl.PkgError(str(exc)) from exc
    if not any(item.get("version") == version for item in data["allocations"]):
        raise add_ctl.PkgError("firmware version is not allocated")


def _package(raw, model, expected_version=None, ledger=None):
    """Strict current NIGHTLY structure, without weakening the legacy composition path."""
    if model not in MODELS:
        raise add_ctl.PkgError("NIGHTLY model must be elite or pro")
    product, logical = add_ctl.logical_of(raw)
    version = _embedded_version(product, model)
    if expected_version is not None:
        try:
            expected_version = canonical_version(expected_version)
        except ValueError as exc:
            raise add_ctl.PkgError(str(exc)) from exc
        if version != expected_version:
            raise add_ctl.PkgError(
                f"package model {product!r} does not match firmware version {expected_version}")
    _require_allocated_version(version, ledger)
    if problem := fm1_install.package_damage(logical):
        raise add_ctl.PkgError(f"package integrity: {problem}")
    hdr = add_ctl._dec(logical[:0x40])
    if struct.unpack_from("<I", hdr, 4)[0] != len(logical):
        raise add_ctl.PkgError("package declared length does not match its bytes")
    count = struct.unpack_from("<H", hdr, 8)[0]
    if count != 2:
        raise add_ctl.PkgError("NIGHTLY package must contain exactly flash.bin and ota.bin")
    previous_end = 0x40 + count * 0x50
    flash = None
    for i, (typ, name) in enumerate(((0, b"flash.bin"), (100, b"ota.bin"))):
        entry = add_ctl._dec(logical[0x40 + i * 0x50:0x40 + (i + 1) * 0x50])
        if struct.unpack_from("<H", entry, 0)[0] != typ or entry[0x40:0x50].split(b"\0")[0] != name:
            raise add_ctl.PkgError("NIGHTLY package file layout is not flash.bin followed by ota.bin")
        off, size = struct.unpack_from("<II", entry, 8)
        if off < previous_end or off % 0x100 or size == 0:
            raise add_ctl.PkgError("NIGHTLY package file range is invalid")
        previous_end = off + size
        if typ == 0:
            flash = logical[off:off + size]
        if typ == 100 and fm1_install.LOADER_MARK not in logical[off:off + size]:
            raise add_ctl.PkgError("NIGHTLY package has no Felucca update loader")
    # Bound the legacy decoder's pointers before calling it. An otherwise CRC-consistent
    # malformed package must produce a package refusal, not index beyond a sliced buffer.
    if len(flash) != pm.BOARDS["smk37"][0]:
        raise add_ctl.PkgError("NIGHTLY SMK-37 flash size/layout is invalid")
    isd = add_ctl._dec(flash[64:96])
    if not isd[16:32].startswith(b"isd_config.ini"):
        raise add_ctl.PkgError("NIGHTLY package has no isd_config.ini in the flash head")
    key_off, key_size = struct.unpack_from("<II", isd, 4)
    if key_size < 32 or key_off < 0xA0 or key_off + key_size > 0x4000:
        raise add_ctl.PkgError("NIGHTLY chip-key range is outside the flash head")
    key = pm.chipkey_decode(bytes(flash[key_off:key_off + 32]))
    area_head = bytearray(flash[0x4000:0x4020])
    pm.sfc(area_head, 0, 32, 0, key)
    blk = struct.unpack_from("<I", area_head, 8)[0]
    area_size = (blk + 31) & ~31
    if blk < 0x120 + add_ctl.SMK_SLOT or area_size > pm.SLOTMAP_OFF - 0x4000:
        raise add_ctl.PkgError("NIGHTLY SMK-37 app area range is invalid")
    st = add_ctl._open(logical)
    area, blk = st["area"], st["blk"]
    if len(st["flash"]) != pm.BOARDS["smk37"][0] or blk < 0x120 + add_ctl.SMK_SLOT or len(area) != st["n"]:
        raise add_ctl.PkgError("NIGHTLY SMK-37 app area size/layout is invalid")
    if pm.crc16(area[2:32]) != struct.unpack_from("<H", area, 0)[0]:
        raise add_ctl.PkgError("app area header CRC")
    if pm.crc16(area[32:blk]) != struct.unpack_from("<H", area, 2)[0]:
        raise add_ctl.PkgError("app area data CRC")
    if pm.crc16(area[34:64]) != struct.unpack_from("<H", area, 32)[0]:
        raise add_ctl.PkgError("app.bin entry CRC")
    if pm.crc16(area[0x120:0x120 + add_ctl.SMK_SLOT]) != struct.unpack_from("<H", area, 34)[0]:
        raise add_ctl.PkgError("app.bin data CRC")
    expected_map = pm.slotmap_blob(pm.SLOTMAP["smk37"])
    if st["flash"][pm.SLOTMAP_OFF:pm.SLOTMAP_OFF + len(expected_map)] != expected_map:
        raise add_ctl.PkgError("NIGHTLY package has no matching current SMK-37 sample slot map")
    return product, logical, st


def inspect(raw, model, expected_sha256, expected_size, expected_version=None, ledger=None):
    """Return byte-derived metadata only after the embedded controller matches its review pin."""
    _pin(expected_sha256, expected_size)
    product, logical, _ = _package(raw, model, expected_version, ledger)
    version = _embedded_version(product, model)
    ctl = add_ctl.controller_of(logical)
    if ctl is None:
        raise add_ctl.PkgError("NIGHTLY package is missing its bundled Bluetooth controller")
    if len(ctl) != expected_size or sha256(ctl) != expected_sha256:
        raise add_ctl.PkgError("bundled Bluetooth controller size/SHA256 does not match the reviewed pin")
    block = ctl + b"\xff" * (add_ctl.BLE_SIZE - len(ctl))
    return {
        "schema": "x0x37-bundled-controller-v1",
        "model": model,
        "product": product,
        "firmware_version": version,
        "release_display": None if version is None else release_display(version),
        "size": len(raw),
        "sha256": sha256(raw),
        "controller_strategy": "bundled",
        "controller_sha256": sha256(ctl),
        "controller_size": len(ctl),
        "controller": {
            "strategy": "bundled", "bundled": True,
            "size": len(ctl), "sha256": sha256(ctl),
            "crc32": f"{struct.unpack_from('<I', ctl, 16)[0]:08x}",
            "block_size": add_ctl.BLE_SIZE,
            "block_crc32": f"{zlib.crc32(block):08x}",
            "flash_offset": pm.BLE_LO,
            "header_version": add_ctl.HDR_VERSION, "abi": add_ctl.ABI,
        },
    }


def compose(raw, ctl, model, expected_sha256, expected_size, expected_version=None, ledger=None):
    """Build one complete package; the exact controller is mandatory before any output write."""
    _pin(expected_sha256, expected_size)
    _package(raw, model, expected_version, ledger)
    if not isinstance(ctl, (bytes, bytearray)):
        raise add_ctl.PkgError("a matching Bluetooth controller image is required")
    if len(ctl) != expected_size or sha256(ctl) != expected_sha256:
        raise add_ctl.PkgError("Bluetooth controller size/SHA256 does not match the reviewed pin")
    if struct.unpack_from("<I", ctl, 12)[0] != len(ctl):
        raise add_ctl.PkgError("controller pin must cover declared image bytes without padding")
    if problem := add_ctl.ctl_problem(ctl):
        raise add_ctl.PkgError(f"Bluetooth controller: {problem}")
    output = add_ctl.add(raw, ctl)
    receipt = inspect(output, model, expected_sha256, expected_size, expected_version, ledger)
    if len(output) != len(raw) or add_ctl.add(output, None) != add_ctl.add(raw, None):
        raise add_ctl.PkgError("controller composition changed bytes outside the controller/checksum fields")
    return output, receipt


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("package", type=Path)
    ap.add_argument("controller", type=Path, nargs="?")
    ap.add_argument("--model", choices=MODELS, required=True)
    ap.add_argument("--controller-sha256", required=True)
    ap.add_argument("--controller-size", type=int, required=True)
    ap.add_argument("--firmware-version", help="require this canonical X.Y.Z machine identity")
    ap.add_argument("--ledger", type=Path, help="open-beta ledger; default releases/openbeta/ledger.json")
    ap.add_argument("--check", action="store_true", help="inspect an already complete package; write no package")
    ap.add_argument("-o", "--out", type=Path)
    args = ap.parse_args(argv)
    if args.check and (args.controller is not None or args.out is not None):
        ap.error("--check takes a complete package only; omit controller and output")
    if not args.check and (args.controller is None or args.out is None):
        ap.error("composition requires PACKAGE CONTROLLER -o OUTPUT; no Bluetooth-less NIGHTLY output")
    try:
        raw = args.package.read_bytes()
        if args.check:
            receipt = inspect(raw, args.model, args.controller_sha256, args.controller_size,
                              args.firmware_version, args.ledger)
        else:
            output, receipt = compose(raw, args.controller.read_bytes(), args.model,
                                      args.controller_sha256, args.controller_size, args.firmware_version,
                                      args.ledger)
            add_ctl.write_package(args.out, output)
            # Read the committed name too, not just the temporary file checked by write_package.
            committed = args.out.read_bytes()
            if committed != output:
                raise OSError("output replacement completed but its bytes changed before readback; inspect before retrying")
            inspect(committed, args.model, args.controller_sha256, args.controller_size,
                    args.firmware_version, args.ledger)
        print(json.dumps(receipt, indent=2, sort_keys=True))
        return 0
    except (OSError, add_ctl.PkgError, ValueError, struct.error) as exc:
        print(f"package_nightly: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
