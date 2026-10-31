#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
"""Install a Felucca package (.fwsc) on an FM-1 (or an M-Vave SMK-37 Pro) over USB-MIDI.

The same update as the web installer (web/fm1ota.js): step 1, the
running firmware reads parts of the package and starts the update loader;
step 2, the loader reads the whole package and writes it. In both steps the
device asks (SysEx read requests on the logical image) and we answer. Then
the FM-1 restarts and the installed identity is checked.

  fm1_install.py PACKAGE.fwsc [--port NAME] [--yes] [--force] [--backup FILE] [--no-slots] [--ctl FILE | --no-ctl]
  fm1_install.py --info [--port NAME]

If the FM-1 is still in update mode (an earlier install was cut off), sample slots
cannot be inspected or backed up there. Resume requires explicit --no-slots, only
after verifying a prior slot backup or confirming no samples require preservation.
Restore any saved slots manually afterward. Needs mido with python-rtmidi.

Sample slots: before the install, the running X0X lists its slots (SMP_INFO). If a slot holds samples
that the package's firmware will not keep (no slot at the same flash offset with room for them: the
package's slot map, an older X0X's 2 x 80 KiB map, or none for a package without the Felucca loader),
the install does this by itself: (1) it reads every slot (SMP_INFO / SMP_READ only) into an .x0xslots
archive and saves it (--backup FILE, default x0x-slots-before-install-DATE.x0xslots here); (2) it
installs; (3) when the new firmware is back, it writes those slots into the new map (old slot N to new
slot N if it fits, else to a free slot that is large enough; others are reported) and reads them back
to compare; (4) if that fails, the archive stays and the message says how to restore it with
tools/x0x_slots.py. --no-slots skips the check. The other slots stay on the flash: no install writes them.

Exit codes: 0 done, 1 cancelled or other error, 2 bad arguments or package,
3 FM-1 not found, 4 connection lost or the device stopped, 5 timeout (no
loader / no restart), 6 wrong model, or another identity after the install,
7 sample protection refused before writing, or installed but slots were not restored,
8 installed, but the Bluetooth controller block read-back failed or its CRC32 is not the expected full-block CRC32,
9 refused by the gate guard (no install was started),
10 refused by the foreground guard or the hardware lock (no device was opened),
11 refused by the bootloader guard: the package head is not the reference head (no device was opened).

Start guards for a real device (rules v4: checks 3 and 14), after the gate guard:
  Foreground (rule 7): an install refuses to start when stdin is not a TTY and STY and TMUX are empty. Run it in the
  foreground or in a named screen session (screen -S x0x-install), never with nohup or &.
  Hardware lock (rule 3, one hardware job at a time): an install and --info take an exclusive lock on the file
  X0X_HW_LOCK [~/.x0x37-hardware.lock] (flock, no wait) for the whole run. If another run holds it, the tool stops.

Bootloader guard (rule 7, check 4 of rules v4, Orchestrator decision 02:55): the bytes [0, HEAD_END = 0x4000) of the
package's flash.bin (flash header, JLFS entries, SPL, isd_config: the bootloader; the app area starts at 0x4000, as
WRITE_MIN in tools/fm1_rescue.py) must have the sha256 of the same bytes of a reference: a flash read-back of this unit
(a raw dump from flash 0, for example the fm1-backup-*.bin of tools/fm1_rescue.py). The reference is --head-ref FILE,
else X0X_HEAD_REF, else ~/.config/x0x/flash-head.bin if that file is there. A different sha256 -> "REFUSED", exit 11.
With no reference, the tool says so and continues: this tool has no flash read-back over USB-MIDI, and the slot backup
holds only the sample slots. --head-check compares only (no device, no gate guard): HEAD OK (exit 0) or HEAD MISMATCH (11).
Order for a real device: gate guard, foreground guard, hardware lock, bootloader guard, then the device.

Gate guard (x0x37 rule 29): before an install on a real device, the tool runs
tools/require_gate_pass.sh <--commit HASH> --fwsc PACKAGE --config dev|rel [--ctl FILE]. The install starts
only if the gate script gave PASS for the exact tree of HASH and the files are the files that the gate built
for that config. Alternatively --dev-record accepts scoped DEV CHECKED evidence for dev only.
An SMK-37 package with an empty controller block requires explicit --ctl FILE; --no-ctl is accepted only
when the package already embeds a valid controller or is an old package whose application owns that range.
--config dev (the default) is the dev build (X0X_DEV_PADCAL=1), rel is the release build (0).
--commit (or X0X_INSTALL_COMMIT) is necessary. --info and the mocked device of the tests do not use it.

Bluetooth MIDI (SMK-37): --ctl FILE adds a controller image that ble/ctl/build_ctl.sh built on this
computer to the package before it is sent (tools/add_ctl.py). Every current SMK install writes the
Bluetooth block, so an empty block is refused instead of erasing the installed controller. The installer
does not use the cached ~/.config/x0x/ctl.img implicitly: select the exact validated file with --ctl.
After the install, the tool reads the complete 64 KiB block back from the unit (editor command 37
CTL_READ, read only) and compares it with the selected image plus erased padding. A mismatch stops
with exit 8.

Packages: 20 interleave blocks (FM-1, X0X, an SMK-37 Pro build of X0X) or more (the stock SMK-37 Pro
v016 has 36: the extra markers are 0x7D); the count is detected and checked with the files' CRCs.
The model is checked: an FM-1 package never goes to an SMK-37 Pro, nor the other way round (the SMK-37
Pro, Elite and MKE-P37 are one keyboard: their packages install on each other).
"""
import argparse
import hashlib
import os
import queue
import re
import sys
import time
import zlib
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import fm1pkg_make  # noqa: E402  (the package's slot map)
import add_ctl  # noqa: E402  (the SMK-37 Bluetooth controller image)
import x0x_slots as xs  # noqa: E402
HS_QUERY = bytes([0xF0, 0x00, 0x32, 0x45, 0x00, 0x00, 0x00, 0x40, 0x7F, 0xF7])
UPGRADE = bytes([0xF0, 0x22, 0x24, 0x35, 0x7F, 0xF7])
FINISH_CHECK, FINISH_WRITE = 0xE0000000, 0xF0000000
MAXDATA = 512
BLOCKS, BLK, KEEP = 20, 0x30, 0x2F         # BLOCKS: the identity blocks (more may follow, see block_count)
MAX_BLOCKS = 64
LOADER_MARK = b"FELUCCA-LOADER-1"
HEAD_END = 0x4000                          # the bootloader end: flash [0, 0x4000) = header, JLFS, SPL, isd_config
# Port names never gate discovery: every port is a candidate, and only the handshake identity
# (model such as "SMK-37 Pro_900" / "SMK-37 Elite_900") decides. USB names change (e.g. "DnBOX <ver> 37PRO").

