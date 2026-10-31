#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
"""tools/fm1_install.py against a simulated SMK-37 Pro (no hardware, no mido).
The fake device is the one in web/test_web.mjs: identity on the handshake,
then "device asks, host answers" reads of the logical image. Run from the repo root:
  python3 tests/install_test.py
Also checks logical_image/product_of against web/fm1pkg.js (needs node and
build/x0x-smk37.fwsc, skipped otherwise)."""
import io
import queue
import shutil
import subprocess
import sys
import tempfile
import threading
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import fm1_install as I  # noqa: E402

# These transport fixtures predate the real encrypted package builder and have no parseable app
# area. Treat their controller range like an old package; tests/ble_pkg_test.py covers real packages.
def synthetic_old_package(_image):
    raise I.add_ctl.NoBlock(I.add_ctl.NO_BLOCK)


I.add_ctl.controller_of = synthetic_old_package

I.DELAY.update(open=0, start=0.01, reply=0, loader=0.01, reboot=0.01, retry=0.02, poll=0.05, hs=0.1,
               idle_check=0.4, idle_write=0.4, wait_loader=2, wait_reboot=2)
I.SLOT_PROBE.update(timeout=0.05, tries=1)
failed = 0


def ok(cond, what):
    global failed
    print(f"{what:<64} {'ok' if cond else 'FAIL'}")
    failed += not cond


# ---------------------------------------------------------- simulated SMK-37 Pro ---

class FakeLink:
    def __init__(self, dev, gen):
        self.dev, self.gen, self.q, self.closed = dev, gen, queue.Queue(), False

    @property
    def lost(self):
        return self.gen != self.dev.gen or not self.dev.connected

    def send(self, pkt):
        if self.lost:
            return False
        self.dev.sent.append(bytes(pkt))
        self.dev.rx(bytes(pkt))
        return True

    def read(self, timeout):
        try:
            return self.q.get(timeout=max(timeout, 0))
        except queue.Empty:
            return None

    def drain(self):
        while not self.q.empty():
            self.q.get_nowait()

    def close(self):
        self.closed = True


class FakeSMK:
    """a MIDI backend with one SMK-37 Pro on it"""

    def __init__(self, image, identity="SMK-37 Pro_016", name="SMK-37 Pro", unplug_after=None, after_write="SMK-37 Pro_900",
                 stall_after=None, bad_addr=None, slot_info=(2, 80, 0, 0, 0, 0, 0, 0)):
        self.image, self.unplug_after, self.after_write = image, unplug_after, after_write
        self.stall_after, self.bad_addr = stall_after, bad_addr
        self.slot_info = slot_info
        self.served = self.bad = self.upgrades = 0
        self.sent, self.links, self.gen, self.lock = [], [], 0, threading.Lock()
        self.boot(identity, name)

    def boot(self, identity, name):
        with self.lock:
            self.gen += 1
            self.identity, self.name, self.connected = identity, name, True
            self.waiting, self.queue = None, []

    def input_names(self):
        return [self.name] if self.connected else []

    output_names = input_names

    def open(self, in_name, out_name):
        if in_name != self.name or not self.connected:
            raise IOError("no such port")
        link = FakeLink(self, self.gen)
        self.links.append(link)
        return link

    def tx(self, pkt):
        for link in self.links:
            if link.gen == self.gen and not link.closed:
                link.q.put(bytes(pkt))

    def rx(self, d):
        # SMP_INFO: default slot_info is a valid empty layout (count/size > 0, all nz=0).
        # Mute with answer_slots=False or slot_info=None: no reply → TimeoutError (fail closed).
        # Do not answer [0,0]: parse_info rejects count/size 0 as malformed, not empty.
        if d == I.HS_QUERY:
            t = self.identity.encode()
            payload = t + bytes(27 - len(t))
            self.tx(b"\xF0" + I.pack7(bytes([0, 0x59, 0x11, 27, 0, 0]) + payload + bytes([~sum(payload) & 255])) + b"\xF7")
        elif d == I.ED_HDR + bytes([I.xs.CMD_INFO, 0xF7]):
            if getattr(self, "answer_slots", True) and self.slot_info is not None:
                self.tx(I.ED_HDR + bytes([I.xs.CMD_INFO]) + bytes(self.slot_info) + b"\xF7")
        elif d == I.UPGRADE:
            self.upgrades += 1
            first = self.bad_addr if self.bad_addr is not None else 0
            self.queue = ([(first + k * 512, 512) for k in range(6)] + [(I.FINISH_WRITE, 8)]
                          if self.identity.startswith("ota-")
                          else [(0, 64), (0x40, 160), (0x1000, 512), (I.FINISH_CHECK, 8)])
            self.next()
        elif self.waiting:
            u = I.unpack7(d[1:-1])
            addr, n = self.waiting
            fin = addr >= I.FINISH_CHECK
            want = b"success\0" if fin else self.image[addr:addr + n]
            if u[14:14 + len(want)] != want or len(u) != 15 + len(want):
                self.bad += 1
            self.waiting = None
            self.served += 1
            if self.unplug_after and self.served >= self.unplug_after:
                self.connected = False
                return
            if self.stall_after and self.served >= self.stall_after:
                return
            if addr == I.FINISH_CHECK:
                threading.Timer(0.05, self.boot, ("ota-SMK-37 Pro_900", "Felucca Update")).start()
            elif addr == I.FINISH_WRITE:
                threading.Timer(0.05, self.boot, (self.after_write, "Felucca")).start()
            else:
                self.next()

    def next(self):
        if not self.queue:
            return
        self.waiting = addr, n = self.queue.pop(0)
        u = bytearray([0, 0x59, 0x30, 0, 0, 0, 0]) + addr.to_bytes(4, "little") + bytes([n & 0xFF, n >> 8, 0])
        u.append(~sum(u[6:14]) & 0xFF)
        self.tx(b"\xF0" + I.pack7(u) + b"\xF7")


