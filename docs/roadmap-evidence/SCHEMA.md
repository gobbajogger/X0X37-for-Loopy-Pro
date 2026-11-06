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
| `queued` | stubs | `true` for evidence queued without source proof |
| `reason` | stubs | Explicit `QUEUED: no source proof yet` |

`test_refs[].result` is exactly `PASS`, `FAIL`, or `NOT_RUN` (never empty).
`test_refs[].source_sha` is the commit/run the result was recorded against.
UNPROVEN rows may omit `code_paths` and `test_refs` (or leave them empty).

## Reporting mode (default, unchanged)

```
sh tests/host/roadmap_evidence_check.sh [tree_root] [evidence_dir]
```

Defaults: `tree_root=.` and `evidence_dir=<tree_root>/docs/roadmap-evidence`.
This is a structural report: PRESENT requires valid JSON, required fields,
in-tree, repo-relative `code_paths` files, and complete `test_refs` with result `PASS`, `FAIL`,
or `NOT_RUN` and a nonempty source SHA. Pins must agree. It does not check
test file existence, test registration, candidate SHA, or required-row coverage.
Required metadata and filename/row-id agreement are checked for UNPROVEN too.
Malformed UNPROVEN rows retain their classification/count, report their schema
errors, and cause failure. Absolute paths and paths resolving outside the tree
(including symlinks) fail in both modes.
UNPROVEN counts as MISSING but does not itself cause failure. PRESENT with FAIL
or NOT_RUN is structurally accepted. These legacy behaviors and output are kept
for existing callers; reporting exit 0 is not final-gate qualification.

## Strict final gate (opt in)

```sh
sh tests/host/roadmap_evidence_check.sh --strict --candidate-sha ce27812951555ff7ed9694bf99d20a65dd5f410b --roadmap /path/to/ROADMAP.pinned.md tree tree/docs/roadmap-evidence
X0X_CANDIDATE_SHA=ce27812951555ff7ed9694bf99d20a65dd5f410b sh tests/host/roadmap_evidence_check.sh tree tree/docs/roadmap-evidence --strict --roadmap /path/to/ROADMAP.pinned.md
```

Strict flags may occur anywhere among the arguments. The two optional operands
are tree root and evidence directory, in that order, with the reporting defaults.
Unknown flags, extra operands, and missing option values exit 2. Without a
`--strict` argument the legacy reporting behavior and argument handling stay
unchanged. `--candidate-sha <40hex>` overrides `X0X_CANDIDATE_SHA`; a missing or
invalid candidate exits 2. SHA comparison accepts either hexadecimal letter case.

`--roadmap <file>` supplies the actual pinned ROADMAP bytes. Its default is
`<tree_root>/docs/ROADMAP.md`. If that file has changed since the pinned
revision, supply a separately retrieved copy of the pinned bytes with
`--roadmap`; the default must fail rather than accept a newer ROADMAP.
The checker computes the Git blob SHA-1 of
`b"blob " + ASCII(byte_length) + b"\0" + file_bytes`, without git or network.
It must equal the in-script constant
`a204a54c989d6f323d619e70f799427603756541`. Missing files and different bytes
exit 2 with `roadmap file missing or not the pinned blob`; JSON pin claims cannot
substitute for this check. Optional `--roadmap-pin <40hex>` and
`X0X_ROADMAP_PIN` assert the caller's expected pin. Each supplied assertion must
match the constant, including when both are supplied; neither replaces it.
A repeated `--roadmap-pin` or a differing/invalid assertion exits 2.
Every evidence object's `roadmap_blob` must also match the checker constant.
Both JSON pin fields must be valid SHA values and agree across evidence objects.

### Mandatory inventory and file naming

Within the `## Master tracker` section, including its subordinate headings and
all its tables, take the first cell of each table row. Extract a leading id
matching `(#\d+|[LM]\d+)(?=\s|$)` after stripping surrounding cell whitespace.
Stop at the next level-two heading. The real pinned file has 149 unique ids:
`#1`–`#122`, `L1`–`L14`, and `M1`–`M13`. Alias descriptions such as L6 still
require their own id. Auxiliary provenance text, `PR #...` source labels,
`FX41-REC`, and `QA1`/`QA2` baseline-audit labels do not belong to these three
canonical tracker id families. The count is derived, never hardcoded by the
checker. The original file is the authority; the earlier parsed hint is unused.

