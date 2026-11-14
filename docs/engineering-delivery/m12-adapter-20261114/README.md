# M12 independent-input acceptance adapter

`tools/m12_acceptance.py` is an offline, read-only validator for an explicitly selected policy. It closes the three reproduced M12 consistency gaps: a reduced local inventory, substituted source bytes and internally consistent but independently wrong ROADMAP pins. It preserves the existing structural checker and its tests. No package, preview, publisher or firmware entry point calls this adapter yet.

This source stage supplies generic tooling and synthetic tests. It selects **no actual product-release policy**. A successful result means the selected policy's evidence is consistent with the supplied immutable trust roots. It does not mean the product is complete, tests have run, a release is approved, or hardware works.

## Trust boundary

The integrator must independently resolve and accept these inputs before invocation:

- Candidate commit and its root tree.
- Canonical ROADMAP commit, root path/blob and all three queue blobs.
- Accepted stage-policy commit, path and blob.
- This adapter's SHA-256.
- For non-source results, accepted receipt identities and their declared test/tool/config/dependency source closure.

Do **not** derive expected pins or acceptance policy from the evidence report being checked. A caller who chooses its own false trust roots can obtain a self-consistent result; this CLI does not authenticate the caller, owner approval or the provenance of an arbitrary receipt. A pinned receipt proves exact reuse of the independently accepted bytes, not that a command really executed. The integrator must authenticate that execution evidence separately before accepting it into policy.

The validator recomputes Git object hashes, reads source from immutable Git trees rather than working-tree files, ignores Git replacement objects and disables lazy fetch. Missing or corrupt objects refuse validation. Paths must be relative regular files; symlinks, traversal and missing paths refuse validation. It does not execute receipt commands or perform network, checkout, Git-write, publication or device operations.

Working-tree changes are intentionally irrelevant to the immutable-source result. Before building or packaging from a working tree, its owner must independently prove that those consumed bytes match the selected Git closure. This tool does not certify an arbitrary working directory.

## Invocation

Python 3.12+ and Git are required. JSON goes to stdout. Exit 0 means every required selected obligation passed; exit 1 means invalid, failed or unrun evidence. Argument syntax errors use argparse's exit 2.

```sh
python3 tools/m12_acceptance.py \
  --repo /path/to/object-complete/repository \
  --candidate CANDIDATE_COMMIT --candidate-tree CANDIDATE_TREE \
  --roadmap-commit ROADMAP_COMMIT --roadmap-path docs/ROADMAP.md \
  --roadmap-blob ROADMAP_ROOT_BLOB \
  --policy-commit ACCEPTED_POLICY_COMMIT --policy-path policy.json \
  --policy-blob ACCEPTED_POLICY_BLOB \
  --tool-sha256 INDEPENDENTLY_EXPECTED_ADAPTER_SHA256 \
  --evidence /path/to/evidence.json
```

All Git pins are full lowercase SHA-1 identities. SHA-256 values are full lowercase digests. Repository source does not need to be checked out, but all objects in the selected closure must already be available. This is not an input download helper.

## Policy schema

The following is a **synthetic shape**, not an accepted DnBOX release selection. Uppercase values are placeholders.

```json
{
  "schema": "dnbox-m12-policy-v1",
  "stage": "source-only-fixture",
  "queues": [
    {"path": "docs/roadmap/Complete.md", "blob": "COMPLETE_BLOB"},
    {"path": "docs/roadmap/Working.md", "blob": "WORKING_BLOB"},
    {"path": "docs/roadmap/Pending.md", "blob": "PENDING_BLOB"}
  ],
  "selected": [{
    "request_id": "#1",
    "source_section": "Owner request list (#1–#122)",
    "row": "#1",
    "obligations": [{
      "id": "source",
      "level": "source",
      "paths": ["firmware/feature.c"]
    }]
  }]
}
```

The root ROADMAP must link all three canonical queue files. Each selected request is matched to one unique `##` canonical record, its `Preserved source section` identity and one exact original table-row first cell. The explicit original row supports reviewed historical aliases without interpreting arbitrary PR numbers in prose as request IDs. Duplicate canonical records, missing/ambiguous rows, duplicate policy/report obligations and duplicate JSON keys refuse validation. Code-fenced examples do not create records.

