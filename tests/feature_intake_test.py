#!/usr/bin/env python3
"""Offline adversarial intake fixtures; no real issue submission or vote."""
import copy
import importlib.util
from pathlib import Path
import unittest

MODULE = Path(__file__).resolve().parents[1] / "coordination/requests/feature_intake.py"
spec = importlib.util.spec_from_file_location("feature_intake", MODULE)
f = importlib.util.module_from_spec(spec)
spec.loader.exec_module(f)
OWNER = {"id": f.OWNER_ID, "node_id": f.OWNER_NODE, "type": "User", "login": "renamed-owner"}


def page(path, items):
    return {"read_status": "success", "endpoint": path,
            "pages": [{"page": 1, "status": 200, "items": items, "next_page": None}]}


def fixture():
    issue = {"id": 9001, "number": 101, "title": "Clear active lane", "body": "See the playing lane on stage.",
             "state": "open", "user": OWNER, "url": "https://api.github.com" + f.endpoint(101)}
    draft = {"behavior": "Highlight current lane; keep mapped buttons unchanged."}
    h = f.scope_hash(issue, draft)
    source = {"firmware": "a" * 40, "controls": "b" * 64}
    receipt = {"repository_id": f.REPOSITORY_ID, "issue_id": 9001, "actor": OWNER,
               "scope_hash": h, "kind": "owner_request", "human_origin_verified": True,
               "action": "bounded_source_tests_docs_draft_pr", "evidence_ref": "verified-human-request:fixture"}
    assessment = {g: "Concrete source-backed finding: " + g for g in f.GATES}
    assessment.update(scope_hash=h, source_hash=f.digest(source), evidence_refs=["source:fixture"], resolution="ready")
    detail = {"read_status": "success", "issue": issue, "spec": draft, "assessment": assessment,
              "reactions": page(f.endpoint(101, "/reactions"), []), "timeline": page(f.endpoint(101, "/timeline"), [])}
    snapshot = {"schema": 1, "read_status": "success", "repository": {"id": f.REPOSITORY_ID, "owner": OWNER},
                "source_closure": source, "issues": page(f.endpoint(), [issue]),
                "details": {"9001": detail}, "approval_receipts": [receipt]}
    snapshot["issues"]["query"] = {"state": "all"}
    return snapshot, {"records": [{"request_id": "LX-REQ-030"}]}, {"schema": 1}


