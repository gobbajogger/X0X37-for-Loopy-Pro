#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Back up and restore the user sample slots of an X0X device (SMK-37 or FM-1) over USB-MIDI.

  tools/x0x_slots.py backup FILE.x0xslots               read every slot (read-only on the device)
  tools/x0x_slots.py restore FILE.x0xslots [--slots 1,2] [--yes] [--no-verify]
                                                        write the archive back (erases those slots)
  tools/x0x_slots.py show FILE.x0xslots                 list an archive (no device)
  tools/x0x_slots.py extract FILE.x0xslots OUTDIR       OUTDIR/slotN.hdr / .bin (the upload_breaks.py
                                                        --dry-run and host `slotimg` format; no device)

The slot count and the slot sizes come from the device (SMP_INFO, cmd 15); nothing here assumes 2 or 3
slots of 80 KiB. Since the SMK-37 flash layout 2 the slots differ in size (80, 120 and 44 KiB): SMP_INFO
then ends with the X0X extension ('X' 1, per slot: flash offset, size, used KiB). An older firmware sends
no extension: all its slots have the one size of the reply. Backup reads each slot that holds samples with SMP_READ (cmd 16): the 512-byte header
area and the data. It sends no command that writes. Restore asks before it writes (--yes skips the
question); for each slot it sends SMP_BEGIN (arms rewrite; flash stays until first WRITE),
SMP_WRITE in 256-byte parts and SMP_END (the editor
protocol, web/EDITOR_PROTOCOL.md, 11-13), or SMP_ERASE (14) for a slot that was empty. Then it reads the
slots back and compares them (--no-verify skips that).

Archive (.x0xslots), all integers little-endian:
  "X0XSLOTS"  u16 version (1)  u16 0  u32 L  L bytes of JSON  the slot images, in order  u32 CRC-32