An id is used verbatim as the JSON filename stem: `#66` maps to `#66.json`,
`L14` to `L14.json`, and `M1` to `M1.json`. No title, hash alias, or numbering
conversion is applied.

`REQUIRED_ROWS.txt` is an additional reviewed inventory, one id per line with
no blank lines or comments. It must cover every Master id and all 54 legacy
DONE/PR/FIX/ROW ids enumerated in the checker's `REQUIRED_EXTRAS` constant.
Those 54 ids preserve the original manifest's rows with their own sources;
they remain mandatory even if deleted from both the manifest and JSON. An id
outside Master plus these extras is a manifest error. The required set is the
union of Master, extras, and manifest; unknown manifest ids fail validation.
The updated manifest has 203 ids (149 Master + 54 extras). Shrinking the editable
manifest cannot reduce the trusted set.

Strict requires:

- A readable, nonempty manifest with unique ids and the coverage above.
- Every Master id has an evidence JSON file, independent of the manifest.
  Its absence emits `STRICT_FAIL <id>: master row has no evidence json`
  (other reasons may be combined on that line).
- Every required id has a JSON file, with no additional `*.json` files,
  including hidden and underscore-prefixed names.
- Valid JSON objects, required fields, ids matching filename stems, and pin
  validation as described above.
- Every required row has status exactly `PRESENT`. UNPROVEN fails even when
  it is the only missing proof.
- Nonempty `code_paths` and `test_refs` for PRESENT rows. Every recorded result
  is exactly `PASS`, and its full source SHA matches the candidate. Prefix
  SHAs, empty SHAs, FAIL, NOT_RUN, and SKIP all fail strict.
- Every supplied code path and test file resolves to a regular file under the
  tree root. Absolute paths, parent escapes and symlinks outside it fail.
- Every test name matches a literal `run <test>` entry in
  `<tree_root>/tests/run_tests.sh`. Inline `else run <test> ...` entries count;
  comments and quoted command text do not register tests. The checker reads
  the runner and never executes it or any recorded tests. Quoting and line
  continuations retain shell context. Here-documents and unquoted backticks
  fail closed for the whole registry.

Strict prints `STRICT_INFO master_rows=N`, the actual manifest bytes' SHA-256
as `STRICT_INFO manifest_sha256=...`, and failure row/reason counts. It emits
one `STRICT_FAIL <row>: <reasons>` line per offending row. Manifest-wide errors
use `REQUIRED_ROWS` as the row. The final line remains
`STRICT_RESULT FAIL|PASS rows=N required=M`, where N counts JSON files found
and M counts distinct required ids. Evidence failures exit 1; complete strict
evidence exits 0. Missing evidence directories and roadmap/pin/argument setup
errors exit 2. Empty or missing required evidence fails strict with exit 1.

The original 146 files report 2 PRESENT_OK and 144 UNPROVEN. The 57 newly queued
Master stubs all remain UNPROVEN, with `queued=true` and
`reason="QUEUED: no source proof yet"`. The expected updated report is 2
PRESENT_OK, 0 PRESENT_BAD, 201 UNPROVEN/MISSING, 203 rows. This inventory increase
is the only intended reporting output delta. No proof status is upgraded.

The self-test uses small synthetic ROADMAP tables and a disposable checker
copy with only its pin constant changed to those fixture bytes. It never adds
a production pin override. The 54 extras are retained in these fixtures.
If `../../ROADMAP.pinned.md` relative to the self-test directory or
`./work/ROADMAP.pinned.md` relative to the caller exists, a separate invocation tests that real blob against the unmodified checker pin;
otherwise it explicitly skips the real-pin, delete-#8-from-both, and empty
UNPROVEN cases. The real-pin deletion case must reject a fixture that removes
`#8` from both the manifest and evidence JSON. All generated
PRESENT objects are synthetic checker fixtures, not firmware proof. Temporary
trees are removed by the self-test trap; use TMPDIR to select their parent.
