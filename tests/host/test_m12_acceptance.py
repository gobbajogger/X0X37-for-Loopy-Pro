#!/usr/bin/env python3
"""Real-Git, synthetic-policy regressions for the standalone M12 adapter."""
import copy
import hashlib
import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

TOOL = Path(__file__).resolve().parents[2] / "tools" / "m12_acceptance.py"
SOURCE_SECTION = "Owner request list (#1–#122)"


class Fixture:
    def __init__(self, root):
        self.root = Path(root)
        self.repo = self.root / "repo"
        subprocess.run(["git", "init", "-q", str(self.repo)], check=True)
        self.source = b"feature_enabled = True\n"
        self.files = {"firmware/feature.c": self.source,
                      "tests/check.py": b"assert feature_enabled\n",
                      "config.json": b'{"enabled": true}\n'}
        self.candidate, self.tree = self.commit(self.files)
        self.documents = {
            "docs/ROADMAP.md": b"# ROADMAP\n[Complete](roadmap/Complete.md)\n[Working](roadmap/Working.md)\n[Pending](roadmap/Pending.md)\n",
            "docs/roadmap/Complete.md": self.record("#1").encode(),
            "docs/roadmap/Working.md": self.record("#2").encode(),
            "docs/roadmap/Pending.md": (self.record("#119", "DEFERRED") + self.record("26.7", "DECLINED") + self.record("M5", "UNDEFINED")).encode(),
        }
        self.roadmap, _ = self.commit(self.documents)
        self.roadmap_blob = self.blob(self.documents["docs/ROADMAP.md"])
        self.policy = {
            "schema": "dnbox-m12-policy-v1", "stage": "source-only-fixture",
            "queues": [{"path": p, "blob": self.blob(b)} for p, b in self.documents.items() if p != "docs/ROADMAP.md"],
            "selected": [self.selection("#1"), self.selection("#2")],
        }
        self.tool_hash = hashlib.sha256(TOOL.read_bytes()).hexdigest() if TOOL.exists() else "0" * 64
        self.evidence = {}
        self.bind_policy()

    def git(self, *args, data=None):
        return subprocess.run(["git", "-C", str(self.repo), *args], input=data,
                              stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True).stdout

    def blob(self, data):
        return self.git("hash-object", "-w", "--stdin", data=data).decode().strip()

    def commit(self, files, modes=None):
        nested = {}
        for path, data in files.items():
            parts = path.split("/")
            node = nested
            for part in parts[:-1]:
                node = node.setdefault(part, {})
            node[parts[-1]] = ((modes or {}).get(path, "100644"), self.blob(data))

        def tree(node):
            entries = []
            for name, value in sorted(node.items()):
                if isinstance(value, dict):
                    entries.append(f"040000 tree {tree(value)}\t{name}\n")
                else:
                    entries.append(f"{value[0]} blob {value[1]}\t{name}\n")
            return self.git("mktree", data="".join(entries).encode()).decode().strip()

        root = tree(nested)
        # Disposable fixture identity is raw object data, not canonical Git config.
        raw = (f"tree {root}\nauthor M12 fixture <fixture@example.invalid> 1 +0000\n"
               "committer M12 fixture <fixture@example.invalid> 1 +0000\n\nfixture\n").encode()
        commit = self.git("hash-object", "-t", "commit", "-w", "--stdin", data=raw).decode().strip()
        return commit, root

    @staticmethod
    def record(request, disposition="Source-stage fixture only"):
        return (f"## {request}\n\n**Current disposition:** {disposition}\n\n"
                f"**Preserved source section:** {SOURCE_SECTION}\n\n"
                "| # | Item |\n|---|---|\n" + f"| {request} | Synthetic fixture, not product qualification. |\n\n")

    @staticmethod
    def selection(request, level="source"):
        return {"request_id": request, "source_section": SOURCE_SECTION, "row": request,
                "obligations": [{"id": "required", "level": level, "paths": ["firmware/feature.c"]}]}

    def proof(self, paths):
        return [{"path": p, "blob": self.blob(self.files[p]),
                 "sha256": hashlib.sha256(self.files[p]).hexdigest()} for p in paths]

    def bind_policy(self):
        raw = (json.dumps(self.policy, sort_keys=True) + "\n").encode()
        self.policy_commit, _ = self.commit({"policy.json": raw})
        self.policy_blob = self.blob(raw)
        self.evidence = {
            "schema": "dnbox-m12-evidence-v1", "candidate_commit": self.candidate,
            "candidate_tree": self.tree, "roadmap_commit": self.roadmap,
            "roadmap_blob": self.roadmap_blob, "policy_commit": self.policy_commit,
            "policy_blob": self.policy_blob, "tool_sha256": self.tool_hash,
            "rows": [{"request_id": s["request_id"], "obligation_id": o["id"],
                      "status": "PASS", "proof": self.proof(o["paths"])}
                     for s in self.policy["selected"] for o in s["obligations"]],
        }

    def receipt(self, level="host", status="PASS"):
        s = self.policy["selected"][0]
        o = s["obligations"][0]
        o["level"] = level
        o["paths"] = list(self.files)
        receipt = {"schema": "dnbox-m12-receipt-v1", "stage": self.policy["stage"],
                   "request_id": s["request_id"], "obligation_id": o["id"], "level": level,
                   "candidate_commit": self.candidate, "candidate_tree": self.tree,
                   "roadmap_commit": self.roadmap, "roadmap_blob": self.roadmap_blob,
                   "status": status, "command": "fixture-only: no product test claim",
                   "exit_code": 0 if status == "PASS" else (1 if status == "FAIL" else None),
                   "inputs": self.proof(o["paths"])}
        raw = (json.dumps(receipt, sort_keys=True) + "\n").encode()
        commit, _ = self.commit({"receipt.json": raw})
        pointer = {"commit": commit, "path": "receipt.json", "blob": self.blob(raw),
                   "sha256": hashlib.sha256(raw).hexdigest()}
        o["receipt"] = pointer
        self.bind_policy()
        self.evidence["rows"][0].pop("proof")
        self.evidence["rows"][0]["receipt"] = copy.deepcopy(pointer)
        self.evidence["rows"][0]["status"] = status
        return receipt, pointer

    def run(self, changes=None, raw_evidence=None):
        path = self.root / "evidence.json"
        path.write_text(raw_evidence if raw_evidence is not None else json.dumps(self.evidence))
        flags = {"repo": str(self.repo), "candidate": self.candidate, "candidate-tree": self.tree,
                 "roadmap-commit": self.roadmap, "roadmap-path": "docs/ROADMAP.md",
                 "roadmap-blob": self.roadmap_blob, "policy-commit": self.policy_commit,
                 "policy-path": "policy.json", "policy-blob": self.policy_blob,
                 "tool-sha256": self.tool_hash, "evidence": str(path)}
        flags.update(changes or {})
        cmd = [sys.executable, str(TOOL)]
        for key, value in flags.items():
            cmd += ["--" + key, value]
        result = subprocess.run(cmd, capture_output=True, text=True)
        try:
            output = json.loads(result.stdout)
        except ValueError:
            raise AssertionError(f"Adapter must emit structured JSON, got {result.returncode}: {result.stderr}")
        return result.returncode, output


class AcceptanceTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.f = Fixture(self.temp.name)

    def accept(self):
        code, report = self.f.run()
        self.assertEqual(code, 0, report)
        self.assertTrue(report["accepted"], report)
        return report

    def reject(self, changes=None, raw_evidence=None):
        code, report = self.f.run(changes, raw_evidence)
        self.assertEqual(code, 1, report)
        self.assertFalse(report["accepted"], report)
        return report

    def test_valid_independent_source_inputs_are_accepted(self):
        report = self.accept()
        self.assertEqual([r["status"] for r in report["results"]], ["PASS", "PASS"])
        self.assertEqual({r["request_id"] for r in report["unselected"]}, {"#119", "26.7", "M5"})

    def test_inventory_shrink_cannot_remove_required_obligation(self):
        self.f.evidence["rows"].pop()
        (self.f.repo / "REQUIRED_ROWS.txt").write_text("#1\n")
        report = self.reject()
        self.assertEqual(report["results"][1]["status"], "NOT_RUN")

    def test_changed_source_hash_under_same_claimed_candidate_is_rejected(self):
        self.f.evidence["rows"][0]["proof"][0]["sha256"] = hashlib.sha256(b"removed\n").hexdigest()
        self.reject()

    def test_self_consistent_substituted_roadmap_pins_are_rejected(self):
        self.f.evidence["roadmap_commit"] = "1" * 40
        self.f.evidence["roadmap_blob"] = "2" * 40
        self.reject()

    def test_worktree_bytes_cannot_replace_git_source(self):
        path = self.f.repo / "firmware/feature.c"
        path.parent.mkdir()
        path.write_bytes(b"different mutable bytes\n")
        self.accept()  # Source-only result explicitly names immutable Git objects.

    def test_candidate_tree_mismatch_is_rejected(self):
        self.reject({"candidate-tree": "1" * 40})

    def test_unavailable_candidate_object_is_rejected(self):
        self.reject({"candidate": "1" * 40})

    def test_wrong_policy_blob_is_rejected(self):
        self.reject({"policy-blob": self.f.roadmap_blob})

    def test_omitted_selected_policy_cannot_pass(self):
        self.f.policy["selected"] = []
        self.f.bind_policy()
        self.reject()

    def test_changed_policy_report_pin_is_rejected(self):
        self.f.evidence["policy_blob"] = "1" * 40
        self.reject()

    def test_missing_policy_object_is_rejected(self):
        self.reject({"policy-commit": "1" * 40})

    def test_stale_report_tool_is_rejected(self):
        self.f.evidence["tool_sha256"] = "0" * 64
        self.reject()

    def test_wrong_externally_expected_tool_is_rejected(self):
        self.reject({"tool-sha256": "0" * 64})

    def test_duplicate_json_key_is_rejected(self):
        raw = json.dumps(self.f.evidence).replace('"rows":', '"schema":"dnbox-m12-evidence-v1","rows":')
        self.reject(raw_evidence=raw)

    def test_duplicate_report_obligation_is_rejected(self):
        self.f.evidence["rows"].append(copy.deepcopy(self.f.evidence["rows"][0]))
        self.reject()

    def test_unknown_report_obligation_is_rejected(self):
        self.f.evidence["rows"][0]["obligation_id"] = "invented"
        self.reject()

    def test_duplicate_policy_obligation_is_rejected(self):
        self.f.policy["selected"][0]["obligations"] *= 2
        self.f.bind_policy()
        self.reject()

    def test_explicit_fail_is_not_promoted_to_pass(self):
        self.f.evidence["rows"][0]["status"] = "FAIL"
        report = self.reject()
        self.assertEqual(report["results"][0]["status"], "FAIL")

    def test_explicit_not_run_is_preserved(self):
        self.f.evidence["rows"][0]["status"] = "NOT_RUN"
        report = self.reject()
        self.assertEqual(report["results"][0]["status"], "NOT_RUN")

    def test_source_presence_cannot_satisfy_host_native_or_device(self):
        for level in ("behavior", "host", "native", "device"):
            with self.subTest(level=level):
                self.f.policy["selected"][0]["obligations"][0]["level"] = level
                self.f.bind_policy()
                report = self.reject()
                self.assertEqual(report["results"][0]["status"], "NOT_RUN")

    def test_independently_pinned_receipt_and_closure_can_be_reused(self):
        self.f.receipt()
        report = self.accept()
        self.assertEqual(report["results"][0]["proof_kind"], "accepted-receipt")

    def test_forged_receipt_pointer_cannot_replace_accepted_receipt(self):
        self.f.receipt()
        self.f.evidence["rows"][0]["receipt"]["sha256"] = "0" * 64
        self.reject()

    def test_receipt_input_closure_change_is_rejected(self):
        receipt, _ = self.f.receipt()
        receipt["inputs"].pop()
        raw = json.dumps(receipt).encode()
        c, _ = self.f.commit({"receipt.json": raw})
        pointer = {"commit": c, "path": "receipt.json", "blob": self.f.blob(raw), "sha256": hashlib.sha256(raw).hexdigest()}
        self.f.policy["selected"][0]["obligations"][0]["receipt"] = pointer
        self.f.bind_policy()
        self.f.evidence["rows"][0]["receipt"] = pointer
        self.f.evidence["rows"][0].pop("proof")
        self.reject()

    def test_receipt_fail_is_not_waived(self):
        self.f.receipt(status="FAIL")
        report = self.reject()
        self.assertEqual(report["results"][0]["status"], "FAIL")

    def test_receipt_wrong_stage_is_rejected(self):
        receipt, pointer = self.f.receipt()
        receipt["stage"] = "different-stage"
        raw = json.dumps(receipt).encode()
        c, _ = self.f.commit({"receipt.json": raw})
        pointer.update(commit=c, blob=self.f.blob(raw), sha256=hashlib.sha256(raw).hexdigest())
        self.f.bind_policy()
        self.f.evidence["rows"][0].update(receipt=copy.deepcopy(pointer))
        self.f.evidence["rows"][0].pop("proof")
        self.reject()

    def test_held_owner_requests_cannot_be_selected(self):
        for request in ("#119", "26.7", "M5"):
            with self.subTest(request=request):
                self.f.policy["selected"] = [self.f.selection(request)]
                self.f.bind_policy()
                self.reject()

    def test_original_source_table_identity_is_required(self):
        self.f.policy["selected"][0]["source_section"] = "nightly PR #119"
        self.f.bind_policy()
        self.reject()

    def test_unknown_selected_request_is_not_silently_excluded(self):
        self.f.policy["selected"][0]["request_id"] = "#8888"
        self.f.bind_policy()
        self.reject()

    def test_omitted_canonical_queue_is_rejected(self):
        self.f.policy["queues"].pop()
        self.f.bind_policy()
        self.reject()

    def test_duplicate_canonical_request_is_rejected(self):
        docs = dict(self.f.documents)
        docs["docs/roadmap/Working.md"] += self.f.record("#1").encode()
        self.f.roadmap, _ = self.f.commit(docs)
        self.f.policy["queues"] = [{"path": p, "blob": self.f.blob(b)} for p, b in docs.items() if p != "docs/ROADMAP.md"]
        self.f.bind_policy()
        self.reject()

    def test_path_traversal_is_rejected(self):
        self.f.policy["selected"][0]["obligations"][0]["paths"] = ["../feature.c"]
        # Preserve an otherwise valid report; the accepted policy itself is invalid.
        raw = json.dumps(self.f.policy).encode()
        self.f.policy_commit, _ = self.f.commit({"policy.json": raw})
        self.f.policy_blob = self.f.blob(raw)
        self.f.evidence.update(policy_commit=self.f.policy_commit, policy_blob=self.f.policy_blob)
        self.reject()

    def test_symlink_is_not_accepted_as_source(self):
        self.f.candidate, self.f.tree = self.f.commit(self.f.files, {"firmware/feature.c": "120000"})
        self.f.bind_policy()
        self.reject()

    def test_git_replace_objects_are_not_trusted(self):
        other, _ = self.f.commit({"different.txt": b"replacement"})
        self.f.git("replace", self.f.candidate, other)
        self.accept()

    def test_malformed_status_is_a_structured_rejection(self):
        self.f.evidence["rows"][0]["status"] = ["PASS"]
        self.reject()

    def test_malformed_level_is_a_structured_rejection(self):
        self.f.policy["selected"][0]["obligations"][0]["level"] = ["source"]
        self.f.bind_policy()
        self.reject()

    def test_corrupted_git_object_is_rejected_by_its_bytes(self):
        import zlib
        sha = self.f.blob(self.f.source)
        path = self.f.repo / ".git" / "objects" / sha[:2] / sha[2:]
        path.chmod(0o644)
        changed = b"removed feature\n"
        path.write_bytes(zlib.compress(f"blob {len(changed)}\0".encode() + changed))
        self.reject()

    def test_table_headers_cannot_replace_missing_original_data_row(self):
        for locator in ("#", "---"):
            with self.subTest(locator=locator):
                docs = dict(self.f.documents)
                docs["docs/roadmap/Complete.md"] = docs["docs/roadmap/Complete.md"].replace(
                    b"| #1 | Synthetic fixture, not product qualification. |\n", b"")
                self.f.roadmap, _ = self.f.commit(docs)
                self.f.policy["queues"] = [{"path": p, "blob": self.f.blob(b)}
                                          for p, b in docs.items() if p != "docs/ROADMAP.md"]
                self.f.policy["selected"][0]["row"] = locator
                self.f.bind_policy()
                self.reject()

    def test_explicit_original_data_row_alias_remains_supported(self):
        docs = dict(self.f.documents)
        docs["docs/roadmap/Complete.md"] = docs["docs/roadmap/Complete.md"].replace(
            b"| #1 | Synthetic", b"| Historical alias | Synthetic")
        self.f.roadmap, _ = self.f.commit(docs)
        self.f.policy["queues"] = [{"path": p, "blob": self.f.blob(b)}
                                  for p, b in docs.items() if p != "docs/ROADMAP.md"]
        self.f.policy["selected"][0]["row"] = "Historical alias"
        self.f.bind_policy()
        self.accept()

    def test_deeply_nested_evidence_is_a_structured_rejection(self):
        raw = json.dumps(self.f.evidence).replace(
            '"rows":', '"extra":' + "[" * 20000 + "0" + "]" * 20000 + ',"rows":')
        self.reject(raw_evidence=raw)

    def test_unpaired_surrogate_source_path_is_a_structured_rejection(self):
        bad = "bad\ud800.c"
        self.f.policy["selected"][0]["obligations"][0]["paths"] = [bad]
        raw = json.dumps(self.f.policy).encode()
        self.f.policy_commit, _ = self.f.commit({"policy.json": raw})
        self.f.policy_blob = self.f.blob(raw)
        self.f.evidence.update(policy_commit=self.f.policy_commit, policy_blob=self.f.policy_blob)
        self.f.evidence["rows"][0]["proof"][0]["path"] = bad
        self.reject()


if __name__ == "__main__":
    unittest.main()
