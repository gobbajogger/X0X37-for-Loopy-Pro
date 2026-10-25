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
                           allocate_file, bind_committed, bind_file, empty_ledger, product_for)


def git(repo, *args):
    subprocess.check_call(["git", "-C", str(repo), "-c", "user.name=t", "-c", "user.email=t@t", *args],
                          stdout=subprocess.DEVNULL)


def git_out(repo, *args):
    return subprocess.check_output(["git", "-C", str(repo), *args], text=True).strip()


def make_source(path, text):
    """A real commit and the Git tree id of that commit."""
    path.mkdir()
    subprocess.check_call(["git", "init", "-q", "-b", "main", str(path)], stdout=subprocess.DEVNULL)
    (path / "firmware.c").write_text(text)
    git(path, "add", "firmware.c")
    git(path, "commit", "-qm", "source")
    commit = git_out(path, "rev-parse", "HEAD")
    tree = git_out(path, "rev-parse", "HEAD^{tree}")
    return commit, tree


def stamp(path):
    return (git_out(path, "rev-parse", "HEAD"),
            git_out(path, "rev-parse", "--abbrev-ref", "HEAD"),
            subprocess.check_output(["git", "-C", str(path), "status", "--porcelain"], text=True))


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
        commit, tree = "a" * 40, "b" * 40
        first = allocate_file(self.ledger, commit, tree)
        second = allocate_file(self.ledger, commit, tree)
        self.assertEqual(first["version"], "0.1.0")
        self.assertEqual(second["version"], first["version"])
        self.assertEqual(len(json.loads(self.ledger.read_text())["allocations"]), 1)
        with self.assertRaisesRegex(ReleaseError, "different source commit"):
            allocate_file(self.ledger, "f" * 40, tree)
        with self.assertRaisesRegex(ReleaseError, "40-character"):
            allocate_file(self.ledger, commit, "d" * 64)
        other = allocate_file(self.ledger, "c" * 40, "d" * 40)
        self.assertEqual(other["version"], "0.1.1")
        with self.assertRaises(ReleaseError):
            allocate_file(self.ledger, PRESERVED_UNVERSIONED, "e" * 40)
        for current, nxt in (("0.1.99", "0.2.0"), ("0.99.99", "1.0.0")):
            major, minor, patch = (int(part) for part in current.split("."))
            ledger = empty_ledger()
            ledger["allocations"] = [{
                "version": current, "major": major, "minor": minor, "patch": patch,
                "display": current + "_openbeta", "source_commit": "1" * 40,
                "source_tree": "2" * 40, "immutable": True, "models": {},
            }]
            self.ledger.write_text(json.dumps(ledger), encoding="utf-8")
            rolled = allocate_file(self.ledger, "3" * 40, "4" * 40)
            self.assertEqual(rolled["version"], nxt)
        major, minor, patch = 99, 99, 99
        ledger = empty_ledger()
        ledger["allocations"] = [{
            "version": "99.99.99", "major": major, "minor": minor, "patch": patch,
            "display": "99.99.99_openbeta", "source_commit": "1" * 40,
            "source_tree": "2" * 40, "immutable": True, "models": {},
        }]
        self.ledger.write_text(json.dumps(ledger), encoding="utf-8")
        with self.assertRaisesRegex(ReleaseError, "exhausted"):
            allocate_file(self.ledger, "5" * 40, "6" * 40)

    def test_models_share_the_number_and_changed_bytes_cannot_reuse_it(self):
        item = allocate_file(self.ledger, "a" * 40, "b" * 40)
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
        nxt = allocate_file(self.ledger, "d" * 40, "e" * 40)
        self.assertEqual(nxt["version"], "0.1.1")

    def test_file_lock_retries_colliding_allocations(self):
        barrier = threading.Barrier(6)
        found = []
        guard = threading.Lock()

        def work(number):
            barrier.wait()
            item = allocate_file(self.ledger, f"{number:040x}", f"{number:040x}")
            with guard:
                found.append(item["version"])

        threads = [threading.Thread(target=work, args=(i,)) for i in range(6)]
        for thread in threads:
            thread.start()
        for thread in threads:
            thread.join()
        self.assertEqual(sorted(found), ["0.1.0", "0.1.1", "0.1.2", "0.1.3", "0.1.4", "0.1.5"])

    def test_real_git_objects_and_a_later_source_keep_the_checkout(self):
        root = Path(self.tmp.name)
        source = root / "source"
        bare = root / "bare.git"
        subprocess.check_call(["git", "init", "-q", "--bare", "-b", "main", str(bare)], stdout=subprocess.DEVNULL)
        first_commit, first_tree = make_source(source, "int build;\n")
        self.assertEqual(len(first_commit), 40)
        self.assertEqual(len(first_tree), 40)
        self.assertNotEqual(first_commit, first_tree)
        (source / "notes.txt").write_text("dirty\n")
        before = stamp(source)
        tool = [sys.executable, str(ROOT / "tools" / "release_alloc.py"), "--repo", str(source)]
        issued = subprocess.run(tool + ["allocate", "--remote", str(bare), "--branch", "release-records",
                                         "--source-commit", first_commit, "--source-tree", first_tree],
                                capture_output=True, text=True)
        self.assertEqual(issued.returncode, 0, issued.stderr)
        first = json.loads(issued.stdout)
        self.assertEqual(first["version"], "0.1.0")
        self.assertEqual(first["source_tree"], first_tree)
        self.assertEqual(stamp(source), before)
        self.assertEqual((source / "notes.txt").read_text(), "dirty\n")
        files = git_out(bare, "ls-tree", "-r", "--name-only", "release-records").splitlines()
        self.assertEqual(files, ["releases/openbeta/ledger.json"])
        # A record branch that already contains a non-ledger file must not block the next source.
        extra = root / "extra"
        subprocess.check_call(["git", "clone", "-q", "-b", "release-records", str(bare), str(extra)],
                              stdout=subprocess.DEVNULL)
        (extra / "old-source.c").write_text("previous\n")
        git(extra, "add", "old-source.c")
        git(extra, "commit", "-qm", "record keeps an older file")
        git(extra, "push", "-q", "origin", "HEAD:refs/heads/release-records")
        (source / "firmware.c").write_text("int build_two;\n")
        git(source, "add", "firmware.c")
        git(source, "commit", "-qm", "second source")
        second_commit = git_out(source, "rev-parse", "HEAD")
        second_tree = git_out(source, "rev-parse", "HEAD^{tree}")
        self.assertNotEqual(second_tree, first_tree)
        held = stamp(source)
        later = allocate_committed(source, second_commit, second_tree, remote=str(bare))
        self.assertEqual(later["version"], "0.1.1")
        self.assertEqual(later["source_commit"], second_commit)
        self.assertEqual(stamp(source), held)
        git(source, "commit", "-qm", "same tree", "--allow-empty")
        empty_commit = git_out(source, "rev-parse", "HEAD")
        self.assertNotEqual(empty_commit, second_commit)
        self.assertEqual(git_out(source, "rev-parse", "HEAD^{tree}"), second_tree)
        held = stamp(source)
        with self.assertRaisesRegex(ReleaseError, "different source commit"):
            allocate_committed(source, empty_commit, second_tree, remote=str(bare))
        again = allocate_committed(source, second_commit, second_tree, remote=str(bare))
        self.assertEqual(again["version"], "0.1.1")
        with self.assertRaisesRegex(ReleaseError, "not a commit"):
            allocate_committed(source, "ab" * 20, second_tree, remote=str(bare))
        with self.assertRaisesRegex(ReleaseError, "does not match"):
            allocate_committed(source, second_commit, first_tree, remote=str(bare))
        self.assertEqual(stamp(source), held)
        self.assertEqual((source / "notes.txt").read_text(), "dirty\n")
        bound = bind_committed(source, "0.1.1", "elite", "ab" * 32, "cd" * 32, 64, remote=str(bare))
        self.assertEqual(bound["models"]["elite"]["product"], "SMK-37 Elite_9000101")
        self.assertEqual(stamp(source), held)

    def test_cli_refuses_a_fabricated_commit_tree_pair(self):
        source = Path(self.tmp.name) / "source"
        commit, tree = make_source(source, "real\n")
        self.assertEqual(len(tree), 40)
        tool = [sys.executable, str(ROOT / "tools" / "release_alloc.py"), "--repo", str(source), "allocate"]
        refused = subprocess.run(tool + ["--source-commit", "ab" * 20, "--source-tree", "cd" * 20],
                                 capture_output=True, text=True)
        self.assertEqual(refused.returncode, 3, refused.stderr)
        self.assertIn("not a commit", refused.stderr)
        mismatch = subprocess.run(tool + ["--source-commit", commit, "--source-tree", "cd" * 20],
                                  capture_output=True, text=True)
        self.assertEqual(mismatch.returncode, 3, mismatch.stderr)
        self.assertIn("does not match", mismatch.stderr)
        digest = subprocess.run(tool + ["--source-commit", commit, "--source-tree", "d" * 64],
                                capture_output=True, text=True)
        self.assertEqual(digest.returncode, 3, digest.stderr)
        self.assertIn("40-character", digest.stderr)
        self.assertFalse((source / "releases" / "openbeta" / "ledger.json").exists())
        self.assertEqual((source / "firmware.c").read_text(), "real\n")
        self.assertEqual(git_out(source, "rev-parse", "HEAD"), commit)

    def test_symbolic_origin_resolves_from_the_source_repo(self):
        root = Path(self.tmp.name)
        bare = (root / "bare.git").resolve()
        source = root / "source"
        elsewhere = root / "elsewhere"
        elsewhere.mkdir()
        subprocess.check_call(["git", "init", "-q", "--bare", "-b", "main", str(bare)], stdout=subprocess.DEVNULL)
        commit, tree = make_source(source, "int build;\n")
        git(source, "remote", "add", "origin", str(bare))
        (source / "notes.txt").write_text("dirty\n")
        tool = [sys.executable, str(ROOT / "tools" / "release_alloc.py")]
        missing = subprocess.run(
            tool + ["--repo", str(source), "allocate", "--remote", "missing",
                    "--source-commit", commit, "--source-tree", tree],
            cwd=elsewhere, capture_output=True, text=True)
        self.assertEqual(missing.returncode, 3, missing.stderr)
        self.assertIn("not configured", missing.stderr)
        self.assertFalse((source / "releases" / "openbeta" / "ledger.json").exists())
        self.assertNotIn("release-records", git_out(bare, "branch", "--list"))
        before = stamp(source)
        issued = allocate_committed(source, commit, tree, remote="origin", retries=1)
        self.assertEqual(issued["version"], "0.1.0")
        self.assertEqual(issued["source_tree"], tree)
        self.assertEqual(stamp(source), before)
        self.assertEqual((source / "notes.txt").read_text(), "dirty\n")
        self.assertFalse((source / "releases" / "openbeta" / "ledger.json").exists())
        files = git_out(bare, "ls-tree", "-r", "--name-only", "release-records").splitlines()
        self.assertEqual(files, ["releases/openbeta/ledger.json"])
        (source / "firmware.c").write_text("int build_two;\n")
        git(source, "add", "firmware.c")
        git(source, "commit", "-qm", "second source")
        second_commit = git_out(source, "rev-parse", "HEAD")
        second_tree = git_out(source, "rev-parse", "HEAD^{tree}")
        held = stamp(source)
        from_cwd = subprocess.run(
            tool + ["allocate", "--remote", "origin", "--branch", "release-records",
                    "--source-commit", second_commit, "--source-tree", second_tree],
            cwd=source, capture_output=True, text=True)
        self.assertEqual(from_cwd.returncode, 0, from_cwd.stderr)
        self.assertEqual(json.loads(from_cwd.stdout)["version"], "0.1.1")
        self.assertEqual(stamp(source), held)
        bound = subprocess.run(
            tool + ["--repo", str(source), "bind", "--remote", "origin", "--branch", "release-records",
                    "--version", "0.1.1", "--model", "elite",
                    "--package-sha256", "ab" * 32, "--controller-sha256", "cd" * 32,
                    "--controller-size", "64"],
            cwd=elsewhere, capture_output=True, text=True)
        self.assertEqual(bound.returncode, 0, bound.stderr)
        self.assertEqual(json.loads(bound.stdout)["models"]["elite"]["product"], "SMK-37 Elite_9000101")
        self.assertEqual(stamp(source), held)
        rel_bare = (root / "rel-bare.git").resolve()
        rel = root / "rel"
        subprocess.check_call(["git", "init", "-q", "--bare", "-b", "main", str(rel_bare)], stdout=subprocess.DEVNULL)
        rel_commit, rel_tree = make_source(rel, "relative\n")
        git(rel, "remote", "add", "origin", "../rel-bare.git")
        relative = allocate_committed(rel, rel_commit, rel_tree, remote="origin", retries=1)
        self.assertEqual(relative["version"], "0.1.0")
        (rel / "firmware.c").write_text("relative-two\n")
        git(rel, "add", "firmware.c")
        git(rel, "commit", "-qm", "second relative")
        later = subprocess.run(
            tool + ["--repo", str(rel), "allocate", "--remote", "origin",
                    "--source-commit", git_out(rel, "rev-parse", "HEAD"),
                    "--source-tree", git_out(rel, "rev-parse", "HEAD^{tree}")],
            cwd=elsewhere, capture_output=True, text=True)
        self.assertEqual(later.returncode, 0, later.stderr)
        self.assertEqual(json.loads(later.stdout)["version"], "0.1.1")
        self.assertFalse((rel / "releases" / "openbeta" / "ledger.json").exists())

    def test_git_push_collision_retries_and_rebuild_reuses(self):
        root = Path(self.tmp.name)
        bare = root / "bare.git"
        subprocess.check_call(["git", "init", "-q", "--bare", "-b", "main", str(bare)], stdout=subprocess.DEVNULL)
        left, right = root / "left", root / "right"
        left_commit, left_tree = make_source(left, "left\n")
        right_commit, right_tree = make_source(right, "right\n")
        self.assertNotEqual(left_tree, right_tree)
        queue = Queue()
        first = Process(target=race_worker, args=(str(left), str(bare), left_commit, left_tree, queue))
        second = Process(target=race_worker, args=(str(right), str(bare), right_commit, right_tree, queue))
        first.start()
        second.start()
        first.join(30)
        second.join(30)
        self.assertFalse(first.is_alive() or second.is_alive())
        results = sorted(queue.get() for _ in range(2))
        self.assertEqual(results, ["0.1.0", "0.1.1"], results)
        reused = allocate_committed(left, left_commit, left_tree, remote=str(bare))
        self.assertEqual(reused["source_commit"], left_commit)
        self.assertEqual(reused["source_tree"], left_tree)
        self.assertIn(reused["version"], results)


if __name__ == "__main__":
    unittest.main()
