#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Restore safety: never rewrite a later slot after earlier readback failure.

Run with: python3 tests/x0x_slots_verify_order_test.py
Uses host mocks only; no USB, firmware, flash or device is required.
"""
import sys
from pathlib import Path
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import x0x_slots as xs  # noqa: E402


class Link:
    def __init__(self, corrupt=None):
        self.corrupt = corrupt
        self.events = []
        self.slots = {}


def layout(link):
    link.events.append(("info",))
    return {"count": 2, "slots": [{"nz": 1}, {"nz": 1}]}


def plan_restore(lay, meta, images, only=None):
    return [(k, img) for k, img in enumerate(images) if only is None or k + 1 in only], []


def write_slot(link, k, img, progress=None):
    link.events.append(("begin", k))
    link.slots[k] = img
    link.events.append(("end", k))


def read_slot(link, k, nz):
    link.events.append(("read", k))
    return b"corrupt" if link.corrupt == k else link.slots[k]


def check(condition, label):
    print(f"{'PASS' if condition else 'FAIL'}: {label}")
    return 0 if condition else 1


def main():
    fails = 0
    images = [b"first-slot-image", b"second-slot-image"]
    with patch.object(xs, "layout", layout), patch.object(xs, "plan_restore", plan_restore), \
            patch.object(xs, "write_slot", write_slot), patch.object(xs, "read_slot", read_slot):
        for label, operation in (
            ("regular", lambda link, **kw: xs.restore(link, {}, images, **kw)),
            ("migration", lambda link, **kw: xs.restore_mapped(link, [(0, 0), (1, 1)], images, **kw)),
        ):
            link = Link(corrupt=0)
            try:
                operation(link)
                fails += check(False, f"{label}: failed first readback must raise SlotError")
            except xs.SlotError:
                fails += check(True, f"{label}: first readback mismatch reported")
            fails += check(("begin", 1) not in link.events and link.slots.get(1) is None,
                           f"{label}: mismatch must not erase/write second slot")
            fails += check(link.events.index(("read", 0)) > link.events.index(("end", 0)),
                           f"{label}: verify first slot after its END")

            good = Link()
            operation(good)
            fails += check(good.slots == {0: images[0], 1: images[1]},
                           f"{label}: successful restore writes both slots")
            fails += check(good.events.index(("read", 0)) < good.events.index(("begin", 1)),
                           f"{label}: first readback precedes second BEGIN")

        no_verify = Link(corrupt=0)
        xs.restore(no_verify, {}, images, verify=False)
        fails += check([e for e in no_verify.events if e[0] == "begin"] == [("begin", 0), ("begin", 1)]
                       and not any(e[0] == "read" for e in no_verify.events),
                       "explicit verify=False retains legacy no-readback behavior")

        only_second = Link(corrupt=0)
        xs.restore(only_second, {}, images, only={2})
        fails += check(only_second.slots == {1: images[1]},
                       "selected slot restore leaves unselected first slot untouched")

        later_bad = Link(corrupt=1)
        try:
            xs.restore(later_bad, {}, images)
            fails += check(False, "second-slot readback mismatch must raise SlotError")
        except xs.SlotError:
            fails += check(later_bad.events.count(("read", 0)) == 1
                           and later_bad.events.count(("read", 1)) == 1,
                           "second-slot mismatch detected after first verified")

    print(f"FAILED_ASSERTIONS={fails}")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
