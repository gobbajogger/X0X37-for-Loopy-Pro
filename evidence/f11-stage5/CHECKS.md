# Stage 5 merge for F11 BF-007

Branch: `cursor/f11-bf007-stage5-0e72`
Stage-5 owner: `bc-432a157b-5c8e-5b0e-8adf-47cddc020e72` (desktop; owner-visible)
Audited / validated source SHA (pre-BF-007): `af4947846df7233c68a6be72c4d0c634052a2752`
Audited firmware reference SHA: `1325f5d34340ea877e65ab86f3663f5dea052c0d`
Validated **source** SHA (merge / install / stage-7 candidate): `4f96b0d9f5f11f700ea3fc935800b8714e11f35d`
Content tree at validation commit (non-`.md`, matches Elite/Pro records): `f27c89e1ac8e11ab57c5ae7e49145ce19b964082bf3ca9a700a99bfa8a97091e`
Firmware tree at validation commit: `5b1a21f07563b0262ece6728051cb0436bd4bfa1`
Audited firmware content tree (pre-BF-005 package bytes baseline): `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`
Content tree (pre-repair package baseline): `1da3bbfbbf0df52256acceecbca2e064a1ef821478a33d78591e01813229643d`
Firmware tree (pre-repair): `bcc21733ea77097a787cfa6cc617fbdf1dca5da1`
Evidence-bearing tip (this branch; docs/logs/records only — **not** an install commit): stage-5 tip on `cursor/f11-bf007-stage5-0e72` descended from stage-4 tip `d601c97a`.
BF-007 repair commit: `78a0452e` (`project.c` blob `240a4b1735166dfb990b52c26686c7ea5ad4317e`)
Package validation commit: `4f96b0d9f5f11f700ea3fc935800b8714e11f35d`
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001), `ce690772` (BF-004), `94f4a012` (BF-005), `37bab7d1` (BF-006), `78a0452e` (BF-007)
DEV CHECKED records authenticate the tree of `4f96b0d9`, not a later tip that only adds evidence. Fast-forward and stage-6 install use the validated source SHA.

## Package identities (from stage-4 DEV CHECKED)

| Model | Result | Package | Package SHA-256 | Controller SHA-256 | pool_free |
| --- | --- | --- | --- | --- | --- |
| elite | DEV CHECKED | `x0x-smk37-elite.fwsc` | `829bb6da6eb0042af0731a236d6238e50cc13581ce059ebbe02882534a94b056` | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` | 8324 |
| pro | DEV CHECKED | `x0x-smk37.fwsc` | `d2186cb0f3fe74376651cb2804973c8255780532a8cd54c33d2bac09aeda60ec` | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` | 8324 |

Records: `evidence/f11-stage4/elite/record.json`, `evidence/f11-stage4/pro/record.json` (both `commit` = `4f96b0d9…`, `tree` = `f27c89e1…`). Local digest/record verify exited 0 (minimum local proof; no package rebuild).
Host tests: `evidence/f11-stage3/CHECKS.md` (ALL_PASS).
Package SHA-256 changed vs F10 (BF-007 `project.c` migration order). Controller SHA-256 unchanged. Content/firmware trees differ from `af494784` / `bcc21733…` as required.

## Tree gate

`tools/dev_validate.py::tree` (same non-`.md` scope as `require_gate_pass.sh` / gate):

| Commit | Role | Tree matches records? |
| --- | --- | --- |
| `4f96b0d9f5f11f700ea3fc935800b8714e11f35d` | validated source | **ACCEPT** (`f27c89e1…`) |
| `78a0452e` | BF-007 repair | repair tree differs (pre-stage-2/3 evidence after repair) |
| `af4947846df7233c68a6be72c4d0c634052a2752` | audited pre-BF-007 | audited ref (`1da3bbfb…`; not package-record tree) |
| stage-4 / stage-5 evidence tip | evidence-bearing tip | **REFUSED** (another source tree) |

Log: `evidence/f11-stage5/gate-tree-check.log`.

Package/controller bytes are intentionally out of Git. Full `tools/require_gate_pass.sh --dev-record … --fwsc … --ctl …` needs those local bytes with the recorded SHA-256 values; the install commit argument must be the validated source SHA above. Against an evidence-only tip, verify fails with `development evidence is for another source tree` before artifact digest.

## Nightly fast-forward

Attempted: `git push origin 4f96b0d9f5f11f700ea3fc935800b8714e11f35d:refs/heads/nightly` (no `--force`).

| Field | Value |
| --- | --- |
| Result | **REFUSED** non-fast-forward |
| `origin/nightly` before/after | `08d3164a265123cb4c84e5104642fd93257d63d7` (unchanged) |
| Merge-base with validated | `c1e83ab9df8b1e64fc69829394e92dbae24bc80f` |
| Commits on validated not in nightly | 124 |
| Commits on nightly not in validated | 160 |
| Force-push | **not performed** |

Logs: `evidence/f11-stage5/nightly-ff-refusal.log`, `nightly-ff-summary.txt`.

Stage 7 and the next audit still use validated source SHA `4f96b0d9…`, not the diverged nightly tip.

## Project PR merge

Proven stage-4 PR [#264](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/264): `tools/merge_project_prs.py --pr 264` hit `Resource not accessible by integration` on retarget to `main`. Merged into existing base `cursor/f11-bf007-stage3-fb90` without retarget (`gh pr merge 264 --merge`). Merge commit `1a5904d6a032c404eb913bea597eed7ad8893e52`. Logs: `merge-pr264-dry-run.log`, `merge-pr264.log`, `merge-pr264-direct.log`, `merge-pr264-result.json`.
Stage-5 PR opened on base `cursor/f11-bf007-stage4-4dfc` (subscribe for local-env review).

## Production

Production `main` and production GitHub Pages: **unchanged**. No device flash from this cloud thread. Deferred items in `evidence/f11-stage2/DEFERRED.md`: not implemented. BF-001–BF-007 not reopened.

## Successor create (API)

`POST https://api.cursor.com/v1/agents` attempted with no `CURSOR_API_KEY` (see attempt JSON). Coordinator CreateAgent required for owner-visible Stage 7. Do not create another Stage 7 once an owner-visible id is recorded.

## Overall

Merge gate complete for nightly refusal + stage-6 prompt save. Validated source SHA and package/controller hashes recorded for stage 6; evidence tip kept distinct from the install/merge SHA. Owner-visible Stage 7 successor id recorded when coordinator CreateAgent returns it.
