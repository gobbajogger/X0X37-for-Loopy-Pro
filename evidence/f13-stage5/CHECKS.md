# Stage 5 merge for F13 BF-012

Branch: `cursor/f13-bf012-stage5-237f`
Stage-5 owner: `bc-3275436b-e808-57c2-86c2-b8a3130dd94f` (Project Stage 5 worker; do not resume Stage 4 `bc-952af67e-b6f6-5318-a3b3-ba4de860c800`)
Audited / validated source SHA (pre-BF-012): `de11b6d7e3ac3322d2db689675d9ed27ed667785`
Validated **source** SHA (merge / install / stage-7 candidate): `b42bcba46ee7255c18f18c10dd1f99b9fc9bb675`
Content tree at validation commit (non-`.md`, matches Elite/Pro records): `519ccdf21b46db9198993587cb9334960e67ce483641c744c5caf71b1c769141`
Firmware tree at validation commit: `5facb3b4405d119a269a05224b9268adfa1baa13`
Git `source_tree` (pre-repair package attribution): `c143a9f8f5b5d0f6804995829ccbe515c77c6103`
Firmware tree (pre-repair): `9195dce5c4beb5fa8aa564504c8d1d114c8264cd`
Evidence-bearing tip (this branch; docs/logs/records only — **not** an install commit): stage-5 tip on `cursor/f13-bf012-stage5-237f` descended from stage-4 tip `d344ad12`.
BF-012 repair commit: `6636cca1` (`master.c` blob `f91a9473f0ae1cdd5e037e521f39fdfcc2022395` + `master_test.c` blob `df2f13b35a6fc7b858b019a1950982c23f6386e3`)
Package validation commit: `b42bcba46ee7255c18f18c10dd1f99b9fc9bb675`
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001), `ce690772` (BF-004), `94f4a012` (BF-005), `37bab7d1` (BF-006), `78a0452e` (BF-007), `de11b6d7` (BF-008)
DEV CHECKED records authenticate the tree of `b42bcba4`, not a later tip that only adds evidence. Fast-forward and stage-6 install use the validated source SHA.

## Package identities (from stage-4 DEV CHECKED)

| Model | Result | Package | Package SHA-256 | Controller SHA-256 | pool_free |
| --- | --- | --- | --- | --- | --- |
| elite | DEV CHECKED | `x0x-smk37-elite.fwsc` | `d9a9ccf4698d9519316d90b36f6158101f7f1c5cd96ba50f6c1be6ef8693ebcc` | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` | 8324 |
| pro | DEV CHECKED | `x0x-smk37.fwsc` | `a6a2387c712e95772ab1ff2f184f6b21357f71da08f0414f8d698b5f94309498` | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` | 8324 |

Records: `evidence/f13-stage4/elite/record.json`, `evidence/f13-stage4/pro/record.json` (both `commit` = `b42bcba4…`, `tree` = `519ccdf2…`). Local digest/record verify: trees MATCH validated; records DEV CHECKED; no package rebuild.
Host tests: `evidence/f13-stage3/CHECKS.md` (ALL_PASS).
Package SHA-256 changed vs live nightly (BF-012 `master.c` look-ahead clear). Controller SHA-256 unchanged. Content/firmware trees differ from audited `de11b6d7` / `9195dce5…` as required.

## Tree gate

`tools/dev_validate.py::tree` (same non-`.md` scope as `require_gate_pass.sh` / gate):

| Commit | Role | Tree matches records? |
| --- | --- | --- |
| `b42bcba46ee7255c18f18c10dd1f99b9fc9bb675` | validated source | **ACCEPT** (`519ccdf2…`) |
| `6636cca1` | BF-012 repair | repair tree differs (pre-stage-2/3 evidence after repair) |
| `de11b6d7e3ac3322d2db689675d9ed27ed667785` | audited pre-BF-012 | audited ref (not package-record tree) |
| stage-4 / stage-5 evidence tip | evidence-bearing tip | **REFUSED** (another source tree) |

Log: `evidence/f13-stage5/gate-tree-check.log`.

Package/controller bytes are intentionally out of Git. Full `tools/require_gate_pass.sh --dev-record … --fwsc … --ctl …` needs those local bytes with the recorded SHA-256 values; the install commit argument must be the validated source SHA above. Against an evidence-only tip, verify fails with `development evidence is for another source tree` before artifact digest.

## Nightly fast-forward

Attempted: `git push origin b42bcba46ee7255c18f18c10dd1f99b9fc9bb675:refs/heads/nightly` (no `--force`).

| Field | Value |
| --- | --- |
| Result | **REFUSED** non-fast-forward |
| `origin/nightly` before/after | `dcd288a4cdf96e71aa45fdc72994283c8fb173a4` (unchanged) |
| Merge-base with validated | `de11b6d7e3ac3322d2db689675d9ed27ed667785` |
| Commits on validated not in nightly | 5 |
| Commits on nightly not in validated | 391 |
| Force-push | **not performed** |

Logs: `evidence/f13-stage5/nightly-ff-refusal.log`, `nightly-ff-summary.txt`.

Stage 7 and the next audit still use validated source SHA `b42bcba4…`, not the diverged nightly tip.

## Project PR / nightly tool

Stage 5 does **not** use `tools/merge_project_prs.py` for firmware `nightly`. No production `main` merge. Stage-4 PR #302 remains on its validate tip; this stage only records nightly refusal + stage-6 prompt + stage-7 handoff.

## Production

Production `main` and production GitHub Pages: **unchanged**. No device flash from this cloud thread. Deferred items in `evidence/f13-stage2/DEFERRED.md` / `evidence/f13-stage3/DEFERRED.md`: not implemented. BF-001–BF-008 / FP-001 not reopened. Arp/Fx #279/#281 leftovers not stolen.

## Successor create (API)

`POST https://api.cursor.com/v1/agents` not authenticated: pod has no `CURSOR_API_KEY` (see `api-create-stage7-attempt{1,2}.json`, `api-create-stage7-note.txt`). **Do not claim Task as Stage 7.** Coordinator CreateAgent for Stage 7 is **needed** with `startingRef=cursor/f13-stage5` tip after push (`cursor/f13-bf012-stage5-237f`) and prompt `evidence/f13-stage5/SUCCESSOR-STAGE7.txt`.

## Overall

Merge gate complete for nightly refusal + stage-6 prompt save. Stage-7 prompt pushed; owner-visible Stage 7 successor id **pending** coordinator CreateAgent (no API key). Validated source SHA and package/controller hashes recorded for stage 6; evidence tip kept distinct from the install/merge SHA.