# ----------------------------------------------------------------- helpers ---

def package(product="SMK-37 Pro_900", marker=True, size=0x2000):
    raw = bytearray((i * 7) & 0xFF for i in range(size + I.BLOCKS))
    for i in range(I.BLOCKS):
        raw[i * I.BLK + I.KEEP] = (ord(product[i]) + i + 1) & 0xFF if i < len(product) else 0x7D
    raw[0x1800:0x1800 + 16] = I.LOADER_MARK if marker else bytes(16)
    return bytes(raw)


TMP = Path(tempfile.mkdtemp(prefix="felucca-install-"))


def pkgfile(name, raw):
    p = TMP / name
    p.write_bytes(raw)
    return str(p)


def cli(args, dev, answer=True):
    out, err = io.StringIO(), io.StringIO()
    old, sys.stderr = sys.stderr, err
    try:
        rc = I.main(args, backend=dev, out=out, ask=lambda _p: answer)
    finally:
        sys.stderr = old
    return rc, out.getvalue(), err.getvalue()


# ------------------------------------------------------------------- tests ---

def wire():
    data = bytes(range(256)) * 3
    ok(I.unpack7(I.pack7(data))[:len(data)] == data, "pack7 / unpack7 round trip")
    pkt = I.response(0x12345, b"\x01\x02\x03", fl=5)
    u = I.unpack7(pkt[1:-1])
    ok(u[:3] == b"\x00\x59\x30" and int.from_bytes(u[7:11], "little") == 0x12345 and u[14:17] == b"\x01\x02\x03"
       and ~sum(u[6:17]) & 0xFF == u[17], "response: header, address, data, checksum")
    raw = package("SMK-37 Pro_906")
    ok(I.product_of(raw) == "SMK-37 Pro_906" and len(I.logical_image(raw)) == len(raw) - 20, "product_of / logical_image (synthetic)")
    raw = package("SMK-37 Pro_9001000")                  # X0X from 0.10: major, minor, patch, two digits each
    ok(I.product_of(raw) == "SMK-37 Pro_9001000", "product_of: a long SMK identity")


