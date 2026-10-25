#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Release numbers, codec boundaries, and serialized allocation."""

import json
from multiprocessing import Process, Queue
from pathlib import Path
import subprocess
import sys
import tempfile
import threading
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
import build  # noqa: E402
from release_alloc import (PRESERVED_UNVERSIONED, ReleaseError, allocate_committed,
                           allocate_file, bind_file, empty_ledger, product_for)


def git(repo, *args):
    subprocess.check_call(["git", "-C", str(repo), "-c", "user.name=t", "-c", "user.email=t@t", *args],
                          stdout=subprocess.DEVNULL)


def seed_repo(path):
    path.mkdir()
    subprocess.check_call(["git", "init", "-q", "-b", "main", str(path)], stdout=subprocess.DEVNULL)
    ledger = path / "releases" / "openbeta"
    ledger.mkdir(parents=True)
    (ledger / "ledger.json").write_text(json.dumps(empty_ledger(), indent=2) + "\n", encoding="utf-8")
    git(path, "add", "releases/openbeta/ledger.json")
    git(path, "commit", "-qm", "empty release ledger")


def race_worker(repo, remote, commit, tree, queue):
    try:
        item = allocate_committed(Path(repo), commit, tree, remote=remote)
        queue.put(item["version"])
    except Exception as exc:
        queue.put("ERR " + type(exc).__name__ + " " + str(exc))


class ReleaseAllocationTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.ledger = Path(self.tmp.name) / "ledger.json"

    def test_codec_boundaries_and_display_split(self):
        ident = build.release_identity("0.1.3")
        self.assertEqual(ident["numeric"], "0.1.3")
        self.assertEqual(ident["digits"], "000103")
        self.assertEqual(ident["release_label"], "0.1.3_openbeta")
        self.assertEqual(ident["version_macro"], "0.1.3")
        same = build.release_identity("0.1.3-openbeta")
        self.assertEqual(same["numeric"], ident["numeric"])
        self.assertEqual(same["digits"], ident["digits"])
        self.assertIsNone(build.release_identity("0.1.3-rc1")["release_label"])
        self.assertEqual(build.release_identity("99.99.99")["digits"], "999999")
        with self.assertRaises(ValueError):
            build.release_identity("0.1.3_openbeta")
        with self.assertRaises(ValueError):
            build.release_identity("100.0.0")
        self.assertEqual(product_for("elite", "0.1.3"), "SMK-37 Elite_9000103")
        self.assertEqual(product_for("pro", "0.1.3"), "SMK-37 Pro_9000103")
        self.assertLessEqual(len(product_for("elite", "99.99.99")), 20)
        self.assertNotIn("openbeta", product_for("elite", "0.1.3"))

    def test_rebuild_preserves_identity_and_rollover_is_explicit(self):
        commit, tree = "a" * 40, "b" * 64
        first = allocate_file(self.ledger, commit, tree)
        second = allocate_file(self.ledger, commit, tree)
        self.assertEqual(first["version"], "0.1.0")
        self.assertEqual(second["version"], first["version"])
        self.assertEqual(len(json.loads(self.ledger.read_text())["allocations"]), 1)
        with self.assertRaisesRegex(ReleaseError, "different source commit"):
            allocate_file(self.ledger, "f" * 40, tree)
        other = allocate_file(self.ledger, "c" * 40, "d" * 64)
        self.assertEqual(other["version"], "0.1.1")
        with self.assertRaises(ReleaseError):
            allocate_file(self.ledger, PRESERVED_UNVERSIONED, "e" * 64)
        for current, nxt in (("0.1.99", "0.2.0"), ("0.99.99", "1.0.0")):
            major, minor, patch = (int(part) for part in current.split("."))
            ledger = empty_ledger()
            ledger["allocations"] = [{
                "version": current, "major": major, "minor": minor, "patch": patch,
                "display": current + "_openbeta", "source_commit": "1" * 40,
                "source_tree": "2" * 64, "immutable": True, "models": {},
            }]
            self.ledger.write_text(json.dumps(ledger), encoding="utf-8")
            rolled = allocate_file(self.ledger, "3" * 40, "4" * 64)
            self.assertEqual(rolled["version"], nxt)
        major, minor, patch = 99, 99, 99
        ledger = empty_ledger()
        ledger["allocations"] = [{
            "version": "99.99.99", "major": major, "minor": minor, "patch": patch,
            "display": "99.99.99_openbeta", "source_commit": "1" * 40,
            "source_tree": "2" * 64, "immutable": True, "models": {},
        }]
        self.ledger.write_text(json.dumps(ledger), encoding="utf-8")
        with self.assertRaisesRegex(ReleaseError, "exhausted"):
            allocate_file(self.ledger, "5" * 40, "6" * 64)

    def test_models_share_the_number_and_changed_bytes_cannot_reuse_it(self):
        item = allocate_file(self.ledger, "a" * 40, "b" * 64)
        elite = bind_file(self.ledger, item["version"], "elite", "ab" * 32, "cd" * 32, 128)
        self.assertFalse(elite["immutable"])
        self.assertEqual(elite["models"]["elite"]["product"], "SMK-37 Elite_9000100")
        again = bind_file(self.ledger, item["version"], "elite", "ab" * 32, "cd" * 32, 128)
        self.assertEqual(again["models"]["elite"], elite["models"]["elite"])
        with self.assertRaisesRegex(ReleaseError, "cannot reuse"):
            bind_file(self.ledger, item["version"], "elite", "ef" * 32, "cd" * 32, 128)
        sealed = bind_file(self.ledger, item["version"], "pro", "11" * 32, "22" * 32, 256)
        self.assertTrue(sealed["immutable"])
        self.assertNotEqual(sealed["models"]["elite"]["product"], sealed["models"]["pro"]["product"])
        self.assertEqual(sealed["models"]["elite"]["package_sha256"], "ab" * 32)
        self.assertEqual(sealed["models"]["pro"]["controller_sha256"], "22" * 32)
        with self.assertRaisesRegex(ReleaseError, "cannot reuse"):
            bind_file(self.ledger, item["version"], "pro", "33" * 32, "22" * 32, 256)
        nxt = allocate_file(self.ledger, "d" * 40, "e" * 64)
        self.assertEqual(nxt["version"], "0.1.1")

    def test_file_lock_retries_colliding_allocations(self):
        barrier = threading.Barrier(6)
        found = []
        guard = threading.Lock()

        def work(number):
            barrier.wait()
            item = allocate_file(self.ledger, f"{number:040x}", f"{number:064x}")
            with guard:
                found.append(item["version"])

        threads = [threading.Thread(target=work, args=(i,)) for i in range(6)]
        for thread in threads:
            thread.start()
        for thread in threads:
            thread.join()
        self.assertEqual(sorted(found), ["0.1.0", "0.1.1", "0.1.2", "0.1.3", "0.1.4", "0.1.5"])

    def test_git_push_collision_retries_and_rebuild_reuses(self):
        root = Path(self.tmp.name)
        bare = root / "bare.git"
        subprocess.check_call(["git", "init", "-q", "--bare", "-b", "main", str(bare)], stdout=subprocess.DEVNULL)
        left, right = root / "left", root / "right"
        seed_repo(left)
        seed_repo(right)
        git(left, "remote", "add", "origin", str(bare))
        git(right, "remote", "add", "origin", str(bare))
        queue = Queue()
        first = Process(target=race_worker, args=(str(left), str(bare), "a" * 40, "1" * 64, queue))
        second = Process(target=race_worker, args=(str(right), str(bare), "b" * 40, "2" * 64, queue))
        first.start()
        second.start()
        first.join(30)
        second.join(30)
        self.assertFalse(first.is_alive() or second.is_alive())
        results = sorted(queue.get() for _ in range(2))
        self.assertEqual(results, ["0.1.0", "0.1.1"], results)
        reused = allocate_committed(left, "a" * 40, "1" * 64, remote=str(bare))
        self.assertIn(reused["version"], results)
        self.assertEqual(reused["source_tree"], "1" * 64)


if __name__ == "__main__":
    unittest.main()
