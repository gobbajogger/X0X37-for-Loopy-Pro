# ROADMAP evidence schema (per-row)

One file per required row: `docs/roadmap-evidence/<row-id>.json`.
Pinned ROADMAP: commit `5f578fc0642a1200a5163802bcd69277c511bfc7`,
blob `a204a54c989d6f323d619e70f799427603756541`. Pin never moves silently.

## Object fields

| Field | Required | Meaning |
|---|---|---|
| `row_id` | yes | Tracker id (`#1`, `DONE-001`, `FIX-UI-TILES`, …); must match the filename stem |
| `roadmap_pin_commit` | yes | Full SHA of the pinned ROADMAP commit |
| `roadmap_blob` | yes | Full SHA of the pinned `docs/ROADMAP.md` blob |
| `status` | yes | `PRESENT` or `UNPROVEN` (`UNPROVEN` counts as MISSING; strict rejects it) |
| `code_paths` | PRESENT | Repo-relative file paths that implement the row |
| `test_refs` | PRESENT | List of `{file, test, result, source_sha}` |
| `proof_source` | yes | Citation: where the proof was recorded (status line, #832 comment id, blob SHA, …) |
| `notes` | no | Free text |

`test_refs[].result` is exactly `PASS`, `FAIL`, or `NOT_RUN` (never empty).
`test_refs[].source_sha` is the commit/run the result was recorded against.
UNPROVEN rows may omit `code_paths` and `test_refs` (or leave them empty).

## Reporting mode (default, unchanged)

```
sh tests/host/roadmap_evidence_check.sh [tree_root] [evidence_dir]
```

Defaults: `tree_root=.` and `evidence_dir=<tree_root>/docs/roadmap-evidence`.
This is a structural report: PRESENT requires valid JSON, required fields,
existing `code_paths` files, and complete `test_refs` with result `PASS`, `FAIL`,
or `NOT_RUN` and a nonempty source SHA. Pins must agree. It does not check
test file existence, test registration, candidate SHA, or required-row coverage.
UNPROVEN counts as MISSING but does not itself cause failure. PRESENT with FAIL
or NOT_RUN is structurally accepted. These legacy behaviors and output are kept
for existing callers; reporting exit 0 is not final-gate qualification.

## Strict final gate (opt in)

```
sh tests/host/roadmap_evidence_check.sh --strict --candidate-sha ce27812951555ff7ed9694bf99d20a65dd5f410b tree docs/roadmap-evidence
X0X_CANDIDATE_SHA=ce27812951555ff7ed9694bf99d20a65dd5f410b sh tests/host/roadmap_evidence_check.sh --strict tree docs/roadmap-evidence
```

`--strict` must be first; optional `--candidate-sha <40hex>` precedes the paths
and overrides `X0X_CANDIDATE_SHA`. A missing or invalid candidate SHA is a setup
error (exit 2). The candidate is the tested source revision, separate from the
ROADMAP pin. SHA comparison accepts either hexadecimal letter case.

`REQUIRED_ROWS.txt` is the inventory for the pinned ROADMAP above: one row id per
line, no blank lines or comments. Its 146 ids were generated from the current
JSON filename stems. Keep this manifest under review with the pin; do not shrink
it to make a partial evidence set pass. Synthetic self-test fixtures use their
own one-row manifests.

Strict requires:

- A readable, nonempty manifest with unique ids, every required JSON file, and
  no additional `*.json` files (including hidden and underscore-prefixed names).
- Valid JSON objects, required fields, ids matching filename stems, valid pin
  SHAs, and agreement on both ROADMAP pin SHAs.
- Every required row status exactly `PRESENT`; UNPROVEN fails even when it is
  the only missing proof.
- Nonempty `code_paths` and `test_refs` for PRESENT rows. Every recorded test
  result must be exactly `PASS`, and every source SHA must match the candidate.
- Every supplied code path and test file must resolve to a regular file under
  the tree root. Absolute paths, parent escapes and symlinks outside it fail.
- Every test name must match a literal `run <test>` entry in
  `<tree_root>/tests/run_tests.sh`. Inline `else run <test> ...` entries count; comments and quoted command text
  do not register tests. The checker
  reads this file; it never executes the runner or any tests. Quoting and line
  continuations retain their shell context. Here-documents and unquoted backticks
  are unsupported and fail closed for the whole registry.

Strict emits one `STRICT_FAIL <row>: <reason>` line per offending row, combining
multiple reasons on that line. Manifest errors use `REQUIRED_ROWS` as the row.
The final line is `STRICT_RESULT FAIL|PASS rows=N required=M`, where N counts
JSON files found and M counts distinct required ids. Evidence failures exit 1;
complete strict evidence exits 0. Missing evidence directories remain setup
errors (exit 2). Empty or missing required evidence fails strict with exit 1.

Today's pinned evidence is expected to fail strict: 144 of 146 rows are
UNPROVEN. This tooling change does not upgrade their evidence status.
The check script does not invent proofs; Dev writes them at admission.
