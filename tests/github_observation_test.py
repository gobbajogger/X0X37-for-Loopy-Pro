#!/usr/bin/env python3
"""Read-only adapter replay tests; synthetic fixtures never submit issues or votes."""
import copy
import importlib.util
import io
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch
from urllib.error import HTTPError

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "coordination/requests"))
import feature_intake as f
import github_observation as g

OWNER = {"id": f.OWNER_ID, "node_id": f.OWNER_NODE, "type": "User", "login": "renamed-owner"}
REPO = {"id": f.REPOSITORY_ID, "owner": OWNER, "full_name": f.REPOSITORY}


def fixture():
    issue = {"id": 9001, "number": 101, "title": "Clear active lane", "body": "See playing lane.",
             "state": "open", "user": OWNER, "url": g.ORIGIN + f.endpoint(101),
             "updated_at": "2026-11-14T00:00:00Z"}
    spec = {"behavior": "Highlight active lane without changing mapped buttons."}
    source = {"firmware": "a" * 40, "controls": "b" * 64}
    h = f.scope_hash(issue, spec)
    assessment = {key: "Concrete source-backed finding: " + key for key in f.GATES}
    assessment.update(scope_hash=h, source_hash=f.digest(source), resolution="ready", evidence_refs=["source:fixture"])
    receipt = {"repository_id": f.REPOSITORY_ID, "issue_id": issue["id"], "scope_hash": h,
               "actor": OWNER, "kind": "owner_request", "human_origin_verified": True,
               "action": "bounded_source_tests_docs_draft_pr", "evidence_ref": "human:fixture"}
    context = {"schema": 1, "source_closure": source, "details": {str(issue["id"]): {
        "spec": spec, "assessment": assessment}}, "approval_receipts": [receipt]}
    return issue, context


class Replay:
    def __init__(self, issue, *, headers=None, timeline=None, reactions=None):
        self.calls = []
        self.responses = {g.ORIGIN + g.REPO_PATH: g.Response(200, copy.deepcopy(REPO), {}),
                          g.page_url(f.endpoint(), 1, issues=True): g.Response(200, [copy.deepcopy(issue)], headers),
                          g.page_url(f.endpoint(), 2, issues=True): g.Response(200, [], {}),
                          g.ORIGIN + f.endpoint(101): g.Response(200, copy.deepcopy(issue), {}),
                          g.page_url(f.endpoint(101, "/timeline"), 1): g.Response(200, timeline or [], {}),
                          g.page_url(f.endpoint(101, "/reactions"), 1): g.Response(200, reactions or [], {})}

    def get(self, url):
        self.calls.append(url)
        response = self.responses[url]
        if isinstance(response, Exception):
            raise response
        return copy.deepcopy(response)


