# SPDX-License-Identifier: GPL-3.0-only
"""A mocked X0X device for the slot tools (no hardware): firmware/src/app/editor.c's slot commands
(SMP_BEGIN .. SMP_READ, 11-16) and app/slots.c over a NOR flash model (erase sets 0xFF, a write can
only clear bits). Each slot has its own flash offset and size (the slot map). SMP_INFO gives the
count, the smallest size (KiB, 7 bits), per slot zones, name and used KiB, then (ext=True, the
firmware since the SMK-37 flash layout 2) the 'X' 1 extension: per slot offset and size in 4 KiB
sectors and used KiB, 14 bits each. ext=False is an older X0X. The flash (Flash) is shared, so a
firmware change (another slot map over the same flash) keeps what a slot held, as on the device.
Used by tests/x0x_slots_test.py, tests/upload_tools_test.py and tests/install_test.py."""
import struct
import sys
import zlib
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import x0x_slots as xs  # noqa: E402

SMK_MAP = [(0xC2000, 0x14000), (0xD6000, 0x1E000), (0xB0000, 0xB000)]    # the 3-slot layout this firmware replaced
SMK_NOW = [(0xC2000, 0x14000), (0xD6000, 0x1E000)]                        # slots.c, BOARD_SMK37: two slots
SMK_OLD_MAP = [(0xC2000, 0x14000), (0xD6000, 0x14000)]                    # X0X before the layout change


class Flash:
    def __init__(self, size=0x200000):
        self.mem = bytearray(b"\xff" * size)

    def program(self, off, b):
        for i, v in enumerate(b):
            self.mem[off + i] &= v

    def erase4k(self, off):
        self.mem[off:off + 0x1000] = b"\xff" * 0x1000


