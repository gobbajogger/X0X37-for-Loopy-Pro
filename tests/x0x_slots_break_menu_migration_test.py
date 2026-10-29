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

    print("x0x_slots_break_menu_migration_test:", "FAILED" if bad else "passed")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
