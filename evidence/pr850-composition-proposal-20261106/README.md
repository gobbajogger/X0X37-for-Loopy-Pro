# PR850 unselected composition evidence

This evidence-only delivery supplies a proposed patch to the existing PR850 owner and assigned independent reviewer. It does not update the owner branch, select a candidate, establish qualification, transfer ownership, or authorize merge/publication/device work. The proposed source remains unselected and uncommitted; the separate evidence branch stores this packet only.

Base commit: `c055f15c350aa259a24e63db6df4bc0bc063415b`.
Base tree: `d97b972a1b4141dd02fac2b6ccb5d3456250129f`.
Donor repair: `727e5c0dabae218f728966705a792e990b5efc00`.
Proposed full source tree: `0020995601b23b6631c23f98b78496bd3df6fbbf`.

The proposed tree is derived from all authenticated base-tree entries plus 61 local changed blobs. The original index reproduced the exact base tree before overlay. This was a scoped checkout, not a complete firmware build. `identity.json` records all changed paths, blob hashes and modes. The evidence commit/tree is a separate identity, not the proposed source tree.

## What was composed

The patch reuses strict-v2's 203-ID inventory, 57 missing UNPROVEN JSON records, trusted ROADMAP-byte guard, mandatory Master/extras checks, CLI semantics and matching tests. It preserves PR850's metadata/path fixes, all eight review-fix negatives, hardened runner-token/here-doc parsing and all eight parser negatives. Existing 146 JSON files remain byte-identical. `tests/run_tests.sh` remains blob `683837cbb2539a7215ef9aed2fdadee00e532466`, preserving optimization-equivalence and lz-max-match coverage. No foreign runner or firmware source changes are included.

A new real-pin regression deletes `#8` from both manifest and JSON. It requires exit 1, exactly one failing row `#8`, missing JSON diagnostics, and `rows=202 required=203`.

## Executed results

- Combined checker self-test: exit 0, **67 PASS / 0 FAIL / no SKIP**.
- Patch dry-run, replay and whitespace validation: exit 0. Replay matches all 61 proposed files; other scoped files remain unchanged.
- Structural report: exit 0; 2 PRESENT_OK, **201 UNPROVEN**, 203 rows.
- Actual strict invocation with the trusted historical ROADMAP: exit 1; FAIL rows=203 required=203. Besides the 201 UNPROVEN rows, **#66 and #68 have source_sha_mismatch**: their historical receipts cite `ce27812951555ff7ed9694bf99d20a65dd5f410b` rather than the published c055 base used for this diagnostic.
- Default ROADMAP invocation: exit 2, correctly rejecting current docs/ROADMAP.md blob `a8c452c9486af461425c0459eec841a29a0088d3` instead of silently changing the trusted pin.

The diagnostic candidate SHA is the existing published c055 base, not a claim that this uncommitted proposal has a commit SHA. Its exact source identity is the proposed tree above. All generated PRESENT objects in the self-test are synthetic fixtures, never firmware evidence.

The first composition attempt selected an incorrect text boundary and dropped a guard diagnostic; existing tests caught it (66 PASS / 1 FAIL). Correcting that boundary produced the final 67/0 result; no assertion was weakened. Restoring authenticated executable modes on API-materialized scripts removed patch-replay mode warnings without source changes.

## Reproduction and next owner action

On an isolated checkout at c055, inspect `composition.patch`, then run:

```sh
git apply --check --whitespace=error /path/to/composition.patch
git apply /path/to/composition.patch
sh tests/host/roadmap_evidence_check_test.sh
sh tests/host/roadmap_evidence_check.sh .
sh tests/host/roadmap_evidence_check.sh --strict \
  --candidate-sha c055f15c350aa259a24e63db6df4bc0bc063415b \
  --roadmap ROADMAP.pinned.md .
```

Before the self-test, retrieve docs/ROADMAP.md at `5f578fc0642a1200a5163802bcd69277c511bfc7` using the GitHub API and save it at checkout root `ROADMAP.pinned.md`. Verify Git blob `a204a54c989d6f323d619e70f799427603756541`. This external test input is excluded from the patch/proposed tree. Without it, real-pin cases explicitly skip; that is not equivalent to the attached 67-case run. Expected report/self-test exits are 0; actual strict exit is 1.

The existing PR850 owner selects or amends the patch and returns its committed immutable head/tree and affected evidence to the existing assigned QA and sole PR698 integrator. Do not relabel historical #66/#68 receipts or UNPROVEN records. Existing full-suite/native receipts at c055 were reused as recorded history, not rerun or newly accepted. No full host/native/package/device qualification is claimed here.

Related handoff: https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/850#issuecomment-6676545076