The JSON (ASCII, no spaces) holds the slot count and size (the smallest slot) of the source device and,
per slot: its number, size, whether it was empty, its flash offset ("flash", if the device sent it), name, zone count, per-zone sample counts and rates (Hz), the data
length, the header's data CRC, and the image's offset, length and CRC-32. An image is the slot from
its start: the 512-byte header area, then the ADPCM data. The final CRC-32 (zlib) covers every byte
before it. web/x0x_breaks.js (makeArchive, readArchive) reads and writes the same format.
Needs mido (and a backend such as python-rtmidi) for the device commands."""
import errno
import json
import os
import struct
import tempfile
import sys
import time
import zlib
from pathlib import Path

CMD_BEGIN, CMD_WRITE, CMD_END, CMD_ERASE, CMD_INFO, CMD_READ = 11, 12, 13, 14, 15, 16
HDR_LEN, DATA_OFF, READ_MAX, WRITE_LEN = 480, 512, 224, 256
MAGIC = 0x504D5346                                   # "FSMP"
ARCH_MAGIC, ARCH_VERSION = b"X0XSLOTS", 1
RC = {1: "size", 2: "header", 3: "data CRC", 4: "flash", 5: "zones"}


class SlotError(Exception):
    pass


def pack7(b):
    """groups of up to 7 bytes, each preceded by their top bits (fm1_sample_upload.pack7)"""
    out = []
    for i in range(0, len(b), 7):
        g = b[i:i + 7]
        out.append(sum(((x >> 7) & 1) << j for j, x in enumerate(g)))
        out += [x & 0x7F for x in g]
    return out


def unpack7(a):
    out, i = bytearray(), 0
    while i < len(a):
        m = a[i]
        i += 1
        for j in range(7):
            if i >= len(a):
                break
            out.append(a[i] | ((m >> j) & 1) << 7)
            i += 1
    return bytes(out)


def ascii_name(b):
    """a slot name as text: printable ASCII kept, anything else '?' (as web/x0x_breaks.js)"""
    return "".join(chr(c) if 32 <= c < 127 else "?" for c in b)


def parse_info(r):
    """Decode a complete SMP_INFO layout; never guess legacy offsets from a broken extension.

    An ambiguous layout is unsafe for sample-preservation decisions made before OTA.
    """
    if len(r) < 2:
        raise SlotError("short SMP_INFO reply")
    count, size, i, slots = r[0], r[1] * 1024, 2, []
    if not count or not size:
        raise SlotError("invalid SMP_INFO slot count or size")
    for k in range(count):
        if i >= len(r):
            raise SlotError(f"slot {k + 1}: truncated SMP_INFO reply")
        nz = r[i]
        try:
            j = r.index(0, i + 1)
            used = r[j + 1]
        except (ValueError, IndexError) as e:
            raise SlotError(f"slot {k + 1}: truncated SMP_INFO descriptor") from e
        if nz > 16 or j - i - 1 > 8:
            raise SlotError(f"slot {k + 1}: invalid zones or used size")
        # On-device SMP_INFO reports zero used KiB and an empty name for an empty slot.
        if nz == 0 and (used != 0 or j != i + 1):
            raise SlotError(f"slot {k + 1}: contradictory empty-slot descriptor")
        slots.append({"nz": nz, "name": ascii_name(r[i + 1:j]), "kb": used, "size": size,
                      "off": None, "used": used})
        i = j + 2
    # A partially received / unknown extension must not be misread as a legacy layout.
    has_marker = i < len(r) and r[i] == ord("X")
    if has_marker and (i + 2 + 6 * count > len(r) or r[i + 1] != 1):
        raise SlotError("truncated or unsupported SMP_INFO flash-map extension")
    if i < len(r) and not has_marker:
        raise SlotError("unknown SMP_INFO trailing metadata")
    ext = has_marker
    if ext and i + 2 + 6 * count != len(r):
        raise SlotError("invalid SMP_INFO extension length")
    if not ext and any(sl["used"] * 1024 > size for sl in slots):
        raise SlotError("legacy SMP_INFO used size exceeds slot size")
    if ext:
        ranges = []
        for k, sl in enumerate(slots):
            v = [r[i + 2 + 6 * k + 2 * m] | r[i + 3 + 6 * k + 2 * m] << 7 for m in range(3)]
            off, slot_size, used = v[0] * 0x1000, v[1] * 0x1000, v[2]
            end = off + slot_size
            if (off == 0 or slot_size < DATA_OFF or used * 1024 > slot_size
                    or any(off < hi and lo < end for lo, hi in ranges)):
                raise SlotError(f"slot {k + 1}: invalid or overlapping SMP_INFO flash range")
            # Legacy used KiB is capped to 7 bits; extension carries full 14-bit count.
            # Both are derived from the same on-device sample header data_len.
            if sl["kb"] != min(127, used):
                raise SlotError(f"slot {k + 1}: legacy/extended used-size disagreement")
            ranges.append((off, end))
            sl.update({"off": off, "size": slot_size, "used": used})
    return {"count": count, "size": size, "ext": ext, "slots": slots}


def parse_header(b):
    """a slot's first 480 bytes -> dict, or None if it is not a slot header"""
    if len(b) < HDR_LEN:
        return None
    magic, ver, nz, _, name, dlen, crc = struct.unpack_from("<IHBB8sII", b, 0)
    if magic != MAGIC or ver != 1 or not 1 <= nz <= 16:
        return None
    zones = []
    for z in range(nz):
        off, n, ls, le, q16 = struct.unpack_from("<5I", b, 32 + z * 28)
        zones.append({"off": off, "n": n, "rate": (q16 * 44100 + 32768) >> 16})
    return {"nz": nz, "name": ascii_name(name.split(b"\0")[0]), "data_len": dlen, "crc": crc,
            "zones": zones}