def installs():
    raw = package()
    image = I.logical_image(raw)
    p = pkgfile("ok.fwsc", raw)

    dev = FakeSMK(image)
    rc, out, err = cli([p, "--yes"], dev)
    ok(rc == 0 and dev.bad == 0 and dev.upgrades == 2 and dev.served == 11 and "runs SMK-37 Pro_900" in out
       and "100%" in out, f"install: running -> loader -> Felucca ({dev.served} reads)")

    # A silent or malformed sample-layout response must never permit an OTA write.
    dev = FakeSMK(image, slot_info=None)
    rc, out, err = cli([p, "--yes"], dev)
    ok(rc == I.EXIT["slots"] and dev.upgrades == 0 and "SMP_INFO timed out" in err,
       "install: missing SMP_INFO -> fail closed before write")
    dev = FakeSMK(image, slot_info=(2, 80, 0, 0))
    rc, out, err = cli([p, "--yes"], dev)
    ok(rc == I.EXIT["slots"] and dev.upgrades == 0 and "truncated SMP_INFO" in err,
       "install: malformed SMP_INFO -> fail closed before write")

    raw10 = package("SMK-37 Pro_9001000")
    dev = FakeSMK(I.logical_image(raw10), identity="SMK-37 Pro_909", after_write="SMK-37 Pro_9001000")
    rc, out, err = cli([pkgfile("x10.fwsc", raw10), "--yes"], dev)
    ok(rc == 0 and dev.bad == 0 and "runs SMK-37 Pro_9001000" in out,
       "install: 0.9 (SMK-37 Pro_909) -> 0.10 (SMK-37 Pro_9001000), the longer identity verified after")

    dev = FakeSMK(image, identity="ota-SMK-37 Pro_900", name="Felucca Update")
    rc, out, err = cli([p, "--yes"], dev)
    ok(rc == I.EXIT["slots"] == 7 and dev.upgrades == 0 and "--no-slots" in err,
       "install: update loader without explicit sample-preservation decision -> fail before write")
    dev = FakeSMK(image, identity="ota-SMK-37 Pro_900", name="Felucca Update")
    rc, out, err = cli([p, "--yes", "--no-slots"], dev)
    ok(rc == 0 and dev.bad == 0 and dev.upgrades == 1 and "update mode" in out,
       "install: already in loader -> explicit --no-slots recovery finishes the write")

    dev = FakeSMK(image)
    rc, out, err = cli([p], dev, answer=False)
    ok(rc == 1 and dev.upgrades == 0 and "cancelled" in out, "install: answer no -> nothing sent but the handshake")

    dev = FakeSMK(image, identity="SMK-37 Pro_905", name="Felucca")
    rc, out, err = cli(["--info"], dev)
    ok(rc == 0 and "SMK-37 Pro_905" in out and "running" in out and dev.upgrades == 0, "--info: identity of the connected SMK-37 Pro")

    dev = FakeSMK(image, identity="ota-SMK-37 Pro_900", name="Felucca Update")
    rc, out, err = cli(["--info"], dev)
    ok(rc == 0 and "update loader" in out, "--info: device in update mode")


def errors():
    raw = package()
    image = I.logical_image(raw)
    p = pkgfile("ok.fwsc", raw)

    rc, out, err = cli([p, "--yes"], FakeSMK(image, name="IAC Driver Bus 1"))
    ok(rc == 0, "a unit on a port named 'IAC Driver Bus 1' is found: the handshake model decides, not the port name")
    dev = FakeSMK(image)
    rc, out, err = cli([p, "--yes", "--port", "Felucca"], dev)
    ok(rc == 3 and dev.upgrades == 0, "--port that matches nothing -> exit 3")
    dev = FakeSMK(image, name="My Interface")
    rc, out, err = cli(["--info", "--port", "my int"], dev)
    ok(rc == 0 and "SMK-37 Pro_016" in out, "--port picks a port the default match skips")

    t0 = time.monotonic()
    rc, out, err = cli([p, "--yes"], FakeSMK(image, unplug_after=2))
    ok(rc == 4 and "disconnected" in err and "nothing was written" in err and time.monotonic() - t0 < 3,
       "unplugged in step 1 -> exit 4 'lost', nothing written")
    t0 = time.monotonic()
    rc, out, err = cli([p, "--yes", "--no-slots"], FakeSMK(image, identity="ota-SMK-37 Pro_900", name="Felucca Update", unplug_after=3))
    ok(rc == 4 and "run the install again" in err and time.monotonic() - t0 < 3, "unplugged during the write -> exit 4 at once")
    rc, out, err = cli([p, "--yes", "--no-slots"], FakeSMK(image, identity="ota-SMK-37 Pro_900", name="Felucca Update", stall_after=3))
    ok(rc == 4 and "stopped answering" in err, "loader stops answering -> exit 4 after the idle time")
    rc, out, err = cli([p, "--yes", "--no-slots"], FakeSMK(image, identity="ota-SMK-37 Pro_900", name="Felucca Update", bad_addr=len(image)))
    ok(rc == 4 and "outside the package" in err, "read past the package -> exit 4")

    rc, out, err = cli([p, "--yes"], FakeSMK(image, after_write="SMK-37 Pro_016"))
    ok(rc == 6 and "reports SMK-37 Pro_016" in err, "another identity after the restart -> exit 6")
    dev = FakeSMK(image, identity="XY-9_001", name="usb-midi")
    rc, out, err = cli([p, "--yes"], dev)
    ok(rc == 6 and dev.upgrades == 0, "another model -> exit 6, nothing sent")
    dev = FakeSMK(image, identity="ota-FM-1_900", name="Felucca Update")
    rc, out, err = cli([p, "--yes", "--no-slots"], dev)
    ok(rc == 6 and dev.upgrades == 0, "SMK package on an FM-1 in update mode -> exit 6 (wrong model), nothing sent")
    # USB names never decide: a renamed Pro/Elite is found, and identity comes only from the handshake model
    for name, ident in (("DnBOX 0.0.14 37PRO", "SMK-37 Pro_900"), ("DnBOX 0.0.14 37ELITE", "SMK-37 Elite_900"),
                        ("Any Name", "SMK-37 Pro_900")):
        dev = FakeSMK(image, identity=ident, name=name)
        rc, out, err = cli([p, "--yes"], dev)
        ok(rc == 0 and dev.bad == 0 and dev.upgrades == 2, f"install: {ident} named {name!r} is discovered and updated")
    dev = FakeSMK(image, identity="FM-1_900", name="DnBOX 0.0.14 37PRO")
    rc, out, err = cli([p, "--yes"], dev)
    ok(rc == 6 and dev.upgrades == 0, "a '37PRO' USB name never overrides the reported model (FM-1 -> exit 6)")

    class Stuck(FakeSMK):          # the loader never shows up
        def boot(self, identity, name):
            super().boot(identity, name)
            if identity.startswith("ota-"):
                self.connected = False
    rc, out, err = cli([p, "--yes"], Stuck(image))
    ok(rc == 5 and "loader did not appear" in err, "no loader after step 1 -> exit 5")

    plain = pkgfile("plain.fwsc", package(marker=False))
    dev = FakeSMK(image)
    rc, out, err = cli([plain, "--yes"], dev)
    ok(rc == 2 and "no Felucca update loader" in err and dev.sent == [], "package without the loader marker -> exit 2, no MIDI")
    dev = FakeSMK(I.logical_image(package(marker=False)))
    rc, out, err = cli([plain, "--yes", "--force"], dev)
    ok(rc == 0 and dev.bad == 0, "... installs with --force")
    rc, out, err = cli([pkgfile("short.fwsc", b"\0" * 100), "--yes"], FakeSMK(image))
    ok(rc == 2 and "too short" in err, "not a package -> exit 2")
    rc, out, err = cli([str(TMP / "missing.fwsc"), "--yes"], FakeSMK(image))
    ok(rc == 2, "missing file -> exit 2")


