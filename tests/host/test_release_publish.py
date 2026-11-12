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
from firmware_build_labels import inspect_package, version_of_product  # noqa: E402
from fm1pkg_make import ufw  # noqa: E402
from publish_nightly import (LABEL, NightlyPublishError, _embedded_version, _model_entry_ok,  # noqa: E402
                             _version_ok, publish_valid_nightly)
from release_alloc import ReleaseError, bind_file, empty_ledger  # noqa: E402


def git(repo, *args):
    return subprocess.check_output(["git", "-C", str(repo), *args], text=True).strip()


def make_pkg(product):
    return ufw(b"\x00" * 1024, b"FELUCCA-LOADER-1" + b"\x00" * 128, product)


def grant(repo, version, commit=None):
    """Place one allocated number for this repo's real commit and Git tree."""
    commit = commit or git(repo, "rev-parse", "HEAD")
    tree = git(repo, "rev-parse", commit + "^{tree}")
    path = repo / "releases" / "openbeta" / "ledger.json"
    ledger = empty_ledger()
    if path.is_file():
        ledger = json.loads(path.read_text(encoding="utf-8"))
    if not any(item.get("version") == version for item in ledger["allocations"]):
        major, minor, patch = (int(part) for part in version.split("."))
        ledger["allocations"].append({
            "version": version, "major": major, "minor": minor, "patch": patch,
            "display": version + "_openbeta", "source_commit": commit, "source_tree": tree,
            "immutable": False, "models": {},
        })
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(json.dumps(ledger, indent=2) + "\n", encoding="utf-8")
    return path