def req(link, cmd, args, timeout=3.0, tries=3):
    """one command; a lost frame (no reply) is sent again. Every slot command is safe to repeat:
    INFO and READ do not write, BEGIN only arms (may restore a mid-WRITE shadow), ERASE wipes,
    WRITE writes the same bytes, END writes the same header."""
    for t in range(tries):
        try:
            return link.req(cmd, args, timeout)
        except TimeoutError:
            if t == tries - 1:
                raise


def layout(link):
    return parse_info(req(link, CMD_INFO, []))


def read_bytes(link, k, off, n, progress=None):
    """n bytes of slot k from off (SMP_READ: up to 224 a frame, asked for in pairs)"""
    out = bytearray()
    while len(out) < n:
        want, adr = min(READ_MAX, n - len(out)), off + len(out)
        r = req(link, CMD_READ, [k, adr & 0x7F, (adr >> 7) & 0x7F, (adr >> 14) & 0x7F, (want + 1) >> 1])
        # Every reply must have the four echoed address bytes and packed data.
        # SMP_READ requests pairs, so only a final odd-byte request may be
        # rounded up by one. Never silently consume surplus bytes from a
        # malformed frame as if they belonged to a requested flash range.
        if len(r) < 6:
            raise SlotError(f"slot {k + 1}: short SMP_READ reply at {adr:#x}")
        got = unpack7(r[4:])
        # SMP_READ must use the canonical pack7 wire encoding. Unpack7 alone
        # silently ignores orphan masks and unused high bits in a short group.
        if pack7(got) != list(r[4:]):
            raise SlotError(f"slot {k + 1}: noncanonical SMP_READ frame at {adr:#x}")
        if (r[0] != k or any(not 0 <= b < 128 for b in r[:4])
                or (r[1] | r[2] << 7 | r[3] << 14) != adr
                or not got or len(got) > want + (want & 1)):
            raise SlotError(f"slot {k + 1}: bad SMP_READ reply at {adr:#x}")
        out += got[:n - len(out)]
        if progress:
            progress(len(out), n)
    return bytes(out)


def read_slot(link, k, nz, progress=None):
    """slot k's image (header area + data), or None for an empty slot"""
    if not nz:
        return None
    head = read_bytes(link, k, 0, DATA_OFF)
    h = parse_header(head)
    if not h:
        raise SlotError(f"slot {k + 1}: the device lists {nz} zone(s) but the header does not parse")
    data = read_bytes(link, k, DATA_OFF, h["data_len"], progress=progress)
    if zlib.crc32(data) != h["crc"]:
        raise SlotError(f"slot {k + 1}: the data read back does not match the header CRC")
    return head + data


def make_archive(lay, images, created=None):
    """lay: parse_info(); images: per slot, bytes or None -> archive bytes"""
    entries, blobs, off = [], [], 0
    for k, img in enumerate(images):
        sl = lay["slots"][k] if k < len(lay.get("slots", [])) else {}
        e = {"slot": k + 1, "size": sl.get("size", lay["size"]), "empty": img is None}
        if sl.get("off") is not None:
            e["flash"] = sl["off"]
        if img is not None:
            h = parse_header(img)
            e.update({"name": h["name"], "zones": h["nz"], "samples": [z["n"] for z in h["zones"]],
                      "rates": [z["rate"] for z in h["zones"]], "data_len": h["data_len"], "data_crc32": h["crc"],
                      "offset": off, "length": len(img), "crc32": zlib.crc32(img)})
            blobs.append(img)
            off += len(img)
        entries.append(e)
    meta = {"format": "x0xslots", "version": ARCH_VERSION, "created": created or time.strftime("%Y-%m-%dT%H:%M:%S%z"),
            "count": lay["count"], "size": lay["size"], "slots": entries}
    js = json.dumps(meta, separators=(",", ":"), ensure_ascii=True).encode("ascii")
    body = ARCH_MAGIC + struct.pack("<HHI", ARCH_VERSION, 0, len(js)) + js + b"".join(blobs)
    return body + struct.pack("<I", zlib.crc32(body))