Non-table canonical records remain visible but cannot be selected by this table-backed schema. At the pinned current ROADMAP this applies to COMP1. A future policy requiring COMP1 needs an independently reviewed non-table identity extension; leaving it unselected is not permission to waive an already-required obligation.

The accepted policy, not `REQUIRED_ROWS.txt` or the evidence's row count, defines every required obligation. Missing evidence produces `NOT_RUN` and a nonzero exit. Unselected records remain listed with their current disposition; they are not deleted or called complete. Current owner-held IDs `#119`, `26.7` and `M5` cannot be selected by this implementation. The tool cannot reopen deferred/declined work or define M5.

Levels are `source`, `behavior`, `host`, `native` and `device`. An arbitrary nonempty `stage` label travels with the result and accepted receipts; it does not itself grant permission or qualification. Policy input closure paths must be nonempty and unique.

## Evidence schema and source proof

```json
{
  "schema": "dnbox-m12-evidence-v1",
  "candidate_commit": "CANDIDATE_COMMIT",
  "candidate_tree": "CANDIDATE_TREE",
  "roadmap_commit": "ROADMAP_COMMIT",
  "roadmap_blob": "ROADMAP_ROOT_BLOB",
  "policy_commit": "ACCEPTED_POLICY_COMMIT",
  "policy_blob": "ACCEPTED_POLICY_BLOB",
  "tool_sha256": "EXPECTED_ADAPTER_SHA256",
  "rows": [{
    "request_id": "#1", "obligation_id": "source", "status": "PASS",
    "proof": [{"path": "firmware/feature.c", "blob": "SOURCE_BLOB", "sha256": "SOURCE_SHA256"}]
  }]
}
```

Every report input must match the externally supplied pins. A source proof must cover exactly the policy-declared paths and match both the actual candidate Git blob and its SHA-256. This proves the selected bytes exist in the selected immutable source. It does not prove those bytes implement the intended feature or pass a behavioral test.

`FAIL` and `NOT_RUN` are retained, never promoted by file existence or another evidence level. Unknown statuses refuse validation. Unknown/unselected report obligations are rejected instead of being used as extra acceptance claims. A missing required row stays visible as `NOT_RUN` even if an evidence producer shrinks its own inventory.

## Accepted non-source receipts

For a non-source obligation, policy must independently pin a receipt:

```json
{"commit":"RECEIPT_COMMIT","path":"receipt.json","blob":"RECEIPT_BLOB","sha256":"RECEIPT_SHA256"}
```

Place this object in the obligation's `receipt` field and the corresponding evidence row's `receipt` field. They must match exactly. The pinned receipt JSON has:

- `schema: "dnbox-m12-receipt-v1"`.
- Exact `stage`, `request_id`, `obligation_id`, `level` and `status`.
- Exact `candidate_commit`, `candidate_tree`, `roadmap_commit` and `roadmap_blob`.
- Nonempty `command`, observed integer `exit_code: 0` for PASS.
- `inputs`: the same path/blob/SHA-256 objects as source proof, covering exactly the policy-declared source/test/tool/config/dependency paths.

The receipt is independently pinned by policy rather than claiming that policy's own digest, avoiding a circular hash dependency. Without a pinned accepted receipt, a claimed non-source PASS becomes `NOT_RUN`. Source proof cannot satisfy it. Negative/unrun statuses remain negative/unrun. Closure completeness and receipt authenticity remain responsibilities of the independent policy/evidence review; listing one runner is not proof that all dependencies or execution were verified.

## Validation and remaining adoption

```sh
python3 tests/host/test_m12_acceptance.py -v
python3 -m py_compile tools/m12_acceptance.py tests/host/test_m12_acceptance.py
```

The tests build disposable real Git objects and run the actual CLI. They use synthetic policies and receipts only, not production features or device evidence. Exact current three-queue parsing is also checked separately against #887 at `96070f0b328c8375ce4946e07da9fddb1f11c820`: 187 canonical records and all 122 numbered requests, without duplicate IDs.

The independent review and verification receipt accompany the frozen source. No full firmware suites, SDK builds, emulator, device checks, package/preview hooks or uploader enforcement are established by these focused tests. Existing owners must choose and independently accept actual stage policies, authenticate their receipt closure and adopt the adapter at their own boundaries. The sole publisher must separately prove zero external writes on rejected inputs before any real publication gate can be claimed. Production/device rules remain unchanged.