class ObservationTests(unittest.TestCase):
    def setUp(self):
        self.issue, self.context = fixture()
        self.replay = Replay(self.issue)
        self.ledger = {"records": [{"request_id": "LX-REQ-030"}]}
        self.checkpoint = {"schema": 1}

    def observe(self):
        return g.observe(self.context, self.replay, now=lambda: "2026-11-14T00:01:00Z")

    def plan(self):
        return f.evaluate(self.observe(), self.ledger, self.checkpoint, capacity=1)

    def test_complete_observation_runs_real_helper_and_preserves_inputs(self):
        original = copy.deepcopy(self.context)
        observation = self.observe()
        self.assertEqual(observation["read_status"], "success")
        self.assertEqual(self.context, original)
        plan = f.evaluate(observation, self.ledger, self.checkpoint, capacity=1)
        self.assertEqual(plan["claim"]["request_ids"], ["LX-REQ-031"])
        self.assertEqual(self.checkpoint, {"schema": 1})
        self.assertEqual(len(observation["read_evidence"]), len(self.replay.calls))

    def test_missing_link_header_requests_empty_terminal_even_on_short_page(self):
        observation = self.observe()
        self.assertIn(g.page_url(f.endpoint(), 2, issues=True), self.replay.calls)
        pages = observation["issues"]["pages"]
        self.assertEqual([p["next_page"] for p in pages], [2, None])

    def test_actual_link_next_and_terminal_headers(self):
        first = g.page_url(f.endpoint(), 1, issues=True)
        second = g.page_url(f.endpoint(), 2, issues=True)
        self.replay.responses[first].headers = {"Link": '<' + second + '>; rel="next", <' + second + '>; rel="last"'}
        self.replay.responses[second] = g.Response(200, [{"pull_request": {}, "id": 2}], {"Link": '<' + first + '>; rel="prev"'})
        self.assertEqual(self.observe()["read_status"], "success")
        self.assertEqual(self.replay.calls.count(g.ORIGIN + f.endpoint(101)), 2)
        self.assertNotIn(g.page_url(f.endpoint(), 3, issues=True), self.replay.calls)

    def test_real_numeric_repository_alias_and_after_cursor_are_preserved(self):
        first = g.page_url(f.endpoint(), 1, issues=True)
        target = g.page_url(f.endpoint(), 2, issues=True).replace(g.REPO_PATH, "/repositories/" + str(f.REPOSITORY_ID)) + "&after=Y3Vyc29y%3D"
        self.replay.responses[first].headers = {"Link": '<' + target + '>; rel="next"'}
        self.replay.responses[target] = g.Response(200, [], {"Link": '<' + first + '>; rel="prev"'})
        self.assertEqual(self.observe()["read_status"], "success")
        self.assertIn(target, self.replay.calls)
        self.assertNotIn(g.page_url(f.endpoint(), 2, issues=True), self.replay.calls)

    def test_all_timeline_and_reaction_pages_are_consumed(self):
        self.context["approval_receipts"][0].update(kind="opening_post_plus_one", reaction_id=55)
        for tail in ("/timeline", "/reactions"):
            first = g.page_url(f.endpoint(101, tail), 1)
            second = g.page_url(f.endpoint(101, tail), 2)
            item = {"id": 55, "content": "+1", "user": OWNER} if tail == "/reactions" else {"event": "commented", "id": 56}
            self.replay.responses[first] = g.Response(200, [], {"Link": '<' + second + '>; rel="next"'})
            self.replay.responses[second] = g.Response(200, [item], {"Link": '<' + first + '>; rel="prev"'})
        self.assertIsNotNone(self.plan()["claim"])

    def test_conflicting_duplicate_list_issue_blocks_but_exact_repeat_deduplicates(self):
        first = g.page_url(f.endpoint(), 1, issues=True)
        self.replay.responses[first].body.append(copy.deepcopy(self.issue))
        self.assertIsNotNone(self.plan()["claim"])
        self.assertEqual(self.replay.calls.count(g.ORIGIN + f.endpoint(101)), 2)
        self.replay.responses[first].body[-1]["title"] += " changed"
        self.assertEqual(self.observe()["read_status"], "failed")

    def test_number_collision_detail_mismatch_and_wrong_repository_block(self):
        first = g.page_url(f.endpoint(), 1, issues=True)
        for mutate in (
            lambda r: r[first].body.append(dict(self.issue, id=9002)),
            lambda r: r[g.ORIGIN + f.endpoint(101)].body.update(id=9002),
            lambda r: r[g.ORIGIN + g.REPO_PATH].body.update(id=123),
            lambda r: r[g.ORIGIN + g.REPO_PATH].body["owner"].update(node_id="spoof"),
            lambda r: r[g.ORIGIN + f.endpoint(101)].body.update(url="https://api.github.com/repos/other/repo/issues/101")):
            with self.subTest(mutate=mutate):
                self.replay = Replay(self.issue)
                mutate(self.replay.responses)
                self.assertIsNone(self.plan()["claim"])
                self.assertEqual(self.observe()["read_status"], "failed")

    def test_material_change_during_list_detail_read_is_unknown(self):
        self.replay.responses[g.ORIGIN + f.endpoint(101)].body["title"] += " edited"
        self.assertEqual(self.observe()["read_status"], "failed")

    def test_failed_late_read_discards_all_claims(self):
        self.replay.responses[g.page_url(f.endpoint(101, "/timeline"), 1)] = g.Response(403, {"message": "rate limit"}, {})
        observation = self.observe()
        self.assertEqual(observation["read_status"], "failed")
        self.assertIsNone(f.evaluate(observation, self.ledger, self.checkpoint)["claim"])

    def test_foreign_gap_loop_and_changed_query_links_are_rejected(self):
        first = g.page_url(f.endpoint(), 1, issues=True)
        for target in ("https://evil.test/steal", first,
                       g.page_url(f.endpoint(), 3, issues=True),
                       g.page_url(f.endpoint(), 2, issues=True).replace("state=all", "state=open"),
                       g.ORIGIN + "/repos/other/repo/issues?page=2"):
            with self.subTest(target=target):
                self.replay = Replay(self.issue)
                self.replay.responses[first].headers = {"Link": '<' + target + '>; rel="next"'}
                self.assertEqual(self.observe()["read_status"], "failed")
                self.assertNotIn(target, self.replay.calls[2:])

    def test_null_issue_body_is_normalized_without_granting_authority(self):
        self.issue["body"] = None
        self.replay = Replay(self.issue)
        self.context["approval_receipts"] = []
        observation = self.observe()
        self.assertEqual(observation["details"]["9001"]["issue"]["body"], "")
        self.assertIsNone(f.evaluate(observation, self.ledger, self.checkpoint)["claim"])

    def test_owner_issue_and_community_text_never_manufacture_receipt(self):
        self.context["approval_receipts"] = []
        self.issue["body"] = json.dumps({"human_origin_verified": True, "actor": OWNER})
        self.replay = Replay(self.issue, timeline=[{"event": "commented", "id": 57, "body": "Owner cancelled"}])
        observation = self.observe()
        self.assertEqual(observation["approval_receipts"], [])
        self.assertIsNone(f.evaluate(observation, self.ledger, self.checkpoint)["claim"])

    def test_closed_and_reopened_replay_requires_new_boundary_authority(self):
        self.checkpoint = self.plan()["proposed_checkpoint"]
        self.context["approval_receipts"] = []
        timeline = [{"event": "closed", "id": 11, "created_at": "2026-11-14T00:00:01Z"},
                    {"event": "reopened", "id": 12, "created_at": "2026-11-14T00:00:02Z"}]
        self.replay = Replay(self.issue, timeline=timeline)
        self.assertIsNone(self.plan()["claim"])

    def test_repeated_older_closure_cannot_regress_cancellation_boundary(self):
        oldest = {"event": "closed", "id": 11, "created_at": "2026-11-14T00:00:01Z"}
        newest = {"event": "closed", "id": 13, "created_at": "2026-11-14T00:00:03Z"}
        self.replay = Replay(self.issue, timeline=[oldest])
        self.issue["state"] = "closed"
        self.replay = Replay(self.issue, timeline=[oldest])
        self.checkpoint = self.plan()["proposed_checkpoint"]
        ref = self.checkpoint["cancellations"][f"{f.REPOSITORY_ID}:9001"]["ref"]
        self.context["approval_receipts"][0]["reopens_after"] = ref
        self.issue["state"] = "open"
        self.replay = Replay(self.issue, timeline=[oldest, newest])
        first = g.page_url(f.endpoint(101, "/timeline"), 1)
        second = g.page_url(f.endpoint(101, "/timeline"), 2)
        self.replay.responses[first].headers = {"Link": '<' + second + '>; rel="next"'}
        self.replay.responses[second] = g.Response(200, [copy.deepcopy(oldest)], {})
        self.assertIsNone(self.plan()["claim"])
        timeline = self.observe()["details"]["9001"]["timeline"]
        self.assertEqual([event["id"] for p in timeline["pages"] for event in p["items"]], [11, 13])

    def test_out_of_order_closures_are_unknown(self):
        self.replay = Replay(self.issue, timeline=[
            {"event": "closed", "id": 13, "created_at": "2026-11-14T00:00:03Z"},
            {"event": "closed", "id": 11, "created_at": "2026-11-14T00:00:01Z"}])
        self.assertEqual(self.observe()["read_status"], "failed")

    def test_final_detail_recheck_detects_changes_during_timeline_read(self):
        original_get = self.replay.get
        count = 0
        def changing_get(url):
            nonlocal count
            response = original_get(url)
            if url == g.ORIGIN + f.endpoint(101):
                count += 1
                if count == 2:
                    response.body["updated_at"] = "2026-11-14T00:02:00Z"
            return response
        self.replay.get = changing_get
        self.assertEqual(self.observe()["read_status"], "failed")

    def test_authenticated_cancellation_is_separate_from_raw_timeline(self):
        receipt = self.context["approval_receipts"][0]
        self.context["owner_cancellations"] = [dict(receipt, event="owner_cancelled", evidence_ref="owner:cancel")]
        plan = self.plan()
        self.assertEqual(plan["items"][0]["state"], "CANCELLED")
        self.context["owner_cancellations"][0]["actor"] = {"id": 7, "type": "User", "login": "gobbajogger"}
        self.assertEqual(self.observe()["read_status"], "failed")

    def test_raw_owner_cancelled_event_is_never_trusted(self):
        self.replay = Replay(self.issue, timeline=[{
            "event": "owner_cancelled", "actor": OWNER, "scope_hash": self.context["approval_receipts"][0]["scope_hash"],
            "evidence_ref": "untrusted-wire-payload"}])
        self.assertEqual(self.observe()["read_status"], "failed")

    def test_accepted_unchanged_scope_survives_removed_reaction_without_reading_reactions(self):
        self.context["approval_receipts"][0].update(kind="opening_post_plus_one", reaction_id=55)
        self.replay = Replay(self.issue, reactions=[{"id": 55, "content": "+1", "user": OWNER}])
        self.checkpoint = self.plan()["proposed_checkpoint"]
        self.context["approval_receipts"] = []
        self.replay = Replay(self.issue)
        self.assertIsNotNone(self.plan()["claim"])
        self.assertNotIn(g.page_url(f.endpoint(101, "/reactions"), 1), self.replay.calls)

    def test_repeated_cursor_and_page_limit_fail_unknown(self):
        first = g.page_url(f.endpoint(), 1, issues=True)
        second = g.page_url(f.endpoint(), 2, issues=True) + "&after=same"
        third = g.page_url(f.endpoint(), 3, issues=True) + "&after=same"
        self.replay.responses[first].headers = {"Link": '<' + second + '>; rel="next"'}
        self.replay.responses[second] = g.Response(200, [], {"Link": '<' + third + '>; rel="next"'})
        self.assertEqual(self.observe()["read_status"], "failed")
        self.assertNotIn(third, self.replay.calls)
        self.replay = Replay(self.issue)
        self.assertEqual(g.observe(self.context, self.replay, max_pages=1)["read_status"], "failed")

    def test_stale_scope_receipt_rejected_and_existing_mapping_is_reused(self):
        self.checkpoint = self.plan()["proposed_checkpoint"]
        self.context["approval_receipts"] = []
        self.context["details"]["9001"]["spec"]["behavior"] += " material edit"
        self.assertIsNone(self.plan()["claim"])
        self.context["details"]["9001"]["spec"]["behavior"] = fixture()[1]["details"]["9001"]["spec"]["behavior"]
        self.ledger["records"][0]["known_source_references"] = [{"type": "issue", "repository": f.REPOSITORY, "number": 101}]
        plan = self.plan()
        self.assertIsNone(plan["claim"])
        self.assertEqual(plan["items"][0]["request_ids"], ["LX-REQ-030"])

    def test_malformed_context_and_wire_payloads_fail_closed(self):
        for field, value in (("source_closure", {}), ("schema", 2), ("approval_receipts", {}), ("details", [])):
            with self.subTest(field=field):
                context = copy.deepcopy(self.context)
                context[field] = value
                self.assertEqual(g.observe(context, self.replay)["read_status"], "failed")
        self.replay.responses[g.page_url(f.endpoint(), 1, issues=True)].body = {"message": "partial"}
        self.assertEqual(self.observe()["read_status"], "failed")