def read_archive(b):
    """archive bytes -> (meta, images per slot: bytes or None); raises SlotError on any damage"""
    if len(b) < 20 or b[:8] != ARCH_MAGIC:
        raise SlotError("not an .x0xslots archive")
    if struct.unpack_from("<I", b, len(b) - 4)[0] != zlib.crc32(b[:-4]):
        raise SlotError("archive CRC mismatch: the file is damaged")
    ver, _, n = struct.unpack_from("<HHI", b, 8)
    if ver != ARCH_VERSION:
        raise SlotError(f"archive version {ver}, this tool reads {ARCH_VERSION}")
    meta = json.loads(b[16:16 + n].decode("ascii"))
    slots = meta.get("slots") if isinstance(meta, dict) else None
    if (not isinstance(slots, list) or type(meta.get("count")) is not int
            or meta["count"] != len(slots)):
        raise SlotError("archive slot count mismatch")
    for k, e in enumerate(slots):
        if not isinstance(e, dict) or type(e.get("slot")) is not int or e["slot"] != k + 1:
            raise SlotError(f"archive slot {k + 1}: noncanonical slot number")
    base, images = 16 + n, []
    if base > len(b) - 4:
        raise SlotError("archive metadata exceeds file length")
    used = 0
    for e in slots:
        if type(e.get("empty")) is not bool:
            raise SlotError(f"slot {e['slot']}: invalid empty flag")
        if e["empty"]:
            if any(k in e for k in ("offset", "length", "crc32")):
                raise SlotError(f"slot {e['slot']}: empty slot has image data pointers")
            images.append(None)
            continue
        if (type(e.get("offset")) is not int or e["offset"] != used
                or type(e.get("length")) is not int or e["length"] < DATA_OFF
                or e["length"] > len(b) - 4 - base - used):
            raise SlotError(f"slot {e['slot']}: noncanonical image offset or length")
        img = b[base + used:base + used + e["length"]]
        if zlib.crc32(img) != e["crc32"]:
            raise SlotError(f"slot {e['slot']}: image CRC mismatch")
        h = parse_header(img)
        if not h or zlib.crc32(img[DATA_OFF:]) != h["crc"] or len(img) != DATA_OFF + h["data_len"]:
            raise SlotError(f"slot {e['slot']}: the image is not a valid slot")
        # Match smp_user_scan() before restore can erase the target slot.
        for z in range(h["nz"]):
            zone = 32 + z * 28
            off, samples, _, _, rate_q16 = struct.unpack_from("<5I", img, zone)
            if (off > h["data_len"] or (samples + 1) // 2 > h["data_len"] - off
                    or img[zone + 24] > 88 or not 0 < rate_q16 <= (4 << 16)):
                raise SlotError(f"slot {e['slot']}: invalid sample zone {z + 1}")
        images.append(img)
        used += len(img)
    if base + used != len(b) - 4:
        raise SlotError("archive contains unreferenced sample data")
    return meta, images


