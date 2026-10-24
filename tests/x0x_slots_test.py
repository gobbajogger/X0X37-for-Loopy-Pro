#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""tools/x0x_slots.py (slot backup and restore) against a mocked device. No hardware.

MockDevice follows firmware/src/app/editor.c (SMP_BEGIN .. SMP_READ, 11-16) and slots.c: NOR flash
(erase sets 0xFF, a write can only clear bits), WRITE erases the 4 KiB sector at a 4 KiB-aligned
offset and refuses a slot that holds samples, END checks the header and the data CRC, INFO lists
the slots. The slot count and size are parameters: the tool must not assume 2 or 3 slots of 80 KiB.
  tests/x0x_slots_test.py"""
import json
import random
import struct
import sys
import zlib
from pathlib import Path

SRC = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SRC / "tools"))
import sampleio as sio  # noqa: E402
import x0x_slots as xs  # noqa: E402
sys.path.insert(0, str(SRC / "tests"))
from slot_mock import MockDevice, Flash, SMK_MAP, SMK_OLD_MAP  # noqa: E402,F401

FAILS = 0


def check(cond, what):
    global FAILS
    print(f"  {'ok  ' if cond else 'FAIL'} {what}")
    FAILS += not cond


def tone(n, f, sr, seed):
    r = random.Random(seed)
    return [int(12000 * __import__("math").sin(6.283185307 * f * i / sr) + r.randint(-500, 500)) for i in range(n)]


def slot_bytes(name, lens, rate, size=sio.SLOT_SIZE):
    return sio.user_slot(name, [(tone(n, 220 * (z + 1), rate, z), 36 + z, None, None) for z, n in enumerate(lens)],
                         rate=rate, slot_size=size)


def main():
    # 1. backup of an SMK-37-like device (2 x 80 KiB): slot 1 two zones at 44.1 kHz, slot 2 empty
    dev = MockDevice(2, 0x14000)
    hdr, data = slot_bytes("BR1", [40000, 30000], 44100)
    dev.load(0, hdr, data)
    before = [bytes(f) for f in dev.flash]
    lay, images = xs.backup(dev)
    check(lay["count"] == 2 and lay["size"] == 0x14000, "SMP_INFO: 2 slots of 80 KiB")
    check(set(dev.log) <= {xs.CMD_INFO, xs.CMD_READ}, "backup sends only SMP_INFO and SMP_READ (read-only)")
    check([bytes(f) for f in dev.flash] == before, "backup leaves the flash unchanged")
    check(images[0] == dev.image(0) and images[1] is None, "backup images: slot 1 exact, slot 2 empty")
    arch = xs.make_archive(lay, images, created="2026-10-15T00:00:00+1000")
    meta, back = xs.read_archive(arch)
    e = meta["slots"][0]
    check(meta["count"] == 2 and meta["size"] == 0x14000 and e["name"] == "BR1" and e["rates"] == [44100, 44100]
          and e["samples"] == [40000, 30000] and e["data_crc32"] == zlib.crc32(data) and meta["slots"][1]["empty"],
          "archive header: count, sizes, names, rates, samples, CRC")
    check(back == images, "archive round trip")

    # A well-formed outer CRC must not let altered slot labels erase/overwrite the wrong device slot.
    # The restore planner indexes images by position, so count and canonical slot numbers are binding.
    archive_json_len = struct.unpack_from("<I", arch, 12)[0]
    for label, ids, declared_count in (
        ("swapped slot identities", (2, 1), 2),
        ("duplicate slot identities", (1, 1), 2),
        ("skipped slot identity", (1, 3), 2),
        ("inconsistent slot count", (1, 2), 3),
    ):
        bad_meta = json.loads(arch[16:16 + archive_json_len])
        bad_meta["count"] = declared_count
        for entry, slot_num in zip(bad_meta["slots"], ids):
            entry["slot"] = slot_num
        bad_json = json.dumps(bad_meta, separators=(",", ":"), ensure_ascii=True).encode("ascii")
        bad_body = arch[:12] + struct.pack("<I", len(bad_json)) + bad_json + arch[16 + archive_json_len:-4]
        bad_archive = bad_body + struct.pack("<I", zlib.crc32(bad_body))
        try:
            xs.read_archive(bad_archive)
            check(False, f"archive refuses {label} even with a valid outer CRC")
        except xs.SlotError:
            check(True, f"archive refuses {label} even with a valid outer CRC")

    # A CRC-valid archive can still lie about whether a populated slot is empty or alias
    # another image. Restore must never silently erase/copy user samples from such metadata.
    def reseal(mutate, payload_suffix=b""):
        n = struct.unpack_from("<I", arch, 12)[0]
        altered = json.loads(arch[16:16 + n])
        mutate(altered)
        raw_json = json.dumps(altered, separators=(",", ":"), ensure_ascii=True).encode("ascii")
        body = (arch[:12] + struct.pack("<I", len(raw_json)) + raw_json
                + arch[16 + n:-4] + payload_suffix)
        return body + struct.pack("<I", zlib.crc32(body))

    def poison_empty_string(m):
        m["slots"][0]["empty"] = "false"   # truthy in Python -> silently erase a populated slot

    def poison_empty_integer(m):
        m["slots"][0]["empty"] = 1

    def poison_empty_descriptor(m):
        m["slots"][1].update({"offset": 0, "length": len(images[0]),
                               "crc32": zlib.crc32(images[0])})

    def poison_image_alias(m):
        m["slots"][1] = dict(m["slots"][0], slot=2)  # both point at slot one's data

    def unchanged(m):
        pass

    for label, mutation, suffix in (
        ("string false silently erases a populated slot", poison_empty_string, b""),
        ("numeric empty flag erases a populated slot", poison_empty_integer, b""),
        ("empty entry carrying hidden image pointers", poison_empty_descriptor, b""),
        ("aliased image regions copy slot one's bytes into slot two", poison_image_alias, b""),
        ("orphan payload bytes after referenced images", unchanged, b"orphan"),
    ):
        malformed = reseal(mutation, suffix)
        try:
            xs.read_archive(malformed)
            check(False, f"CRC-valid archive refuses {label}")
        except xs.SlotError:
            check(True, f"CRC-valid archive refuses {label}")

    # End-to-end refusal: bad metadata must fail before any SMP_BEGIN/ERASE/WRITE/END.
    guarded = MockDevice(2, 0x14000)
    guarded.load(0, hdr, data)
    guarded.load(1, hdr, data)
    untouched = [bytes(f) for f in guarded.flash]
    try:
        bad_meta, bad_images = xs.read_archive(reseal(poison_empty_string))
        xs.restore(guarded, bad_meta, bad_images)
    except xs.SlotError:
        pass
    check([bytes(f) for f in guarded.flash] == untouched
          and not any(cmd in guarded.log for cmd in (xs.CMD_BEGIN, xs.CMD_ERASE, xs.CMD_WRITE, xs.CMD_END)),
          "false-string empty flag cannot erase samples or send flash-write commands")

    # 2. damage is detected
    bad = bytearray(arch)
    bad[len(bad) // 2] ^= 1
    try:
        xs.read_archive(bytes(bad))
        check(False, "a damaged archive is refused")
    except xs.SlotError:
        check(True, "a damaged archive is refused")

    # 3. restore onto a device with other content: slot 1 written, slot 2 erased, read back identical
    dev2 = MockDevice(2, 0x14000)
    h2, d2 = slot_bytes("OTHER", [60000], 22050)
    dev2.load(0, h2, d2)
    dev2.load(1, h2, d2)
    skipped = xs.restore(dev2, meta, back)
    check(not skipped and dev2.image(0) == images[0] and dev2.image(1) is None, "restore: slot 1 rewritten, slot 2 erased")
    check(dev2.nz == [2, 0], "restore: SMP_INFO after restore lists the archive's zones")
    check(xs.CMD_ERASE in dev2.log and xs.CMD_BEGIN in dev2.log and xs.CMD_END in dev2.log,
          "restore uses SMP_ERASE / SMP_BEGIN, SMP_WRITE, SMP_END")

    # 4. --slots: restore only slot 2 (erase), slot 1 untouched
    dev3 = MockDevice(2, 0x14000)
    dev3.load(0, h2, d2)
    dev3.load(1, h2, d2)
    xs.restore(dev3, meta, back, only={2})
    check(dev3.image(0) == bytes(h2.ljust(xs.DATA_OFF, b"\xff")) + d2 and dev3.image(1) is None, "restore --slots 2 only")

    # 5. another layout (3 slots of 120 KiB): backup and restore work from SMP_INFO alone
    big = 120 * 1024
    dev4 = MockDevice(3, big, drop_every=7)          # and every 7th frame is lost: reads and writes retry
    hb, db = slot_bytes("BIG", [200000], 44100, size=big)
    dev4.load(2, hb, db)
    dev4.load(0, hdr, data)
    lay4, img4 = xs.backup(dev4)
    check(lay4["count"] == 3 and lay4["size"] == big and img4[2] == dev4.image(2) and img4[1] is None,
          "3 x 120 KiB layout: backup, 100 KB in one slot, lost frames retried")
    meta4, back4 = xs.read_archive(xs.make_archive(lay4, img4))
    dev5 = MockDevice(3, big, drop_every=11)
    xs.restore(dev5, meta4, back4)
    check([dev5.image(k) for k in range(3)] == img4, "3 x 120 KiB layout: restore, verified")

    # 6. a 3-slot archive onto a 2 x 80 KiB device: slot 3 skipped, and a 100 KB image does not fit
    dev6 = MockDevice(2, 0x14000)
    todo, skipped = xs.plan_restore(xs.layout(dev6), meta4, back4)
    check([k for k, _ in todo] == [0, 1] and len(skipped) == 1, "3-slot archive on 2 slots: slot 3 skipped")
    meta7 = dict(meta4)
    imgs7 = [back4[2], None]                         # the 100 KB image as slot 1
    todo, skipped = xs.plan_restore(xs.layout(dev6), meta7, imgs7)
    check([k for k, _ in todo] == [1] and "do not fit" in skipped[0], "an image larger than the slot is skipped")

    # 7. extract: the --dry-run / slotimg files
    out = SRC / "build" / "host" / "x0x_slots_extract"
    xs.main(["extract", str(_write(arch)), str(out)])
    check((out / "slot1.hdr").read_bytes() == hdr and (out / "slot1.bin").read_bytes() == data,
          "extract: slot1.hdr / .bin equal the uploaded header and data")
    print("x0x slots: all ok" if not FAILS else "X0X SLOTS FAILED")
    return 1 if FAILS else 0


def _write(b):
    p = SRC / "build" / "host" / "test.x0xslots"
    p.parent.mkdir(parents=True, exist_ok=True)
    p.write_bytes(b)
    return p


if __name__ == "__main__":
    sys.exit(main())
