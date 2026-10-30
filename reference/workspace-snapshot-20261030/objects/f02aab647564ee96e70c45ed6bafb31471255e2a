#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Host-only restoration planning regressions, no MIDI/device or destructive writes.

    python3 tests/x0x_slots_migration_matching_test.py
    python3 tests/x0x_slots_migration_matching_test.py /path/to/source.py
"""
import importlib.util
import random
import sys
from pathlib import Path
import unittest

source = (Path(sys.argv.pop(1)) if len(sys.argv) > 1
          else Path(__file__).resolve().parents[1] / "tools" / "x0x_slots.py")
spec = importlib.util.spec_from_file_location("x0x_slots_migration_source", source)
slots = importlib.util.module_from_spec(spec)
spec.loader.exec_module(slots)


def plan(images, size, *, kept=(), occupied=(), risk=None):
    count = len(images)
    old = [(0x100000 + 0x20000 * k, 131072) for k in range(count)]
    new = [(old[k][0] if k in kept else 0x200000 + 0x20000 * k, n)
           for k, n in enumerate(size)]
    layout = {"slots": [{"nz": int(k in occupied)} for k in range(len(size))]}
    if risk is None:
        risk = [k for k in range(count) if k not in kept]
    todo, skipped = slots.plan_migration(images, risk, old, layout, new)
    return todo, skipped


class MigrationMatchingTest(unittest.TestCase):
    def test_large_image_gets_its_only_fitting_slot(self):
        images = [b"a" * 48, b"b" * 108]
        todo, skipped = plan(images, [120, 64])
        self.assertEqual(todo, [(0, 1), (1, 0)])
        self.assertEqual(skipped, [])

    def test_prefer_same_slot_on_equal_capacity(self):
        todo, skipped = plan([b"a" * 30, b"b" * 40, b"c" * 20], [64, 64, 64])
        self.assertEqual(todo, [(0, 0), (1, 1), (2, 2)])
        self.assertEqual(skipped, [])

    def test_never_overwrite_kept_slot(self):
        images = [b"a" * 40, b"b" * 110, b"c" * 30]
        todo, skipped = plan(images, [120, 64, 64], kept=(2,))
        self.assertEqual(todo, [(0, 1), (1, 0)])
        self.assertFalse(skipped)
        self.assertNotIn(2, [j for _, j in todo])

    def test_occupied_destination_is_not_selected(self):
        images = [b"a" * 10, b"b" * 70]
        todo, skipped = plan(images, [120, 64], occupied=(0,))
        self.assertEqual(todo, [(0, 1)])
        self.assertEqual(len(skipped), 1)
        self.assertIn("slot 2", skipped[0])

    def test_no_destinations_means_no_writes(self):
        todo, skipped = plan([b"a" * 12, b"b" * 10], [])
        self.assertEqual(todo, [])
        self.assertEqual(len(skipped), 2)

    def test_oversized_image_is_skipped_without_blocking_smaller(self):
        todo, skipped = plan([b"a" * 100, b"b" * 10], [64, 32])
        self.assertEqual(todo, [(1, 1)])
        self.assertEqual(len(skipped), 1)
        self.assertIn("slot 1", skipped[0])

    def test_keeps_risk_order_in_write_plan(self):
        todo, skipped = plan([b"a" * 10, b"b" * 100], [120, 64])
        self.assertEqual(todo, [(0, 1), (1, 0)])
        self.assertFalse(skipped)

    def test_no_risk_is_noop(self):
        todo, skipped = plan([b"a" * 8], [64], risk=[])
        self.assertEqual((todo, skipped), ([], []))

    def test_randomized_assignment_has_optimal_cardinality_and_is_safe(self):
        rand = random.Random(20261024)
        for trial in range(250):
            n = rand.randint(1, 5)
            images = [b"s" * rand.randint(1, 160) for _ in range(n)]
            size = [rand.choice([16, 32, 64, 96, 128, 160]) for _ in range(n)]
            kept = {k for k in range(n) if rand.random() < 0.15}
            occupied = {k for k in range(n) if rand.random() < 0.2}
            risk = [k for k in range(n) if k not in kept]
            todo, skipped = plan(images, size, risk=risk, kept=kept, occupied=occupied)
            self.assertEqual(len(todo) + len(skipped), len(risk), (trial, todo, skipped))
            self.assertEqual(len({k for k, _ in todo}), len(todo), trial)
            self.assertEqual(len({j for _, j in todo}), len(todo), trial)
            for k, j in todo:
                self.assertNotIn(j, kept)
                self.assertNotIn(j, occupied)
                self.assertLessEqual(len(images[k]), size[j])
            eligible = {k: [j for j in range(n)
                            if j not in kept and j not in occupied and len(images[k]) <= size[j]]
                        for k in risk}
            def maximum(i, used):
                if i == len(risk):
                    return 0
                k = risk[i]
                return max([maximum(i + 1, used)] +
                           [1 + maximum(i + 1, used | {j})
                            for j in eligible[k] if j not in used])
            self.assertEqual(len(todo), maximum(0, set()), (trial, images, size, todo, skipped))


if __name__ == "__main__":
    unittest.main(verbosity=2)
