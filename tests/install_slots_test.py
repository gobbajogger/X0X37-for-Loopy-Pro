#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""tools/fm1_install.py with sample slots on a simulated SMK-37 (no hardware, no device writes):
an install that would lose slot data saves every slot to an .x0xslots file (SMP_INFO / SMP_READ),
installs, and writes the slots back into the new firmware's slot map, then reads them back.
The fake keyboard is tests/install_test.py's FakeSMK with tests/slot_mock.py's slot commands over one
flash model, so the slot bytes stay over an install as on the device. Run from the repo root:
  python3 tests/install_slots_test.py"""
import math
import os
import sys
import tempfile
import zlib
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import install_test as T  # noqa: E402  (FakeSMK, package, cli; its tests run only as __main__)
from slot_mock import Flash, MockDevice, SMK_MAP, SMK_NOW, SMK_OLD_MAP  # noqa: E402
import fm1pkg_make  # noqa: E402
import x0x_slots as xs  # noqa: E402
import sampleio as sio  # noqa: E402

I = T.I
ok = T.ok
TMP = Path(tempfile.mkdtemp(prefix="x0x-install-slots-"))
os.chdir(TMP)                                         # the default backup file lands here
PRO = "SMK-37 Pro_900"


def package(product=PRO, slotmap=None, marker=True):
    raw = bytearray(T.package(product, marker=marker))
    if slotmap is not None:
        b = fm1pkg_make.slotmap_blob(slotmap)
        raw[0x1A00:0x1A00 + len(b)] = b
    return bytes(raw)


def pkgfile(name, raw):
    p = TMP / name
    p.write_bytes(raw)
    return str(p)


class FakeX0X(T.FakeSMK):
    """an SMK-37 with X0X: firmware = {identity: (slot map, SMP_INFO extension) or None (no X0X slots)}"""

    def __init__(self, image, identity, after_write, firmware, flash, fail_write_at=None, stock_erases=()):
        self.firmware, self.fl, self.fail = firmware, flash, fail_write_at
        self.stock_erases, self.slot_cmds, self.mock = stock_erases, [], None
        super().__init__(image, identity=identity, name="SMK-37 Pro", after_write=after_write)

    def boot(self, identity, name):
        super().boot(identity, name)
        fw = self.firmware.get(identity)
        if fw is None and not identity.startswith("ota-"):
            for lo, hi in self.stock_erases:             # stock firmware: its VM init may clear the area
                for o in range(lo, hi, 0x1000):
                    self.fl.erase4k(o)
        # the fail point is armed only on the firmware after the install
        fail = self.fail if identity == self.after_write else None
        self.mock = MockDevice(slots=fw[0], ext=fw[1], flash=self.fl, fail_write_at=fail) if fw else None

    def rx(self, d):
        if d[:4] == I.ED_HDR and d[-1] == 0xF7:
            if not self.mock:
                return                                   # stock firmware or loader: no answer
            cmd = d[4]
            self.slot_cmds.append((self.identity, cmd))
            try:
                r = self.mock.req(cmd, list(d[5:-1]))
            except TimeoutError:
                return
            self.tx(I.ED_HDR + bytes([cmd]) + bytes(r) + b"\xF7")
            return
        super().rx(d)


def tone(n, seed):
    return [int(9000 * math.sin(0.01 * seed * i)) + ((i * seed) % 97) - 48 for i in range(n)]


def put(flash, slot_map, k, name, samples, seed):
    """an upload into slot k of a firmware with slot_map (as upload_breaks leaves it) -> the slot image"""
    hdr, data = sio.user_slot(name, [(tone(samples, seed), 36, None, None)], rate=44100, slot_size=slot_map[k][1])
    m = MockDevice(slots=slot_map, flash=flash)
    m.load(k, hdr, data)
    return m.image(k)


def read(flash, slot_map, ext=True):
    m = MockDevice(slots=slot_map, ext=ext, flash=flash)
    return [m.image(k) for k in range(m.count)]


NEW = (SMK_MAP, True)                  # the previous 3-slot X0X (installer scenarios)
NOW = (SMK_NOW, True)                  # this firmware: two slots, same offsets for 1 and 2
NOW_PKG = "SMK-37 Pro_9001200"
OLD = (SMK_OLD_MAP, False)             # X0X before the layout change: 2 x 80 KiB, no extension
MOVED = [(0xC2000, 0x14000), (0xD7000, 0x1D000), (0xB0000, 0xB000)]    # a later map: slot 2 one sector up
NEW_PKG = "SMK-37 Pro_9001100"
OLD_PKG = "SMK-37 Pro_9001000"


def backups():
    return sorted(TMP.glob("*.x0xslots"))


def clear():
    for f in backups():
        f.unlink()


def upgrade_keeps():
    fl = Flash()
    a = put(fl, SMK_OLD_MAP, 0, "BR1", 120000, 3)
    b = put(fl, SMK_OLD_MAP, 1, "BR2", 140000, 5)
    raw = package(NEW_PKG, SMK_MAP)
    dev = FakeX0X(I.logical_image(raw), OLD_PKG, NEW_PKG, {OLD_PKG: OLD, NEW_PKG: NEW}, fl)
    rc, out, err = T.cli([pkgfile("new.fwsc", raw), "--yes"], dev)
    writes = [c for _, c in dev.slot_cmds if c != xs.CMD_INFO]
    ok(rc == 0 and not backups() and writes == [] and read(fl, SMK_MAP)[:2] == [a, b] and "lose" not in out,
       "old X0X -> this X0X: the slots stay, no backup, no slot command but SMP_INFO")


def explicit_backup_keeps():
    """--backup is a fresh verified prewrite backup even when no slot is at risk."""
    fl = Flash()
    a = put(fl, SMK_MAP, 0, "BR1", 120000, 3)
    b = put(fl, SMK_MAP, 1, "BR2", 140000, 5)
    raw = package(NEW_PKG, SMK_MAP)
    dev = FakeX0X(I.logical_image(raw), NEW_PKG, NEW_PKG, {NEW_PKG: NEW}, fl)
    rc, out, err = T.cli([pkgfile("same.fwsc", raw), "--yes", "--backup", "fresh.x0xslots"], dev)
    f = TMP / "fresh.x0xslots"
    meta, imgs = xs.read_archive(f.read_bytes()) if f.exists() else (None, None)
    first_install = next(i for i, p in enumerate(dev.sent) if p == I.UPGRADE)
    ok(rc == 0 and imgs[:2] == [a, b] and "slots saved: fresh.x0xslots" in out,
       "explicit --backup: every slot saved and verified with zero at-risk slots")
    ok(dev.sent[first_install - 1][:4] == I.ED_HDR and dev.upgrades == 2,
       "explicit --backup: reads finish before the first update command")
    clear()


def downgrade():
    """this X0X -> older X0X: skipping oversized slot 2 must not assign its LOOP index to slot 3"""
    fl = Flash()
    a = put(fl, SMK_MAP, 0, "BR1", 120000, 3)
    b = put(fl, SMK_MAP, 1, "BIG", 200000, 5)          # about 98 KiB: more than an old 80 KiB slot
    c = put(fl, SMK_MAP, 2, "BR3", 60000, 7)
    raw = package(OLD_PKG)                             # Felucca loader, no slot map: 2 x 80 KiB
    dev = FakeX0X(I.logical_image(raw), NEW_PKG, OLD_PKG, {OLD_PKG: OLD, NEW_PKG: NEW}, fl)
    rc, out, err = T.cli([pkgfile("old.fwsc", raw), "--yes", "--backup", "before.x0xslots"], dev)
    f = TMP / "before.x0xslots"
    meta, imgs = xs.read_archive(f.read_bytes()) if f.exists() else (None, None)
    ok(rc == I.EXIT["slots"] == 7 and imgs == [a, b, c],
       "downgrade: every slot saved before install; incomplete restoration returns exit 7")
    first_install = next(i for i, p in enumerate(dev.sent) if p == I.UPGRADE)
    reads_before = [c for idn, c in dev.slot_cmds if idn == NEW_PKG]
    ok(set(reads_before) <= {xs.CMD_INFO, xs.CMD_READ} and dev.sent[first_install - 1][:4] == I.ED_HDR,
       "downgrade: the backup sends SMP_INFO / SMP_READ only, and before the install")
    ok("slot 2 (BIG, 98 KiB), slot 3 (BR3, 30 KiB)" in out, "downgrade: the warning names the slots at risk")
    now = read(fl, SMK_OLD_MAP, ext=False)
    ok(now == [a, None] and not any(cmd in (xs.CMD_BEGIN, xs.CMD_WRITE, xs.CMD_END, xs.CMD_ERASE)
                                     for identity, cmd in dev.slot_cmds if identity == OLD_PKG)
       and "LOOP menu order or indices" in out and "not written back: slot 2" in out,
       "downgrade: kept slot unchanged, no restore writes before manual LOOP index reconciliation")
    ok("x0x_slots.py restore before.x0xslots" in err and "not written back" in err,
       "downgrade: exit 7 exposes retained archive recovery instructions")


def moved():
    """a later map moves slot 2: it is saved, installed and written back into the moved slot 2"""
    fl = Flash()
    a = put(fl, SMK_MAP, 0, "BR1", 120000, 3)
    b = put(fl, SMK_MAP, 1, "BR2", 150000, 5)
    raw = package(NEW_PKG, MOVED)
    dev = FakeX0X(I.logical_image(raw), "SMK-37 Pro_9001000", NEW_PKG, {"SMK-37 Pro_9001000": NEW, NEW_PKG: (MOVED, True)}, fl)
    rc, out, err = T.cli([pkgfile("moved.fwsc", raw), "--yes"], dev)
    ok(rc == 0 and len(backups()) == 1 and backups()[0].name.startswith("x0x-slots-before-install-")
       and read(fl, MOVED)[:2] == [a, b] and "slot 2 written back to slot 2 and read back: ok" in out,
       "moved slot: default backup file, slot 2 written back at its new offset, slot 1 kept")
    clear()
    return fl


def fails():
    """the write back fails in slot 2: exit 7, the backup stays, the message tells how to restore it"""
    fl = Flash()
    a = put(fl, SMK_MAP, 0, "BR1", 120000, 3)
    b = put(fl, SMK_MAP, 1, "BR2", 150000, 5)
    raw = package(NEW_PKG, MOVED)
    dev = FakeX0X(I.logical_image(raw), "SMK-37 Pro_9001000", NEW_PKG,
                  {"SMK-37 Pro_9001000": NEW, NEW_PKG: (MOVED, True)}, fl, fail_write_at=(1, 0x4200))
    rc, out, err = T.cli([pkgfile("moved.fwsc", raw), "--yes", "--backup", "keep.x0xslots"], dev)
    f = TMP / "keep.x0xslots"
    ok(rc == I.EXIT["slots"] == 7 and f.exists() and xs.read_archive(f.read_bytes())[1][:2] == [a, b],
       "failure mid-restore: exit 7, the backup file stays and reads back")
    ok("not written back" in err and "x0x_slots.py restore keep.x0xslots" in err and "Restore" in err,
       "failure mid-restore: the message gives the Restore button and the x0x_slots.py command")
    ok("done: the FM-1 runs" in out or "done:" in out, "failure mid-restore: the firmware install itself completed")
    # the manual restore with the tool then works (the fail point was one write)
    dev2 = MockDevice(slots=MOVED, ext=True, flash=fl)
    meta, imgs = xs.read_archive(f.read_bytes())
    lay = xs.layout(dev2)
    todo, skipped = xs.plan_migration(imgs, [1], SMK_MAP, lay, MOVED)
    xs.restore_mapped(dev2, todo, imgs)
    ok(read(fl, MOVED)[:2] == [a, b], "failure mid-restore: a later restore from the backup completes")


def backup_fails():
    fl = Flash()
    put(fl, SMK_MAP, 1, "BIG", 200000, 5)
    raw = package(OLD_PKG)
    dev = FakeX0X(I.logical_image(raw), NEW_PKG, OLD_PKG, {OLD_PKG: OLD, NEW_PKG: NEW}, fl)
    dev.firmware = dict(dev.firmware)
    real = dev.mock.req
    dev.mock.req = lambda cmd, a, timeout=3.0: (_ for _ in ()).throw(TimeoutError()) if cmd == xs.CMD_READ else real(cmd, a)
    rc, out, err = T.cli([pkgfile("old.fwsc", raw), "--yes", "--backup", "bad.x0xslots"], dev)
    ok(rc == 7 and dev.upgrades == 0 and "nothing was written" in err, "backup fails: exit 7, nothing installed")


def stock():
    fl = Flash()
    a = put(fl, SMK_MAP, 0, "BR1", 120000, 3)
    c = put(fl, SMK_MAP, 2, "BR3", 60000, 7)
    raw = package("SMK-37 Pro_016", marker=False)
    dev = FakeX0X(I.logical_image(raw), NEW_PKG, "SMK-37 Pro_016", {NEW_PKG: NEW}, fl, stock_erases=[(0x9D000, 0xC0000)])
    rc, out, err = T.cli([pkgfile("stock.fwsc", raw), "--yes", "--force", "--backup", "stock.x0xslots"], dev)
    f = TMP / "stock.x0xslots"
    ok(rc == I.EXIT["slots"] == 7 and f.exists() and xs.read_archive(f.read_bytes())[1] == [a, None, c]
       and "x0x_slots.py restore stock.x0xslots" in err and "no X0X sample slots" in err,
       "stock package: archived samples retained, no slot destination, incomplete exit 7 with restore steps")
    clear()


def opt_out():
    fl = Flash()
    put(fl, SMK_MAP, 2, "BR3", 60000, 7)
    raw = package(OLD_PKG)
    dev = FakeX0X(I.logical_image(raw), NEW_PKG, OLD_PKG, {OLD_PKG: OLD, NEW_PKG: NEW}, fl)
    rc, out, err = T.cli([pkgfile("old.fwsc", raw), "--yes", "--no-slots"], dev)
    ok(rc == 0 and dev.slot_cmds == [] and not backups(), "--no-slots: no slot command, no backup")
    dev = FakeX0X(I.logical_image(raw), NEW_PKG, OLD_PKG, {OLD_PKG: OLD, NEW_PKG: NEW}, fl)
    rc, out, err = T.cli([pkgfile("old.fwsc", raw)], dev, answer=False)
    ok(rc == 1 and not backups() and dev.upgrades == 0 and "lose" in out, "answer no: warning shown, nothing saved or sent")


def old_package_without_controller():
    """an old-package transport fixture still installs without implicit cached-controller selection"""
    fl = Flash()
    a = put(fl, SMK_OLD_MAP, 0, "BR1", 120000, 3)
    raw = package(OLD_PKG)
    dev = FakeX0X(I.logical_image(raw), NEW_PKG, OLD_PKG, {OLD_PKG: OLD, NEW_PKG: NEW}, fl)
    rc, out, err = T.cli([pkgfile("old.fwsc", raw), "--yes", "--no-slots"], dev)
    ok(rc == 0 and "done:" in out and "added to the package" not in out,
       "old package transport: no controller is selected implicitly")


def drop_third():
    """a 3-slot keyboard -> this 2-slot map. Slots 1 and 2 stay. A filled slot 3 is saved."""
    fl = Flash()
    a = put(fl, SMK_MAP, 0, "BR1", 120000, 3)
    b = put(fl, SMK_MAP, 1, "BR2", 140000, 5)
    c = put(fl, SMK_MAP, 2, "BR3", 60000, 7)
    raw = package(NOW_PKG, SMK_NOW)
    dev = FakeX0X(I.logical_image(raw), NEW_PKG, NOW_PKG, {NEW_PKG: NEW, NOW_PKG: NOW}, fl)
    rc, out, err = T.cli([pkgfile("two.fwsc", raw), "--yes", "--backup", "drop3.x0xslots"], dev)
    f = TMP / "drop3.x0xslots"
    meta, imgs = xs.read_archive(f.read_bytes()) if f.exists() else (None, None)
    ok(rc == I.EXIT["slots"] == 7 and imgs == [a, b, c] and read(fl, SMK_NOW) == [a, b],
       "3 slots -> 2: slots 1 and 2 stay, slot 3 is saved, both remaining slots are full")
    ok("slot 3 (BR3, 30 KiB)" in out and "not written back: slot 3" in out,
       "3 slots -> 2: the warning names slot 3 and the restore reports it")
    clear()
    fl = Flash()
    a = put(fl, SMK_MAP, 1, "LOOPA", 120000, 5)
    b = put(fl, SMK_MAP, 2, "LOOPB", 60000, 7)
    raw = package(NOW_PKG, SMK_NOW)
    dev = FakeX0X(I.logical_image(raw), NEW_PKG, NOW_PKG, {NEW_PKG: NEW, NOW_PKG: NOW}, fl)
    rc, out, err = T.cli([pkgfile("two-free.fwsc", raw), "--yes", "--backup", "drop3b.x0xslots"], dev)
    ok(rc == I.EXIT["slots"] == 7 and read(fl, SMK_NOW) == [None, a]
       and "not restored automatically" in out and "LOOP menu order" in out,
       "3 slots -> 2: empty slot 1 + A in 2 + B in 3 must not auto-restore B into slot 1 (break indices)")
    ok("x0x_slots.py restore drop3b.x0xslots" in err,
       "3 slots -> 2: loop-menu guard leaves manual restore instructions")
    clear()


def maps():
    ok(I.package_slot_map(I.logical_image(package(NEW_PKG, SMK_MAP)), NEW_PKG) == SMK_MAP
       and I.package_slot_map(I.logical_image(package(OLD_PKG)), OLD_PKG) == SMK_OLD_MAP
       and I.package_slot_map(I.logical_image(package("SMK-37 Pro_016", marker=False)), "SMK-37 Pro_016") == [],
       "package slot map: the map record, else the old 2 x 80 KiB (Felucca loader), else none (stock)")
    fm1, fm1_now = "FM-1_900", fm1pkg_make.SLOTMAP["fm1"]
    fm1_old = [(0xA0000 + k * 0x14000, 0x14000) for k in range(3)]
    ok(I.package_slot_map(I.logical_image(package(fm1, fm1_now)), fm1) == fm1_now
       and I.package_slot_map(I.logical_image(package(fm1)), fm1) == fm1_old,
       "FM-1: a package with the record has two slots; an older package with no record stays at three")
    sdk = Path(os.environ.get("AC79_SDK", Path.home() / "fw-AC79_AIoT_SDK"))
    if (sdk / "cpu/wl82/tools/uboot.boot").is_file():
        os.environ["AC79_SDK"] = str(sdk)
        fm1pkg_make.set_board("fm1")
        flash = fm1pkg_make.flash_image(b"\x00" * 64, fm1pkg_make.KEY)
        ok(fm1pkg_make.slotmap_of(flash) == fm1_now, "an FM-1 image carries the two-slot map")
        fm1pkg_make.set_board("smk37")
    p = ROOT / "build/x0x-smk37.fwsc"
    if p.exists():
        prod, img = I.load_package(str(p), False)
        ok(I.package_slot_map(img, prod) == fm1pkg_make.SLOTMAP["smk37"] == SMK_NOW,
           "build/x0x-smk37.fwsc carries this firmware's slot map")


ROOT = HERE.parent
maps()
upgrade_keeps()
explicit_backup_keeps()
downgrade()
clear()
drop_third()
moved()
fails()
backup_fails()
stock()
opt_out()
old_package_without_controller()
os.chdir(HERE)
__import__("shutil").rmtree(TMP, ignore_errors=True)
print(f"INSTALL SLOT TESTS FAILED ({T.failed})" if T.failed else "install slot tests passed")
sys.exit(1 if T.failed else 0)