def bind_models(repo, version, elite, pro):
    path = repo / "releases" / "openbeta" / "ledger.json"
    bind_file(path, version, "elite", dev_validate.digest(elite), "ab" * 32, 128)
    bind_file(path, version, "pro", dev_validate.digest(pro), "cd" * 32, 256)
    return path


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
        grant(self.repo, "0.1.3")
        elite_record = write_record(self.repo, "elite", elite, Path(self.tmp.name) / "elite", "0.1.3")
        pro_record = write_record(self.repo, "pro", pro, Path(self.tmp.name) / "pro", "0.1.3")
        bind_models(self.repo, "0.1.3", elite, pro)
        catalog = self.publish(elite, pro, elite_record, pro_record)
        self.assertEqual(catalog["latest"], "0.1.3")
        entry = catalog["versions"][0]
        self.assertEqual(entry["firmware_version"], "0.1.3")
        self.assertEqual(entry["release_display"], "0.1.3_openbeta")
        self.assertEqual(entry["models"]["elite"]["product"], "SMK-37 Elite_9000103")
        self.assertEqual(entry["models"]["pro"]["product"], "SMK-37 Pro_9000103")
        self.assertEqual(Path(entry["models"]["elite"]["pkg"]).name, "x0x37-0.1.3-elite.fwsc")
        self.assertNotIn("9.9.9", entry["models"]["elite"]["pkg"])
        self.assertNotIn("openbeta", entry["models"]["elite"]["product"])
        self.assertEqual(entry["models"]["elite"]["controller_sha256"], "ab" * 32)
        self.assertEqual(entry["models"]["pro"]["controller_sha256"], "cd" * 32)

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
        grant(self.repo, "0.1.3")
        grant(self.repo, "0.1.4")
        elite_record = write_record(self.repo, "elite", elite, Path(self.tmp.name) / "e", "0.1.3")
        pro_record = write_record(self.repo, "pro", pro, Path(self.tmp.name) / "p", "0.1.3")
        other_record = write_record(self.repo, "pro", other, Path(self.tmp.name) / "o", "0.1.4")
        bind_models(self.repo, "0.1.3", elite, pro)
        bind_file(self.repo / "releases" / "openbeta" / "ledger.json", "0.1.4", "pro",
                  dev_validate.digest(other), "cd" * 32, 256)
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

    def test_unallocated_numeric_catalog_is_refused(self):
        elite = self.package("elite.fwsc", "SMK-37 Elite_9000103")
        with self.assertRaisesRegex(ValueError, "not allocated"):
            write_record(self.repo, "elite", elite, Path(self.tmp.name) / "bare", "0.1.3")
        grant(self.repo, "0.1.3")
        pro = self.package("pro.fwsc", "SMK-37 Pro_9000103")
        elite_record = write_record(self.repo, "elite", elite, Path(self.tmp.name) / "e", "0.1.3")
        pro_record = write_record(self.repo, "pro", pro, Path(self.tmp.name) / "p", "0.1.3")
        ledger = self.repo / "releases" / "openbeta" / "ledger.json"
        ledger.write_text(json.dumps(empty_ledger(), indent=2) + "\n", encoding="utf-8")
        page = Path(self.tmp.name) / "unallocated"
        with self.assertRaisesRegex(NightlyPublishError, "not allocated"):
            publish_valid_nightly(self.repo, elite_record, pro_record, elite, pro, page, push=False)
        self.assertFalse(page.exists())

    def test_same_version_collision_refuses_and_exact_rebuild_republishes(self):
        elite = self.package("elite.fwsc", "SMK-37 Elite_9000103")
        pro = self.package("pro.fwsc", "SMK-37 Pro_9000103")
        grant(self.repo, "0.1.3")
        elite_record = write_record(self.repo, "elite", elite, Path(self.tmp.name) / "e", "0.1.3")
        pro_record = write_record(self.repo, "pro", pro, Path(self.tmp.name) / "p", "0.1.3")
        bind_models(self.repo, "0.1.3", elite, pro)
        original = self.publish(elite, pro, elite_record, pro_record)
        original_sha = original["versions"][0]["models"]["elite"]["sha256"]

        def collide(name, mutate):
            history = Path(self.tmp.name) / name
            history.mkdir()
            changed = json.loads(json.dumps(original))
            mutate(changed["versions"][0])
            (history / "versions.json").write_text(json.dumps(changed), encoding="utf-8")
            page = Path(self.tmp.name) / (name + "-out")
            with self.assertRaisesRegex(NightlyPublishError, "different source or package"):
                publish_valid_nightly(self.repo, elite_record, pro_record, elite, pro, page,
                                      push=False, merge=history)
            self.assertFalse(page.exists())

        collide("hash", lambda entry: entry["models"]["elite"].__setitem__("sha256", "ef" * 32))
        collide("source", lambda entry: entry.__setitem__("source_commit", "ab" * 20))
        collide("controller", lambda entry: entry["models"]["pro"].__setitem__("controller_sha256", "11" * 32))
        with self.assertRaisesRegex(ReleaseError, "cannot reuse"):
            bind_file(self.repo / "releases" / "openbeta" / "ledger.json", "0.1.3", "elite",
                      "ef" * 32, "ab" * 32, 128)
        again_dir = Path(self.tmp.name) / "again"
        publish_valid_nightly(self.repo, elite_record, pro_record, elite, pro, again_dir,
                              push=False, merge=Path(self.tmp.name) / "page")
        again = json.loads((again_dir / "versions.json").read_text(encoding="utf-8"))
        self.assertEqual([item["version"] for item in again["versions"]], ["0.1.3"])
        self.assertEqual(again["versions"][0]["models"]["elite"]["sha256"], original_sha)

    def test_null_catalog_product_is_a_structured_refusal(self):
        try:
            version_of_product(None, "elite")
        except AttributeError:
            self.fail("null product raised AttributeError")
        except ValueError as exc:
            self.assertIn("text", str(exc))
        else:
            self.fail("null product was accepted")
        with self.assertRaises(NightlyPublishError):
            _embedded_version(None, "elite")
        self.assertFalse(_model_entry_ok("elite", {"product": None}))
        self.assertFalse(_version_ok({
            "version": "nightly-nullproduct",
            "label": LABEL,
            "status": "dev-checked",
            "config": "dev",
            "hardware_tested": False,
            "source_commit": "ab" * 20,
            "models": {"elite": {"product": None}, "pro": {"product": None}},
        }))
        history = Path(self.tmp.name) / "history"
        firmware = history / "firmware"
        firmware.mkdir(parents=True)
        kept_elite = firmware / "x0x37-nightly-keptkeptkee-elite.fwsc"
        kept_pro = firmware / "x0x37-nightly-keptkeptkee.fwsc"
        kept_elite.write_bytes(make_pkg("SMK-37 Elite_900"))
        kept_pro.write_bytes(make_pkg("SMK-37 Pro_900"))

        def model(path, product):
            return {
                "product": product,
                "pkg": "../firmware/" + path.name,
                "sha256": dev_validate.digest(path),
                "size": path.stat().st_size,
                "status": "dev-checked",
                "hardware_tested": False,
            }

        kept = {
            "version": "nightly-keptkeptkee",
            "label": LABEL,
            "status": "dev-checked",
            "config": "dev",
            "hardware_tested": False,
            "source_commit": "ab" * 20,
            "models": {
                "elite": model(kept_elite, "SMK-37 Elite_900"),
                "pro": model(kept_pro, "SMK-37 Pro_900"),
            },
        }
        broken = json.loads(json.dumps(kept))
        broken["version"] = "nightly-nullproduct"
        broken["models"]["elite"]["product"] = None
        broken["models"]["pro"]["product"] = None
        (history / "versions.json").write_text(
            json.dumps({"versions": [broken, kept]}), encoding="utf-8")
        elite = self.package("elite.fwsc", "SMK-37 Elite_900")
        pro = self.package("pro.fwsc", "SMK-37 Pro_900")
        page = Path(self.tmp.name) / "null-product"
        publish_valid_nightly(
            self.repo,
            write_record(self.repo, "elite", elite, Path(self.tmp.name) / "null-e"),
            write_record(self.repo, "pro", pro, Path(self.tmp.name) / "null-p"),
            elite, pro, page, push=False, merge=history)
        catalog = json.loads((page / "versions.json").read_text(encoding="utf-8"))
        versions = [item["version"] for item in catalog["versions"]]
        self.assertIn("nightly-keptkeptkee", versions)
        self.assertNotIn("nightly-nullproduct", versions)
        self.assertTrue(any(item.startswith("nightly-") and item != "nightly-keptkeptkee" for item in versions))


if __name__ == "__main__":
    unittest.main()