class MockDevice:
    """slots: [(flash offset, size)]; default: count slots of `size` side by side from 0xC2000"""

    def __init__(self, count=2, size=0x14000, drop_every=0, slots=None, ext=True, flash=None, fail_write_at=None):
        self.slots = list(slots) if slots is not None else [(0xC2000 + k * size, size) for k in range(count)]
        self.count = len(self.slots)
        self.ext, self.fl = ext, flash or Flash()
        self.log = []
        self.drop_every, self.n = drop_every, 0
        self.fail_write_at = fail_write_at              # (slot, offset): that SMP_WRITE returns rc 3 (flash)
        self.nz = [0] * self.count
        self.importing = [False] * self.count           # SMP_BEGIN armed; flash intact until first sector erase
        self.sec0_erased = [False] * self.count
        # SMK FL_SMPV: per-slot 4 KiB shadow of sector 0 (firmware editor.c / slots.c)
        self.backup_base = 0xB0000
        for k in range(self.count):
            self.scan(k)

    @property
    def size(self):
        return min(n for _, n in self.slots)

    @property
    def flash(self):
        return [memoryview(self.fl.mem)[o:o + n] for o, n in self.slots]

    def scan(self, k):
        o, size = self.slots[k]
        h = xs.parse_header(bytes(self.fl.mem[o:o + xs.HDR_LEN]))
        ok = h and h["data_len"] <= size - xs.DATA_OFF and all(z["off"] + (z["n"] + 1) // 2 <= h["data_len"]
                                                                for z in h["zones"]) and \
            zlib.crc32(bytes(self.fl.mem[o + xs.DATA_OFF:o + xs.DATA_OFF + h["data_len"]])) == h["crc"]
        self.nz[k] = h["nz"] if ok else 0

    def data_len(self, k):
        return struct.unpack_from("<I", self.fl.mem, self.slots[k][0] + 16)[0] if self.nz[k] else 0

    def _backup_off(self, k):
        return self.backup_base + k * 0x1000

    def _backup_present(self, k):
        return xs.parse_header(bytes(self.fl.mem[self._backup_off(k):self._backup_off(k) + xs.HDR_LEN])) is not None

    def _backup_sec0(self, k):
        base = self.slots[k][0]
        dst = self._backup_off(k)
        self.fl.erase4k(dst)
        self.fl.program(dst, bytes(self.fl.mem[base:base + 0x1000]))

    def _clear_backup(self, k):
        if self._backup_present(k):
            self.fl.erase4k(self._backup_off(k))

    def _try_restore_sec0(self, k):
        if self.nz[k]:
            self._clear_backup(k)
            return
        if not self._backup_present(k):
            return
        base = self.slots[k][0]
        src = self._backup_off(k)
        self.fl.erase4k(base)
        self.fl.program(base, bytes(self.fl.mem[src:src + 0x1000]))
        self.scan(k)
        self.sec0_erased[k] = False
        self.importing[k] = False
        self._clear_backup(k)

    def req(self, cmd, a, timeout=3.0):
        self.log.append(cmd)
        self.n += 1
        if self.drop_every and self.n % self.drop_every == 0:
            raise TimeoutError(f"no reply to cmd {cmd}")     # a lost frame
        if cmd == 1:
            return [0]
        if cmd == xs.CMD_INFO:
            r = [self.count, min(127, self.size // 1024)]
            for k, (o, _) in enumerate(self.slots):
                nm = bytes(self.fl.mem[o + 8:o + 16]).split(b"\0")[0] if self.nz[k] else b""
                r += [self.nz[k]] + [c & 0x7F for c in nm] + [0, min(127, (self.data_len(k) + 1023) // 1024)]
            if self.ext:
                r += [ord("X"), 1]
                for k, (o, n) in enumerate(self.slots):
                    for v in (o // 0x1000, n // 0x1000, (self.data_len(k) + 1023) // 1024):
                        r += [v & 0x7F, (v >> 7) & 0x7F]
            return r
        k = a[0]
        if k >= self.count:
            raise TimeoutError("no reply (slot out of range)")
        base, size = self.slots[k]
        if cmd == xs.CMD_READ:
            off = a[1] | a[2] << 7 | a[3] << 14
            if off >= size:
                raise TimeoutError("no reply (offset past the slot)")
            n = min(xs.READ_MAX if a[4] > xs.READ_MAX // 2 else a[4] * 2, size - off)
            return [k, off & 0x7F, (off >> 7) & 0x7F, (off >> 14) & 0x7F] + \
                xs.pack7(bytes(self.fl.mem[base + off:base + off + n]))
        if cmd == xs.CMD_BEGIN:
            # Arm rewrite only. Do not erase: cancel before the first WRITE keeps last good.
            # A prior mid-WRITE abandon restores sector 0 from FL_SMPV first.
            self._try_restore_sec0(k)
            self.importing[k] = True
            self.sec0_erased[k] = False
            return [k, 0]
        if cmd == xs.CMD_ERASE:
            self.nz[k] = 0
            self.importing[k] = False
            self.sec0_erased[k] = False
            self._clear_backup(k)
            for s in range(size // 0x1000):
                self.fl.erase4k(base + s * 0x1000)
            return [k, 0]
        if cmd == xs.CMD_WRITE:
            off = a[1] | a[2] << 7 | a[3] << 14
            b = xs.unpack7(a[4:])
            rc = 0
            if off < xs.DATA_OFF or off & 0xFF or not b or len(b) > 256 or off + len(b) > size:
                rc = 1
            elif (not self.importing[k] and self.nz[k]):
                rc = 4
            elif self.fail_write_at == (k, off):
                rc = 3
            else:
                if off < 0x1000:
                    if not self.sec0_erased[k]:
                        if self.nz[k]:
                            self._backup_sec0(k)
                        self.fl.erase4k(base)
                        self.nz[k] = 0
                        self.sec0_erased[k] = True
                elif not off & 0xFFF:
                    # Higher sector: FL_SMPV only shadows sector 0. Wipe sec0 too if
                    # still live, then drop any shadow so abort cannot restore a
                    # header over destroyed higher-sector last-good.
                    if not self.sec0_erased[k]:
                        self.fl.erase4k(base)
                        self.sec0_erased[k] = True
                    self.fl.erase4k(base + off)
                    self.nz[k] = 0
                    self._clear_backup(k)
                self.fl.program(base + off, b)
            return [k, off & 0x7F, (off >> 7) & 0x7F, (off >> 14) & 0x7F, rc]
        if cmd == xs.CMD_END:
            h = xs.unpack7(a[1:])
            p = xs.parse_header(h)
            if len(h) != xs.HDR_LEN:
                self._try_restore_sec0(k)
                return [k, 1]
            if not p or p["data_len"] > size - xs.DATA_OFF:
                self._try_restore_sec0(k)
                return [k, 2]
            # NOR cannot replace set bits: refuse END until a WRITE erased sector 0.
            if not self.sec0_erased[k]:
                self.importing[k] = False
                return [k, 6]
            if zlib.crc32(bytes(self.fl.mem[base + xs.DATA_OFF:base + xs.DATA_OFF + p["data_len"]])) != p["crc"]:
                self._try_restore_sec0(k)
                return [k, 3]
            self.fl.program(base, h)
            self.scan(k)
            self.importing[k] = False
            self.sec0_erased[k] = False
            self._clear_backup(k)
            return [k, 0 if self.nz[k] else 5]
        raise TimeoutError(f"no reply to cmd {cmd}")

    def load(self, k, hdr, data):
        """as an upload would leave it"""
        base, size = self.slots[k]
        for s in range(0, size, 0x1000):
            self.fl.erase4k(base + s)
        self.fl.program(base, hdr)
        self.fl.program(base + xs.DATA_OFF, data)
        self.scan(k)

    def image(self, k):
        if not self.nz[k]:
            return None
        o = self.slots[k][0]
        return bytes(self.fl.mem[o:o + xs.DATA_OFF + self.data_len(k)])