class IntakeTests(unittest.TestCase):
    def setUp(self):
        self.s, self.l, self.c = fixture()

    def run_plan(self):
        return f.evaluate(self.s, self.l, self.c, capacity=1)

    def test_owner_needs_no_redundant_vote_and_is_deterministic(self):
        first = self.run_plan()
        self.assertEqual(first, self.run_plan())
        self.assertEqual(first["claim"]["request_ids"], ["LX-REQ-031"])
        self.assertEqual(self.c, {"schema": 1})

    def test_owner_account_automation_cannot_self_authorize(self):
        self.s["approval_receipts"][0]["human_origin_verified"] = False
        self.assertIsNone(self.run_plan()["claim"])

    def test_spoofed_actor_login_counts_and_node(self):
        for actor in ({"id": 123, "login": "gobbajogger", "type": "User"}, dict(OWNER, node_id="spoof")):
            with self.subTest(actor=actor):
                self.s["approval_receipts"][0]["actor"] = actor
                self.s["details"]["9001"]["issue"]["reactions"] = {"+1": 9999}
                self.assertIsNone(self.run_plan()["claim"])

    def test_community_votes_not_authority(self):
        self.s["approval_receipts"] = []
        self.s["details"]["9001"]["reactions"]["pages"][0]["items"] = [
            {"id": 5, "content": "+1", "user": {"id": 7, "login": "gobbajogger", "type": "User"}}]
        self.assertEqual(self.run_plan()["state"], "BLOCKED")

    def test_owner_opening_reaction_requires_exact_scope_receipt(self):
        r = self.s["approval_receipts"][0]
        r.update(kind="opening_post_plus_one", reaction_id=5)
        self.s["details"]["9001"]["issue"]["user"] = {"id": 7, "type": "User"}
        self.s["details"]["9001"]["reactions"]["pages"][0]["items"] = [{"id": 5, "content": "+1", "user": OWNER}]
        self.assertIsNotNone(self.run_plan()["claim"])
        r["reaction_id"] = 6
        self.assertIsNone(self.run_plan()["claim"])

    def test_accepted_approval_survives_reaction_removal(self):
        self.c = self.run_plan()["proposed_checkpoint"]
        self.s["approval_receipts"] = []
        del self.s["details"]["9001"]["reactions"]
        self.assertIsNotNone(self.run_plan()["claim"])

    def test_scope_title_body_and_spec_edits_need_reevaluation(self):
        self.c = self.run_plan()["proposed_checkpoint"]
        for target, key in [("issue", "title"), ("issue", "body"), ("spec", "behavior")]:
            with self.subTest(key=key):
                original = copy.deepcopy(self.s)
                self.s["details"]["9001"][target][key] += " edited"
                result = self.run_plan()
                self.assertIsNone(result["claim"])
                self.assertIn("scope edited", result["items"][0]["reason"])
                self.s = original

    def test_source_closure_change_keeps_approval_but_rechecks_assessment(self):
        self.c = self.run_plan()["proposed_checkpoint"]
        self.s["source_closure"]["controls"] = "c" * 64
        result = self.run_plan()
        self.assertIsNone(result["claim"])
        self.assertIn("assessment", result["items"][0]["reason"])

    def test_closed_and_explicit_owner_cancelled(self):
        self.c = self.run_plan()["proposed_checkpoint"]
        self.s["details"]["9001"]["issue"]["state"] = "closed"
        self.s["details"]["9001"]["timeline"]["pages"][0]["items"] = [
            {"event": "closed", "id": 11, "created_at": "2026-10-25T01:00:00Z"}]
        result = self.run_plan()
        self.assertEqual(result["items"][0]["state"], "CANCELLED")
        self.c = result["proposed_checkpoint"]
        self.s["details"]["9001"]["issue"]["state"] = "open"
        self.assertIsNone(self.run_plan()["claim"])
        self.s["approval_receipts"][0]["reopens_after"] = self.c["cancellations"][f"{f.REPOSITORY_ID}:9001"]["ref"]
        self.assertIsNotNone(self.run_plan()["claim"])
        self.s["details"]["9001"]["timeline"]["pages"][0]["items"] = [{
            "event": "owner_cancelled", "actor": OWNER, "scope_hash": self.s["approval_receipts"][0]["scope_hash"],
            "evidence_ref": "owner-explicit-cancel:fixture"}]
        self.assertEqual(self.run_plan()["items"][0]["state"], "CANCELLED")

    def test_initial_closed_observation_also_blocks_replayed_receipt(self):
        self.s["details"]["9001"]["issue"]["state"] = "closed"
        self.s["details"]["9001"]["timeline"]["pages"][0]["items"] = [
            {"event": "closed", "id": 11, "created_at": "2026-10-25T01:00:00Z"}]
        self.c = self.run_plan()["proposed_checkpoint"]
        self.s["details"]["9001"]["issue"]["state"] = "open"
        self.assertIsNone(self.run_plan()["claim"])

    def test_second_close_cannot_replay_first_reauthorization(self):
        detail = self.s["details"]["9001"]
        detail["issue"]["state"] = "closed"
        detail["timeline"]["pages"][0]["items"] = [{"event": "closed", "id": 11, "created_at": "2026-10-25T01:00:00Z"}]
        self.c = self.run_plan()["proposed_checkpoint"]
        detail["issue"]["state"] = "open"
        later = dict(self.s["approval_receipts"][0], evidence_ref="verified-owner-reauthorization",
                     reopens_after=self.c["cancellations"][f"{f.REPOSITORY_ID}:9001"]["ref"])
        self.s["approval_receipts"].append(later)
        result = self.run_plan()
        self.assertIsNotNone(result["claim"])
        self.c = result["proposed_checkpoint"]
        detail["issue"]["state"] = "closed"
        detail["timeline"]["pages"][0]["items"].append({"event": "closed", "id": 12, "created_at": "2026-10-25T02:00:00Z"})
        self.c = self.run_plan()["proposed_checkpoint"]
        detail["issue"]["state"] = "open"
        self.assertIsNone(self.run_plan()["claim"])

    def test_closed_without_closure_provenance_is_unknown(self):
        self.s["details"]["9001"]["issue"]["state"] = "closed"
        self.assertTrue(self.run_plan()["reason"].startswith("unknown:"))

    def test_close_and_reopen_between_polls_does_not_reuse_old_authority(self):
        self.c = self.run_plan()["proposed_checkpoint"]
        self.s["approval_receipts"] = []
        self.s["details"]["9001"]["timeline"]["pages"][0]["items"] = [
            {"event": "closed", "id": 11, "created_at": "2026-10-25T01:00:00Z"},
            {"event": "reopened", "id": 12, "created_at": "2026-10-25T02:00:00Z"}]
        self.assertIsNone(self.run_plan()["claim"])

    def test_explicit_cancellation_reauthorization_is_reusable(self):
        detail = self.s["details"]["9001"]
        detail["timeline"]["pages"][0]["items"] = [{"event": "owner_cancelled", "actor": OWNER,
            "scope_hash": self.s["approval_receipts"][0]["scope_hash"], "evidence_ref": "cancel:fixture"}]
        self.c = self.run_plan()["proposed_checkpoint"]
        self.s["approval_receipts"][0]["reopens_after"] = self.c["cancellations"][f"{f.REPOSITORY_ID}:9001"]["ref"]
        self.c = self.run_plan()["proposed_checkpoint"]
        self.s["approval_receipts"] = []
        self.assertIsNotNone(self.run_plan()["claim"])

    def test_new_reaction_reauthorization_after_unseen_close(self):
        self.c = self.run_plan()["proposed_checkpoint"]
        detail = self.s["details"]["9001"]
        detail["timeline"]["pages"][0]["items"] = [{"event": "closed", "id": 11, "created_at": "2026-10-25T01:00:00Z"}]
        result = self.run_plan()
        ref = result["proposed_checkpoint"]["cancellations"][f"{f.REPOSITORY_ID}:9001"]["ref"]
        self.s["approval_receipts"][0].update(kind="opening_post_plus_one", reaction_id=5, reopens_after=ref)
        detail["reactions"]["pages"][0]["items"] = [{"id": 5, "content": "+1", "user": OWNER}]
        self.assertIsNotNone(self.run_plan()["claim"])

    def test_nonmaterial_edit_keeps_immutable_approved_scope_with_equivalence(self):
        self.c = self.run_plan()["proposed_checkpoint"]
        old_hash = self.s["approval_receipts"][0]["scope_hash"]
        self.s["approval_receipts"] = []
        detail = self.s["details"]["9001"]
        detail["issue"]["title"] += " (wording clarified)"
        h = f.scope_hash(detail["issue"], detail["spec"])
        detail["assessment"].update(scope_hash=h, scope_equivalence={"previous_scope_hash": old_hash,
            "current_scope_hash": h, "material_change": False, "evidence_ref": "review:wording-only"})
        result = self.run_plan()
        self.assertIsNotNone(result["claim"])
        accepted = result["proposed_checkpoint"]["approvals"][f"{f.REPOSITORY_ID}:9001"]
        self.assertEqual(accepted["scope_hash"], old_hash)
        self.assertEqual(accepted["effective_scope_hash"], h)
        detail["timeline"]["pages"][0]["items"] = [{"event": "owner_cancelled", "actor": OWNER,
            "scope_hash": old_hash, "evidence_ref": "owner-cancels-approved-action"}]
        self.assertIsNone(self.run_plan()["claim"])
        detail["timeline"]["pages"][0]["items"] = []
        detail["assessment"]["scope_equivalence"]["material_change"] = True
        self.assertIsNone(self.run_plan()["claim"])

    def test_tombstone_id_is_not_reused(self):
        self.l["tombstones"] = [{"request_id": "LX-REQ-031", "superseded_by": ["LX-REQ-030"]}]
        self.assertEqual(self.run_plan()["claim"]["request_ids"], ["LX-REQ-032"])

    def test_existing_ledger_and_canonical_mapping_deduplicate(self):
        self.l["records"][0]["known_source_references"] = [{"type": "issue", "repository": f.REPOSITORY, "number": 101}]
        self.l["records"].append({"request_id": "LX-REQ-029", "known_source_references": copy.deepcopy(
            self.l["records"][0]["known_source_references"])})
        result = self.run_plan()
        self.assertIsNone(result["claim"])
        self.assertEqual(result["items"][0]["request_ids"], ["LX-REQ-029", "LX-REQ-030"])
        self.c = result["proposed_checkpoint"]
        self.l["records"][0].pop("known_source_references")
        self.l["records"][1].pop("known_source_references")
        self.assertIsNone(self.run_plan()["claim"])

    def test_duplicate_queue_and_pr_filter(self):
        issue = self.s["issues"]["pages"][0]["items"][0]
        self.s["issues"]["pages"][0]["items"].extend([copy.deepcopy(issue), {"pull_request": {}, "id": 1}])
        self.assertEqual(len(self.run_plan()["items"]), 1)

    def test_all_pagination_must_be_complete(self):
        self.s["approval_receipts"][0].update(kind="opening_post_plus_one", reaction_id=5)
        self.s["details"]["9001"]["reactions"]["pages"][0]["items"] = [{"id": 5, "content": "+1", "user": OWNER}]
        for name in ("issues", "reactions", "timeline"):
            with self.subTest(name=name):
                s = copy.deepcopy(self.s)
                env = s[name] if name == "issues" else s["details"]["9001"][name]
                env["pages"][0]["next_page"] = 2
                self.assertEqual(f.evaluate(s, self.l, self.c, capacity=1)["items"], [])
                env["pages"].append({"page": 2, "status": 200, "items": [], "next_page": None})
                self.assertIsNotNone(f.evaluate(s, self.l, self.c, capacity=1)["claim"])
                env["pages"][1]["status"] = 500
                self.assertEqual(f.evaluate(s, self.l, self.c, capacity=1)["state"], "BLOCKED")

    def test_failed_stale_and_missing_reads_are_unknown(self):
        for status in ("failed", "incomplete", "stale"):
            self.s["read_status"] = status
            self.assertTrue(self.run_plan()["reason"].startswith("unknown:"))

    def test_missing_tracked_issue_is_not_no_change(self):
        self.c["issue_map"] = {f"{f.REPOSITORY_ID}:123": ["LX-REQ-030"]}
        self.assertEqual(self.run_plan()["items"], [])

    def test_missing_approved_issue_is_unknown_even_without_mapping(self):
        self.c = self.run_plan()["proposed_checkpoint"]
        self.s["issues"]["pages"][0]["items"] = []
        self.assertEqual(self.run_plan()["state"], "BLOCKED")

    def test_ambiguous_and_incomplete_assessment(self):
        a = self.s["details"]["9001"]["assessment"]
        a.update(resolution="conflict", proposed_resolution="Keep existing transport binding and preview alternative.")
        self.assertIn("preview alternative", self.run_plan()["items"][0]["reason"])
        a["resolution"] = "ready"
        del a["acceptance_tests"]
        self.assertIsNone(self.run_plan()["claim"])

    def test_repository_id_mismatch(self):
        self.s["repository"]["id"] = 1
        self.assertIsNone(self.run_plan()["claim"])

    def test_batch_intake_respects_capacity_and_preserves_single_writer(self):
        for number, iid in ((102, 9002), (103, 9003)):
            detail = copy.deepcopy(self.s['details']['9001'])
            detail['issue'].update(number=number, id=iid, url='https://api.github.com' + f.endpoint(number))
            h = f.scope_hash(detail['issue'], detail['spec'])
            detail['assessment']['scope_hash'] = h
            detail['timeline'] = page(f.endpoint(number, '/timeline'), [])
            detail['reactions'] = page(f.endpoint(number, '/reactions'), [])
            self.s['issues']['pages'][0]['items'].append(detail['issue'])
            self.s['details'][str(iid)] = detail
            self.s['approval_receipts'].append(dict(self.s['approval_receipts'][0], issue_id=iid, scope_hash=h))
        before = copy.deepcopy((self.s, self.l, self.c))
        result = f.evaluate(self.s, self.l, self.c, capacity=2)
        self.assertEqual([c['request_ids'] for c in result['claims']], [['LX-REQ-031'], ['LX-REQ-032']])
        self.assertEqual(result['claim'], result['claims'][0])
        self.assertEqual({c['owner'] for c in result['claims']}, {'existing sole request coordinator'})
        self.assertEqual((self.s, self.l, self.c), before)
        self.assertEqual(f.evaluate(self.s, self.l, self.c)['claims'], [])
        self.assertEqual(f.evaluate(self.s, self.l, self.c, capacity=-1)['state'], 'BLOCKED')
        self.s['details']['9001']['assessment']['resolution'] = 'blocked'
        result = f.evaluate(self.s, self.l, self.c, capacity=2)
        self.assertEqual(len(result['claims']), 2)
        self.assertEqual(result['items'][0]['state'], 'BLOCKED')
        self.s['read_status'] = 'stale'
        self.assertEqual(f.evaluate(self.s, self.l, self.c, capacity=2)['claims'], [])


if __name__ == "__main__":
    unittest.main()
