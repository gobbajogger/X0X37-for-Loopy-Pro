# M12 Independent Acceptance Adapter Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox syntax for tracking.

**Goal:** Authenticate an explicit stage's evidence against independently supplied immutable candidate, canonical ROADMAP and accepted-policy identities, without changing existing structural checks or release entry points.

**Architecture:** Add a standalone, offline Python standard-library CLI. It reads and hashes actual Git objects, maps selected current canonical queue records, derives required obligations from the pinned policy and checks exact source bytes or independently pinned accepted receipts. The calling integrator remains responsible for independently accepting the expected pins, policy and receipts; this tool authenticates their consistency, not who approved them or whether arbitrary claimed tests really ran.

**Tech Stack:** Python 3.12+, Git object reads, unittest; no network, dependency installation or shell command execution from evidence.

**Spec:** [Reviewed M12 proposal](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/9d00e0e0faa8e06cbf4c340b3cfc3e7993070375/docs/evidence/m12-acceptance-20261114/PROPOSAL.md), with REPORT.md, REVIEW.md and the three intentionally red boundary probes at that same commit. Root assigned this source-only generic implementation with synthetic fixture policies. It does not select a production policy or adopt publisher/package hooks.

## Global Constraints

- Base: canonical nightly `83fc6d60289869116f93352cf845a9a7e271cdba`; isolated branch `wip/m12-independent-acceptance-adapter-20261114`.
- One source writer: the assigned M12 Dot agent. Existing firmware integration and package/publisher owners retain their files and authority.
- No firmware, existing checker, existing runner, package, preview, publisher, integration ref, production policy or device changes.
- Deferred #119, declined 26.7 and undefined M5 must not become required selected work. Unselected canonical records remain visible rather than deleted or called complete.
- Source, behavior, host, native and device evidence levels remain separate. Missing proof is NOT_RUN; FAIL remains FAIL. A source-only result cannot satisfy a higher-level obligation.
- Repository delivery uses the connected account's default identity and an isolated non-main branch, with exact remote readback. No force push.

## Review Focus

- Self-consistent forged pins and reduced inventories must not replace the independent expected inputs or required selection.
- Missing/corrupt/replaced Git objects, symlinks, path traversal and duplicate JSON keys/record IDs must fail closed.
- Current queue headings, original source-section identity and original row identity must disambiguate request IDs from PR numbers in prose.
- A PASS string is not execution proof. Non-source proof needs an independently pinned accepted receipt and complete policy-declared input closure.
- Working-tree mutation, stale tool/report inputs and altered receipt/config/dependency bytes must not masquerade as the selected immutable source.

## Task: Standalone validator and its regression suite

**Files:**
- Create `tools/m12_acceptance.py`: CLI, authenticated read-only Git object access, canonical-record parser and evidence validation.
- Create `tests/host/test_m12_acceptance.py`: disposable real-Git integration fixtures and CLI negatives.
- Create `docs/engineering-delivery/m12-adapter-20261114/README.md`: schema, use, trust boundary, exact scope and downstream adoption requirements.
- Create sanitized verification/review receipts in that same delivery directory after exact candidate validation.

**Interface:** Required external CLI inputs are repository path, full candidate commit/tree, full ROADMAP commit/root blob/path, full policy commit/blob/path, expected adapter SHA-256 and evidence JSON path. JSON output goes to stdout and the exit status is nonzero unless every selected required obligation passes. The policy pins all canonical queue blobs and accepted non-source receipt/input identities; fixture policies are not product-release approval.

- [ ] Write the smallest failing CLI positive/negative fixture tests before implementation and retain the expected failure output.
- [ ] Implement authenticated Git-object reads and pin/report binding. Read actual immutable blobs, not mutable working-tree source or Git replacement objects.
- [ ] Implement current canonical queue record selection, duplicate rejection, held-request protection and required obligation inventory.
- [ ] Implement source-byte proof and exact independently pinned accepted-receipt/input-closure validation. Retain negative and unrun statuses.
- [ ] Extend regression tests for all three original gaps, unchanged-valid input, wrong/missing/changed policy, canonical-layout ambiguity, stale/altered report and receipt, unavailable objects, path safety and evidence-level separation. Observe new tests fail before each corresponding behavior is added.
- [ ] Run authoritative focused tests and syntax checks, then freeze all changed bytes for one independent Sol ultra review and executable validation. Correct warranted findings and recheck affected scope with the same reviewer.
- [ ] Deliver only the independently reviewed source/docs/tests on the authorized branch and verify remote parent/tree/path/mode/blob/bytes. Route the immutable result to the existing firmware release-readiness consumer and verify its substantive intake.

**Acceptance:** Synthetic integration tests exercise real Git objects and the actual CLI. The three previously false-acceptance cases are rejected; valid independent policy/source proof is accepted; source presence cannot qualify host/native/device obligations. Review accepts the exact frozen change. Remote source matches it and the existing consumer performs a matching first use. No actual release-policy, product qualification or publisher adoption is claimed.
