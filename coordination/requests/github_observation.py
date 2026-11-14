#!/usr/bin/env python3
"""Read GitHub into the intake contract; print observations or dry-run proposals only.

Coordinator context is trusted input, not an authority source derived from GitHub
text. This module never writes a ledger/checkpoint, submits a vote or creates work.
"""
import argparse
import copy
from dataclasses import dataclass
from datetime import datetime, timezone
import json
import os
from pathlib import Path
import re
from urllib.error import HTTPError, URLError
from urllib.parse import parse_qsl, urlencode, urlsplit
import urllib.request

import feature_intake as intake

ORIGIN = "https://api.github.com"
REPO_PATH = "/repos/" + intake.REPOSITORY
REPO_ALIAS = "/repositories/" + str(intake.REPOSITORY_ID)


@dataclass
class Response:
    status: int
    body: object
    # None means the caller cannot supply headers. Empty dict means observed
    # headers have no Link; GitHub has declared this page terminal.
    headers: dict | None = None


def validate_url(url):
    """Allow only the repository and issue read endpoints used by this adapter."""
    parts = urlsplit(url)
    if (parts.scheme != "https" or parts.netloc != "api.github.com"
            or parts.fragment or parts.username or parts.password
            or not re.fullmatch("(?:" + re.escape(REPO_PATH) + "|" + re.escape(REPO_ALIAS)
                                + r")(?:/issues(?:/[1-9][0-9]*(?:/(?:timeline|reactions))?)?)?", parts.path)):
        raise ValueError("unapproved GitHub read URL")
    pairs = parse_qsl(parts.query, keep_blank_values=True, strict_parsing=True)
    if len(dict(pairs)) != len(pairs):
        raise ValueError("duplicate query parameter")
    query = dict(pairs)
    if any(k not in ("state", "per_page", "page", "sort", "direction", "after") for k in query):
        raise ValueError("unapproved query parameter")
    if query:
        if not (parts.path.endswith("/issues") or parts.path.endswith(("/timeline", "/reactions"))):
            raise ValueError("query on detail endpoint")
        if query.get("per_page") != "100" or not re.fullmatch(r"[1-9][0-9]*", query.get("page", "")):
            raise ValueError("invalid page parameters")
        expected = {"per_page", "page"}
        if parts.path in (intake.endpoint(), REPO_ALIAS + "/issues"):
            expected.update(("state", "sort", "direction"))
            if query.get("state") != "all" or query.get("sort") != "created" or query.get("direction") != "asc":
                raise ValueError("issue query must preserve complete stable ordering")
        if "after" in query:
            expected.add("after")
            if not query["after"] or len(query["after"]) > 4096:
                raise ValueError("invalid pagination cursor")
        if set(query) != expected:
            raise ValueError("unexpected collection parameters")
    return parts


class NoRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, req, fp, code, msg, headers, newurl):
        # A transfer/rename must be resolved by the coordinator. Never forward
        # credentials to a redirected host or a different repository.
        raise HTTPError(req.full_url if req else ORIGIN, code, "redirect refused", headers, fp)


class GitHubTransport:
    """Fresh stdlib HTTPS GETs with an optional token; no mutating methods."""
    def __init__(self, token=None, timeout=30):
        self.token = token
        self.timeout = timeout
        self.opener = urllib.request.build_opener(NoRedirect())

    def get(self, url):
        validate_url(url)
        headers = {"Accept": "application/vnd.github+json", "X-GitHub-Api-Version": "2022-11-28",
                   "User-Agent": "DnBOX-read-only-feature-intake", "Cache-Control": "no-cache"}
        if self.token:
            headers["Authorization"] = "Bearer " + self.token
        request = urllib.request.Request(url, headers=headers, method="GET")
        try:
            with self.opener.open(request, timeout=self.timeout) as response:
                if response.geturl() != url:
                    raise ValueError("unexpected response URL")
                return Response(response.status, json.load(response), dict(response.headers.items()))
        except HTTPError as error:
            # Error bodies and request headers may contain credentials or private
            # server diagnostics. Retain only the status and generic outcome.
            return Response(error.code, None, {})


def page_url(endpoint, page, issues=False):
    query = {"state": "all", "sort": "created", "direction": "asc"} if issues else {}
    query.update(per_page=100, page=page)
    return ORIGIN + endpoint + "?" + urlencode(query)


