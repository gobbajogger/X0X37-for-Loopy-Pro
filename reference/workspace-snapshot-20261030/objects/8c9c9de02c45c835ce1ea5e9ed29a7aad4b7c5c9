#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""F03: on the same-keyboard slot-preserving path, a failed sample-slot layout probe
must fail closed before any write. Do not treat probe None / timeout as "no backup needed".

No real MIDI ports are opened and nothing is written. Models today's silent skip and
asserts the repaired fail-closed rule. Run from the repo root:
  python3 tests/install_f03_slot_probe_test.py
"""
import io
import sys
import tempfile
from pathlib import Path
from types import SimpleNamespace

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tests"))
sys.path.insert(0, str(ROOT / "tools"))
import fm1_install as fi  # noqa: E402
import install_test as T  # noqa: E402
import x0x_slots as xs  # noqa: E402

fi.DELAY.update(open=0, start=0.01, reply=0, loader=0.01, reboot=0.01, retry=0.02, poll=0.05, hs=0.1,
                idle_check=0.4, idle_write=0.4, wait_loader=2, wait_reboot=2)
fi.SLOT_PROBE.update(timeout=0.05, tries=1)
failed = 0


def ok(cond, what):
    global failed
    print(f"{what:<72} {'ok' if cond else 'FAIL'}")
    failed += not cond


def empty_layout():
    return {"count": 0, "size": 0, "ext": False, "slots": []}


def silent_skip_plan(dev, image, product, require_backup=False):
    """Pre-repair plan_slots: probe None skips backup unless --backup."""
    lay = None
    try:
        lay = xs.parse_info(xs.req(fi.EditorLink(dev.link), xs.CMD_INFO, [],
                                   fi.SLOT_PROBE["timeout"], fi.SLOT_PROBE["tries"]))
    except (TimeoutError, xs.SlotError, ValueError, IndexError):
        lay = None
    if not lay:
        if require_backup:
            raise xs.SlotError("the running firmware did not provide its slot layout")
        return None
    if not require_backup and not any(sl["nz"] for sl in lay["slots"]):
        return None
    return "would-plan"


def main():
    raw = T.package()
    image = fi.logical_image(raw)
    product = "SMK-37 Pro_900"
    tmp = Path(tempfile.mkdtemp(prefix="x0x-f03-"))
    pkg = tmp / "ok.fwsc"
    pkg.write_bytes(raw)

    # --- model today's silent skip on a device that never answers SMP_INFO ---
    silent = T.FakeSMK(image)
    silent.answer_slots = False
    silent_dev = silent.open(silent.name, silent.name)
    # FakeSMK.open returns a link; wrap as Device-like for plan helpers
    class Dev:
        def __init__(self, link, text):
            self.link = link
            self.id = SimpleNamespace(text=text, loader=False)

    d = Dev(silent_dev, "SMK-37 Pro_016")
    old = silent_skip_plan(d, image, product)
    ok(old is None, "model: pre-repair plan treats probe failure as None (silent skip)")
    silent_dev.close()

    # --- repaired slot_probe / plan_slots: probe failure raises ---
    mute = T.FakeSMK(image)
    mute.answer_slots = False
    link = mute.open(mute.name, mute.name)
    try:
        fi.slot_probe(link)
        ok(False, "slot_probe: timeout raises")
    except (TimeoutError, xs.SlotError) as e:
        # slot_probe wraps TimeoutError as SlotError("SMP_INFO timed out…") for CLI fail-closed.
        ok(isinstance(e, TimeoutError) or "timed out" in str(e).lower(),
           "slot_probe: timeout raises")
    link.close()

    mute2 = T.FakeSMK(image)
    mute2.answer_slots = False
    d2 = Dev(mute2.open(mute2.name, mute2.name), "SMK-37 Pro_016")
    try:
        fi.plan_slots(d2, image, product)
        ok(False, "plan_slots: probe failure raises (fail closed)")
    except (TimeoutError, xs.SlotError) as e:
        ok(isinstance(e, TimeoutError) or "timed out" in str(e).lower(),
           "plan_slots: probe failure raises (fail closed)")
    d2.link.close()

    # --- successful empty layout: no backup needed ---
    empty = T.FakeSMK(image)  # answers empty SMP_INFO by default
    d3 = Dev(empty.open(empty.name, empty.name), "SMK-37 Pro_016")
    move = fi.plan_slots(d3, image, product)
    ok(move is None, "plan_slots: successful empty layout -> None (no backup)")
    d3.link.close()

    # --- run(): probe failure stops before write; --no-slots still skips ---
    mute3 = T.FakeSMK(image)
    mute3.answer_slots = False
    rc, out, err = T.cli([str(pkg), "--yes"], mute3)
    # Production wraps mute as SlotError → InstallError("slots", "…backup cannot start (SMP_INFO timed out…)")
    ok(rc == fi.EXIT["slots"] == 7 and mute3.upgrades == 0
       and "nothing was written" in err
       and ("timed out" in err.lower() or "slot backup cannot start" in err),
       "run: probe failure -> exit 7, no install write")

    mute4 = T.FakeSMK(image)
    mute4.answer_slots = False
    rc, out, err = T.cli([str(pkg), "--yes", "--no-slots"], mute4)
    ok(rc == 0 and mute4.upgrades == 2 and "runs SMK-37 Pro_900" in out,
       "run: --no-slots skips the probe and still installs")

    # successful empty parse still installs without --no-slots
    ok_dev = T.FakeSMK(image)
    rc, out, err = T.cli([str(pkg), "--yes"], ok_dev)
    ok(rc == 0 and ok_dev.upgrades == 2, "run: successful empty layout installs without --no-slots")

    # require_backup still fails closed on mute device
    mute5 = T.FakeSMK(image)
    mute5.answer_slots = False
    rc, out, err = T.cli([str(pkg), "--yes", "--backup", str(tmp / "x.x0xslots")], mute5)
    ok(rc == 7 and mute5.upgrades == 0 and "nothing was written" in err,
       "run: --backup with probe failure -> exit 7, nothing written")

    print(f"F03 SLOT PROBE TESTS FAILED ({failed})" if failed else "f03 slot probe tests passed")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