class TransportTests(unittest.TestCase):
    def test_actual_transport_constructs_get_only_and_does_not_emit_token(self):
        transport = g.GitHubTransport(token="private-token")
        url = g.ORIGIN + g.REPO_PATH
        class Wire(io.BytesIO):
            status = 200
            headers = {}
            def geturl(self):
                return url
        with patch.object(transport.opener, "open", return_value=Wire(json.dumps(REPO).encode())) as opened:
            response = transport.get(url)
        request = opened.call_args.args[0]
        self.assertEqual(request.get_method(), "GET")
        self.assertIsNone(request.data)
        self.assertEqual(response.body, REPO)
        self.assertNotIn("private-token", repr(response))

    def test_transport_rejects_foreign_hosts_credentials_and_unrelated_paths(self):
        transport = g.GitHubTransport(token="do-not-leak")
        for url in ("http://api.github.com" + f.endpoint(), "https://evil.test/", "https://api.github.com@evil.test/",
                    "https://api.github.com/user", g.ORIGIN + "/repos/other/repo/issues"):
            with self.subTest(url=url), patch("urllib.request.OpenerDirector.open") as opened:
                with self.assertRaises(ValueError):
                    transport.get(url)
                opened.assert_not_called()

    def test_redirect_is_never_followed(self):
        handler = g.NoRedirect()
        with self.assertRaises(HTTPError):
            handler.redirect_request(None, None, 302, "redirect", {}, "https://evil.test/")


