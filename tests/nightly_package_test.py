#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""NIGHTLY composition and admission boundaries on synthetic Elite/Pro packages; no SDK/device.

--fixtures DIR writes synthetic test-only .fwsc files for browser/parser parity tests.
The files contain no executable real firmware or vendor controller and are never install candidates.
"""
import argparse
import contextlib
import io
import json
import struct
import sys
import tempfile
import zlib
from pathlib import Path
from unittest.mock import patch

SRC = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SRC / "tools"))
import add_ctl
import fm1pkg_make as pm
import package_nightly as nightly


def make_controller(size=4096, **overrides):
    values = dict(magic=add_ctl.HDR_MAGIC, version=2, header=68, firmware=0x20000,
                  size=size, crc=0, abi=2, init=add_ctl.XIP + 128, poll=add_ctl.XIP + 144,
                  stack_lo=0x01C06E80, stack_hi=0x01C07680, ram_lo=add_ctl.RAM_LO,
                  ram_hi=0x01C07680, irq_lo=1 << 29, irq_hi=1 << 8)
    values.update(overrides)
    data = bytearray((i * 13 + 7) & 255 for i in range(size))
    struct.pack_into("<IHHIIIIIIIIIIII", data, 0, *values.values())
    struct.pack_into("<I", data, 16, zlib.crc32(data[:16] + b"\0" * 4 + data[20:values["size"]]))
    return bytes(data)


def make_package(model, loader=True):
    """Real packer/layout/CRCs; synthetic SDK placeholders and non-executable application bytes."""
    fake_sdk = {"uboot.boot": b"SYNTHETIC-BOOT-NOT-FIRMWARE" * 4,
                "cfg_tool.bin": b"SYNTHETIC-CFG" * 4,
                "cfg/eq_cfg_hw.bin": b"SYNTHETIC-EQ" * 4}
    previous = pm.BOARD
    try:
        pm.set_board("smk37")
        with patch.object(pm, "sdk_file", side_effect=lambda name: fake_sdk[name]):
            flash = pm.flash_image(b"SYNTHETIC-APPLICATION-NOT-FIRMWARE" * 16, pm.KEY)
        ota = b"SYNTHETIC-LOADER-NOT-FIRMWARE " + (b"FELUCCA-LOADER-1" if loader else b"NO-UPDATE-LOADER")
        return pm.ufw(flash, ota, nightly.MODELS[model] + "_900")
    finally:
        pm.set_board(previous)


def change_area(raw, edit):
    """Keep outer CRCs valid while intentionally corrupting a nested app-area contract."""
    product, logical = add_ctl.logical_of(raw)
    st = add_ctl._open(logical)
    edit(st["area"])
    area = st["area"]
    pm.sfc(area, 0, st["n"], 0, st["key"])
    st["flash"][0x4000:0x4000 + st["n"]] = area
    st["lg"][st["off"]:st["off"] + st["size"]] = st["flash"]
    struct.pack_into("<H", st["e"], 4, pm.crc16(st["flash"]))
    pm.enc(st["e"], 0, 0x50)
    st["lst"][st["i"] * 0x50:(st["i"] + 1) * 0x50] = st["e"]
    struct.pack_into("<H", st["hdr"], 2, pm.crc16(st["lst"]))
    struct.pack_into("<H", st["hdr"], 0, pm.crc16(st["hdr"][2:0x40]))
    pm.enc(st["hdr"], 0, 0x40)
    st["lg"][:0x40] = st["hdr"]
    st["lg"][0x40:0x40 + len(st["lst"])] = st["lst"]
    return add_ctl.fwsc_of(st["lg"], product)


def change_flash(raw, edit):
    """Keep outer CRCs valid while editing flash metadata before the app-area decoder."""
    product, logical = add_ctl.logical_of(raw)
    state = add_ctl._open(logical)
    edit(state)
    state["lg"][state["off"]:state["off"] + state["size"]] = state["flash"]
    struct.pack_into("<H", state["e"], 4, pm.crc16(state["flash"]))
    pm.enc(state["e"], 0, 0x50)
    state["lst"][state["i"] * 0x50:(state["i"] + 1) * 0x50] = state["e"]
    struct.pack_into("<H", state["hdr"], 2, pm.crc16(state["lst"]))
    struct.pack_into("<H", state["hdr"], 0, pm.crc16(state["hdr"][2:0x40]))
    pm.enc(state["hdr"], 0, 0x40)
    state["lg"][:0x40] = state["hdr"]
    state["lg"][0x40:0x40 + len(state["lst"])] = state["lst"]
    return add_ctl.fwsc_of(state["lg"], product)


def bad_key_offset(state):
    isd = add_ctl._dec(state["flash"][64:96])
    struct.pack_into("<I", isd, 4, 0xFFFFFFF0)
    struct.pack_into("<H", isd, 0, pm.crc16(isd[2:32]))
    pm.enc(isd, 0, 32)
    state["flash"][64:96] = isd


def bad_area_size(state):
    area = state["area"]
    struct.pack_into("<I", area, 8, len(state["flash"]))
    struct.pack_into("<H", area, 0, pm.crc16(area[2:32]))
    pm.sfc(area, 0, state["n"], 0, state["key"])
    state["flash"][0x4000:0x4000 + state["n"]] = area


def bad_slotmap(state, kind):
    current = pm.slotmap_blob(pm.SLOTMAP["smk37"])
    state["flash"][pm.SLOTMAP_OFF:pm.SLOTMAP_OFF + len(current)] = bytes([255]) * len(current)
    if kind == "malformed":
        state["flash"][pm.SLOTMAP_OFF:pm.SLOTMAP_OFF + len(current)] = current[:-1] + bytes([current[-1] ^ 1])
    elif kind == "foreign":
        foreign = pm.slotmap_blob([(offset + 0x1000, size) for offset, size in pm.SLOTMAP["smk37"]])
        state["flash"][pm.SLOTMAP_OFF:pm.SLOTMAP_OFF + len(foreign)] = foreign
    elif kind == "misplaced":
        state["flash"][pm.SLOTMAP_OFF + 0x80:pm.SLOTMAP_OFF + 0x80 + len(current)] = current


def fixture_set():
    ctl = make_controller()
    output = {"controller.img": ctl}
    metadata = {"controller_sha256": nightly.sha256(ctl), "controller_size": len(ctl), "models": {}}
    for model in nightly.MODELS:
        raw = make_package(model)
        combined = add_ctl.add(raw, ctl)
        output[model + "-blank.fwsc"] = raw
        output[model + "-bundled.fwsc"] = combined
        output[model + "-app-crc.fwsc"] = change_area(combined, lambda area: area.__setitem__(34, area[34] ^ 1))
        output[model + "-key-range.fwsc"] = change_flash(combined, bad_key_offset)
        output[model + "-area-range.fwsc"] = change_flash(combined, bad_area_size)
        for kind in ("empty", "malformed", "foreign", "misplaced"):
            output[model + "-map-" + kind + ".fwsc"] = change_flash(combined, lambda state: bad_slotmap(state, kind))
        bad = bytearray(combined)
        bad[-1] ^= 1
        output[model + "-outer-crc.fwsc"] = bytes(bad)
        metadata["models"][model] = {"blank_sha256": nightly.sha256(raw),
                                     "bundled_sha256": nightly.sha256(combined), "size": len(raw)}
    return output, metadata


def refused(action, expected):
    try:
        action()
    except (add_ctl.PkgError, ValueError, struct.error) as exc:
        assert expected in str(exc), (expected, str(exc))
        return
    raise AssertionError("accepted: " + expected)


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--fixtures", type=Path)
    args = ap.parse_args(argv)
    fixtures, metadata = fixture_set()
    if args.fixtures is not None:
        args.fixtures.mkdir(parents=True, exist_ok=True)
        for name, raw in fixtures.items():
            (args.fixtures / name).write_bytes(raw)
        (args.fixtures / "metadata.json").write_text(json.dumps(metadata, indent=2) + "\n")
        print(f"synthetic fixtures written to {args.fixtures}")
        return 0
    ctl, digest, size = fixtures["controller.img"], metadata["controller_sha256"], metadata["controller_size"]
    for model in nightly.MODELS:
        raw, complete = fixtures[model + "-blank.fwsc"], fixtures[model + "-bundled.fwsc"]
        output, receipt = nightly.compose(raw, ctl, model, digest, size)
        assert output == complete, "NIGHTLY composition must match proven add_ctl bytes"
        assert receipt == nightly.inspect(output, model, digest, size)
        assert receipt["model"] == model and receipt["controller_strategy"] == "bundled"
        assert receipt["controller_size"] == size and receipt["controller_sha256"] == digest
        block_crc = zlib.crc32(ctl + bytes([255]) * (65536 - size))
        assert receipt["controller"]["block_crc32"] == f"{block_crc:08x}"
        assert nightly.compose(complete, ctl, model, digest, size)[0] == complete, "repack is idempotent"
        refused(lambda: nightly.inspect(raw, model, digest, size), "missing its bundled")
        refused(lambda: nightly.inspect(complete, "pro" if model == "elite" else "elite", digest, size), "does not match")
        _, logical = add_ctl.logical_of(complete)
        for version in ("901", "016"):
            rewrapped = add_ctl.fwsc_of(logical, nightly.MODELS[model] + "_" + version)
            refused(lambda raw=rewrapped, m=model: nightly.inspect(raw, m, digest, size), "does not match")
        stamped = add_ctl.fwsc_of(logical, nightly.MODELS[model] + "_9000103")
        refused(lambda raw=stamped, m=model: nightly.inspect(raw, m, digest, size, "0.1.3"), "not allocated")
        granted = Path(tempfile.mkdtemp()) / "ledger.json"
        granted.write_text(json.dumps({
            "schema": 1, "channel": "openbeta", "display_suffix": "_openbeta",
            "preserved_unversioned": ["892bf60d912eca93597dcf03be25a9756540c949"],
            "allocations": [{
                "version": "0.1.3", "major": 0, "minor": 1, "patch": 3,
                "display": "0.1.3_openbeta", "source_commit": "a" * 40, "source_tree": "b" * 40,
                "immutable": False, "models": {},
            }],
        }), encoding="utf-8")
        receipt_v = nightly.inspect(stamped, model, digest, size, "0.1.3", granted)
        assert receipt_v["firmware_version"] == "0.1.3", receipt_v
        assert receipt_v["release_display"] == "0.1.3_openbeta"
        assert receipt_v["product"].endswith("_9000103") and "_openbeta" not in receipt_v["product"]
        refused(lambda raw=stamped, m=model: nightly.inspect(raw, m, digest, size, "0.1.4"), "does not match")
        refused(lambda raw=stamped, m=model: nightly.inspect(raw, m, digest, size, "0.1.3_openbeta"), "not a firmware version")
        assert nightly.inspect(complete, model, digest, size)["firmware_version"] is None
        refused(lambda: nightly.inspect(complete, model, "0" * 64, size), "reviewed pin")
        refused(lambda: nightly.inspect(complete, model, digest, size - 1), "reviewed pin")
        refused(lambda: nightly.inspect(fixtures[model + "-outer-crc.fwsc"], model, digest, size), "package integrity")
        refused(lambda: nightly.inspect(fixtures[model + "-app-crc.fwsc"], model, digest, size), "app area data CRC")
        refused(lambda: nightly.inspect(fixtures[model + "-key-range.fwsc"], model, digest, size), "chip-key range")
        refused(lambda: nightly.inspect(fixtures[model + "-area-range.fwsc"], model, digest, size), "app area range")
        for kind in ("empty", "malformed", "foreign", "misplaced"):
            refused(lambda: nightly.inspect(fixtures[model + "-map-" + kind + ".fwsc"], model, digest, size), "slot map")
        refused(lambda: nightly.compose(raw, None, model, digest, size), "required")
        refused(lambda: nightly.compose(raw, ctl[:-1], model, digest, size), "reviewed pin")
        refused(lambda: nightly.compose(raw, ctl + b"\xff", model, nightly.sha256(ctl + b"\xff"), size + 1), "without padding")
        invalid_abi = make_controller(abi=1)
        refused(lambda: nightly.compose(raw, invalid_abi, model, nightly.sha256(invalid_abi), len(invalid_abi)), "ABI")
        # Controller-only CRC and erased-padding errors are caught even with correct package CRCs.
        product, logical = add_ctl.logical_of(complete)
        state = add_ctl._open(logical)
        state["area"][0x120 + add_ctl.CODE_MAX + 100] ^= 1
        bad_ctl_crc = add_ctl.fwsc_of(add_ctl._close(state), product)
        refused(lambda: nightly.inspect(bad_ctl_crc, model, digest, size), "CRC mismatch")
        state = add_ctl._open(logical)
        state["area"][0x120 + add_ctl.CODE_MAX + size] = 0
        bad_padding = add_ctl.fwsc_of(add_ctl._close(state), product)
        refused(lambda: nightly.inspect(bad_padding, model, digest, size), "non-erased")
        with tempfile.TemporaryDirectory(prefix="nightly-package-test-") as tmp:
            path = Path(tmp)
            pkg, control, out = path / "input.fwsc", path / "ctl.img", path / "complete.fwsc"
            pkg.write_bytes(raw)
            control.write_bytes(ctl)
            command = [str(pkg), str(control), "--model", model, "--controller-sha256", digest,
                       "--controller-size", str(size), "-o", str(out)]
            stdout = io.StringIO()
            with contextlib.redirect_stdout(stdout):
                assert nightly.main(command) == 0
            assert out.read_bytes() == complete and json.loads(stdout.getvalue()) == receipt
            out.write_bytes(b"KEEP-EXISTING")
            control.write_bytes(ctl[:-1])
            with contextlib.redirect_stderr(io.StringIO()):
                assert nightly.main(command) == 1
            assert out.read_bytes() == b"KEEP-EXISTING", "mismatch must refuse before output replacement"
            control.write_bytes(ctl)
            for malformed in ("key-range", "area-range"):
                pkg.write_bytes(fixtures[model + "-" + malformed + ".fwsc"])
                stderr = io.StringIO()
                with contextlib.redirect_stderr(stderr):
                    assert nightly.main(command) == 1
                assert "range" in stderr.getvalue() and out.read_bytes() == b"KEEP-EXISTING"
        print(f"PASS {model}: composition, embedded admission, exact pin, model, CRCs, padding, ABI, output preservation")
    refused(lambda: nightly.inspect(make_package("elite", loader=False), "elite", digest, size), "no Felucca")
    refused(lambda: nightly.inspect(fixtures["elite-bundled.fwsc"], "elite", "f", size), "64 lowercase")
    # Valid outer CRCs must not permit an OTA file range that overlaps flash.bin.
    product, overlap = add_ctl.logical_of(fixtures["elite-bundled.fwsc"])
    hdr = add_ctl._dec(overlap[:0x40])
    entries = bytearray(overlap[0x40:0xE0])
    ota = add_ctl._dec(entries[0x50:0xA0])
    struct.pack_into("<II", ota, 8, 0x400, 32)
    struct.pack_into("<H", ota, 4, pm.crc16(overlap[0x400:0x420]))
    pm.enc(ota, 0, 0x50)
    entries[0x50:0xA0] = ota
    struct.pack_into("<H", hdr, 2, pm.crc16(entries))
    struct.pack_into("<H", hdr, 0, pm.crc16(hdr[2:0x40]))
    pm.enc(hdr, 0, 0x40)
    overlap[:0x40], overlap[0x40:0xE0] = hdr, entries
    refused(lambda: nightly.inspect(add_ctl.fwsc_of(overlap, product), "elite", digest, size), "file range")
    print("nightly_package_test: PASS (synthetic data; no SDK, native build, hardware or distribution claim)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
