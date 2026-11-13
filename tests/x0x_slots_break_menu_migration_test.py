#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Host regression: slot migration must not reorder the LOOP menu (pattern slot_a/slot_b).

    python3 tests/x0x_slots_break_menu_migration_test.py
"""
import sys
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import x0x_slots as xs  # noqa: E402
from slot_mock import SMK_MAP, SMK_NOW  # noqa: E402


def lay3():
    return {"count": 3, "ext": True, "size": 0x14000,
            "slots": [{"nz": 0, "name": "", "used": 0},
                      {"nz": 1, "name": "LOOPA", "used": 30},
                      {"nz": 1, "name": "LOOPB", "used": 20}]}


def lay2_empty():
    return {"count": 2, "ext": True, "size": 0x14000,
            "slots": [{"nz": 0, "name": "", "used": 0},
                      {"nz": 1, "name": "LOOPA", "used": 30}]}


def img(tag):
    return bytes([tag]) * 800


def check(cond, msg):
    if not cond:
        print("FAIL", msg)
        return 1
    print("ok", msg)
    return 0


def main():
    bad = 0
    old_lay = lay3()
    images = [None, img(1), img(2)]
    old_map = SMK_MAP
    new_map = SMK_NOW
    new_lay = lay2_empty()
    risk = [2]
    todo, skipped = xs.plan_migration(images, risk, old_map, new_lay, new_map)
    bad += check(todo == [(2, 0)], f"planner moves slot 3 -> slot 1: {todo}")
    bad += check(xs.migration_reorders_loop_menu(old_lay, images, risk, old_map, new_lay, new_map, todo),
                 "empty slot 1 + A in 2 + B in 3 reorders LOOP menu")
    safe_todo, extra = xs.guard_migration_loop_menu(old_lay, images, risk, old_map, new_lay, new_map, todo, skipped)
    bad += check(safe_todo == [] and any("LOOP menu order" in s for s in extra),
                 "guard blocks automatic restore")

    # Slots 1 and 2 kept at the same offsets: menu order unchanged.
    full = lay3()
    full["slots"][0]["nz"] = 1
    images3 = [img(9), img(1), img(2)]
    risk3 = [2]
    todo3, _ = xs.plan_migration(images3, risk3, old_map, new_lay, new_map)
    bad += check(not xs.migration_reorders_loop_menu(full, images3, risk3, old_map, new_lay, new_map, todo3),
                 "dropping only slot 3 does not reorder remaining LOOP entries")

    # A skipped middle image must not assign its saved numeric LOOP index to C.
    # Use two zones in B: the production engine lists every zone, not every slot.
    old = {"count": 3, "slots": [{"nz": 1}, {"nz": 2}, {"nz": 1}]}
    imgs = [b"A" * 40000, b"B" * 100000, b"C" * 30000]
    om = [(0x1000, 50000), (0x20000, 120000), (0x40000, 50000)]
    nm = [om[0], (0x50000, 80000)]
    nl = {"count": 2, "slots": [{"nz": 1}, {"nz": 0}]}
    todo, skipped = xs.plan_migration(imgs, [1, 2], om, nl, nm)
    bad += check(todo == [(2, 1)] and len(skipped) == 1, "oversized middle B leaves C as the only proposed write")
    guarded, messages = xs.guard_migration_loop_menu(old, imgs, [1, 2], om, nl, nm, todo, skipped)
    bad += check(not guarded and len(messages) == 2 and "compatible layout" in messages[-1],
                 "absolute zone indices prevent skipped B from resolving to restored C")
    bad += check(xs._user_loop_menu_ids(old, imgs) == [("old", 0, 0), ("old", 1, 0), ("old", 1, 1), ("old", 2, 0)],
                 "each zone has its own original identity")
    # Identical image bytes cannot hide original-slot reordering.
    same = [img(4), img(4)]
    two = {"slots": [{"nz": 1}, {"nz": 1}]}
    empty = {"slots": [{"nz": 0}, {"nz": 0}]}
    mm = [(0x1000, 1000), (0x2000, 1000)]
    bad += check(xs.migration_reorders_loop_menu(two, same, [0, 1], mm, empty, mm, [(0, 1), (1, 0)]),
                 "identical CRCs do not hide distinct original slot identities")
    bad += check(not xs.migration_reorders_loop_menu(two, same, [0, 1], mm, empty, mm, [(0, 0), (1, 1)]),
                 "same absolute menu permits physical relocation")
    # Dropping only trailing loops leaves all surviving absolute indices intact.
    trailing = {"slots": [{"nz": 1}, {"nz": 1}, {"nz": 1}]}
    bad += check(not xs.migration_reorders_loop_menu(trailing, [img(1), img(2), img(3)], [1, 2], om, nl, nm, [(1, 1)]),
                 "safe restored prefix with trailing omission remains allowed")
    appended = {"slots": [{"nz": 0}, {"nz": 1}]}
    one = {"slots": [{"nz": 1}]}
    bad += check(not xs.migration_reorders_loop_menu(one, [img(1)], [0], mm[:1], appended, mm, [(0, 0)]),
                 "new trailing occupied loop does not shift existing indices")
    bad += check(xs.migration_reorders_loop_menu(one, [img(1)], [0], mm[:1], {"slots": [{"nz": 1}, {"nz": 0}]}, mm, [(0, 1)]),
                 "unknown occupied prefix cannot silently shift a restored loop")

    print("x0x_slots_break_menu_migration_test:", "FAILED" if bad else "passed")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