# seconds; the tests shorten them
DELAY = {"open": 0.3, "start": 2.0, "reply": 0.01, "loader": 3.0, "reboot": 3.0, "retry": 1.0,
         "poll": 1.0, "hs": 1.0, "idle_check": 8.0, "idle_write": 180.0,
         "wait_loader": 30.0, "wait_reboot": 40.0}

EXIT = {"usage": 2, "badpkg": 2, "notfound": 3, "model": 6, "lost": 4, "stopped": 4, "badreq": 4,
        "noloader": 5, "noreturn": 5, "mismatch": 6, "slots": 7, "ctlcrc": 8, "head": 11}


class InstallError(Exception):
    def __init__(self, code, msg):
        super().__init__(msg)
        self.code = code


# ------------------------------------------------------------ wire format ---

def pack7(data):
    out, acc, nb = [], 0, 0
    for b in data:
        acc |= b << nb
        nb += 8
        while nb >= 7:
            out.append(acc & 0x7F)
            acc >>= 7
            nb -= 7
    if nb:
        out.append(acc & 0x7F)
    return bytes(out)


def unpack7(s):
    out, acc, nb = [], 0, 0
    for b in s:
        acc |= (b & 0x7F) << nb
        nb += 7
        while nb >= 8:
            out.append(acc & 0xFF)
            acc >>= 8
            nb -= 8
    return bytes(out)


def response(addr, data, fl=0):
    n = len(data)
    body = bytearray([0x00, 0x59, 0x30, (n + 8) & 0xFF, ((n + 8) >> 8) & 0xFF, 0, fl])
    body += addr.to_bytes(4, "little") + bytes([n & 0xFF, (n >> 8) & 0xFF, 0]) + bytes(data)
    body.append(~sum(body[6:]) & 0xFF)
    return b"\xF0" + pack7(body) + b"\xF7"


def parse_request(pkt):
    """-> (flags, addr, len) or None"""
    if len(pkt) < 2 or pkt[0] != 0xF0 or pkt[-1] != 0xF7:
        return None
    u = unpack7(pkt[1:-1])
    if len(u) != 15 or u[:3] != b"\x00\x59\x30" or ~sum(u[6:14]) & 0xFF != u[14]:
        return None
    return u[6], int.from_bytes(u[7:11], "little"), int.from_bytes(u[11:14], "little")


class Identity:
    def __init__(self, text, model, version):
        self.text, self.model, self.version = text, model, version

    @property
    def loader(self):
        return self.model.lower().startswith("ota-")


def parse_identity(pkt):
    if len(pkt) < 2 or pkt[0] != 0xF0 or pkt[-1] != 0xF7:
        return None
    wire = bytes(pkt[1:-1])
    if any(b & 0x80 for b in wire):
        return None
    d = unpack7(wire)
    # ota_send_msg(0x11): 27 payload bytes, declared length, complement checksum.
    if (len(d) != 34 or d[:6] != b"\x00\x59\x11\x1b\x00\x00"
            or (~sum(d[6:33]) & 0xFF) != d[33] or pack7(d) != wire):
        return None
    txt = d[6:33].rstrip(b"\0").decode("latin-1")
    m = re.fullmatch(r"([^_]+)_(\d+)", txt)
    return Identity(txt, m[1], int(m[2])) if m else None


# --------------------------------------------------------------- package ---

def product_of(raw):
    """the package identity ("FM-1_906", "SMK-37 Pro_900"): one marker byte after each of the first 20 blocks"""
    if len(raw) < BLOCKS * BLK:
        raise InstallError("badpkg", "not an FM-1 package (too short)")
    return "".join(chr((m - i - 1) & 0xFF) for i in range(BLOCKS) if (m := raw[i * BLK + KEEP]) != 0x7D)


def block_count(raw):
    """interleave blocks: 20, then more while the marker is the unused 0x7D (SMK-37 Pro v016: 36).
    A 20-block package can have 0x7D there by chance: deinterleave checks the CRCs and falls back."""
    n = BLOCKS
    while n < MAX_BLOCKS and (n + 1) * BLK <= len(raw) and raw[n * BLK + KEEP] == 0x7D:
        n += 1
    return n


def logical_image(raw, blocks=BLOCKS):
    """the image the device reads during an update: the package without the marker bytes"""
    out = bytearray()
    for i in range(blocks):
        out += raw[i * BLK:i * BLK + KEEP]
    return bytes(out + raw[blocks * BLK:])


def deinterleave(raw):
    """-> (logical image, block count): the detected count when the files' CRCs agree, else 20"""
    n = block_count(raw)
    image = logical_image(raw, n)
    if n != BLOCKS and package_damage(image):
        alt = logical_image(raw, BLOCKS)
        if not package_damage(alt):
            return alt, BLOCKS
    return image, n


def _crc16(data, crc=0):
    """CRC-16/XMODEM, as the package and the device use"""
    for b in data:
        crc ^= b << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) & 0xFFFF if crc & 0x8000 else (crc << 1) & 0xFFFF
    return crc


def _dec(b):
    """JieLi ENC stream cipher (its own inverse)"""
    out, key = bytearray(b), 0xFFFF
    for i in range(len(out)):
        out[i] ^= key & 0xFF
        key = ((key << 1) ^ (0x1021 if key & 0x8000 else 0)) & 0xFFFF
    return bytes(out)