def utc_now():
    return datetime.now(timezone.utc).isoformat().replace("+00:00", "Z")


def _issue(value):
    if not isinstance(value, dict):
        raise ValueError("malformed issue")
    issue = copy.deepcopy(value)
    if "pull_request" in issue:
        return issue
    if (type(issue.get("id")) is not int or issue["id"] <= 0
            or type(issue.get("number")) is not int or issue["number"] <= 0):
        raise ValueError("missing stable issue identity")
    if issue.get("url") != ORIGIN + intake.endpoint(issue["number"]):
        raise ValueError("foreign issue identity")
    if issue.get("body") is None:
        issue["body"] = ""
    actor = issue.get("user", {})
    if (not isinstance(issue.get("title"), str) or not isinstance(issue["body"], str)
            or issue.get("state") not in ("open", "closed") or not isinstance(actor, dict)
            or type(actor.get("id")) is not int or actor["id"] <= 0):
        raise ValueError("malformed issue scope/state/actor")
    return issue


def _same_issue(first, later):
    fields = ("id", "number", "url", "title", "body", "state", "updated_at")
    actor_fields = ("id", "type", "node_id")
    return (all(first.get(k) == later.get(k) for k in fields)
            and all(first["user"].get(k) == later["user"].get(k) for k in actor_fields))


def _deduplicate(items, kind, seen):
    result = []
    for item in items:
        if not isinstance(item, dict):
            raise ValueError("malformed " + kind)
        if kind == "timeline" and item.get("event") == "owner_cancelled":
            raise ValueError("raw timeline cannot supply trusted owner cancellation")
        identity = item.get("id")
        if identity is not None:
            if type(identity) is not int or identity <= 0:
                raise ValueError("invalid stable " + kind + " identity")
            key = (item.get("event") if kind == "timeline" else kind, identity)
            if key in seen:
                if seen[key] != item:
                    raise ValueError("conflicting duplicate " + kind)
                continue
            seen[key] = item
        elif kind == "reaction":
            raise ValueError("missing stable reaction identity")
        result.append(item)
    return result


def _normalize_event_pages(envelope, kind):
    """Deduplicate normalized pages; read_evidence retains each raw response hash."""
    seen = {}
    last_closed = None
    for page in envelope["pages"]:
        page["items"] = _deduplicate(page["items"], kind, seen)
        if kind == "timeline":
            for event in page["items"]:
                if event.get("event") != "closed":
                    continue
                if type(event.get("id")) is not int or not isinstance(event.get("created_at"), str):
                    raise ValueError("missing exact closure identity/time")
                stamp = datetime.fromisoformat(event["created_at"].replace("Z", "+00:00"))
                if stamp.tzinfo is None or (last_closed is not None and stamp < last_closed):
                    raise ValueError("out-of-order closure evidence")
                # Preserve server order for equal timestamps; the stable event ID
                # remains part of the helper's exact cancellation boundary.
                last_closed = stamp


