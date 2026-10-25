# Stage 5 merge for F09 BF-005

Branch: `cursor/f09-bf005-stage5-fb90`
Stage-5 owner: `bc-984a1ae4-5c90-5679-a709-70f06fef126a` (desktop; owner-visible)
Audited / validated source SHA (pre-BF-005): `8a74ff52cba842c3f2d8365600286f94e6bccdf0`
Audited firmware reference SHA: `1325f5d34340ea877e65ab86f3663f5dea052c0d`
Validated **source** SHA (merge / install / stage-7 candidate): `d0d00da23c3ce2268f06f31f8c062716d5a37ad0`
Content tree at validation commit (non-`.md`, matches Elite/Pro records): `62339630bc6e133da464eed7924645c0bfc09e4de66a8bccafdc60fd2d660c83`
Firmware tree at validation commit: `32a36f68d1ce7a3c304b447475afe7e2bbb116cf`
Audited firmware content tree (pre-BF-005 package bytes baseline): `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`
Content tree (pre-repair package baseline): `1a1e7a22d136fef6e30702c9d3207d22798a6dd382717acc5ab0f40d2f235acf`
Firmware tree (pre-repair): `581d1cb213e6ceda3e9fbbcdc324ef8dce67c42f`
Evidence-bearing tip (this branch; docs/logs/records only — **not** an install commit): stage-5 tip on `cursor/f09-bf005-stage5-fb90` descended from stage-4 tip `0a290da4`.
BF-005 repair commit: `94f4a012` (sequencer blob `fd3f65306df166494d10facf8ddae058287cc17b`)
Package validation commit: `d0d00da23c3ce2268f06f31f8c062716d5a37ad0`
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001), `ce690772` (BF-004), `94f4a012` (BF-005)
DEV CHECKED records authenticate the tree of `d0d00da2`, not a later tip that only adds evidence. Fast-forward and stage-6 install use the validated source SHA.

## Package identities (from stage-4 DEV CHECKED)

| Model | Result | Package | Package SHA-256 | Controller SHA-256 | pool_free |
| --- | --- | --- | --- | --- | --- |
| elite | DEV CHECKED | `x0x-smk37-elite.fwsc` | `de0bb0773c0b6de17140b180f79ae0e157c1690fcc485e6ffba28ae299eebc77` | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` | 8324 |
| pro | DEV CHECKED | `x0x-smk37.fwsc` | `25bebb261432e56b20f24b2100e05c369b9d3e8c277811ac300bae1050bd5e40` | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` | 8324 |

Records: `evidence/f09-stage4/elite/record.json`, `evidence/f09-stage4/pro/record.json` (both `commit` = `d0d00da2…`, `tree` = `62339630…`).
Host tests: `evidence/f09-stage3/CHECKS.md` (ALL_PASS).
Package SHA-256 differ from F08 attested bytes (firmware tree changed for BF-005). Controller SHA-256 unchanged.

## Tree gate

`tools/dev_validate.py::tree` (same non-`.md` scope as `require_gate_pass.sh` / gate):

| Commit | Role | Tree matches records? |
| --- | --- | --- |
| `d0d00da23c3ce2268f06f31f8c062716d5a37ad0` | validated source | **ACCEPT** (`62339630…`) |
| `94f4a012` | BF-005 repair | repair tree differs (pre-stage-2/3 evidence after repair) |
| `8a74ff52cba842c3f2d8365600286f94e6bccdf0` | audited pre-BF-005 | audited ref (`1a1e7a22…`; not package-record tree) |
| stage-4 / stage-5 evidence tip | evidence-bearing tip | **REFUSED** (another source tree) |

Log: `evidence/f09-stage5/gate-tree-check.log`.

Package/controller bytes are intentionally out of Git. Full `tools/require_gate_pass.sh --dev-record … --fwsc … --ctl …` needs those local bytes with the recorded SHA-256 values; the install commit argument must be the validated source SHA above. Against an evidence-only tip, verify fails with `development evidence is for another source tree` before artifact digest.

## Nightly fast-forward

Attempted: `git push origin d0d00da23c3ce2268f06f31f8c062716d5a37ad0:refs/heads/nightly` (no `--force`).

| Field | Value |
| --- | --- |
| Result | **REFUSED** non-fast-forward |
| `origin/nightly` before/after | `08d3164a265123cb4c84e5104642fd93257d63d7` (unchanged) |
| Merge-base with validated | `c1e83ab9df8b1e64fc69829394e92dbae24bc80f` |
| Commits on validated not in nightly | 88 |
| Commits on nightly not in validated | 160 |
| Force-push | **not performed** |

Logs: `evidence/f09-stage5/nightly-ff-refusal.log`, `nightly-ff-summary.txt`.

Stage 7 and the next audit still use validated source SHA `d0d00da2…`, not the diverged nightly tip.

## Project PR merge

Proven stage-4 PR [#243](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/243): `tools/merge_project_prs.py --pr 243` hit `Resource not accessible by integration` on retarget to `main`. Merged into existing base `cursor/f09-bf005-stage3-fb90` without retarget (`gh pr merge 243 --merge`). Merge commit `44e8c5b172bcda7e8b658534816731f8a3a432dd`. Logs: `merge-pr243-dry-run.log`, `merge-pr243.log`, `merge-pr243-direct.log`, `merge-pr243-result.json`.
Stage-5 PR [#244](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/244) opened on base `cursor/f09-bf005-stage4-fb90` (subscribe for local-env review).

## Production

Production `main` and production GitHub Pages: **unchanged**. No device flash. Deferred items in `evidence/f09-stage2/DEFERRED.md`: not implemented. BF-006 / BF-007 not taken. BF-001–BF-005 not reopened.

## Successor create (API)

`POST https://api.cursor.com/v1/agents` attempted twice with no `CURSOR_API_KEY`.

| Attempt | Result |
| --- | --- |
| 1 | `NO_CURSOR_API_KEY` |
| 2 | `NO_CURSOR_API_KEY` |

Logs: `api-create-stage7-attempt1.json`, `api-create-stage7-attempt2.json`.
Coordinator CreateAgent launched owner-visible Stage 7: `bc-bb6e5cd5-b939-53c7-a83d-6b012405e45c` (https://cursor.com/agents/bc-bb6e5cd5-b939-53c7-a83d-6b012405e45c; `startingRef` `cursor/f09-bf005-stage5-fb90`). Handoff complete on the remote tip. Do not create another Stage 7.
PR [#244](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/244) subscribed for local-env review.

## Overall

Merge gate complete for this stage: nightly refusal recorded without force-push; validated source SHA and package/controller hashes recorded for stage 6; evidence tip kept distinct from the install/merge SHA; owner-visible Stage 7 successor id on the remote.
