# Stage 5 merge for F08 BF-004

Branch: `cursor/f08-bf004-stage5-fb90`
Stage-5 owner: `bc-1940db7e-48d9-5cd0-9460-96008b16579d` (desktop; owner-visible)
Audited / validated source SHA (pre-BF-004): `6df0f81fbbe0adc3207f67d912d2fe0ab676be04`
Audited firmware reference SHA: `1325f5d34340ea877e65ab86f3663f5dea052c0d`
Validated **source** SHA (merge / install / stage-7 candidate): `8a74ff52cba842c3f2d8365600286f94e6bccdf0`
Content tree at validation commit (non-`.md`, matches Elite/Pro records): `1a1e7a22d136fef6e30702c9d3207d22798a6dd382717acc5ab0f40d2f235acf`
Firmware tree at validation commit: `581d1cb213e6ceda3e9fbbcdc324ef8dce67c42f`
Audited firmware content tree (pre-BF-004 package bytes baseline): `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`
Evidence-bearing tip (this branch; docs/logs/records only — **not** an install commit): stage-5 tip on `cursor/f08-bf004-stage5-fb90` descended from stage-4 tip `aeb707e4`.
BF-004 repair commit: `ce690772` (engine blob `ab6d9667e9cd40ae12cc8c8ece8d7bb12be29216`)
Package validation commit: `8a74ff52cba842c3f2d8365600286f94e6bccdf0`
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001)
DEV CHECKED records authenticate the tree of `8a74ff52`, not a later tip that only adds evidence. Fast-forward and stage-6 install use the validated source SHA.

## Package identities (from stage-4 DEV CHECKED)

| Model | Result | Package | Package SHA-256 | Controller SHA-256 | pool_free |
| --- | --- | --- | --- | --- | --- |
| elite | DEV CHECKED | `x0x-smk37-elite.fwsc` | `aab3cd8e245dedaf561cd973b3ce05aac33eb84ba0077b9b81cb1d9aa3b260b4` | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` | 8324 |
| pro | DEV CHECKED | `x0x-smk37.fwsc` | `066e2b1aeb6ed4abc8b3fba48606eaec1b42aa029a29ce07ffb0049ad7064a1e` | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` | 8324 |

Records: `evidence/f08-stage4/elite/record.json`, `evidence/f08-stage4/pro/record.json` (both `commit` = `8a74ff52…`, `tree` = `1a1e7a22…`).
Host tests: `evidence/f08-stage3/CHECKS.md` (ALL_PASS).
Package SHA-256 differ from F07 attested bytes (firmware tree changed for BF-004). Controller SHA-256 unchanged.

## Tree gate

`tools/dev_validate.py::tree` (same non-`.md` scope as `require_gate_pass.sh` / gate):

| Commit | Role | Tree matches records? |
| --- | --- | --- |
| `8a74ff52cba842c3f2d8365600286f94e6bccdf0` | validated source | **ACCEPT** (`1a1e7a22…`) |
| `ce690772` | BF-004 repair | repair tree differs only by post-repair stage-2 JSON evidence; firmware tree same `581d1cb2…` |
| `6df0f81fbbe0adc3207f67d912d2fe0ab676be04` | audited pre-BF-004 | audited ref (`69ccb3d7…`; not package-record tree) |
| `aeb707e4a50a425ff195c86a922d0d0fa9dc3d67` | evidence-bearing stage-4 tip | **REFUSED** (another source tree `847d5ce0…`) |

Log: `evidence/f08-stage5/gate-tree-check.log`.

Package/controller bytes are intentionally out of Git. Full `tools/require_gate_pass.sh --dev-record … --fwsc … --ctl …` needs those local bytes with the recorded SHA-256 values; the install commit argument must be the validated source SHA above. Against an evidence-only tip, verify fails with `development evidence is for another source tree` before artifact digest.

## Nightly fast-forward

Attempted: `git push origin 8a74ff52cba842c3f2d8365600286f94e6bccdf0:refs/heads/nightly` (no `--force`).

| Field | Value |
| --- | --- |
| Result | **REFUSED** non-fast-forward |
| `origin/nightly` before/after | `08d3164a265123cb4c84e5104642fd93257d63d7` (unchanged) |
| Merge-base with validated | `c1e83ab9df8b1e64fc69829394e92dbae24bc80f` |
| Commits on validated not in nightly | 73 |
| Commits on nightly not in validated | 160 |
| Force-push | **not performed** |

Logs: `evidence/f08-stage5/nightly-ff-refusal.log`, `nightly-ff-summary.txt`.

Stage 7 and the next audit still use validated source SHA `8a74ff52…`, not the diverged nightly tip.

## Project PR merge

Proven stage-4 PR [#232](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/232): `tools/merge_project_prs.py --pr 232` hit `Resource not accessible by integration` on retarget to `main`. Merged into existing base `cursor/f08-bf004-stage3-fb90` without retarget (`gh pr merge 232 --merge`). Merge commit `18c1a516…`. Logs: `merge-pr232-dry-run.log`, `merge-pr232.log`.

## Production

Production `main` and production GitHub Pages: **unchanged**. No device flash. Deferred items in `evidence/f08-stage2/DEFERRED.md`: not implemented. BF-005 / BF-006 / BF-007 not taken.

## Successor create (API)

`POST https://api.cursor.com/v1/agents` attempted twice with no `CURSOR_API_KEY`.

| Attempt | Result |
| --- | --- |
| 1 | `NO_CURSOR_API_KEY` |
| 2 | `NO_CURSOR_API_KEY` |

Logs: `api-create-stage7-attempt1.json`, `api-create-stage7-attempt2.json`.
Coordinator CreateAgent launched owner-visible Stage 7: `bc-bcce5c32-d708-521d-8a09-7d4547fb5ca5` (https://cursor.com/agents/bc-bcce5c32-d708-521d-8a09-7d4547fb5ca5; `startingRef` `cursor/f08-bf004-stage5-fb90`). Handoff complete on the remote tip. Do not create another Stage 7.
PR [#233](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/233) subscribed for local-env review.

## Overall

Merge gate complete for this stage: nightly refusal recorded without force-push; validated source SHA and package/controller hashes recorded for stage 6; evidence tip kept distinct from the install/merge SHA; owner-visible Stage 7 successor id on the remote.
