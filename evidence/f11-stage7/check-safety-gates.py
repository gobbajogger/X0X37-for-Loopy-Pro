#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Candidate-specific safety gates for F11 Stage 7 page digests.

Chain:
  blank Elite/Pro + ctl (stage-4 / stage-7 rebuild)
  -> add_ctl join == page Elite/Pro digests 4841110a… / b35d5875…
  -> host installer-slots / slot-backup / f03-probe
  -> FakeX0X (with CTL_READ) backup+install and mid-restore interrupt using blank .fwsc + --ctl
  -> ota product identity on blank and page packages
"""
from __future__ import annotations

import hashlib
import math
import os
import subprocess
import sys
import tempfile
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(ROOT / "tests"), str(ROOT / "tools")]
PKG = Path(os.environ.get("F11_STAGE7_PKGS", "/tmp/f11-stage7-packages"))

BLANK_ELITE = "829bb6da6eb0042af0731a236d6238e50cc13581ce059ebbe02882534a94b056"
BLANK_PRO = "d2186cb0f3fe74376651cb2804973c8255780532a8cd54c33d2bac09aeda60ec"
PAGE_ELITE = "4841110a274e2212d56abf9234cf0d921b45d915d1ff25fad0727e52edb7d18a"
PAGE_PRO = "b35d587557d9f1d4c72b2c81bc944a1dd129bee625e98ae35f06fe7f653fc720"
CTL = "956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a"
SRC = "4f96b0d9f5f11f700ea3fc935800b8714e11f35d"


def sha(p: Path) -> str:
    h = hashlib.sha256()
    with p.open("rb") as f:
        for c in iter(lambda: f.read(1 << 20), b""):
            h.update(c)
    return h.hexdigest()


def run(script: str) -> None:
    r = subprocess.run([sys.executable, str(ROOT / script)], cwd=str(ROOT), capture_output=True, text=True)
    sys.stdout.write(r.stdout)
    if r.stderr:
        sys.stdout.write(r.stderr)
    if r.returncode:
        raise SystemExit(f"FAIL {script} exit={r.returncode}")


def main() -> int:
    be, bp, ctl_p = PKG / "elite.fwsc", PKG / "pro.fwsc", PKG / "ctl.img"
    ee, ep = PKG / "elite-bundled.fwsc", PKG / "pro-bundled.fwsc"
    for p, exp, name in (
        (be, BLANK_ELITE, "blank-elite"), (bp, BLANK_PRO, "blank-pro"), (ctl_p, CTL, "ctl"),
        (ee, PAGE_ELITE, "page-elite"), (ep, PAGE_PRO, "page-pro"),
    ):
        assert p.is_file(), p
        got = sha(p)
        print(f"digest {name} size={p.stat().st_size} sha256={got} {'MATCH' if got == exp else 'FAIL'}")
        assert got == exp, name

    import add_ctl
    import importlib
    importlib.reload(add_ctl)
    REAL_CTL_OF = add_ctl.controller_of

    # Prove page digests are the add_ctl join of blank+ctl (candidate-specific byte chain)
    for blank, page_exp, label in ((be, PAGE_ELITE, "elite"), (bp, PAGE_PRO, "pro")):
        out = Path(tempfile.mkdtemp()) / f"{label}-join.fwsc"
        subprocess.check_call([sys.executable, str(ROOT / "tools/add_ctl.py"), str(blank), str(ctl_p), "-o", str(out)])
        got = sha(out)
        print(f"join {label} blank+ctl -> {got} {'MATCH page' if got == page_exp else 'FAIL'}")
        assert got == page_exp, label

    print("--- host backup/restore/interruption suites ---")
    run("tests/install_slots_test.py"); print("PASS installer-slots")
    run("tests/x0x_slots_test.py"); print("PASS slot-backup")
    run("tests/install_f03_slot_probe_test.py"); print("PASS installer-f03-probe")

    import install_test as T
    import fm1_install as I
    import x0x_slots as xs
    import sampleio as sio
    from slot_mock import Flash, MockDevice, SMK_MAP

    add_ctl.controller_of = REAL_CTL_OF
    I.add_ctl.controller_of = REAL_CTL_OF

    def tone(n, seed):
        return [int(9000 * math.sin(0.01 * seed * i)) + ((i * seed) % 97) - 48 for i in range(n)]

    def put(fl, smap, idx, name, samples, seed):
        hdr, data = sio.user_slot(name, [(tone(samples, seed), 36, None, None)], rate=44100, slot_size=smap[idx][1])
        m = MockDevice(slots=smap, flash=fl)
        m.load(idx, hdr, data)
        return m.image(idx)

    # Full 64 KiB Bluetooth block as firmware/src/app/editor.c ED_CTL_READ serves it.
    CTL_BLOCK = 0x10000
    ctl_bytes = ctl_p.read_bytes()
    ctl_block = ctl_bytes + b"\xFF" * (CTL_BLOCK - len(ctl_bytes))
    assert len(ctl_block) == CTL_BLOCK
    I.CTL_READ.update(timeout=0.2, tries=2)

    def ctl_read_reply(args):
        if len(args) < 4:
            return None
        off = args[0] | args[1] << 7 | args[2] << 14
        n = 224 if args[3] > 112 else args[3] * 2
        if off >= CTL_BLOCK:
            return None
        n = min(n, CTL_BLOCK - off)
        return [off & 0x7F, (off >> 7) & 0x7F, (off >> 14) & 0x7F] + xs.pack7(ctl_block[off:off + n])

    class FakeX0X(T.FakeSMK):
        def __init__(self, image, identity, after_write, firmware, flash, fail_write_at=None):
            self.firmware, self.fl, self.fail = firmware, flash, fail_write_at
            self.slot_cmds, self.mock, self.ctl_reads = [], None, 0
            name = "SMK-37 Pro" if "Pro" in after_write else "SMK-37 Elite"
            super().__init__(image, identity=identity, name=name, after_write=after_write)

        def boot(self, identity, name):
            super().boot(identity, name)
            fw = self.firmware.get(identity)
            fail = self.fail if identity == self.after_write else None
            self.mock = MockDevice(slots=fw[0], ext=fw[1], flash=self.fl, fail_write_at=fail) if fw else None

        def rx(self, d):
            if d[:4] == I.ED_HDR and d[-1] == 0xF7:
                cmd = d[4]
                # install_ctl_test.FakeBT: answer ED_CTL_READ for the candidate controller block
                if cmd == I.ED_CTL_READ and not self.identity.startswith("ota-"):
                    r = ctl_read_reply(list(d[5:-1]))
                    self.ctl_reads += 1
                    if r is not None:
                        self.tx(I.ED_HDR + bytes([cmd]) + bytes(r) + b"\xF7")
                    return
                if not self.mock:
                    return
                self.slot_cmds.append((self.identity, cmd))
                try:
                    r = self.mock.req(cmd, list(d[5:-1]))
                except TimeoutError:
                    return
                self.tx(I.ED_HDR + bytes([cmd]) + bytes(r) + b"\xF7")
                return
            super().rx(d)

    print("--- candidate blank+ctl identity / slot map ---")
    for label, path, blank_exp, prod_exp in (
        ("pro", bp, BLANK_PRO, "SMK-37 Pro_900"),
        ("elite", be, BLANK_ELITE, "SMK-37 Elite_900"),
    ):
        prod, img = I.load_package(str(path), False)
        assert prod == prod_exp and sha(path) == blank_exp
        assert I.package_slot_map(img, prod) == SMK_MAP
        assert hashlib.sha256(ctl_p.read_bytes()).hexdigest() == CTL
        print(f"PASS {label} blank load+slotmap+ctl ({blank_exp[:12]}…)")

    print("--- candidate blank+ctl backup+install and mid-restore interrupt ---")
    tmp = Path(tempfile.mkdtemp(prefix="f11-cand-safety-"))
    os.chdir(tmp)
    for label, path, page_exp, prod_exp in (
        ("pro", bp, PAGE_PRO, "SMK-37 Pro_900"),
        ("elite", be, PAGE_ELITE, "SMK-37 Elite_900"),
    ):
        prod, img = I.load_package(str(path), False)
        logical = I.logical_image(path.read_bytes())
        fl = Flash()
        put(fl, SMK_MAP, 0, "BR1", 24000, 41)
        put(fl, SMK_MAP, 1, "SD1", 24000, 42)
        dev = FakeX0X(logical, prod, prod, {prod: (SMK_MAP, True)}, fl)
        bak = tmp / f"{label}-backup.x0xslots"
        rc, out, err = T.cli([str(path), "--yes", "--backup", str(bak), "--ctl", str(ctl_p)], dev)
        assert rc == 0, (label, "backup+install", rc, out[-800:], err[-800:])
        assert bak.is_file() and bak.stat().st_size > 32
        assert any(c[1] in (xs.CMD_INFO, xs.CMD_READ) for c in dev.slot_cmds), dev.slot_cmds
        assert dev.ctl_reads > 0 and f"unit CRC32 {zlib.crc32(ctl_block) & 0xFFFFFFFF:08x}" in out, (dev.ctl_reads, out[-400:])
        print(f"PASS {label} backup-restore-readback page={page_exp[:12]}… backup_bytes={bak.stat().st_size} ctl_reads={dev.ctl_reads}")

        # Same pattern as tests/install_slots_test.fails: pre-install map differs so
        # write-back runs; mid-restore SMP_WRITE fails closed (exit 7), backup retained.
        moved = [(0xC2000, 0x14000), (0xD7000, 0x1D000), (0xB0000, 0xB000)]
        fl2 = Flash()
        put(fl2, moved, 0, "BR1", 120000, 61)
        put(fl2, moved, 1, "BR2", 150000, 62)
        pre = "SMK-37 Pro_9001000" if "Pro" in prod else "SMK-37 Elite_9001000"
        fail_at = (1, 0x4200)
        firmware2 = {pre: (moved, True), prod: (SMK_MAP, True)}
        dev2 = FakeX0X(logical, pre, prod, firmware2, fl2, fail_write_at=fail_at)
        bak2 = tmp / f"{label}-interrupt.x0xslots"
        rc2, out2, err2 = T.cli([str(path), "--yes", "--backup", str(bak2), "--ctl", str(ctl_p)], dev2)
        assert rc2 == I.EXIT["slots"] == 7, (label, "expected interrupt exit 7", rc2, out2[-600:], err2[-600:])
        assert bak2.is_file() and bak2.stat().st_size > 32
        assert "not written back" in err2 and str(bak2) in err2
        assert "done:" in out2  # firmware install completed before fail-closed restore
        print(f"PASS {label} interruption-recovery fail-closed page={page_exp[:12]}… exit={rc2}")

    print("--- ota-target-binding (blank + page package product identity) ---")
    for label, blank, page, prod_exp in (
        ("pro", bp, ep, "SMK-37 Pro_900"),
        ("elite", be, ee, "SMK-37 Elite_900"),
    ):
        for kind, path in (("blank", blank), ("page", page)):
            prod, _ = I.load_package(str(path), False)
            assert prod == prod_exp, (label, kind, prod, prod_exp)
            assert ("Elite" in prod) == (label == "elite")
            assert ("Pro" in prod) == (label == "pro")
        assert sha(page) == (PAGE_PRO if label == "pro" else PAGE_ELITE)
        print(f"PASS {label} ota-target-binding blank+page product={prod_exp} page={sha(page)[:12]}…")

    print("ALL_PASS")
    print(f"source_commit {SRC}")
    print(f"page_elite {PAGE_ELITE}")
    print(f"page_pro {PAGE_PRO}")
    print(f"blank_elite {BLANK_ELITE}")
    print(f"blank_pro {BLANK_PRO}")
    print(f"ctl {CTL}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as e:
        print("ASSERT", e)
        raise SystemExit(1)