def package_damage(image):
    """what is wrong with the update image (header, file list, each file's CRC), or None"""
    if len(image) < 0x40:
        return "too short"
    hdr = _dec(image[0:0x40])
    if _crc16(hdr[2:0x40]) != int.from_bytes(hdr[0:2], "little"):
        return "header CRC"
    count = int.from_bytes(hdr[8:10], "little")
    if not 1 <= count <= 8 or len(image) < 0x40 + 0x50 * count:
        return "file list"
    lst = image[0x40:0x40 + 0x50 * count]
    if _crc16(lst) != int.from_bytes(hdr[2:4], "little"):
        return "file list CRC"
    for i in range(count):
        e = _dec(lst[i * 0x50:(i + 1) * 0x50])
        dcrc = int.from_bytes(e[4:6], "little")
        off = int.from_bytes(e[8:12], "little")
        size = int.from_bytes(e[12:16], "little")
        name = e[0x40:0x50].split(b"\0")[0].decode("ascii", "replace")
        if off + size > len(image):
            return f"{name}: cut short"
        typ = int.from_bytes(e[0:2], "little")
        # flash.bin (0) and ota.bin (100), what the device installs, carry the CRC of their bytes as
        # served. The stock packer's other entries store a CRC of something else: bounds only.
        if typ in (0, 100) and _crc16(image[off:off + size]) != dcrc:
            return f"{name}: CRC"
    return None


def model_of(text):
    return text.split("_")[0].removeprefix("ota-")


# the SMK-37 Pro, SMK-37 Elite, MKE-P37, Starrykey 37 Play and the SMK-37+VA mod: one keyboard under several names
SMK_FAMILY = re.compile(r"SMK-37[ +].*|MKE-P37|STARRYKEY-37.*", re.I)


def same_keyboard(a, b):
    """the models of identities a and b are the same hardware (never an FM-1 and an SMK)"""
    ma, mb = model_of(a), model_of(b)
    return ma == mb or bool(SMK_FAMILY.fullmatch(ma) and SMK_FAMILY.fullmatch(mb))


# ------------------------------------------------------------------ MIDI ---

class MidoLink:
    """one MIDI in/out pair with a SysEx queue"""

    def __init__(self, backend, in_name, out_name):
        import mido
        self.backend, self.name, self.mido = backend, in_name, mido
        self.q = queue.Queue()
        self.inp = mido.open_input(in_name, callback=self._rx)
        try:
            self.out = mido.open_output(out_name)
        except Exception:
            self.inp.close()
            raise
        time.sleep(DELAY["open"])   # CoreMIDI may drop a message sent just after opening

    def _rx(self, msg):
        if msg.type == "sysex":
            self.q.put(bytes([0xF0, *msg.data, 0xF7]))

    @property
    def lost(self):
        return self.name not in self.backend.input_names()

    def send(self, pkt):
        try:
            self.out.send(self.mido.Message("sysex", data=pkt[1:-1]))
            return True
        except Exception:
            return False

    def read(self, timeout):
        try:
            return self.q.get(timeout=max(timeout, 0))
        except queue.Empty:
            return None

    def drain(self):
        while not self.q.empty():
            self.q.get_nowait()

    def close(self):
        for p in (self.inp, self.out):
            try:
                p.close()
            except Exception:
                pass


class MidoBackend:
    def __init__(self):
        import mido
        self.mido = mido

    def input_names(self):
        return list(dict.fromkeys(self.mido.get_input_names()))

    def output_names(self):
        return list(dict.fromkeys(self.mido.get_output_names()))

    def open(self, in_name, out_name):
        return MidoLink(self, in_name, out_name)


def pair_output(name, outs):
    if name in outs:
        return name
    base = re.sub(r"\s+\d+$", "", name)     # Windows numbers each port
    same = [o for o in outs if re.sub(r"\s+\d+$", "", o) == base]
    if len(same) == 1:
        return same[0]
    return outs[0] if len(outs) == 1 else None


# --------------------------------------------------------------- updater ---

class Device:
    def __init__(self, link, ident, name):
        self.link, self.id, self.name = link, ident, name


def handshake(link, tries=3):
    link.drain()
    for _ in range(tries):
        if not link.send(HS_QUERY):
            return None
        end = time.monotonic() + DELAY["hs"]
        while (left := end - time.monotonic()) > 0:
            p = link.read(left)
            if p is None:
                break
            ident = parse_identity(p)
            if ident:
                return ident
    return None