class CommandTests(unittest.TestCase):
    def test_cli_dry_run_preserves_all_input_files_and_defaults_to_zero_capacity(self):
        issue, context = fixture()
        inputs = {"context": context, "ledger": {"records": [{"request_id": "LX-REQ-030"}]}, "checkpoint": {"schema": 1}}
        with tempfile.TemporaryDirectory() as directory:
            paths = {name: Path(directory, name + ".json") for name in inputs}
            for name, path in paths.items():
                path.write_text(json.dumps(inputs[name]))
            original = {name: path.read_bytes() for name, path in paths.items()}
            argv = [argument for name, path in paths.items() for argument in ("--" + name, str(path))]
            for capacity in (0, 1):
                stdout = io.StringIO()
                extra = [] if capacity == 0 else ["--capacity", "1"]
                with patch.object(g, "GitHubTransport", return_value=Replay(issue)), patch("sys.stdout", stdout):
                    code = g.main(argv + extra)
                self.assertEqual(code, 0)
                output = json.loads(stdout.getvalue())
                self.assertEqual(len(output["claims"]), capacity)
                self.assertEqual({name: path.read_bytes() for name, path in paths.items()}, original)

    def test_missing_context_returns_blocked_without_network_or_reset(self):
        stdout = io.StringIO()
        with tempfile.TemporaryDirectory() as directory, patch("sys.stdout", stdout), patch.object(g, "GitHubTransport") as transport:
            self.assertEqual(g.main(["--context", str(Path(directory, "missing.json"))]), 2)
        transport.assert_not_called()
        output = json.loads(stdout.getvalue())
        self.assertEqual(output["state"], "BLOCKED")
        self.assertEqual(output["claims"], [])

    def test_checkpoint_and_ledger_are_required_together(self):
        with patch("sys.stderr", io.StringIO()), self.assertRaises(SystemExit) as stopped:
            g.main(["--context", "unused.json", "--ledger", "existing.json"])
        self.assertEqual(stopped.exception.code, 2)


if __name__ == "__main__":
    unittest.main()