def _rename_backup_noreplace(staged, path):
    """Atomically publish a validated backup without clobbering an existing path.

    Some filesystems do not implement hard links.  Use only the native exclusive
    rename operations; a check-then-rename or direct-write fallback could expose
    a partial archive or overwrite a concurrently created backup.
    """
    import ctypes

    libc = ctypes.CDLL(None, use_errno=True)
    if sys.platform.startswith("linux"):
        fn = getattr(libc, "renameat2", None)
        flags = 1  # Linux RENAME_NOREPLACE
        at_fdcwd = -100
    elif sys.platform == "darwin":
        fn = getattr(libc, "renameatx_np", None)
        flags = 4  # Darwin RENAME_EXCL
        at_fdcwd = -2
    else:
        fn = None
        flags = None
        at_fdcwd = None
    if fn is None:
        raise SlotError("filesystem cannot atomically publish backup without hard links; "
                        "choose a filesystem supporting exclusive rename or hard links")
    fn.argtypes = [ctypes.c_int, ctypes.c_char_p, ctypes.c_int,
                   ctypes.c_char_p, ctypes.c_uint]
    fn.restype = ctypes.c_int
    if fn(at_fdcwd, os.fsencode(staged), at_fdcwd, os.fsencode(path), flags) == 0:
        return
    code = ctypes.get_errno()
    if code == errno.EEXIST:
        raise FileExistsError(code, os.strerror(code), str(path))
    if code in (errno.ENOTSUP, errno.EOPNOTSUPP, errno.ENOSYS, errno.EINVAL):
        raise SlotError("filesystem cannot atomically publish backup without hard links; "
                        "save to a filesystem supporting exclusive rename or hard links")
    raise OSError(code, os.strerror(code), str(path))


def _publish_backup_noreplace(staged, path):
    try:
        os.link(staged, path)
    except OSError as error:
        # Unsupported syscall is NOT an authorization failure. Do not retry
        # permission errors, destination collisions, or unrelated I/O errors.
        if error.errno not in (errno.ENOTSUP, errno.EOPNOTSUPP, errno.ENOSYS):
            raise
        _rename_backup_noreplace(staged, path)
    else:
        staged.unlink()


def _sync_backup_directory(parent):
    """Make the published sample-backup filename durable before any OTA starts."""
    fd = os.open(parent, os.O_RDONLY | getattr(os, "O_DIRECTORY", 0))
    try:
        os.fsync(fd)
    finally:
        os.close(fd)


def save_backup_archive(path, archive, *, replace=False):
    """Stage, read back and fsync a backup before publishing it at the destination.

    Never truncate the old archive if writing fails. For a new path, use
    hard links or an exclusive atomic rename to prevent concurrent clobbering.
    """
    read_archive(archive)  # Reject corrupt source before any filesystem change.
    staged = None
    try:
        with tempfile.NamedTemporaryFile(mode="wb", prefix=f".{path.name}.", suffix=".tmp",
                                         dir=path.parent, delete=False) as out:
            staged = Path(out.name)
            out.write(archive)
            out.flush()
            os.fsync(out.fileno())
        if staged.read_bytes() != archive:
            raise SlotError("temporary sample backup failed readback")
        if replace:
            os.replace(staged, path)
        else:
            _publish_backup_noreplace(staged, path)
        staged = None
        try:
            _sync_backup_directory(path.parent)
        except OSError as e:
            raise SlotError(f"{path}: backup published, but directory sync failed; "
                            f"durability unknown. Do not flash until a durable backup is verified ({e})") from e
    finally:
        if staged is not None:
            staged.unlink(missing_ok=True)


def backup(link, progress=None):
    """read every slot -> (layout, images); sends SMP_INFO and SMP_READ only"""
    lay = layout(link)
    images = []
    for k, s in enumerate(lay["slots"]):
        images.append(read_slot(link, k, s["nz"], (lambda d, n, k=k: progress(k, d, n)) if progress else None))
    return lay, images


def plan_restore(lay, meta, images, only=None):
    """-> ([(slot index, image or None)], [problems]) for the device layout lay"""
    todo, skipped = [], []
    for k, img in enumerate(images):
        if only is not None and k + 1 not in only:
            continue
        if k >= lay["count"]:
            skipped.append(f"slot {k + 1}: the device has {lay['count']} slot(s)")
        elif img is not None and len(img) > lay["slots"][k]["size"]:
            skipped.append(f"slot {k + 1}: {len(img)} B do not fit in the device's {lay['slots'][k]['size']} B slot")
        else:
            todo.append((k, img))
    return todo, skipped


