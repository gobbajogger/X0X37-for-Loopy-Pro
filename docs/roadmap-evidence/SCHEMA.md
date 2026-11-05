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
| `status` | yes | `PRESENT` or `UNPROVEN` (`UNPROVEN` counts as MISSING at the gate) |
| `code_paths` | PRESENT | Repo-relative file paths that implement the row |
| `test_refs` | PRESENT | List of `{file, test, result, source_sha}` |
| `proof_source` | yes | Citation: where the proof was recorded (status line, #832 comment id, blob SHA, …) |
| `notes` | no | Free text |

`test_refs[].result` is exactly `PASS`, `FAIL`, or `NOT_RUN` (never empty).
`test_refs[].source_sha` is the commit/run the result was recorded against.
UNPROVEN rows may omit `code_paths` and `test_refs` (or leave them empty).

## PRESENT rule (mechanical)

An entry is PRESENT only when: the JSON is valid, required fields are set,
every `code_path` exists in the checked tree, every `test_ref` has a non-empty
result and a source SHA, and all evidence files agree on the pin SHAs.
The check script does not invent proofs; Dev writes them at admission.
