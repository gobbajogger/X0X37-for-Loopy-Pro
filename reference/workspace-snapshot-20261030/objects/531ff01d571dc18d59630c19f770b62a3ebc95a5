#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Private, offline compatibility evidence; no device or publication operations.

Reads the exact pinned source and two existing app-only packages. All generated
flash images are temporary synthetic buffers, never device captures or outputs.
No network fetch, firmware build, default package change, or bootloader delivery.
"""
import argparse
import hashlib
import importlib.util
import io
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
from types import SimpleNamespace

HERE = Path(__file__).resolve().parent
TREE = "025b3e64f43634ade0928af7b2315336cc391e0f"
PACKAGES = {
    "pro": ("x0x-smk37.fwsc", "c0ce49794cb5692c4c9ab65552a8193e445945f06a5488a24b769b9e9172484f"),
    "elite": ("x0x-smk37-elite.fwsc", "67dcf8be40279e8b1e44224b3c1c9b76f1b425837b7ce634ae0ecd41d5c3f172"),
}
CHECKS = []

def checked(condition, message):
    if not condition:
        raise AssertionError(message)

def blob_hash(raw):
    return hashlib.sha1(b"blob " + str(len(raw)).encode() + b"\0" + raw).hexdigest()

def import_file(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module

def pass_check(label):
    CHECKS.append(label)
    print("PASS " + label, flush=True)

def expected_error(call, label):
    try:
        call()
    except ValueError:
        pass_check(label)
    else:
        raise AssertionError("expected refusal: " + label)

def valid_config():
    # Synthetic prefix and minimal required TLVs; not a unit configuration.
    data = bytearray(34)
    for name, value in (("OSC", b"OSC0"), ("OSC_FREQ", struct.pack("<I", 24000000)),
                        ("SYS_CLK", struct.pack("<I", 360000000)),
                        ("HSB_DIV", struct.pack("<I", 1)), ("LSB_DIV", struct.pack("<I", 2))):
        data += bytes([len(value)]) + name.encode() + b"\0" + value
    return bytes(data + b"\0")

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--repo", required=True, type=Path, help="Clean exact-tree firmware checkout; read only")
    ap.add_argument("--packages", required=True, type=Path, help="Existing smk37-pad-diag-evidence directory")
    ap.add_argument("--upstream", type=Path, default=HERE / ".inputs/upstream")
    ap.add_argument("--sanitize", action="store_true")
    ap.add_argument("--receipt", type=Path, help="Nonsecret JSON output; no firmware bytes")
    args = ap.parse_args()
    if not __debug__:
        raise SystemExit("Python optimization is forbidden for this evidence runner")
    sys.dont_write_bytecode = True
    repo, upstream = args.repo.resolve(), args.upstream.resolve()
    git = lambda *a: subprocess.check_output(["git", "-C", str(repo), *a], text=True).strip()
    checked(git("rev-parse", "HEAD^{tree}") == TREE, "wrong firmware source tree")
    checked(not git("status", "--porcelain"), "firmware checkout is not clean")
    manifest = json.loads((HERE / "upstream-manifest.json").read_text())
    for path, pin in manifest["files"].items():
        checked(blob_hash((upstream / path).read_bytes()) == pin["git_blob"], "upstream bytes differ: " + path)
    pass_check("exact firmware tree and upstream source blob pins")
    # These modules contain pure package/head helpers. No backend is instantiated.
    sys.path.insert(0, str(repo / "tools"))
    import add_ctl
    import fm1pkg_make as pm
    import fm1_install
    import lz4blk
    bank = import_file("mboot_bankcb_offline", upstream / "tools/bankcb.py")
    source_hashes = {p.name: hashlib.sha256(p.read_bytes()).hexdigest()
                     for p in (HERE / "run_offline.py", HERE / "compat_core_check.c", HERE / "upstream-manifest.json")}
    with tempfile.TemporaryDirectory(prefix="mboot-offline-") as temp:
        work = Path(temp)
        binary = work / "compat-core-check"
        command = ["cc", "-std=c11", "-O1", "-g", "-Wall", "-Wextra", "-Werror",
                   "-I" + str(upstream / "include"), str(HERE / "compat_core_check.c"),
                   str(upstream / "src/core.c"), str(upstream / "src/contract.c"), "-o", str(binary)]
        if args.sanitize:
            command[1:1] = ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
        subprocess.run(command, check=True)
        native_env = dict(os.environ)
        if args.sanitize:
            # This executor uses ptrace; LSan cannot attach. ASan/UBSan remain on.
            # This is not a leak-check PASS and is explicitly recorded below.
            native_env["ASAN_OPTIONS"] = "detect_leaks=0:halt_on_error=1"
            native_env["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=1"
        def native(*items):
            subprocess.run([str(binary), *map(str, items)], check=True, env=native_env)
        def put(name, data):
            path = work / name; path.write_bytes(data); return path
        native("synthetic")
        pass_check("native synthetic calibration preservation and ABI suite")
        good_cfg = valid_config()
        native("config", put("valid-config", good_cfg), 1)
        native("config", put("truncated-config", good_cfg[:-5]), 0)
        pass_check("required configuration TLVs accepted; truncation refused")
        # A synthetic tagged container, not executable firmware or a release.
        payload = bytearray(240)
        payload[0x40:0x48] = b"AC79BLID"
        payload[0x48:0x50] = b"smk37\0\0\0"
        payload[0x50:0x60] = b"OFFLINE-ONLY\0\0\0\0"
        container = bank.pack(bytes(payload), "smk37")
        container_path = put("synthetic-boot-container", container)
        expected_error(lambda: bank.validate(container, "fm1"), "wrong-board synthetic container refused")
        bad = bytearray(container); bad[-1] ^= 1
        expected_error(lambda: bank.validate(bad, "smk37"), "damaged synthetic container CRC refused")
        for model, (filename, digest) in PACKAGES.items():
            raw = (args.packages / ("official-app-only-" + model) / filename).read_bytes()
            checked(hashlib.sha256(raw).hexdigest() == digest, "wrong package bytes: " + model)
            product, logical = add_ctl.logical_of(raw)
            checked(product == "SMK-37 " + model.capitalize() + "_900", "wrong model identity")
            state = add_ctl._open(logical)
            full = bytearray(b"\xff" * 0x100000)
            full[:len(state["flash"])] = state["flash"]
            key = state["key"]
            native("app", put(model + "-app", full), key, 1)
            corrupted = bytearray(full); corrupted[0x4120 + 64] ^= 1
            native("app", put(model + "-bad-crc", corrupted), key, 2)
            corrupted = bytearray(full); corrupted[0x4000] ^= 1
            native("app", put(model + "-bad-header", corrupted), key, 0)
            native("app", work / (model + "-app"), key ^ 1, 0)
            pass_check(model + ": actual app parses; bad CRC/header/key refused")
            isd = add_ctl._dec(full[64:96]); address, size = struct.unpack_from("<II", isd, 4)
            native("config", put(model + "-package-config", full[address:address+size]), 0)
            pass_check(model + ": generated package config is not a valid m-bootloader boot config")
            # Prove the unmodified local head guard still refuses an altered head.
            ref = put(model + "-same-head", full[:0x4000])
            checked(fm1_install.head_compare(logical, ref)[0], "matching head refused")
            changed_head = bytearray(full[:0x4000]); changed_head[0xa0] ^= 1
            ref = put(model + "-different-head", changed_head)
            capture = io.StringIO()
            code = fm1_install.head_guard(SimpleNamespace(package=str(args.packages / ("official-app-only-" + model) / filename),
                                                        force=False, head_ref=str(ref)), out=capture, env={})
            checked(code == 11 and "REFUSED" in capture.getvalue(), "changed head was not refused")
            pass_check(model + ": unchanged installer head guard accepts same/refuses different head")
            # A generated SDK package head is not a substitute for a unit head.
            expected_error(lambda: bank.install(bytes(full), container), model + ": synthetic SDK boot allocation rejected by upstream transform")
            native("head", work / (model + "-app"), container_path, work / (model + "-app"), 0)
            # Build a valid synthetic old head, preserving all non-boot allocations.
            synthetic = bytearray(full)
            synthetic[64:96] = bank.enc(pm.entry(pm.crc16(good_cfg), 0x38d0, len(good_cfg), 2, 0x80, 0, "isd_config.ini"))
            synthetic[0x38d0:0x38d0+len(good_cfg)] = good_cfg
            synthetic[32:64] = bank.enc(pm.entry(0xffff, 0xa0, 0x3830, 0, 0, 0, "uboot.boot"))
            after = bank.install(bytes(synthetic), container)
            checked(after[:32] == synthetic[:32] and after[64:0xa0] == synthetic[64:0xa0]
                    and after[0x38d0:] == synthetic[0x38d0:], "synthetic transform changed non-boot bytes")
            checked(bank.extract(after) == container, "synthetic boot transform did not round-trip")
            native("head", put(model + "-synthetic-head", synthetic), container_path,
                   put(model + "-expected-head", after), 1)
            unsafe = bytearray(synthetic)
            unsafe[64:96] = bank.enc(pm.entry(pm.crc16(good_cfg), 0x100, len(good_cfg), 2, 0x80, 0, "isd_config.ini"))
            unsafe_path = put(model + "-overlap-head", unsafe)
            native("head", unsafe_path, container_path, unsafe_path, 0)
            unsafe = bytearray(synthetic); unsafe[64] ^= 1
            unsafe_path = put(model + "-corrupt-head-record", unsafe)
            native("head", unsafe_path, container_path, unsafe_path, 0)
            pass_check(model + ": synthetic boot transform preserves config, app, data, calibration and key/MAC allocations")
            # Stage the exact existing Felucca OTA as ota_stage() does: outer wrapper
            # is replaced, compressed body copied unchanged; never execute the bytes.
            entries = [add_ctl._dec(state["lst"][i:i+80]) for i in range(0, len(state["lst"]), 80)]
            ota_entry = next(e for e in entries if struct.unpack_from("<H", e)[0] == 100)
            off, size = struct.unpack_from("<II", ota_entry, 8); ota = bytes(logical[off:off+size])
            checked(len(ota) == 7339, "unexpected OTA fixture size")
            image = bytearray(); pos = 64
            while pos < len(ota):
                compressed, expanded = struct.unpack_from("<II", ota, pos); pos += 8
                block = lz4blk.decompress(ota[pos:pos+compressed])
                checked(len(block) == expanded, "fixture decompression length")
                image += block; pos += compressed
            checked(len(image) == 8420, "unexpected decoded OTA size")
            full[0xbb000:0xbb000+32] = pm.entry(0xffff, 0xbb000, len(ota), 0x41, 0xff, 0xffff, "LOADER.BIN")
            full[0xbb020:0xbb000+len(ota)] = ota[32:]
            request = bytearray(80)
            struct.pack_into("<HHH", request, 2, 0x5a0d, 0x5a01, 0x5441)
            struct.pack_into("<I", request, 72, 0xbb000)
            struct.pack_into("<H", request, 0, pm.crc16(request[2:]))
            full[0xbff00:0xbff50] = request
            request_path = put(model + "-request", request)
            image_path = put(model + "-decoded-ota", image)
            native("updater", put(model + "-staged", full), request_path, image_path, 1)
            broken = bytearray(full); broken[0xbb000] ^= 1
            native("updater", put(model + "-bad-outer", broken), request_path, image_path, 0)
            broken = bytearray(full)
            struct.pack_into("<I", broken, 0xbb028, 0x01c0a802)
            struct.pack_into("<H", broken, 0xbb020, pm.crc16(broken[0xbb022:0xbb040]))
            native("updater", put(model + "-bad-load", broken), request_path, image_path, 0)
            broken = bytearray(full)
            struct.pack_into("<I", broken, 0xbb024, 0x5b1d)
            struct.pack_into("<H", broken, 0xbb020, pm.crc16(broken[0xbb022:0xbb040]))
            native("updater", put(model + "-oversize", broken), request_path, image_path, 0)
            bad_request = bytearray(request); bad_request[10] ^= 1
            native("updater", work / (model + "-staged"), put(model + "-bad-request", bad_request), image_path, 0)
            pass_check(model + ": exact OTA decode/flash-request scan and corrupt header/load/size/request negatives")
    receipt = {"status": "OFFLINE_COMPATIBILITY_ONLY", "firmware_git_tree": TREE,
               "upstream_commit": manifest["commit"], "package_sha256": {k:v[1] for k,v in PACKAGES.items()},
               "sanitize": args.sanitize,
               "leak_sanitizer": "NOT_RUN: ptrace executor incompatibility; detect_leaks=0" if args.sanitize else "NOT_RUN",
               "checks": CHECKS, "source_sha256": source_hashes,
               "limits": ["No firmware or bootloader installed or executed on a target.",
                          "No platform/GPIO/SPI/USB backend compiled or opened.",
                          "No test of physical boot, timing, ROM behavior or power interruption.",
                          "Synthetic boot transform is not a deployable image; it is deleted with all temporary bytes.",
                          "Known package config and adoption-gate gaps remain; no default generation or guard changes."]}
    if args.receipt:
        args.receipt.parent.mkdir(parents=True, exist_ok=True)
        args.receipt.write_text(json.dumps(receipt, indent=2) + "\n")
    print(json.dumps({"status": receipt["status"], "checks": len(CHECKS), "sanitize": args.sanitize}))

if __name__ == "__main__":
    main()