class Updater:
    def __init__(self, backend, port=None):
        self.backend = backend
        self.port = port
        # Port accepted by the last fail-closed transition wait (loader or restart).
        self.returned_port = None
        # Running ports observed while the selected unit is still in the loader (before reboot).
        self._before_running = None

    def candidates(self, only_port):
        names = self.backend.input_names()
        if self.port:
            mine = [n for n in names if self.port.lower() in n.lower()]
            if only_port:
                return mine
            return mine + [n for n in names if n not in mine]
        return list(names)

    def find(self, want=None, only_port=True):
        """the first matching port that answers the handshake (and want(identity))"""
        outs = self.backend.output_names()
        for name in self.candidates(only_port):
            out = pair_output(name, outs)
            if not out:
                continue
            try:
                link = self.backend.open(name, out)
            except Exception:
                continue
            ident = handshake(link, 2)
            if ident and (want is None or want(ident)):
                return Device(link, ident, name)
            link.close()
        return None

    def matching_ports(self, want, only_port=False, skip=None):
        """Port names that answer the handshake and match want (every link closed).

        skip: a port name already held open by the caller; do not reopen it (would
        steal the MIDI callback / collide with the in-flight update link).
        """
        names = []
        outs = self.backend.output_names()
        for name in self.candidates(only_port):
            if skip is not None and name == skip:
                continue
            out = pair_output(name, outs)
            if not out:
                continue
            try:
                link = self.backend.open(name, out)
            except Exception:
                continue
            try:
                ident = handshake(link, 2)
                if ident and (want is None or want(ident)):
                    names.append(name)
            finally:
                link.close()
        return names

    def open_port(self, name, want=None):
        """Open one named port when it still answers and matches want."""
        outs = self.backend.output_names()
        out = pair_output(name, outs)
        if not out:
            return None
        try:
            link = self.backend.open(name, out)
        except Exception:
            return None
        ident = handshake(link, 2)
        if ident and (want is None or want(ident)):
            return Device(link, ident, name)
        link.close()
        return None

    def wait_for(self, want, secs, before):
        """Wait for exactly one newly appeared port matching want.

        before: port names that already matched want before the transition.
        Returns that device, or None when none appear before the timeout.
        Raises InstallError when more than one new matching port appears.
        Never returns a port that was already in before (fail closed).
        """
        end = time.monotonic() + secs
        prior = set(before)
        while time.monotonic() < end:
            names = self.matching_ports(want, only_port=False)
            new = [n for n in names if n not in prior]
            if len(new) > 1:
                raise InstallError(
                    "model",
                    "more than one new device appeared after the update transition; "
                    "disconnect other keyboards and try again. Nothing was written",
                )
            if len(new) == 1:
                dev = self.open_port(new[0], want)
                if dev:
                    self.returned_port = new[0]
                    return dev
            time.sleep(DELAY["retry"])
        return None

    def reconnect_after(self, want, secs):
        """Reconnect after a verified transition: same newly-appeared rule vs other ports."""
        known = self.returned_port
        before = [n for n in self.matching_ports(want, only_port=False) if n != known]
        return self.wait_for(want, secs, before)

    def serve(self, link, image, finish, idle, progress):
        """answer read requests until the device asks for `finish`; -> (served, finished, lost)"""
        served, last = 0, time.monotonic()
        while True:
            pkt = link.read(DELAY["poll"])
            if pkt is None:
                if link.lost:
                    return served, False, True
                if time.monotonic() - last > idle:
                    return served, False, False
                continue
            r = parse_request(pkt)
            if not r:
                continue
            fl, addr, n = r
            last = time.monotonic()
            if addr in (FINISH_CHECK, FINISH_WRITE):
                if not link.send(response(addr, b"success\0")):
                    return served, False, True
                if addr == finish:
                    return served, True, False
                continue
            if n > MAXDATA or addr + n > len(image):
                raise InstallError("badreq", f"the device asked for {addr:#x}+{n}, outside the package")
            time.sleep(DELAY["reply"])
            if not link.send(response(addr, image[addr:addr + n], fl)):
                return served, False, True
            served += 1
            progress(served, addr + n)

    def check(self, dev, image, step):
        """step 1: the running firmware checks the package and starts the loader"""
        before_loaders = self.matching_ports(lambda i: i.loader, only_port=False, skip=dev.name)
        step("start", dev.id.text)
        dev.link.send(UPGRADE)
        time.sleep(DELAY["start"])
        served, finished, lost = self.serve(dev.link, image, FINISH_CHECK, DELAY["idle_check"],
                                            lambda k, end: step("check", k))
        dev.link.close()
        if not finished:
            raise InstallError("lost" if lost else "stopped",
                               f"the FM-1 {'was disconnected' if lost else 'stopped answering'} after "
                               f"{served} requests; nothing was written")
        step("loader")
        time.sleep(DELAY["loader"])
        ota = self.wait_for(lambda i: i.loader, DELAY["wait_loader"], before_loaders)
        if not ota:
            raise InstallError("noloader", "the update loader did not appear. Replug the USB cable and "
                                           "run the install again: the FM-1 stays in update mode until it is done")
        return ota

    def write(self, ota, image, step):
        """step 2: the loader reads and writes the whole package, then restarts"""
        # Snapshot other running keyboards while this unit is still in the loader, so a fast
        # reboot cannot put the selected port into the "before" set used by verify().
        self._before_running = self.matching_ports(lambda i: not i.loader, only_port=False, skip=ota.name)
        step("write", 0)
        ota.link.send(UPGRADE)
        time.sleep(DELAY["start"])
        served, finished, lost = self.serve(ota.link, image, FINISH_WRITE, DELAY["idle_write"],
                                            lambda k, end: step("write", min(99, end * 100 // len(image))))
        ota.link.close()
        if not finished:
            raise InstallError("lost" if lost else "stopped",
                               f"the loader {'was disconnected' if lost else 'stopped answering'} after "
                               f"{served} requests. Replug the USB cable and run the install again to finish; "
                               "if already in update mode, use --no-slots only with a verified backup or no X0X samples")
        step("write", 100)

    def verify(self, product, step):
        before_running = self._before_running if self._before_running is not None else []
        step("reboot")
        time.sleep(DELAY["reboot"])
        back = self.wait_for(lambda i: not i.loader, DELAY["wait_reboot"], before_running)
        if not back:
            raise InstallError("noreturn", "the FM-1 did not come back: power-cycle it")
        back.link.close()
        if back.id.text != product:
            raise InstallError("mismatch", f"written, but the FM-1 reports {back.id.text} (expected {product})")
        step("done", back.id.text)
        return back.id.text

    def install(self, dev, image, product, step=lambda *a: None):
        """running firmware -> loader -> new firmware; dev from find(). A device already in
        update mode only needs the write."""
        if not same_keyboard(dev.id.text, product):
            dev.link.close()
            raise InstallError("model", f"the device is {dev.id.text}, the package is for {product}")
        ota = dev if dev.id.loader else self.check(dev, image, step)
        self.write(ota, image, step)
        return self.verify(product, step)


# ---------------------------------------------------------- sample slots ---

ED_HDR = bytes([0xF0, 0x7D, 0x46, 0x4C])           # X0X's editor SysEx (web/EDITOR_PROTOCOL.md)
SLOT_PROBE = {"timeout": 1.0, "tries": 2}           # SMP_INFO of a firmware that may not have it (stock)


class EditorLink:
    """X0X's slot commands over an install link (x0x_slots.py's link.req)"""

    def __init__(self, link):
        self.link = link

    def req(self, cmd, args, timeout=3.0):
        self.link.drain()
        if not self.link.send(ED_HDR + bytes([cmd]) + bytes(args) + b"\xF7"):
            raise TimeoutError(f"no reply to cmd {cmd} (port lost)")
        end = time.monotonic() + timeout
        while (left := end - time.monotonic()) > 0:
            p = self.link.read(left)
            if p is None:
                break
            if p[:5] == ED_HDR + bytes([cmd]) and p[-1] == 0xF7:
                return list(p[5:-1])
        raise TimeoutError(f"no reply to cmd {cmd}")


def board_of(product):
    return "smk" if SMK_FAMILY.fullmatch(model_of(product)) else "fm1"


def package_slot_map(image, product):
    """[(flash offset, size)] of the package's firmware: its slot map; for an older X0X package (Felucca
    loader, no map) the equal 80 KiB slots of that time; [] for any other package (stock firmware)"""
    m = fm1pkg_make.slotmap_of(image)
    if m is not None:
        return m
    if LOADER_MARK not in image:
        return []
    board = board_of(product)
    return [(xs.LEGACY_BASE[board] + k * 0x14000, 0x14000) for k in range(2 if board == "smk" else 3)]


def slot_probe(link):
    """Verified SMP_INFO or an error; silence is NOT proof of factory-stock firmware."""
    try:
        return xs.parse_info(xs.req(EditorLink(link), xs.CMD_INFO, [], SLOT_PROBE["timeout"], SLOT_PROBE["tries"]))
    except TimeoutError as e:
        raise xs.SlotError("SMP_INFO timed out: sample layout UNKNOWN. Nothing was written. "
                           "Use --no-slots only after independently confirming no X0X samples need preservation") from e
    except (ValueError, IndexError) as e:
        raise xs.SlotError(f"malformed SMP_INFO reply: {e}") from e


class SlotMove:
    """the slots an install would lose: backup before, restore after"""

    def __init__(self, lay, old_map, new_map, risk, board):
        self.lay, self.old_map, self.new_map, self.risk, self.board = lay, old_map, new_map, risk, board
        self.images = self.path = None

    def describe(self):
        return ", ".join(f"slot {k + 1} ({self.lay['slots'][k]['name'] or '-'}, {self.lay['slots'][k]['used']} KiB)"
                         for k in self.risk)

    def backup(self, link, path, out):
        lay, self.images = xs.backup(EditorLink(link))
        self.lay = lay
        # SMP_INFO may have changed since the preflight probe. Plan restoration
        # from the same verified snapshot whose slot bytes are archived.
        self.old_map = xs.flash_map(lay, self.board)
        self.risk = xs.at_risk(lay, self.old_map, self.new_map)
        arch = xs.make_archive(lay, self.images)
        self.path = Path(path)
        xs.save_backup_archive(self.path, arch)
        if xs.read_archive(self.path.read_bytes())[1] != self.images:
            raise xs.SlotError(f"{self.path}: the saved backup does not read back")
        print(f"slots saved: {self.path} ({len(arch)} B)", file=out)

    def restore(self, link, out):
        """-> (written [(old, new)], skipped texts); raises SlotError / TimeoutError"""
        el = EditorLink(link)
        new_lay = xs.layout(el)
        new_map = xs.flash_map(new_lay, self.board)
        todo, skipped = xs.plan_migration(self.images, self.risk, self.old_map, new_lay, new_map)
        todo, skipped = xs.guard_migration_loop_menu(self.lay, self.images, self.risk, self.old_map,
                                                     new_lay, new_map, todo, skipped)
        xs.restore_mapped(el, todo, self.images)
        for k, j in todo:
            print(f"slot {k + 1} written back to slot {j + 1} and read back: ok", file=out)
        for t in skipped:
            print(f"not written back: {t}", file=out)
        return todo, skipped

    def help(self):
        return (f"The slots are in {self.path}. Restore them with the break loops page (Restore a backup) "
                f"or: python3 tools/x0x_slots.py restore {self.path}")


def plan_slots(dev, image, product, require_backup=False):
    """-> SlotMove for at-risk data, or for an explicitly requested full prewrite backup.

    The layout probe must succeed. A failed probe raises (fail closed before write).
    After a successful read, empty slots or no at-risk slots return None (no backup).
    Callers skip this path with --no-slots.
    """
    lay = slot_probe(dev.link)
    if not lay:
        raise xs.SlotError("the running firmware did not provide a verified sample layout")
    if not require_backup and not any(sl["nz"] for sl in lay["slots"]):
        return None
    old_map = xs.flash_map(lay, board_of(dev.id.text))
    new_map = package_slot_map(image, product)
    risk = xs.at_risk(lay, old_map, new_map)
    return SlotMove(lay, old_map, new_map, risk, board_of(dev.id.text)) if risk or require_backup else None


# ------------------------------------------------------------------- CLI ---

def load_package(path, force):
    try:
        raw = open(path, "rb").read()
    except OSError as e:
        raise InstallError("badpkg", f"cannot read {path}: {e.strerror}")
    product = product_of(raw)
    if not re.fullmatch(r"[^_]+_\d+", product):
        raise InstallError("badpkg", f"{path}: not an FM-1 package (identity {product!r})")
    if LOADER_MARK not in raw and not force:
        raise InstallError("badpkg", f"{path}: no Felucca update loader in this package; "
                                     "only Felucca's own packages are installed (--force overrides)")
    return product, deinterleave(raw)[0]


def package_head(image):
    """the bytes [0, HEAD_END) of the flash.bin entry (type 0) of a logical image"""
    hdr = _dec(image[0:0x40])
    count = int.from_bytes(hdr[8:10], "little")
    for i in range(count):
        e = _dec(image[0x40 + i * 0x50:0x40 + (i + 1) * 0x50])
        if int.from_bytes(e[0:2], "little") == 0:
            off, size = int.from_bytes(e[8:12], "little"), int.from_bytes(e[12:16], "little")
            if size < HEAD_END or off + HEAD_END > len(image):
                raise InstallError("badpkg", f"flash.bin is shorter than the bootloader (0x{HEAD_END:X} bytes)")
            return bytes(image[off:off + HEAD_END])
    raise InstallError("badpkg", "no flash.bin in the package: the bootloader guard cannot check it")


def head_ref_path(a=None, env=None):
    """--head-ref, else X0X_HEAD_REF, else ~/.config/x0x/flash-head.bin if it is there, else None"""
    env = os.environ if env is None else env
    p = (getattr(a, "head_ref", None) if a is not None else None) or env.get("X0X_HEAD_REF") or ""
    if p:
        return p
    d = Path.home() / ".config/x0x/flash-head.bin"
    return str(d) if d.is_file() else None


def head_compare(image, ref_path):
    """-> (ok, text): the package head against the first HEAD_END bytes of the reference read-back"""
    try:
        ref = open(ref_path, "rb").read(HEAD_END)
    except OSError as e:
        raise InstallError("head", f"cannot read the reference head {ref_path}: {e.strerror}")
    if len(ref) < HEAD_END:
        raise InstallError("head", f"the reference {ref_path} has {len(ref)} bytes, less than 0x{HEAD_END:X}")
    pk = package_head(image)
    hp, hr = hashlib.sha256(pk).hexdigest(), hashlib.sha256(ref).hexdigest()
    if hp == hr:
        return True, f"sha256 {hp[:16]} = {ref_path}"
    at = next(i for i in range(HEAD_END) if pk[i] != ref[i])
    n = sum(1 for i in range(HEAD_END) if pk[i] != ref[i])
    return False, (f"package sha256 {hp[:16]}, reference {hr[:16]} ({ref_path}): {n} bytes differ, "
                   f"the first at 0x{at:04X}")


def head_guard(a, out=None, env=None):
    """check 4: 0 = same head, or no reference; 11 = refused. A package that does not load is left to run()."""
    out = out or sys.stderr
    ref = head_ref_path(a, env)
    if ref is None:
        print("bootloader guard: no reference head (--head-ref, X0X_HEAD_REF or ~/.config/x0x/flash-head.bin); "
              "not checked", file=out)
        return 0
    try:
        _, image = load_package(a.package, a.force)
    except InstallError:
        return 0                              # run() loads it again and reports the error
    try:
        same, text = head_compare(image, ref)
    except InstallError as e:
        print(f"bootloader guard: REFUSED: {e}", file=out)
        return EXIT["head"]
    if not same:
        print(f"bootloader guard: REFUSED: the package bytes 0..0x{HEAD_END:X} are not the reference head: {text}. "
              "Nothing was written.", file=out)
        return EXIT["head"]
    print(f"bootloader guard: head OK, {text}", file=out)
    return 0


def not_found(up):
    names = up.backend.input_names()
    where = f"no MIDI port matching {up.port!r} answered" if up.port else "FM-1 not found"
    return InstallError("notfound", f"{where} (USB data cable? another app using it?)"
                        f"\n  MIDI inputs: {', '.join(names) if names else '(none)'}")


class Progress:
    def __init__(self, out):
        self.out, self.line = out, False

    def __call__(self, k, a=None):
        if k == "check":
            self.put(f"checking the package ({a} reads)")
        elif k == "write":
            self.put(f"writing {a:3d}%")
        else:
            self.end()
            msg = {"start": f"starting the update on {a}", "loader": "switching to update mode",
                   "reboot": "restarting", "done": f"done: the FM-1 runs {a}"}[k]
            print(msg, file=self.out, flush=True)

    def put(self, s):
        print(f"\r{s}\033[K", end="", file=self.out, flush=True)
        self.line = True

    def end(self):
        if self.line:
            print(file=self.out, flush=True)
            self.line = False


def with_ctl(a, product, image, out):
    """Return an installable image; current SMK packages require an explicit or embedded controller."""
    path = getattr(a, "ctl", None)
    smk = board_of(product) == "smk"
    if not smk:
        if path is not None:
            raise InstallError("usage", "--ctl: the Bluetooth controller image is for the SMK-37 family only")
        return image
    if path is None:
        try:
            ctl = add_ctl.controller_of(image)
        except add_ctl.NoBlock as e:
            print(f"Bluetooth controller image: not applicable: {e}", file=out)
            return image
        except add_ctl.PkgError as e:
            raise InstallError("badpkg", f"Bluetooth controller image: {e}")
        if ctl is None:
            raise InstallError("badpkg", "SMK-37 package has an empty Bluetooth controller block. Give --ctl FILE "
                                        "for the exact validated controller; --no-ctl does not preserve the unit block")
        a.ctl_bytes = ctl
        print(f"Bluetooth controller image: {len(ctl)} B already embedded in the package", file=out)
        return image
    try:
        ctl = Path(path).read_bytes()
    except OSError as e:
        raise InstallError("badpkg", f"cannot read {path}: {e.strerror}")
    try:
        if (p := add_ctl.ctl_problem(ctl)):
            raise add_ctl.PkgError(f"{path}: {p}")
        image = add_ctl.insert(image, ctl)
        a.ctl_bytes = ctl                   # read back from the unit after the install (check_ctl)
    except add_ctl.PkgError as e:
        raise InstallError("badpkg", f"Bluetooth controller image: {e}")
    print(f"Bluetooth controller image: {path} (added to the package)", file=out)
    return image


ED_CTL_READ = 37                    # editor.c: read only, the Bluetooth block (0x8B000, 64 KiB) only
CTL_READ_N = 224                    # bytes a reply (editor.c ED_READ_MAX)
CTL_READ = {"timeout": 3.0, "tries": 3}   # a lost reply is asked for again (the read changes nothing)


def read_ctl(ed, size):
    """the first size bytes of the unit's Bluetooth block (ED_CTL_READ)"""
    got = bytearray()
    while len(got) < size:
        off, n = len(got), min(CTL_READ_N, size - len(got))
        r = xs.req(ed, ED_CTL_READ, [off & 0x7F, (off >> 7) & 0x7F, (off >> 14) & 0x7F, (n + 1) // 2],
                   CTL_READ["timeout"], CTL_READ["tries"])
        if (len(r) < 3 or any(b & 0x80 for b in r[:3])
                or r[0] | r[1] << 7 | r[2] << 14 != off):
            raise TimeoutError(f"bad ED_CTL_READ reply at {off:#x}")
        wire = r[3:]
        d = xs.unpack7(wire)
        # The reply is a canonical pack7 frame for exactly the requested byte
        # count; a final odd-byte request may be rounded up by one byte.
        # Truncating surplus or accepting orphan masks could falsely certify
        # controller readback after OTA, even if the supplied bytes match.
        if (len(d) < n or len(d) > n + (n & 1)
                or xs.pack7(d) != list(wire)):
            raise TimeoutError(f"malformed ED_CTL_READ payload at {off:#x} ({len(d)} of {n} B)")
        got += d[:n]
    return bytes(got)


def check_ctl(link, ctl, out):
    """Read and compare the complete 64 KiB controller block, including erased padding."""
    expected = ctl + b"\xFF" * (add_ctl.BLE_SIZE - len(ctl))
    want = zlib.crc32(expected) & 0xFFFFFFFF
    try:
        got = zlib.crc32(read_ctl(EditorLink(link), add_ctl.BLE_SIZE)) & 0xFFFFFFFF
    except TimeoutError as e:
        raise InstallError("ctlcrc", f"installed, but the Bluetooth controller image was not read back ({e})")
    print(f"Bluetooth controller block read back: {add_ctl.BLE_SIZE} B, unit CRC32 {got:08x}, "
          f"expected block CRC32 {want:08x}", file=out)
    if got != want:
        raise InstallError("ctlcrc", f"Bluetooth controller block: unit CRC32 {got:08x} is not the expected "
                                     f"ctl.img plus erased padding CRC32 {want:08x}")
    return got


def run(a, backend, out, ask):
    up = Updater(backend, a.port)
    if a.info:
        dev = up.find()
        if not dev:
            raise not_found(up)
        dev.link.close()
        mode = "update loader (update not finished)" if dev.id.loader else "running"
        print(f"{dev.id.text}  [{mode}]  port: {dev.name}", file=out)
        return 0
    product, image = load_package(a.package, a.force)
    print(f"package: {product}  ({a.package})", file=out)
    image = with_ctl(a, product, image, out)
    dev = up.find()
    if not dev:
        raise not_found(up)
    print(f"device:  {dev.id.text}  ({dev.name})" + ("  in update mode: the write will be finished" if dev.id.loader else ""),
          file=out)
    move = None
    if a.backup and dev.id.loader:
        dev.link.close()
        raise InstallError("slots", "--backup needs the running firmware; the device is already in update mode. "
                                    "Nothing was written")
    if dev.id.loader and not a.no_slots:
        dev.link.close()
        raise InstallError("slots", "the device is already in update mode, where sample layout and backup "
                                    "cannot be checked. No update was started by this attempt. "
                                    "Resume with --no-slots only after verifying an existing slot backup "
                                    "or confirming that no X0X samples need preservation; restore "
                                    "any saved slots after the firmware update")
    if not dev.id.loader and not a.no_slots and same_keyboard(dev.id.text, product):
        try:
            move = plan_slots(dev, image, product, require_backup=bool(a.backup))
        except (xs.SlotError, TimeoutError) as e:
            dev.link.close()
            raise InstallError("slots", f"the requested slot backup cannot start ({e}); nothing was written")
        if move and move.risk:
            print(f"This install changes the sample slot map. These slots would lose their samples: {move.describe()}.",
                  file=out)
            print("They are saved to a file first and written back after the install." if move.new_map else
                  "They are saved to a file first. This firmware has no X0X slots: restore them later in X0X.", file=out)
    if not a.yes and not ask("Install? Do not unplug the FM-1 while writing. [y/N] "):
        dev.link.close()
        print("cancelled", file=out)
        return 1
    if move:
        path = a.backup or f"x0x-slots-before-install-{time.strftime('%Y%m%d-%H%M%S')}.x0xslots"
        try:
            move.backup(dev.link, path, out)
        except (xs.SlotError, TimeoutError, OSError) as e:
            dev.link.close()
            raise InstallError("slots", f"the slot backup failed ({e}); nothing was written. Run "
                                        "tools/x0x_slots.py backup FILE, then install with --no-slots")
    prog = Progress(out)
    try:
        up.install(dev, image, product, prog)
    finally:
        prog.end()
    if move and move.risk and move.new_map:
        back = up.reconnect_after(lambda i: not i.loader, DELAY["wait_reboot"])
        try:
            if not back:
                raise TimeoutError("the device did not answer after the install")
            try:
                _, skipped = move.restore(back.link, out)
                if skipped:
                    raise xs.SlotError(f"{len(skipped)} backed-up sample slot(s) could not be restored")
            finally:
                back.link.close()
        except (xs.SlotError, TimeoutError, KeyError, IndexError) as e:
            raise InstallError("slots", f"installed, but the slots were not written back ({e}). {move.help()}")
    elif move and move.risk:
        raise InstallError("slots", f"installed, but this firmware has no X0X sample slots for restoration. "
                                    f"The backed-up slots were not written back. {move.help()}")
    ctl = getattr(a, "ctl_bytes", None)
    if ctl:
        back = up.reconnect_after(lambda i: not i.loader, DELAY["wait_reboot"])
        if not back:
            raise InstallError("ctlcrc", "installed, but the device did not answer for the Bluetooth image read-back")
        try:
            check_ctl(back.link, ctl, out)
        finally:
            back.link.close()
    return 0


def ask_tty(prompt):
    try:
        return input(prompt).strip().lower() in ("y", "yes")
    except EOFError:
        return False


def gate_guard(a):
    """Require exact gate or scoped dev evidence before opening the device."""
    import os
    import subprocess
    commit = a.commit or os.environ.get("X0X_INSTALL_COMMIT", "")
    if not commit:
        print("REFUSED: give --commit HASH (the gate guard needs the commit of the package)", file=sys.stderr)
        return 9
    dev_record = getattr(a, "dev_record", None)
    if dev_record and (a.config != "dev" or (not a.ctl and not a.no_ctl)):
        print("REFUSED: --dev-record needs --config dev and explicit --ctl or --no-ctl", file=sys.stderr)
        return 9
    guard = Path(__file__).resolve().parent / "require_gate_pass.sh"
    cmd = ["bash", str(guard), commit, "--fwsc", a.package, "--config", a.config] + (["--ctl", a.ctl] if a.ctl else [])
    if dev_record:
        cmd += ["--dev-record", dev_record]
    try:
        r = subprocess.run(cmd, capture_output=True, text=True)
    except OSError as e:
        print(f"REFUSED {commit}: cannot run {guard}: {e}", file=sys.stderr)
        return 9
    print((r.stdout + r.stderr).strip())
    lines = r.stdout.splitlines()
    expected = "DEV OK " if dev_record else "GATE OK "
    return 0 if r.returncode == 0 and lines and lines[-1].startswith(expected) else 9


def foreground_guard(stdin=None, env=None):
    """rule 7: 0 if the install runs in a terminal or in a screen/tmux session, else 10 (no install)"""
    import os
    stdin = sys.stdin if stdin is None else stdin
    env = os.environ if env is None else env
    try:
        tty = stdin.isatty()
    except (AttributeError, ValueError):
        tty = False
    if tty or env.get("STY") or env.get("TMUX"):
        return 0
    print("REFUSED: stdin is not a terminal and STY and TMUX are empty. Run the install in the foreground or in a "
          "named screen session (screen -S x0x-install), never with nohup or & (rule 7)", file=sys.stderr)
    return 10


_HW_LOCK = None


def hardware_lock():
    """rule 3: take the hardware lock file (no wait). 0 if taken (kept until the process ends), else 10"""
    import fcntl
    import os
    global _HW_LOCK
    path = os.environ.get("X0X_HW_LOCK") or os.path.join(os.path.expanduser("~"), ".x0x37-hardware.lock")
    try:
        f = open(path, "a")
    except OSError as e:
        print(f"REFUSED: cannot open the hardware lock {path}: {e.strerror}", file=sys.stderr)
        return 10
    try:
        fcntl.flock(f, fcntl.LOCK_EX | fcntl.LOCK_NB)
    except OSError:
        f.close()
        print(f"REFUSED: another install or device run holds the hardware lock {path}. "
              "Run one hardware job at a time (rule 3)", file=sys.stderr)
        return 10
    _HW_LOCK = f
    return 0


def main(argv=None, backend=None, out=sys.stdout, ask=ask_tty):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("package", nargs="?", help="Felucca package (.fwsc)")
    ap.add_argument("--info", action="store_true", help="print the identity of the connected FM-1")
    ap.add_argument("--port", metavar="NAME", help="MIDI port to use (part of its name)")
    ap.add_argument("--yes", action="store_true", help="do not ask for confirmation")
    ap.add_argument("--force", action="store_true", help="install a package without the Felucca loader marker")
    ap.add_argument("--backup", metavar="FILE", help="save and verify every slot here before writing, even if none is at risk")
    ap.add_argument("--no-slots", action="store_true", help="explicit risk opt-out: skip sample checking and backup only after confirming no X0X samples need saving")
    ap.add_argument("--ctl", metavar="FILE", help="SMK-37: add this Bluetooth controller image (ble/ctl/out/ctl.img)")
    ap.add_argument("--no-ctl", action="store_true",
                    help="SMK-37: use only a valid controller already embedded in the package (never preserves the unit block)")
    ap.add_argument("--commit", metavar="HASH", help="the source commit covered by the package validation record")
    ap.add_argument("--dev-record", metavar="FILE", help="scoped development validation record; requires config dev and explicit --ctl or --no-ctl")
    ap.add_argument("--head-ref", metavar="FILE", help="a flash read-back of this unit (raw, from flash 0): the bootloader guard reference")
    ap.add_argument("--head-check", action="store_true", help="compare the package head with the reference only (no device)")
    ap.add_argument("--config", choices=("dev", "rel"), default="dev",
                    help="the build config of the package for the gate guard: dev (X0X_DEV_PADCAL=1, default) or rel (0)")
    a = ap.parse_args(argv)
    if a.backup and a.no_slots:
        ap.error("--backup and --no-slots cannot be used together")
    if bool(a.info) == bool(a.package):
        ap.error("give a PACKAGE.fwsc or --info")
    if a.head_check:                        # no device: only the bootloader guard
        if not a.package or head_ref_path(a) is None:
            ap.error("--head-check needs a PACKAGE.fwsc and a reference (--head-ref FILE or X0X_HEAD_REF)")
        rc = head_guard(a, out=out)
        print("HEAD OK" if rc == 0 else "HEAD MISMATCH" if rc == EXIT["head"] else "HEAD ERROR", file=out)
        return rc
    if a.package and backend is None:       # a real device: the gate guard is the first step
        rc = gate_guard(a)
        if rc:
            return rc
        rc = foreground_guard()
        if rc:
            return rc
    if backend is None:                     # a real device (install or --info): one hardware job at a time
        rc = hardware_lock()
        if rc:
            return rc
    if a.package and backend is None:       # a real device: the bootloader guard (check 4), before any device is opened
        rc = head_guard(a)
        if rc:
            return rc
    try:
        if backend is None:
            try:
                backend = MidoBackend()
            except ImportError:
                raise InstallError("usage", "needs mido and python-rtmidi (pip install mido python-rtmidi)")
        return run(a, backend, out, ask)
    except InstallError as e:
        print(f"error: {e}", file=sys.stderr)
        return EXIT.get(e.code, 1)
    except KeyboardInterrupt:
        print("\ninterrupted: if the FM-1 is in update mode, run the install again to finish", file=sys.stderr)
        return 130


if __name__ == "__main__":
    sys.exit(main())
