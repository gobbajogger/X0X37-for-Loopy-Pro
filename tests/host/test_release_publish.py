#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Catalog identity comes from package bytes, not from a file name."""

import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
import dev_validate  # noqa: E402
import validation_inputs  # noqa: E402
from firmware_build_labels import inspect_package  # noqa: E402
from fm1pkg_make import ufw  # noqa: E402
from publish_nightly import NightlyPublishError, publish_valid_nightly  # noqa: E402


def git(repo, *args):
    return subprocess.check_output(["git", "-C", str(repo), *args], text=True).strip()


def make_pkg(product):
    return ufw(b"\x00" * 1024, b"FELUCCA-LOADER-1" + b"\x00" * 128, product)


def write_record(repo, model, fwsc, directory, version=None):
    directory.mkdir()
    commit = git(repo, "rev-parse", "HEAD")
    tree = dev_validate.tree(repo, commit)
    plan = {"base": commit, "scope": "fixture behavior",
            "checks": [{"reason": "changed fixture", "argv": [sys.executable, "test.py"]}]}
    build_log = "app code area 100 B, 40 B free\nRAM .data+.bss 40 B of 100; pool 100 B of 10000\n"
    (directory / "build.log").write_text(build_log)
    (directory / "check-0.log").write_text("affected test passed\n")
    inputs = {
        "schema": validation_inputs.SCHEMA,
        "mode": "dev",
        "source": tree,
        "configuration": {
            "model": model, "config": "dev", "controller": False,
            "plan": plan, "dependencies": [],
        },
    }
    data = {
        "schema": 2,
        "result": "DEV CHECKED",
        "config": "dev",
        "model": model,
        "commit": commit,
        "tree": tree,
        "plan": plan,
        "controller": False,
        "budgets": dev_validate.budgets(build_log),
        "logs": {
            "build": {"returncode": 0, "sha256": dev_validate.digest(directory / "build.log")},
            "check-0": {"returncode": 0, "sha256": dev_validate.digest(directory / "check-0.log")},
        },
        "artifacts": {"package": {"file": Path(fwsc).name, "sha256": dev_validate.digest(fwsc)}},
    }
    if version is not None:
        firmware = inspect_package(fwsc, model, commit, version)
        inputs["configuration"]["firmware_version"] = version
        data["firmware_version"] = firmware["firmware_version"]
        data["firmware"] = firmware
    data["validation_inputs"] = {
        "schema": validation_inputs.SCHEMA,
        "sha256": validation_inputs.digest(validation_inputs.encoded(inputs)),
        "inputs": inputs,
    }
    record = directory / "record.json"
    record.write_text(json.dumps(data), encoding="utf-8")
    checked = dev_validate.verify(repo, commit, record, fwsc)
    if checked["model"] != model:
        raise AssertionError("fixture record did not verify")
    return record


class ReleasePublishTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        root = Path(self.tmp.name)
        self.repo = root / "repo"
        self.repo.mkdir()
        subprocess.check_call(["git", "init", "-q", "-b", "main"], cwd=self.repo)
        (self.repo / "notes.txt").write_text("fixture\n", encoding="utf-8")
        subprocess.check_call(["git", "-C", str(self.repo), "add", "."])
        subprocess.check_call(["git", "-C", str(self.repo), "-c", "user.name=t", "-c", "user.email=t@t",
                               "commit", "-qm", "fixture"])

    def package(self, name, product):
        path = Path(self.tmp.name) / name
        path.write_bytes(make_pkg(product))
        return path

    def publish(self, elite, pro, elite_record, pro_record):
        page = Path(self.tmp.name) / "page"
        publish_valid_nightly(self.repo, elite_record, pro_record, elite, pro, page, push=False)
        return json.loads((page / "versions.json").read_text(encoding="utf-8"))

    def test_shared_numeric_version_ignores_misleading_names(self):
        elite = self.package("x0x37-9.9.9_openbeta-elite.fwsc", "SMK-37 Elite_9000103")
        pro = self.package("renamed-pro.fwsc", "SMK-37 Pro_9000103")
        catalog = self.publish(
            elite, pro,
            write_record(self.repo, "elite", elite, Path(self.tmp.name) / "elite", "0.1.3"),
            write_record(self.repo, "pro", pro, Path(self.tmp.name) / "pro", "0.1.3"))
        self.assertEqual(catalog["latest"], "0.1.3")
        entry = catalog["versions"][0]
        self.assertEqual(entry["firmware_version"], "0.1.3")
        self.assertEqual(entry["release_display"], "0.1.3_openbeta")
        self.assertEqual(entry["models"]["elite"]["product"], "SMK-37 Elite_9000103")
        self.assertEqual(entry["models"]["pro"]["product"], "SMK-37 Pro_9000103")
        self.assertEqual(Path(entry["models"]["elite"]["pkg"]).name, "x0x37-0.1.3-elite.fwsc")
        self.assertNotIn("9.9.9", entry["models"]["elite"]["pkg"])
        self.assertNotIn("openbeta", entry["models"]["elite"]["product"])

    def test_legacy_name_does_not_invent_a_version(self):
        elite = self.package("0.1.3_openbeta.fwsc", "SMK-37 Elite_900")
        pro = self.package("0.1.3_openbeta-pro.fwsc", "SMK-37 Pro_900")
        catalog = self.publish(
            elite, pro,
            write_record(self.repo, "elite", elite, Path(self.tmp.name) / "elite-old"),
            write_record(self.repo, "pro", pro, Path(self.tmp.name) / "pro-old"))
        entry = catalog["versions"][0]
        self.assertTrue(entry["version"].startswith("nightly-"))
        self.assertNotIn("firmware_version", entry)
        self.assertNotIn("release_display", entry)
        self.assertEqual(entry["models"]["elite"]["product"], "SMK-37 Elite_900")
        self.assertEqual(entry["models"]["pro"]["product"], "SMK-37 Pro_900")

    def test_wrong_model_version_and_hash_are_refused(self):
        elite = self.package("elite.fwsc", "SMK-37 Elite_9000103")
        pro = self.package("pro.fwsc", "SMK-37 Pro_9000103")
        other = self.package("other.fwsc", "SMK-37 Pro_9000104")
        elite_record = write_record(self.repo, "elite", elite, Path(self.tmp.name) / "e", "0.1.3")
        pro_record = write_record(self.repo, "pro", pro, Path(self.tmp.name) / "p", "0.1.3")
        other_record = write_record(self.repo, "pro", other, Path(self.tmp.name) / "o", "0.1.4")
        page = Path(self.tmp.name) / "refused"
        with self.assertRaises(NightlyPublishError):
            publish_valid_nightly(self.repo, elite_record, other_record, elite, other, page, push=False)
        self.assertFalse(page.exists())
        swapped = Path(self.tmp.name) / "swapped"
        with self.assertRaises(NightlyPublishError):
            publish_valid_nightly(self.repo, elite_record, pro_record, pro, elite, swapped, push=False)
        self.assertFalse(swapped.exists())
        tampered = Path(self.tmp.name) / "tampered.fwsc"
        raw = bytearray(elite.read_bytes())
        raw[-1] ^= 1
        tampered.write_bytes(raw)
        hashed = Path(self.tmp.name) / "hashed"
        with self.assertRaises(NightlyPublishError):
            publish_valid_nightly(self.repo, elite_record, pro_record, tampered, pro, hashed, push=False)
        self.assertFalse(hashed.exists())


if __name__ == "__main__":
    unittest.main()
