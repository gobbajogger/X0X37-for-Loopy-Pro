#!/usr/bin/env python3
"""Host behavior tests and bounded fixture I/O comparison; no network or firmware."""
import copy
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("checkpoint", ROOT / "tools/workflow_checkpoint.py")
cp = importlib.util.module_from_spec(spec)
spec.loader.exec_module(cp)


def fixture():
    output = {"closure": {"source_abi_sdk_toolchain_scripts_config_flags": "a" * 64},
              "policy": {"build": "b" * 64}, "sha256": "c" * 64,
              "accepted_evidence": {"status": "ACCEPTED", "identity": {"receipt": "d" * 64}}}
    return {"schema": 1, "role": "builder", "workflow_revision": "git:contract-v1",
            "input_identity": {"source_tree": "e" * 64},
            "last_accepted_stage": "BUILD_ONLY", "evidence_identity": {"ledger": "f" * 64},
            "observed_remote_heads": {"nightly": "1" * 40}, "read_status": "success",
            "pending_work": [], "retry_conditions": [],
            "outputs": {"app": copy.deepcopy(output), "controller": copy.deepcopy(output)}}


class CheckpointTests(unittest.TestCase):
    def setUp(self):
        self.old = fixture()
        self.now = copy.deepcopy(self.old)

    def test_unchanged_exit(self):
        self.assertEqual(cp.decide(self.old, self.now)["action"], "NO_CHANGE")

    def test_changed_input(self):
        self.now["input_identity"]["source_tree"] = "2" * 64
        self.assertEqual(cp.decide(self.old, self.now)["action"], "WORK_REQUIRED")

    def test_missing_checkpoint_and_failed_reads(self):
        self.assertEqual(cp.decide(None, self.now)["reuse"], [])
        for status in ("failed", "stale", "incomplete", "unknown"):
            self.now["read_status"] = status
            self.assertEqual(cp.decide(self.old, self.now)["reason"], "invalid_or_failed_observation")

    def test_pending_same_sha(self):
        for field in ("pending_work", "retry_conditions"):
            for condition in ("review", "artifact", "deploy", "hardware", "retry"):
                self.now = copy.deepcopy(self.old)
                self.now[field] = [condition]
                self.assertEqual(cp.decide(self.old, self.now)["action"], "WORK_REQUIRED")

    def test_per_output_reuse(self):
        self.now["outputs"]["app"]["closure"]["source_abi_sdk_toolchain_scripts_config_flags"] = "2" * 64
        result = cp.decide(self.old, self.now)
        self.assertEqual(result["reuse"], ["controller"])
        self.assertEqual(result["affected"], ["app"])

    def test_policy_only_invalidates_relevant_stage(self):
        self.old["outputs"]["package"] = copy.deepcopy(self.old["outputs"]["app"])
        self.now = copy.deepcopy(self.old)
        self.now["workflow_revision"] = "git:contract-v2"
        self.now["outputs"]["package"]["policy"]["package"] = "2" * 64
        result = cp.decide(self.old, self.now)
        self.assertEqual(result["reuse"], ["app", "controller"])
        self.assertEqual(result["affected"], ["package"])

    def test_malformed_and_changed_proof_fail_closed(self):
        for field in ("closure", "policy", "accepted_evidence"):
            self.now = copy.deepcopy(self.old)
            self.now["outputs"]["app"][field] = {}
            self.assertEqual(cp.decide(self.old, self.now)["reuse"], [])
        self.now = copy.deepcopy(self.old)
        self.now["outputs"]["app"]["accepted_evidence"]["identity"]["receipt"] = "2" * 64
        self.assertEqual(cp.decide(self.old, self.now)["affected"], ["app"])

    def test_remote_and_workflow_revision_change(self):
        for field, value in (("workflow_revision", "v2"), ("observed_remote_heads", {"nightly": "2" * 40})):
            self.now = copy.deepcopy(self.old)
            self.now[field] = value
            self.assertEqual(cp.decide(self.old, self.now)["action"], "WORK_REQUIRED")

    def test_receipt_cannot_overwrite_decision_inputs(self):
        with tempfile.TemporaryDirectory() as folder:
            folder = Path(folder)
            before, now = folder / "checkpoint.json", folder / "observation.json"
            before.write_text(json.dumps(self.old))
            now.write_text(json.dumps(self.now))
            symlink, hardlink = folder / "symbolic.json", folder / "hard.json"
            symlink.symlink_to(before)
            os.link(now, hardlink)
            for target in (before, now, symlink, hardlink):
                result = subprocess.run([sys.executable, str(ROOT / "tools/workflow_checkpoint.py"), str(now), "--checkpoint", str(before), "--receipt", str(target)], capture_output=True, text=True)
                self.assertEqual(result.returncode, 2)
                self.assertIn("must not alias", result.stderr)
                self.assertEqual(result.stdout, "")
                self.assertEqual(json.loads(before.read_text()), self.old)
                self.assertEqual(json.loads(now.read_text()), self.now)

    def test_cli_receipt_and_bounded_fixture_io(self):
        with tempfile.TemporaryDirectory() as folder:
            folder = Path(folder)
            before, now, receipt = (folder / name for name in ("checkpoint.json", "observation.json", "receipt.json"))
            before.write_text(json.dumps(self.old))
            now.write_text(json.dumps(self.now))
            proofs = [folder / (str(i) + ".proof") for i in range(20)]
            for proof in proofs:
                proof.write_text("immutable authenticated fixture evidence\n")
            baseline_reads = sum(1 for path in [before, now] + proofs if path.read_bytes())
            result = subprocess.run([sys.executable, str(ROOT / "tools/workflow_checkpoint.py"), str(now), "--checkpoint", str(before), "--receipt", str(receipt)], capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            data = json.loads(result.stdout)
            self.assertEqual(data["checkpoint_before"], data["checkpoint_after"])
            self.assertIsNone(data["tokens"])
            self.assertIsNone(data["time_to_owner_installable_seconds"])
            self.assertEqual(data["produced_verified_outputs"], [])
            self.assertEqual(json.loads(receipt.read_text()), data)
            actual_open = open
            opened = []
            def measured_open(path, *args, **kwargs):
                opened.append(Path(path))
                return actual_open(path, *args, **kwargs)
            with patch("builtins.open", side_effect=measured_open):
                measured = cp.decide(cp.load(before), cp.load(now))
            self.assertEqual(measured["action"], "NO_CHANGE")
            self.assertEqual(opened, [before, now])
            self.assertFalse(set(opened) & set(proofs))
            incremental_reads = len(opened)
            print(json.dumps({"fixture_baseline_file_reads": baseline_reads,
                              "fixture_incremental_file_reads": incremental_reads,
                              "immutable_fixture_proof_reads_avoided": len(proofs),
                              "production_token_savings": None}, sort_keys=True))
            now.write_text("{")
            failed = subprocess.run([sys.executable, str(ROOT / "tools/workflow_checkpoint.py"), str(now), "--checkpoint", str(before)], capture_output=True, text=True)
            self.assertEqual(failed.returncode, 10)
            self.assertEqual(json.loads(failed.stdout)["action"], "WORK_REQUIRED")


if __name__ == "__main__":
    unittest.main()