def _slot_ack(r, k, command, address=None):
    """Reject stale/wrong-slot or truncated write acknowledgements before continuing.

    The device echoes slot for ERASE, BEGIN and END (two bytes), and both
    slot and 21-bit offset for WRITE (five bytes). The host transport matches
    the command only, so a delayed response must not certify another write.
    """
    count = 5 if address is not None else 2
    if (len(r) != count or r[0] != k or any(not 0 <= b < 128 for b in r) or
            (address is not None and (r[1] | r[2] << 7 | r[3] << 14) != address)):
        raise SlotError(f"slot {k + 1}: mismatched or malformed {command} acknowledgement")
    return r[-1]


def write_slot(link, k, img, progress=None):
    """erase slot k and write the image (None: erase only); raises SlotError on a device error"""
    if img is None:
        r = req(link, CMD_ERASE, [k], 30.0)
        if _slot_ack(r, k, "erase"):
            raise SlotError(f"slot {k + 1}: erase failed")
        return
    r = req(link, CMD_BEGIN, [k], 10.0)
    if _slot_ack(r, k, "begin"):
        raise SlotError(f"slot {k + 1}: begin (erase) failed")
    data = img[DATA_OFF:]
    for off in range(0, len(data), WRITE_LEN):
        adr = DATA_OFF + off
        args = [k, adr & 0x7F, (adr >> 7) & 0x7F, (adr >> 14) & 0x7F] + pack7(data[off:off + WRITE_LEN])
        r = req(link, CMD_WRITE, args)
        rc = _slot_ack(r, k, "write", adr)
        if rc:
            raise SlotError(f"slot {k + 1}: write at {adr:#x} failed ({rc})")
        if progress:
            progress(min(off + WRITE_LEN, len(data)), len(data))
    r = req(link, CMD_END, [k] + pack7(img[:HDR_LEN]), 5.0)
    rc = _slot_ack(r, k, "end")
    if rc:
        raise SlotError(f"slot {k + 1}: the device refused the header ({rc}, {RC.get(rc, '?')})")

def restore(link, meta, images, only=None, verify=True, progress=None):
    """write the archive's slots back -> list of skipped-slot messages"""
    lay = layout(link)
    todo, skipped = plan_restore(lay, meta, images, only)
    for k, img in todo:
        write_slot(link, k, img, progress=(lambda d, n, k=k: progress(k, d, n)) if progress else None)
        # Refuse a failed readback before erasing the next slot.
        if verify:
            after = layout(link)
            got = read_slot(link, k, after["slots"][k]["nz"])
            if got != img:
                raise SlotError(f"slot {k + 1}: read back differs from the archive")
    return skipped


# ---- an install that changes the slot map: which slots lose their data, and where they go back ----
# A slot keeps its data over an install when the new firmware has a slot at the same flash offset that
# is large enough: no install writes the slots (the loader writes only the app area), so the bytes stay.
LEGACY_BASE = {"smk": 0xC2000, "fm1": 0xA0000}      # X0X before SMP_INFO had the extension: equal slots


def flash_map(lay, board="smk"):
    """[(flash offset, size)] per slot of a parse_info() layout"""
    if lay.get("ext"):
        return [(sl["off"], sl["size"]) for sl in lay["slots"]]
    return [(LEGACY_BASE[board] + k * lay["size"], lay["size"]) for k in range(lay["count"])]


def at_risk(lay, old_map, new_map):
    """slot indexes that hold samples and that the new slot map does not keep (no slot at the same
    offset with room for the header area and the used KiB). new_map []: a firmware without X0X slots."""
    risk = []
    for k, sl in enumerate(lay["slots"]):
        if not sl["nz"]:
            continue
        off, need = old_map[k][0], DATA_OFF + sl["used"] * 1024
        if not any(o == off and n >= need for o, n in new_map):
            risk.append(k)
    return risk