def observe(context, transport=None, now=utc_now, max_pages=10000):
    """Read every issue and return a fail-closed helper-compatible observation.

    `context` comes from the authenticated coordinator: source_closure, optional
    details[stable_issue_id] with spec/assessment, approval_receipts and optional
    owner_cancellations. It must never be populated from issue/community text.
    A response transport can be supplied for connector bridges or offline replay.
    """
    result = {"schema": 1, "read_status": "failed", "details": {},
              "approval_receipts": [], "read_evidence": [], "observation_started_at": now()}
    transport = transport if transport is not None else GitHubTransport()

    def get(url):
        validate_url(url)
        response = transport.get(url)
        if not isinstance(response, Response):
            raise ValueError("transport must return Response")
        result["read_evidence"].append({"url": url, "status": response.status,
            "response_hash": intake.digest(response.body), "observed_at": now(),
            "headers_available": response.headers is not None,
            "link": next((v for k, v in (response.headers or {}).items() if k.lower() == "link"), None)})
        if response.status != 200:
            raise ValueError("GitHub read failed with HTTP " + str(response.status))
        if response.headers is not None and not isinstance(response.headers, dict):
            raise ValueError("malformed response headers")
        return response

    def collection(endpoint, issues=False):
        envelope = {"endpoint": endpoint, "read_status": "success", "pages": []}
        url = page_url(endpoint, 1, issues)
        seen_cursors = set()
        for page in range(1, max_pages + 1):
            response = get(url)
            if not isinstance(response.body, list):
                raise ValueError("collection response is not a list")
            nxt = None
            next_url = None
            if response.headers is None:
                # Connector bridges without response headers cannot establish
                # completion from a short page. Read an actual empty page.
                nxt = page + 1 if response.body else None
                if nxt is not None:
                    if "after" in dict(parse_qsl(urlsplit(url).query)):
                        raise ValueError("cursor continuation requires response Link header")
                    next_url = page_url(endpoint, nxt, issues)
            else:
                link_values = [v for k, v in response.headers.items() if k.lower() == "link"]
                if len(link_values) > 1:
                    raise ValueError("duplicate Link header")
                if link_values:
                    links = []
                    for segment in link_values[0].split(","):
                        match = re.fullmatch(r'\s*<([^>]+)>\s*;\s*rel="([a-z ]+)"\s*', segment)
                        if not match:
                            raise ValueError("malformed Link header")
                        if "next" in match[2].split():
                            links.append(match[1])
                    if len(links) > 1:
                        raise ValueError("duplicate next link")
                    if links:
                        target = validate_url(links[0])
                        expected = urlsplit(page_url(endpoint, page + 1, issues))
                        target_path = target.path.replace(REPO_ALIAS, REPO_PATH, 1) if target.path.startswith(REPO_ALIAS + "/") else target.path
                        target_query = dict(parse_qsl(target.query))
                        cursor = target_query.pop("after", None)
                        if (target_path != expected.path or target_query != dict(parse_qsl(expected.query))):
                            raise ValueError("pagination gap/loop/query change")
                        if cursor is not None:
                            if cursor in seen_cursors:
                                raise ValueError("pagination cursor loop")
                            seen_cursors.add(cursor)
                        nxt = page + 1
                        next_url = links[0]
            envelope["pages"].append({"page": page, "status": response.status,
                                      "items": copy.deepcopy(response.body), "next_page": nxt})
            if nxt is None:
                return envelope
            url = next_url
        raise ValueError("pagination limit reached before terminal page")

    try:
        if not isinstance(context, dict) or context.get("schema") != 1:
            raise ValueError("missing coordinator context schema")
        source = context.get("source_closure")
        if (not isinstance(source, dict) or not source or not all(isinstance(v, str)
                and re.fullmatch(r"[0-9a-f]{40}|[0-9a-f]{64}", v) for v in source.values())):
            raise ValueError("missing current source closure")
        details_context = context.get("details", {})
        receipts = context.get("approval_receipts", [])
        cancellations = context.get("owner_cancellations", [])
        if (not isinstance(details_context, dict) or not all(isinstance(v, dict) for v in details_context.values())
                or not isinstance(receipts, list) or not all(isinstance(r, dict) for r in receipts)
                or not isinstance(cancellations, list) or not all(isinstance(c, dict) for c in cancellations)):
            raise ValueError("malformed trusted coordinator context")
        if type(max_pages) is not int or max_pages <= 0:
            raise ValueError("invalid pagination limit")
        result.update(source_closure=copy.deepcopy(source), approval_receipts=copy.deepcopy(receipts),
                      context_identity=intake.digest(context))
        repo = get(ORIGIN + REPO_PATH).body
        if (not isinstance(repo, dict) or type(repo.get("id")) is not int or repo["id"] != intake.REPOSITORY_ID
                or repo.get("full_name") != intake.REPOSITORY or not isinstance(repo.get("owner"), dict)
                or not intake.owner(repo["owner"])):
            raise ValueError("repository/owner identity mismatch")
        result["repository"] = copy.deepcopy(repo)
        result["issues"] = collection(intake.endpoint(), issues=True)
        result["issues"]["query"] = {"state": "all"}
        issues, numbers = {}, {}
        for page in result["issues"]["pages"]:
            page["items"] = [_issue(item) for item in page["items"]]
            for issue in page["items"]:
                if "pull_request" in issue:
                    continue
                iid, number = issue["id"], issue["number"]
                if iid in issues and issues[iid] != issue:
                    raise ValueError("conflicting duplicate issue")
                if number in numbers and numbers[number] != iid:
                    raise ValueError("issue number identity collision")
                issues[iid], numbers[number] = issue, iid
        for iid, listed in sorted(issues.items()):
            issue = _issue(get(ORIGIN + intake.endpoint(listed["number"])).body)
            if "pull_request" in issue or not _same_issue(listed, issue):
                raise ValueError("issue identity/scope changed between list and detail")
            supplied = details_context.get(str(iid), {})
            detail = {"read_status": "success", "issue": issue,
                      "spec": copy.deepcopy(supplied.get("spec", {})),
                      "assessment": copy.deepcopy(supplied.get("assessment", {})),
                      "timeline": collection(intake.endpoint(issue["number"], "/timeline"))}
            _normalize_event_pages(detail["timeline"], "timeline")
            # Only caller-supplied exact-scope receipts can make reaction reads
            # necessary. Owner issue authorship or reaction counts grant nothing.
            if any(r.get("issue_id") == iid and r.get("kind") == "opening_post_plus_one" for r in receipts):
                detail["reactions"] = collection(intake.endpoint(issue["number"], "/reactions"))
                _normalize_event_pages(detail["reactions"], "reaction")
            result["details"][str(iid)] = detail
        for cancellation in cancellations:
            iid = cancellation.get("issue_id")
            if (cancellation.get("repository_id") != intake.REPOSITORY_ID or type(iid) is not int or iid not in issues
                    or not isinstance(cancellation.get("actor"), dict) or not intake.owner(cancellation["actor"])
                    or not isinstance(cancellation.get("scope_hash"), str)
                    or not re.fullmatch(r"[0-9a-f]{64}", cancellation["scope_hash"])
                    or not isinstance(cancellation.get("evidence_ref"), str) or not cancellation["evidence_ref"].strip()):
                raise ValueError("invalid authenticated owner cancellation")
            normalized = {k: copy.deepcopy(cancellation[k]) for k in ("actor", "scope_hash", "evidence_ref")}
            normalized["event"] = "owner_cancelled"
            result["details"][str(iid)]["timeline"]["pages"][-1]["items"].append(normalized)
        # Recheck every issue after all detail/timeline reads. GitHub REST is not
        # transactional; changes observed within the read window invalidate it.
        for iid, listed in sorted(issues.items()):
            final = _issue(get(ORIGIN + intake.endpoint(listed["number"])).body)
            if "pull_request" in final or not _same_issue(result["details"][str(iid)]["issue"], final):
                raise ValueError("issue changed during observation")
        result["read_status"] = "success"
    except (ValueError, TypeError, KeyError, AttributeError, OSError, URLError, TimeoutError):
        # Preserve no actionable partial data. Transport errors may contain token
        # strings or untrusted response bodies, so emit a stable generic reason.
        result.update(read_status="failed", reason="unknown: required GitHub/context read failed validation", details={})
    result["observation_finished_at"] = now()
    return result


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--context", type=Path, required=True, help="authenticated coordinator context JSON")
    parser.add_argument("--ledger", type=Path, help="existing ledger; requires --checkpoint")
    parser.add_argument("--checkpoint", type=Path, help="authenticated accepted checkpoint; requires --ledger")
    parser.add_argument("--capacity", type=int, default=0, help="verified free intake slots for dry-run; default none")
    parser.add_argument("--token-env", default="GITHUB_TOKEN", help="environment variable containing optional read token")
    args = parser.parse_args(argv)
    if bool(args.ledger) != bool(args.checkpoint):
        parser.error("--ledger and --checkpoint must be supplied together")
    try:
        context = json.loads(args.context.read_text())
        # Read continuity inputs before network work; absent or corrupt state is
        # unknown, never grounds for constructing an empty checkpoint.
        ledger = json.loads(args.ledger.read_text()) if args.ledger else None
        checkpoint = json.loads(args.checkpoint.read_text()) if args.checkpoint else None
        snapshot = observe(context, GitHubTransport(os.environ.get(args.token_env)))
        output = intake.evaluate(snapshot, ledger, checkpoint, capacity=args.capacity) if args.ledger else snapshot
        code = 2 if output.get("state") == "BLOCKED" or output.get("read_status") == "failed" else 0
    except (OSError, ValueError, TypeError):
        output = {"schema": 1, "state": "BLOCKED", "reason": "unknown: missing/corrupt input", "claim": None, "claims": []}
        code = 2
    print(json.dumps(output, sort_keys=True, indent=2, ensure_ascii=False))
    return code


if __name__ == "__main__":
    raise SystemExit(main())
