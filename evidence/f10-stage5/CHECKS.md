# Stage 5 merge for F10 BF-006

Branch: `cursor/f10-bf006-stage5-fb90`
Stage-5 owner: `bc-d573d60c-9080-5a6b-836a-2166e4fa7a82` (desktop; owner-visible)
Audited / validated source SHA (pre-BF-006): `d0d00da23c3ce2268f06f31f8c062716d5a37ad0`
Audited firmware reference SHA: `1325f5d34340ea877e65ab86f3663f5dea052c0d`
Validated **source** SHA (merge / install / stage-7 candidate): `af4947846df7233c68a6be72c4d0c634052a2752`
Content tree at validation commit (non-`.md`, matches Elite/Pro records): `1da3bbfbbf0df52256acceecbca2e064a1ef821478a33d78591e01813229643d`
Firmware tree at validation commit: `bcc21733ea77097a787cfa6cc617fbdf1dca5da1`
Audited firmware content tree (pre-BF-005 package bytes baseline): `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`
Content tree (pre-repair package baseline): `62339630bc6e133da464eed7924645c0bfc09e4de66a8bccafdc60fd2d660c83`
Firmware tree (pre-repair): `32a36f68d1ce7a3c304b447475afe7e2bbb116cf`
Evidence-bearing tip (this branch; docs/logs/records only — **not** an install commit): stage-5 tip on `cursor/f10-bf006-stage5-fb90` descended from stage-4 tip `b3356e96`.
BF-006 repair commit: `37bab7d1` (drum808 blob `90f463295fb1a642bad81700d73fc22eb8ec7a38`)
Package validation commit: `af4947846df7233c68a6be72c4d0c634052a2752`
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001), `ce690772` (BF-004), `94f4a012` (BF-005), `37bab7d1` (BF-006)
DEV CHECKED records authenticate the tree of `af494784`, not a later tip that only adds evidence. Fast-forward and stage-6 install use the validated source SHA.

## Package identities (from stage-4 DEV CHECKED)

| Model | Result | Package | Package SHA-256 | Controller SHA-256 | pool_free |
| --- | --- | --- | --- | --- | --- |
| elite | DEV CHECKED | `x0x-smk37-elite.fwsc` | `de0bb0773c0b6de17140b180f79ae0e157c1690fcc485e6ffba28ae299eebc77` | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` | 8324 |
| pro | DEV CHECKED | `x0x-smk37.fwsc` | `25bebb261432e56b20f24b2100e05c369b9d3e8c277811ac300bae1050bd5e40` | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` | 8324 |

Records: `evidence/f10-stage4/elite/record.json`, `evidence/f10-stage4/pro/record.json` (both `commit` = `af494784…`, `tree` = `1da3bbfb…`). Local digest/record verify exited 0 (minimum local proof; no package rebuild).
Host tests: `evidence/f10-stage3/CHECKS.md` (ALL_PASS).
Package SHA-256 match F09 attested bytes (identical object code for the defined cast). Controller SHA-256 unchanged. Content/firmware trees differ from `d0d00da2` / `32a36f68…` as required.

## Tree gate

`tools/dev_validate.py::tree` (same non-`.md` scope as `require_gate_pass.sh` / gate):

| Commit | Role | Tree matches records? |
| --- | --- | --- |
| `af4947846df7233c68a6be72c4d0c634052a2752` | validated source | **ACCEPT** (`1da3bbfb…`) |
| `37bab7d1` | BF-006 repair | repair tree differs (pre-stage-2/3 evidence after repair) |
| `d0d00da23c3ce2268f06f31f8c062716d5a37ad0` | audited pre-BF-006 | audited ref (`62339630…`; not package-record tree) |
| stage-4 / stage-5 evidence tip | evidence-bearing tip | **REFUSED** (another source tree) |

Log: `evidence/f10-stage5/gate-tree-check.log`.

Package/controller bytes are intentionally out of Git. Full `tools/require_gate_pass.sh --dev-record … --fwsc … --ctl …` needs those local bytes with the recorded SHA-256 values; the install commit argument must be the validated source SHA above. Against an evidence-only tip, verify fails with `development evidence is for another source tree` before artifact digest.

## Nightly fast-forward

Attempted: `git push origin af4947846df7233c68a6be72c4d0c634052a2752:refs/heads/nightly` (no `--force`).

| Field | Value |
| --- | --- |
| Result | **REFUSED** non-fast-forward |
| `origin/nightly` before/after | `08d3164a265123cb4c84e5104642fd93257d63d7` (unchanged) |
| Merge-base with validated | `c1e83ab9df8b1e64fc69829394e92dbae24bc80f` |
| Commits on validated not in nightly | 102 |
| Commits on nightly not in validated | 160 |
| Force-push | **not performed** |

Logs: `evidence/f10-stage5/nightly-ff-refusal.log`, `nightly-ff-summary.txt`.

Stage 7 and the next audit still use validated source SHA `af494784…`, not the diverged nightly tip.

## Project PR merge

Proven stage-4 PR [#253](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/253): `tools/merge_project_prs.py --pr 253` hit `Resource not accessible by integration` on retarget to `main`. Merged into existing base `cursor/f10-bf006-stage3-fb90` without retarget (`gh pr merge 253 --merge`). Merge commit `c46576693f56d83225ae14e34b6a821fb80e6186`. Logs: `merge-pr253-dry-run.log`, `merge-pr253.log`, `merge-pr253-direct.log`, `merge-pr253-result.json`.
Stage-5 PR [#255](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/255) opened on base `cursor/f10-bf006-stage4-fb90` (subscribe for local-env review).

## Production

Production `main` and production GitHub Pages: **unchanged**. No device flash. Deferred items in `evidence/f10-stage2/DEFERRED.md`: not implemented. BF-007 not taken. BF-001–BF-006 not reopened.

## Successor create (API)

`POST https://api.cursor.com/v1/agents` attempted with no `CURSOR_API_KEY` (see attempt JSON). Coordinator CreateAgent required for owner-visible Stage 7. Handoff incomplete until that id is on the remote tip.

## Overall

Merge gate complete for nightly refusal + stage-6 prompt save. Stage 7 successor id pending coordinator CreateAgent (pod has no `CURSOR_API_KEY`). Validated source SHA and package/controller hashes recorded for stage 6; evidence tip kept distinct from the install/merge SHA.