def plan_migration(images, risk, old_map, new_lay, new_map):
    """Maximize safe sample restoration without touching occupied or kept new slots.

    Prefer the same slot number when possible, but allow reassignment so an early
    small image cannot consume the only slot that fits a later large image.
    """
    kept = {j for j, (o, _) in enumerate(new_map)
            for k, (ok_, _) in enumerate(old_map) if images[k] is not None and k not in risk and ok_ == o}
    choices = {}
    for k in risk:
        order = ([k] if k < len(new_map) else []) + [j for j in range(len(new_map)) if j != k]
        choices[k] = [j for j in order if j not in kept and not new_lay["slots"][j]["nz"]
                      and len(images[k]) <= new_map[j][1]]

    # Most-constrained images first, then the larger ones. If the preferred
    # destination is in use, recursively move its occupant to another safe slot.
    # This finds a maximum-cardinality matching, unlike first-fit placement.
    assigned = {}  # destination slot -> original image index

    def place(k, seen):
        for j in choices[k]:
            if j in seen:
                continue
            seen.add(j)
            if j not in assigned or place(assigned[j], seen):
                assigned[j] = k
                return True
        return False

    for k in sorted(risk, key=lambda k: (len(choices[k]), -len(images[k]), k)):
        place(k, set())

    destinations = {k: j for j, k in assigned.items()}
    todo = [(k, destinations[k]) for k in risk if k in destinations]
    skipped = [f"slot {k + 1} ({len(images[k])} B): no free slot of that size on the new firmware"
               for k in risk if k not in destinations]
    return todo, skipped


X0X_NBREAKS_BUILTIN = 2                    # factory loops listed before BRn (engine_brk_slot_names / pattern.h)


def _user_loop_menu_ids(lay, images, origins=None):
    """Original slot/zone identities in the engine's absolute user LOOP menu order.

    Content CRC is not identity: two distinct slots may contain identical bytes.
    Each zone is one menu entry (engine.scan_loops), rather than one entry per slot.
    Unknown occupied destinations must also count, since they can shift later entries.
    """
    return [("old", k if origins is None else origins[k], z)
            if origins is None or origins[k] is not None else ("new", k, z)
            for k, sl in enumerate(lay["slots"]) for z in range(sl.get("nz", 0))]


def _loop_menu_after_migration(old_lay, images, risk, old_map, new_map, new_lay, todo):
    n = len(new_map)
    imgs, origins = [None] * n, [None] * n
    slots = [dict(sl) for sl in new_lay["slots"]]
    for k, img in enumerate(images):
        if k in risk or img is None:
            continue
        off = old_map[k][0]
        for j, (noff, _) in enumerate(new_map):
            if noff == off:
                imgs[j], origins[j] = img, k
                slots[j] = dict(old_lay["slots"][k])
                break
    for k, j in todo:
        imgs[j], origins[j] = images[k], k
        slots[j] = dict(old_lay["slots"][k])
    return _user_loop_menu_ids({**new_lay, "slots": slots}, imgs, origins)


def migration_reorders_loop_menu(old_lay, images, risk, old_map, new_lay, new_map, todo):
    """True if automatic restoration changes an existing absolute LOOP index.

    The historical function name includes skipped-middle compaction, not just relative
    reordering. An unchanged prefix with only trailing omissions or additions is safe.
    """
    if not todo:
        return False
    before = _user_loop_menu_ids(old_lay, images)
    after = _loop_menu_after_migration(old_lay, images, risk, old_map, new_map, new_lay, todo)
    return any(a != b for a, b in zip(before, after))


LOOP_MENU_GUARD = ("LOOP menu order or indices would change; saved break pattern A/B indices could point at "
                   "different loops. The backup is retained. Restore samples manually to a compatible layout, "
                   "then check and re-assign LOOP A/B in every saved pattern before playback.")


