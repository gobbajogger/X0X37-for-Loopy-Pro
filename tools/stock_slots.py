#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Deliver the LX-REQ-032 stock bank as exact user-slot data, entirely offline.

build copies approved ADPCM into FSMP headers; it never decodes or re-encodes it.
inspect validates the complete distribution. plan describes explicitly selected
replacements using an optional existing backup file. None opens a device or writes
samples. Output files are published exclusively; existing paths are never replaced.
"""
import argparse
import hashlib
import io
import json
import os
from pathlib import Path
import struct
import sys
import tempfile
import zipfile
import zlib

import break_pack
import fm1pkg_make as package
import sampleio
import x0x_slots as slots

REQUEST = "LX-REQ-032"
PROFILE = "lx-req-032-compact"
MODELS = ("elite", "pro")
SCHEMA = "x0x37-exact-stock-slots-v1"
BANK_SHA256 = "cbfe682a9c69b51dac4ee96db7ab4605c8ecdb2839230efcc5445aa0a8fe199e"
BANK_BYTES = 131152
STOCK = (("CRAMEN", 70589, 1), ("TASTY", 105827, 1), ("HOTPANT", 85725, 1))
ROOT_BASE = 36
ZONE = struct.Struct("<5I2h4B")
HEADER = struct.Struct("<IHBB8sII8x")
MAX_ZIP_BYTES = 2 * 1024 * 1024
MAX_CONTENT_BYTES = 4 * 1024 * 1024
# Stable container timestamps make repeated exports of the same inputs identical.
ZIP_TIME = (2026, 10, 29, 0, 0, 0)


class StockError(ValueError):
    pass


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def json_bytes(value):
    return (json.dumps(value, sort_keys=True, indent=2, ensure_ascii=True) + "\n").encode("ascii")


def identity(model, profile):
    if model not in MODELS or profile != PROFILE:
        raise StockError(f"requires model elite or pro and profile {PROFILE}")


def layout():
    """Use the maintained package map and cross-check shared format constants."""
    result = tuple(package.SLOTMAP["smk37"])
    if (result != ((0xC2000, 0x14000), (0xD6000, 0x1E000))
            or slots.DATA_OFF != sampleio.SLOT_DATA_OFF
            or slots.HDR_LEN != sampleio.SLOT_HDR_LEN
            or slots.HDR_LEN != HEADER.size + sampleio.SLOT_ZONES * ZONE.size
            or sampleio.SLOT_ZONES != 16
            or slots.DATA_OFF != 512):
        raise StockError("slot map or FSMP format changed; this stock profile needs review")
    return result


def parse_bank(data):
    parsed = break_pack.parse_pack(data)
    if parsed is None:
        raise StockError("break pack header, bounds, rate, bars, length or CRC is invalid")
    rate, loops = parsed
    offset = 20
    for loop in loops:
        raw_name = data[offset:offset + 8]
        name = raw_name.split(b"\0", 1)[0]
        if (not 1 <= len(name) <= 7 or any(c < 32 or c > 126 for c in name)
                or raw_name != name.ljust(8, b"\0") or data[offset + 13:offset + 16] != bytes(3)):
            raise StockError("break pack name or reserved bytes are noncanonical")
        offset += 20 + len(loop["adpcm"])
    return rate, loops


def approved_bank(data):
    if len(data) != BANK_BYTES or sha256(data) != BANK_SHA256:
        raise StockError("stock bank size/SHA256 differs from the approved LX-REQ-032 input")
    rate, loops = parse_bank(data)
    if rate != 44100 or tuple((p["name"], p["nsamples"], p["bars"]) for p in loops) != STOCK:
        raise StockError("stock bank metadata differs from the approved full-rate loops")
    return loops


def inferred_bars(samples, rate):
    """engine.c loop_bars policy; exported stock is far from decision boundaries."""
    seconds, best, result = samples / rate, float("inf"), 1
    for bars in (1, 2, 4, 8):
        bpm = bars * 240.0 / seconds
        distance = abs(bpm - 120.0)
        if 80.0 <= bpm <= 180.0 and distance < best:
            result, best = bars, distance
    return result


def encode_slot(parts, name, slot_size):
    """Copy whole 44.1 kHz ADPCM zones. No PCM conversion or audio processing."""
    if (not isinstance(name, str) or not 1 <= len(name) <= 8
            or any(not 32 <= ord(c) <= 126 for c in name)):
        raise StockError("slot name must be 1..8 printable ASCII characters")
    if not 1 <= len(parts) <= sampleio.SLOT_ZONES:
        raise StockError("a slot needs 1..16 complete loops")
    header, payload = bytearray(slots.HDR_LEN), bytearray()
    split = sampleio.key_split([ROOT_BASE + i for i in range(len(parts))])
    for i, part in enumerate(parts):
        raw, count = bytes(part["adpcm"]), part["nsamples"]
        if type(count) is not int or count < 64 or len(raw) != (count + 1) // 2:
            raise StockError("zone ADPCM length is not the whole loop")
        if inferred_bars(count, 44100) != part["bars"]:
            raise StockError("FSMP cannot preserve this loop's bars through firmware inference")
        if len(payload) + len(raw) > slot_size - slots.DATA_OFF:
            raise StockError("complete loops exceed the selected user-slot capacity")
        ZONE.pack_into(header, HEADER.size + i * ZONE.size, len(payload), count,
                       0, count - 1, 1 << 16, (ROOT_BASE + i) * 16, 0, 0,
                       split[i][0], split[i][1], 0)
        payload.extend(raw)
    HEADER.pack_into(header, 0, slots.MAGIC, 1, len(parts), 0,
                     name.encode("ascii").ljust(8, b"\0"), len(payload), zlib.crc32(payload))
    image = bytes(header).ljust(slots.DATA_OFF, b"\xff") + bytes(payload)
    validate_slot(image, slot_size)
    return image


def validate_slot(image, slot_size):
    """Strict export checks, including fields ignored by the shallow archive parser."""
    if not slots.DATA_OFF <= len(image) <= slot_size:
        raise StockError("slot image size is outside the selected slot")
    h = slots.parse_header(image)
    if (not h or image[7] != 0 or image[24:32] != bytes(8)
            or len(image) != slots.DATA_OFF + h["data_len"]
            or zlib.crc32(image[slots.DATA_OFF:]) != h["crc"]):
        raise StockError("slot header, exact size or data CRC is invalid")
    name = image[8:16].split(b"\0", 1)[0]
    if not name or any(not 32 <= c <= 126 for c in name) or image[8:16] != name.ljust(8, b"\0"):
        raise StockError("slot name is noncanonical")
    used = HEADER.size + h["nz"] * ZONE.size
    if image[used:slots.HDR_LEN] != bytes(slots.HDR_LEN - used):
        raise StockError("unused slot zone descriptors must be zero")
    if image[slots.HDR_LEN:slots.DATA_OFF] != b"\xff" * (slots.DATA_OFF - slots.HDR_LEN):
        raise StockError("slot padding must be erased bytes")
    offset, result = 0, []
    split = sampleio.key_split([ROOT_BASE + i for i in range(h["nz"])])
    for i in range(h["nz"]):
        off, n, ls, le, rate, root, pred, idx, lo, hi, looped = ZONE.unpack_from(
            image, HEADER.size + i * ZONE.size)
        length = (n + 1) // 2
        if (off != offset or n < 64 or n > 2 * slot_size or length > h["data_len"] - off
                or ls != 0 or le != n - 1 or rate != 1 << 16 or pred != 0 or idx != 0
                or root != (ROOT_BASE + i) * 16 or (lo, hi) != split[i] or looped != 0):
            raise StockError(f"slot zone {i + 1}: invalid range, rate or decoder metadata")
        raw = image[slots.DATA_OFF + off:slots.DATA_OFF + off + length]
        result.append({"nsamples": n, "rate": 44100, "adpcm": raw})
        offset += length
    if offset != h["data_len"]:
        raise StockError("slot has unreferenced ADPCM data")
    return result


def validate_receipt(raw):
    try:
        receipt = json.loads(raw)
    except (ValueError, UnicodeError) as exc:
        raise StockError("extraction receipt is not JSON") from exc
    if (not isinstance(receipt, dict) or receipt.get("bank_bytes") != BANK_BYTES
            or receipt.get("bank_sha256") != BANK_SHA256):
        raise StockError("extraction receipt does not identify the approved stock bank")
    return receipt


def stock_layout(parts):
    return ((parts[0],), (parts[1], parts[2]))


def loop_metadata(part):
    return {"name": part["name"], "samples": part["nsamples"], "bars": part["bars"],
            "rate_hz": 44100, "adpcm_bytes": len(part["adpcm"]),
            "adpcm_sha256": sha256(part["adpcm"]), "initial_predictor": 0,
            "initial_step_index": 0, "nibble_order": "low-first",
            "firmware_inferred_bars": inferred_bars(part["nsamples"], 44100)}


def slot_metadata(parts, images):
    result, menu = [], 0
    for i, ((off, size), group, image) in enumerate(zip(layout(), stock_layout(parts), images)):
        zones, data_off = [], 0
        for z, part in enumerate(group):
            zones.append({**loop_metadata(part), "zone_index": z, "data_offset": data_off,
                          "menu_index": menu, "menu_label": f"BR{menu + 1}"})
            data_off += len(part["adpcm"])
            menu += 1
        result.append({"slot": i + 1, "slot_index": i, "flash_offset": off,
                       "capacity_bytes": size, "header_area_bytes": slots.DATA_OFF,
                       "image_bytes": len(image), "image_sha256": sha256(image),
                       "unused_capacity_bytes": size - len(image),
                       "file": f"slots/slot{i + 1}.fsmp", "zones": zones})
    return result


def stock_archive(images, model, profile):
    lay = {"count": len(layout()), "size": min(n for _, n in layout()), "ext": True,
           "slots": [{"off": off, "size": size} for off, size in layout()]}
    # make_archive is the maintained offline serializer. Extra identity fields
    # are checked by this tool; older restore tools do not enforce them.
    ordinary = slots.make_archive(lay, images, created="source-sha256:" + BANK_SHA256)
    meta, readback = slots.read_archive(ordinary)
    if readback != images:
        raise StockError("native slot archive did not preserve the images")
    meta.update({"request_id": REQUEST, "model": model, "profile": profile,
                 "kind": "stock-distribution", "requires_explicit_replacement": True})
    raw = json.dumps(meta, sort_keys=True, separators=(",", ":"), ensure_ascii=True).encode("ascii")
    body = slots.ARCH_MAGIC + struct.pack("<HHI", slots.ARCH_VERSION, 0, len(raw)) + raw + b"".join(images)
    return body + struct.pack("<I", zlib.crc32(body))


README = """LX-REQ-032: exact external stock loops