def against_js():
    clean = ROOT / "build/x0x-smk37.fwsc"
    if not shutil.which("node") or not clean.exists():
        print("logical image vs fm1pkg.js: skipped (needs node and build/x0x-smk37.fwsc)")
        return
    js = ("import { logicalImage, productOf } from %r; import { readFileSync } from 'node:fs';"
          "const p = readFileSync(process.argv[1]); process.stderr.write(productOf(p));"
          "process.stdout.write(logicalImage(p));") % str(ROOT / "web/fm1pkg.js")
    r = subprocess.run(["node", "--input-type=module", "-e", js, str(clean)], capture_output=True, check=True)
    raw = clean.read_bytes()
    ok(I.logical_image(raw) == r.stdout and I.product_of(raw) == r.stderr.decode(),
       f"logical_image / product_of == fm1pkg.js ({clean.name}, {I.product_of(raw)})")
    ok(I.LOADER_MARK in raw, f"{clean.name} carries the Felucca loader marker")


if __name__ == "__main__":
    wire()
    installs()
    errors()
    against_js()
    shutil.rmtree(TMP, ignore_errors=True)
    # the SMK-37 Pro, Elite and MKE-P37 are one keyboard: packages install across them, never across FM-1 / SMK
    ok(I.same_keyboard("SMK-37 Elite_016", "SMK-37 Pro_900") and I.same_keyboard("ota-MKE-P37_900", "SMK-37 Elite_900")
       and I.same_keyboard("STARRYKEY-37 PLAY_01", "SMK-37 Pro_900") and I.same_keyboard("FM-1_015", "FM-1_900")
       and not I.same_keyboard("FM-1_015", "SMK-37 Pro_900") and not I.same_keyboard("SMK-37 Elite_016", "FM-1_900")
       and not I.same_keyboard("XY-9_001", "SMK-37 Pro_900"),
       "models: SMK-37 Pro / Elite / MKE-P37 / Starrykey are one keyboard, FM-1 is another")
    ok(not hasattr(I, "PORT_RE"), "port names never gate discovery (no PORT_RE)")
    for f, gate in (("web/fm1ota.js", "test(input.name"), ("web/x0x_breaks.html", "test(input.name")):
        ok(gate not in (ROOT / f).read_text(), f"{f}: no USB-name discovery gate")
    print(f"INSTALL TESTS FAILED ({failed})" if failed else "install tests passed")
    sys.exit(1 if failed else 0)
