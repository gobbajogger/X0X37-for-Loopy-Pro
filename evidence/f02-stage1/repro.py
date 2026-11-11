#!/usr/bin/env python3
"""Read-only model of F02 on the audited installer.

Imports tools/fm1_install.py and asks Updater.wait_for for a loader while
another keyboard's loader enumerates first. No MIDI ports are opened.
"""
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
import fm1_install as fi  # noqa: E402


def identity_packet(text):
    body = bytearray(34)
    body[0:3] = b"\x00\x59\x11"
    raw = text.encode("latin-1")
    if len(raw) > 27:
        raise SystemExit("identity text does not fit the handshake")
    body[6:6 + len(raw)] = raw
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
        self.devices = devices

    def input_names(self):
        return list(self.devices)

    def output_names(self):
        return list(self.devices)

    def open(self, in_name, _out_name):
        return Link(in_name, self.devices[in_name])


def main():
    # The other keyboard is already in its update loader and is enumerated first.
    # The selected SMK is still the running firmware.
    backend = Backend({
        "Other OTA": "ota-FM-1_906",
        "Selected SMK": "SMK-37 Pro_900",
    })
    fi.DELAY["hs"] = 0.01
    fi.DELAY["retry"] = 0
    found = fi.Updater(backend).wait_for(lambda ident: ident.loader, 0.05)
    if found is None:
        raise SystemExit("model did not find a loader")
    print(f"wait_for loader returned {found.id.text} on {found.name}")
    if found.name != "Other OTA":
        raise SystemExit("expected the unrelated loader")
    print("BUG REPRODUCED: loader search accepted a different keyboard")


if __name__ == "__main__":
    main()
