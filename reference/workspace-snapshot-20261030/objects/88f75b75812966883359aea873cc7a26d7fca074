#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""F02: after a commanded loader transition, wait_for must not attach to another keyboard.

No real MIDI ports are opened and nothing is written. Models the audited first-match
bug (evidence/f02-stage1/repro.py) and asserts the fail-closed newly-appeared rule.
Run from the repo root: python3 tests/install_f02_loader_bind_test.py
"""
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import fm1_install as fi  # noqa: E402

fi.DELAY["hs"] = 0.01
fi.DELAY["retry"] = 0
failed = 0


def ok(cond, what):
    global failed
    print(f"{what:<72} {'ok' if cond else 'FAIL'}")
    failed += not cond


def identity_packet(text):
    body = bytearray(34)
    body[0:4] = b"\x00\x59\x11\x1b"
    raw = text.encode("latin-1")
    if len(raw) > 27:
        raise SystemExit("identity text does not fit the handshake")
    body[6:6 + len(raw)] = raw
    body[33] = ~sum(body[6:33]) & 255
    return b"\xF0" + fi.pack7(bytes(body)) + b"\xF7"


class Link:
    def __init__(self, name, text):
        self.name = name
        self.text = text
        self.closed = False
        self._given = False

    def send(self, _pkt):
        return True

    def read(self, _timeout):
        if self._given:
            return None
        self._given = True
        return identity_packet(self.text)

    def drain(self):
        return None

    def close(self):
        self.closed = True


class Backend:
    def __init__(self, devices):
        self.devices = dict(devices)

    def input_names(self):
        return list(self.devices)

    def output_names(self):
        return list(self.devices)

    def open(self, in_name, _out_name):
        return Link(in_name, self.devices[in_name])


def first_match_loader(backend):
    """Current pre-repair behaviour: first enumerated loader wins."""
    up = fi.Updater(backend)
    return up.find(lambda ident: ident.loader, only_port=False)


def main():
    # Other keyboard already in its update loader and enumerated first; selected SMK still running.
    backend = Backend({
        "Other OTA": "ota-FM-1_906",
        "Selected SMK": "SMK-37 Pro_900",
    })
    wrong = first_match_loader(backend)
    ok(wrong is not None and wrong.name == "Other OTA",
       "model: first-match would attach to the unrelated loader")
    if wrong:
        wrong.link.close()

    up = fi.Updater(backend)
    before = up.matching_ports(lambda i: i.loader)
    ok(before == ["Other OTA"], "snapshot before transition is the pre-existing loader only")

    # Selected keyboard has not entered the loader yet: fail closed (do not take the old one).
    none = up.wait_for(lambda i: i.loader, 0.05, before)
    ok(none is None, "wait_for: only a pre-existing loader -> timeout (noloader), not that port")

    # Selected keyboard appears as a new loader while the unrelated one remains.
    backend.devices["Selected SMK"] = "ota-SMK-37 Pro_900"
    got = up.wait_for(lambda i: i.loader, 0.05, before)
    ok(got is not None and got.name == "Selected SMK" and got.id.loader,
       "wait_for: accepts exactly the newly appeared loader, not Other OTA")
    if got:
        got.link.close()
    ok(up.returned_port == "Selected SMK", "returned_port records the accepted transition port")

    # Two newly appeared loaders: refuse to choose (names must match PORT_RE).
    backend2 = Backend({
        "Other OTA": "ota-FM-1_906",
        "Felucca New A": "ota-SMK-37 Pro_900",
        "Felucca New B": "ota-SMK-37 Elite_900",
    })
    up2 = fi.Updater(backend2)
    try:
        up2.wait_for(lambda i: i.loader, 0.05, ["Other OTA"])
        ok(False, "wait_for: two new loaders raise InstallError")
    except fi.InstallError as e:
        ok(e.code == "model" and "more than one new device" in str(e),
           "wait_for: two new loaders raise InstallError (fail closed)")

    # Post-write / slot-restore reconnect: do not hand another running keyboard.
    running = Backend({
        "Other SMK": "SMK-37 Pro_800",
        "Selected SMK": "SMK-37 Pro_900",
    })
    up3 = fi.Updater(running)
    up3.returned_port = "Selected SMK"
    back = up3.reconnect_after(lambda i: not i.loader, 0.05)
    ok(back is not None and back.name == "Selected SMK",
       "reconnect_after: reopens the transition port, not the sibling keyboard")
    if back:
        back.link.close()

    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
