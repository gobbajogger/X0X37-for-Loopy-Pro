#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Native source-AST regression: legacy RtMidi sample link must never send destructive SysEx.

Run: python3 -B tests/host/sample_link_readonly_test.py [tools/fm1_sample_upload.py]
Simulated USB/MIDI endpoint changes; this does NOT prove authenticated device hardware.
"""
import ast
import sys
from pathlib import Path
from types import ModuleType, SimpleNamespace


class Clock:
    def __init__(self):
        self.now = 0.0

    def time(self):
        self.now += 0.5
        return self.now

    def sleep(self, unused):
        pass


class FakeMido(ModuleType):
    def __init__(self):
        super().__init__("mido")
        self.sent, self.queue, self.device = [], [], "unit-A"

    def get_input_names(self):
        return ["Felucca USB MIDI"]

    def get_output_names(self):
        return ["Felucca USB MIDI"]

    def open_output(self, name):
        outer = self

        class Output:
            def send(self, msg):
                outer.sent.append((outer.device, tuple(msg.data)))
                outer.queue.append(SimpleNamespace(type="sysex", data=tuple([0x7D, 0x46, 0x4C, msg.data[3], 0])))

            def close(self):
                pass
        return Output()

    def open_input(self, name):
        outer = self

        class Input:
            def iter_pending(self):
                while outer.queue:
                    yield outer.queue.pop(0)

            def close(self):
                pass
        return Input()

    @staticmethod
    def Message(kind, data):
        return SimpleNamespace(type=kind, data=data)


def read_link(path):
    root = ast.parse(Path(path).read_text(encoding="utf-8"), str(path))
    nodes = [x for x in root.body if isinstance(x, ast.ClassDef) and x.name == "Link"]
    assert len(nodes) == 1 and {"__init__", "req"}.issubset({x.name for x in nodes[0].body})
    ns = {"sys": sys, "time": Clock(), "HDR": [0x7D, 0x46, 0x4C]}
    exec(compile(ast.Module(body=nodes, type_ignores=[]), str(path), "exec"), ns)
    return ns["Link"]


def main(source):
    link_type = read_link(source)
    outcomes = []
    scenarios = [
        ("unmodified single keyboard", "unit-A"),
        ("outgoing endpoint reassigned under identical MIDI name", "foreign-unit-B"),
        ("same-port device disconnect/reconnect ABA", "unit-A-after-swap"),
        ("stale sibling reply could acknowledge wrong target", "foreign-unit-stale-ACK"),
    ]
    for name, destination in scenarios:
        mido = FakeMido()
        old = sys.modules.get("mido")
        sys.modules["mido"] = mido
        try:
            link = link_type()
        finally:
            if old is None:
                sys.modules.pop("mido", None)
            else:
                sys.modules["mido"] = old
        check = len(mido.sent) == 1 and mido.sent[0][1][3] == 1
        outcomes.append(check)
        print(("PASS" if check else "FAIL") + " initial read-only handshake / " + name)
        mido.device = destination
        for cmd in (11, 12, 13, 14, 17, True):
            prior = len(mido.sent)
            refused = False
            try:
                link.req(cmd, [0])
            except SystemExit as exc:
                message = str(exc).lower()
                refused = "unsupported" in message and "no erase/write" in message
            good = refused and len(mido.sent) == prior
            outcomes.append(good)
            print(("PASS" if good else "FAIL") + f" {name} cmd={cmd}: refused={refused}, packets={len(mido.sent)-prior}")
        for cmd in (1, 15, 16):
            prior = len(mido.sent)
            reply = link.req(cmd, [])
            good = (reply == [0] and len(mido.sent) == prior + 1 and mido.sent[-1][1][3] == cmd)
            outcomes.append(good)
            print(("PASS" if good else "FAIL") + f" {name} read-only cmd={cmd}")
    print(f"SUMMARY {sum(outcomes)}/{len(outcomes)}")
    return 0 if all(outcomes) else 1


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1] if len(sys.argv) > 1 else "tools/fm1_sample_upload.py"))