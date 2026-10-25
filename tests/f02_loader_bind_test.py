#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""F02 regression: post-transition wait_for must not attach to another keyboard.

No real MIDI, no device write. Models the stage-1 repro: an unrelated loader is
enumerated first while the selected keyboard is still running. With an empty
known set (legacy first-match), wait_for returns the wrong loader. With the
pre-transition snapshot, wait_for ignores that loader and returns None until a
new loader appears — then accepts exactly that one.
"""
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import fm1_install as fi  # noqa: E402

failed = 0


def ok(cond, what):
    global failed
    print(f"{what:<72} {'ok' if cond else 'FAIL'}")
    failed += not cond


def identity_packet(text):
    body = bytearray(34)
    body[0:3] = b"\x00\x59\x11"
    raw = text.encode("latin-1")
    body[6:6 + len(raw)] = raw
    return b"\xF0" + fi.pack7(bytes(body)) + b"\xF7"


class Link:
    def __init__(self, name, text):
        self.name, self.text, self.closed, self._given = name, text, False, False

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


def main():
    fi.DELAY["hs"] = 0.01
    fi.DELAY["retry"] = 0

    # Unrelated loader enumerated first; selected SMK still running (stage-1 repro).
    backend = Backend({
        "Other OTA": "ota-FM-1_906",
        "Selected SMK": "SMK-37 Pro_900",
    })
    up = fi.Updater(backend)

    # Legacy first-match shape: empty known set treats every match as new.
    # With exactly one loader present that is the unrelated one — still wrong.
    legacy = up.wait_for(lambda ident: ident.loader, 0.05, known=set())
    ok(legacy is not None and legacy.name == "Other OTA",
       "legacy empty-known: first loader is the unrelated keyboard (bug shape)")

    # Repair: snapshot loaders before the transition (Other OTA already present).
    known = up.matching_ports(lambda ident: ident.loader)
    ok(known == {"Other OTA"}, "snapshot before transition includes the unrelated loader")
    bound = up.wait_for(lambda ident: ident.loader, 0.05, known=known)
    ok(bound is None, "bound wait ignores the pre-existing loader (fail closed, no write)")

    # Selected keyboard enters its loader: exactly one new port appears.
    backend.devices["Selected SMK"] = "ota-SMK-37 Pro_900"
    got = up.wait_for(lambda ident: ident.loader, 0.05, known=known)
    ok(got is not None and got.name == "Selected SMK" and got.id.text.startswith("ota-SMK"),
       "bound wait accepts the one newly appeared loader")

    # Two new loaders at once: refuse (port names must match PORT_RE).
    backend.devices["Elite OTA"] = "ota-SMK-37 Elite_900"
    try:
        up.wait_for(lambda ident: ident.loader, 0.05, known=known)
        ok(False, "two new loaders raise ambiguous")
    except fi.InstallError as e:
        ok(e.code == "ambiguous", "two new loaders raise ambiguous")

    # Post-write: other running keyboard must not be chosen as the restart.
    backend2 = Backend({
        "Other FM-1": "FM-1_906",
        "Felucca Update": "ota-SMK-37 Pro_900",
    })
    up2 = fi.Updater(backend2)
    known_run = up2.matching_ports(lambda ident: not ident.loader)
    ok(known_run == {"Other FM-1"}, "reboot snapshot keeps the other running keyboard")
    backend2.devices.pop("Felucca Update")
    backend2.devices["Selected SMK Back"] = "SMK-37 Pro_900"
    back = up2.wait_for(lambda ident: not ident.loader, 0.05, known=known_run)
    ok(back is not None and back.name == "Selected SMK Back",
       "bound reboot wait accepts the one newly appeared running keyboard")

    print(f"F02 LOADER BIND TESTS FAILED ({failed})" if failed else "f02 loader bind tests passed")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