This data is separate from the firmware image. No sample has been shortened,
normalised, resampled, decoded or re-encoded by the exporter. The source bank
and each original ADPCM stream are included for byte-for-byte recovery.

For the complete set, slot 1 contains CRAMEN; slot 2 contains TASTY and HOTPANT
as separate zones. Both SMK-37 Elite and Pro have the same two sample regions,
but use the distribution whose manifest names the intended model and profile.
With zero built-in loops, LOOP A/B list BR1=CRAMEN, BR2=TASTY, BR3=HOTPANT.
Indices are zero-based 0, 1, 2. FSMP has one name per slot and no bars field;
the manifest preserves each original name/bars, and the firmware infers one bar.

slots/*.fsmp are full raw slot images. slots/*.hdr and *.bin are the existing
offline host/import pair format. individual/*.fsmp provides one complete loop
per image, each small enough for either slot. No .brk import format is claimed.
The .x0xslots file is compatible with the existing archive parser. Older restore
tools do not enforce its model/profile metadata and can erase occupied slots.

Use tools/stock_slots.py inspect and plan with the exact model and profile to
review the intended replacement. plan is offline; an optional --current-backup
reads only a local archive, which may be stale. plan never connects to a device.
Replacing a slot replaces every zone in that slot. Existing user samples must
be retained in a verified backup before a separately authorised replacement.
Review every saved pattern's LOOP A/B after replacement: menu indices follow
the actual zones and built-in count. No device reads or writes are performed by
this tool, and it provides no write command or automatic restoration.
"""


def build_distribution(bank, receipt_raw, model, profile):
    identity(model, profile)
    parts = approved_bank(bank)
    validate_receipt(receipt_raw)
    images = [encode_slot(group, f"STOCK{i + 1}", layout()[i][1])
              for i, group in enumerate(stock_layout(parts))]
    archive_name = f"stock-{model}-{profile}.x0xslots"
    files = {"stock-bank.x0xbrk": bank, "provenance/stock-extraction.json": receipt_raw,
             archive_name: stock_archive(images, model, profile), "README.txt": README.encode("ascii")}
    for part in parts:
        files[f"loops/{part['name']}.adpcm"] = part["adpcm"]
        files[f"individual/{part['name']}.fsmp"] = encode_slot([part], part["name"], min(n for _, n in layout()))
    for i, image in enumerate(images, 1):
        files[f"slots/slot{i}.fsmp"] = image
        files[f"slots/slot{i}.hdr"] = image[:slots.HDR_LEN]
        files[f"slots/slot{i}.bin"] = image[slots.DATA_OFF:]
    manifest = {"schema": SCHEMA, "request_id": REQUEST, "model": model, "profile": profile,
                "kind": "external-stock-data", "device_access": "none",
                "firmware_embedded_loops": 0, "menu_mapping_requires": "both stock slots and zero built-in loops",
                "source": {"file": "stock-bank.x0xbrk", "bytes": BANK_BYTES, "sha256": BANK_SHA256,
                           "receipt": "provenance/stock-extraction.json"},
                "archive": archive_name, "slots": slot_metadata(parts, images),
                "files": {name: {"bytes": len(raw), "sha256": sha256(raw)} for name, raw in sorted(files.items())}}
    files["manifest.json"] = json_bytes(manifest)
    output = io.BytesIO()
    with zipfile.ZipFile(output, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as out:
        for name, raw in sorted(files.items()):
            entry = zipfile.ZipInfo(name, ZIP_TIME)
            entry.compress_type = zipfile.ZIP_DEFLATED
            entry.external_attr = 0o100644 << 16
            out.writestr(entry, raw)
    data = output.getvalue()
    inspect_distribution(data, model, profile)
    return data


def inspect_distribution(data, model, profile):
    identity(model, profile)
    if len(data) > MAX_ZIP_BYTES:
        raise StockError("stock distribution exceeds the bounded ZIP size")
    try:
        with zipfile.ZipFile(io.BytesIO(data)) as archive:
            entries = archive.infolist()
            names = [e.filename for e in entries]
            if (len(entries) != len(set(names)) or len(entries) > 24
                    or sum(e.file_size for e in entries) > MAX_CONTENT_BYTES
                    or any(e.file_size > MAX_ZIP_BYTES or e.is_dir() or e.flag_bits & 1
                           or e.filename.startswith("/") or ".." in e.filename.split("/")
                           or "\\" in e.filename or (e.external_attr >> 16) & 0o170000 == 0o120000
                           for e in entries)):
                raise StockError("unsafe or unbounded stock archive members")
            files = {e.filename: archive.read(e) for e in entries}
        manifest = json.loads(files["manifest.json"])
        if (manifest["schema"] != SCHEMA or manifest["request_id"] != REQUEST
                or manifest["model"] != model or manifest["profile"] != profile
                or manifest["kind"] != "external-stock-data" or manifest["device_access"] != "none"
                or manifest["firmware_embedded_loops"] != 0):
            raise StockError("distribution model/profile/schema does not match the requested target")
        recorded = manifest["files"]
        if set(files) != set(recorded) | {"manifest.json"}:
            raise StockError("distribution file list mismatch")
        for name, meta in recorded.items():
            if meta != {"bytes": len(files[name]), "sha256": sha256(files[name])}:
                raise StockError(f"distribution file size/SHA256 mismatch: {name}")
        bank = files["stock-bank.x0xbrk"]
        parts = approved_bank(bank)
        validate_receipt(files["provenance/stock-extraction.json"])
        if manifest["source"] != {"file": "stock-bank.x0xbrk", "bytes": BANK_BYTES,
                                  "sha256": BANK_SHA256, "receipt": "provenance/stock-extraction.json"}:
            raise StockError("distribution source identity mismatch")
        images = []
        expected_names = {"manifest.json", "stock-bank.x0xbrk", "provenance/stock-extraction.json", "README.txt"}
        for i, group in enumerate(stock_layout(parts), 1):
            image = files[f"slots/slot{i}.fsmp"]
            expected = encode_slot(group, f"STOCK{i}", layout()[i - 1][1])
            if image != expected or files[f"slots/slot{i}.hdr"] != image[:slots.HDR_LEN] or files[f"slots/slot{i}.bin"] != image[slots.DATA_OFF:]:
                raise StockError(f"slot {i} does not contain the exact approved zones")
            images.append(image)
            expected_names.update(f"slots/slot{i}.{ext}" for ext in ("fsmp", "hdr", "bin"))
        for part in parts:
            if (files[f"loops/{part['name']}.adpcm"] != part["adpcm"]
                    or files[f"individual/{part['name']}.fsmp"] != encode_slot(
                        [part], part["name"], min(n for _, n in layout()))):
                raise StockError(f"individual loop differs: {part['name']}")
            expected_names.update((f"loops/{part['name']}.adpcm", f"individual/{part['name']}.fsmp"))
        archive_name = f"stock-{model}-{profile}.x0xslots"
        expected_names.add(archive_name)
        if set(files) != expected_names or manifest["archive"] != archive_name:
            raise StockError("unexpected stock distribution files")
        if manifest["slots"] != slot_metadata(parts, images):
            raise StockError("slot placement or original metadata does not match the payload")
        if files[archive_name] != stock_archive(images, model, profile):
            raise StockError("native slot archive identity or payload mismatch")
        if files["README.txt"] != README.encode("ascii"):
            raise StockError("distribution instructions differ from this profile")
        return manifest, files
    except (zipfile.BadZipFile, KeyError, TypeError, AttributeError, IndexError, UnicodeError, json.JSONDecodeError,
            slots.SlotError, struct.error) as exc:
        raise StockError(f"malformed stock distribution: {exc}") from exc


def replacement_plan(bundle, model, profile, replace_slots, loop=None, current_backup=None):
    manifest, files = inspect_distribution(bundle, model, profile)
    if (any(type(k) is not int or k not in (1, 2) for k in replace_slots)
            or len(set(replace_slots)) != len(replace_slots)):
        raise StockError("replacement must explicitly select valid, unique slot numbers 1 or 2")
    if loop is None and tuple(replace_slots) != (1, 2):
        raise StockError("the full stock set requires the explicit ordered selection 1,2")
    if loop is not None and (loop not in {p[0] for p in STOCK} or len(replace_slots) != 1):
        raise StockError("one individual stock loop requires one explicit target slot")
    old_images = [None, None]
    baseline = "unknown; no device was accessed"
    if current_backup is not None:
        try:
            meta, old_images = slots.read_archive(current_backup)
        except (slots.SlotError, ValueError, KeyError, TypeError, struct.error) as exc:
            raise StockError(f"current backup is invalid: {exc}") from exc
        if (meta.get("count") != len(layout()) or len(old_images) != len(layout())
                or [(s.get("flash"), s.get("size")) for s in meta["slots"]] != list(layout())):
            raise StockError("current backup has no matching explicit two-slot SMK flash map")
        if meta.get("model", model) != model:
            raise StockError("current backup model differs from the selected target")
        if any(image is not None and len(image) > layout()[i][1] for i, image in enumerate(old_images)):
            raise StockError("current backup image exceeds the target slot")
        baseline = "local backup only; present device state and model are not verified"
    actions = []
    for number in replace_slots:
        file = f"individual/{loop}.fsmp" if loop is not None else f"slots/slot{number}.fsmp"
        raw, old = files[file], old_images[number - 1]
        validate_slot(raw, layout()[number - 1][1])
        prior = ({"bytes": len(old), "sha256": sha256(old), "header": slots.parse_header(old)}
                 if old is not None else {"state": "empty in supplied backup" if current_backup is not None else "unknown"})
        actions.append({"slot": number, "flash_offset": layout()[number - 1][0],
                        "capacity_bytes": layout()[number - 1][1], "image": file,
                        "image_bytes": len(raw), "image_sha256": sha256(raw),
                        "replaces": "all zones and contents in the selected slot", "prior": prior})
    return {"schema": "x0x37-stock-replacement-plan-v1", "request_id": REQUEST,
            "model": model, "profile": profile, "mode": "INSPECT_ONLY", "device_access": "none",
            "executed": False, "target_state": baseline, "actions": actions,
            "untouched_slots": [k for k in (1, 2) if k not in replace_slots],
            "menu_mapping": manifest["slots"] if loop is None else "depends on all other valid zones",
            "later_requirements": ["verify the target model/profile and current slot state",
                                   "retain a verified backup of all current user samples",
                                   "obtain explicit authority for the selected slot replacement",
                                   "check saved patterns' LOOP A/B indices after any replacement"],
            "write_command": None}


def publish_exclusive(path, data):
    """Stage and read back before atomically publishing without replacing a path."""
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(dir=path.parent, prefix=".stock-", delete=False) as stream:
            temporary = Path(stream.name)
            stream.write(data)
            stream.flush()
            os.fsync(stream.fileno())
        if temporary.read_bytes() != data:
            raise StockError("staged output readback differs")
        os.link(temporary, path)  # EEXIST fails closed, including a concurrent writer.
        temporary.unlink()
        temporary = None
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    for command in ("build", "inspect", "plan"):
        sub = commands.add_parser(command)
        sub.add_argument("--model", choices=MODELS, required=True)
        sub.add_argument("--profile", choices=(PROFILE,), required=True)
        if command == "build":
            sub.add_argument("--bank", type=Path, required=True)
            sub.add_argument("--receipt", type=Path, required=True)
            sub.add_argument("--out", type=Path, required=True)
        else:
            sub.add_argument("--bundle", type=Path, required=True)
        if command == "plan":
            sub.add_argument("--replace-slots", required=True, help="1,2 for the set; one slot for --loop")
            sub.add_argument("--loop", choices=[p[0] for p in STOCK])
            sub.add_argument("--current-backup", type=Path)
    args = parser.parse_args(argv)
    try:
        if args.command == "build":
            data = build_distribution(args.bank.read_bytes(), args.receipt.read_bytes(), args.model, args.profile)
            publish_exclusive(args.out, data)
            result = {"file": str(args.out), "bytes": len(data), "sha256": sha256(data),
                      "model": args.model, "profile": args.profile, "device_access": "none"}
        elif args.command == "inspect":
            result, _ = inspect_distribution(args.bundle.read_bytes(), args.model, args.profile)
        else:
            result = replacement_plan(args.bundle.read_bytes(), args.model, args.profile,
                                      [int(k) for k in args.replace_slots.split(",")], args.loop,
                                      args.current_backup.read_bytes() if args.current_backup else None)
        sys.stdout.buffer.write(json_bytes(result))
    except (StockError, OSError, ValueError) as exc:
        parser.exit(2, f"stock_slots: {exc}\n")


if __name__ == "__main__":
    main()