def guard_migration_loop_menu(old_lay, images, risk, old_map, new_lay, new_map, todo, skipped):
    """Drop a migration plan that would change existing absolute LOOP indices (fail closed on automatic restore)."""
    if migration_reorders_loop_menu(old_lay, images, risk, old_map, new_lay, new_map, todo):
        extra = list(skipped)
        for k, _j in todo:
            extra.append(f"slot {k + 1}: not restored automatically ({LOOP_MENU_GUARD})")
        return [], extra
    return todo, skipped


def restore_mapped(link, todo, images, progress=None):
    """write images[old] into new slot j for each (old, j); then read each back and compare (SlotError)"""
    for k, j in todo:
        write_slot(link, j, images[k], progress=(lambda d, n, j=j: progress(j, d, n)) if progress else None)
        # Stop migration at the first mismatch; later target slots remain untouched.
        after = layout(link)
        if read_slot(link, j, after["slots"][j]["nz"]) != images[k]:
            raise SlotError(f"slot {j + 1}: read back differs from the backup of slot {k + 1}")


def describe(meta):
    lines = [f"{meta['count']} slot(s), saved {meta['created']}"]
    for e in meta["slots"]:
        if e["empty"]:
            lines.append(f"  slot {e['slot']} ({e['size'] // 1024} KiB): empty")
        else:
            rates = "/".join(str(r) for r in sorted(set(e["rates"])))
            lines.append(f"  slot {e['slot']} ({e['size'] // 1024} KiB): {e['name'] or '-'}, {e['zones']} zone(s), "
                         f"{e['data_len']} B, {rates} Hz")
    return "\n".join(lines)


def _bar(k, d, n):
    print(f"\r  slot {k + 1}: {d} / {n} B", end="", flush=True)


def main(argv):
    if len(argv) < 2 or argv[0] not in ("backup", "restore", "show", "extract"):
        sys.exit(__doc__)
    cmd, path, rest = argv[0], Path(argv[1]), argv[2:]
    if cmd == "show":
        print(describe(read_archive(path.read_bytes())[0]))
        return
    if cmd == "extract":
        if not rest:
            sys.exit(__doc__)
        meta, images = read_archive(path.read_bytes())
        out = Path(rest[0])
        out.mkdir(parents=True, exist_ok=True)
        for k, img in enumerate(images):
            if img is not None:
                (out / f"slot{k + 1}.hdr").write_bytes(img[:HDR_LEN])
                (out / f"slot{k + 1}.bin").write_bytes(img[DATA_OFF:])
        print(f"{sum(i is not None for i in images)} slot image(s) in {out}")
        return
    import fm1_sample_upload as up                     # the MIDI link (mido)
    if cmd == "backup":
        if path.exists() and "--force" not in rest:
            sys.exit(f"{path} exists (--force to replace it)")
        link = up.Link()
        lay, images = backup(link, _bar)
        b = make_archive(lay, images)
        save_backup_archive(path, b, replace="--force" in rest)
        print(f"\n{path}: {len(b)} B\n" + describe(read_archive(b)[0]))
        return
    meta, images = read_archive(path.read_bytes())
    only, yes, verify = None, "--yes" in rest, "--no-verify" not in rest
    if "--slots" in rest:
        only = {int(v) for v in rest[rest.index("--slots") + 1].split(",")}
    link = up.Link()
    lay = layout(link)
    todo, skipped = plan_restore(lay, meta, images, only)
    print(describe(meta))
    for m in skipped:
        print("skip", m)
    if not todo:
        sys.exit("nothing to restore")
    names = ", ".join(str(k + 1) for k, _ in todo)
    if not yes and input(f"This erases slot(s) {names} on the device and writes the archive. Type yes: ") != "yes":
        sys.exit("not restored")
    restore(link, meta, images, only, verify, _bar)
    print(f"\nrestored slot(s) {names}" + (", verified" if verify else ""))


if __name__ == "__main__":
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    try:
        main(sys.argv[1:])
    except (SlotError, TimeoutError) as e:
        sys.exit(f"x0x_slots: {e}")
