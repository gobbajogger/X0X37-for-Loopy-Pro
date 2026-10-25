# Stage 5 merge for F14 BF-013

Branch: `cursor/f14-bf013-stage5-237f`
Stage-5 owner: `bc-b08d7670-f066-5496-8972-62767b638eb4` (Project Stage 5 worker; do not resume Stage 4 `bc-3789dd85-75ee-53a1-9be7-ddf256b127a9`)
Audited / validated source SHA (pre-BF-013): `b42bcba46ee7255c18f18c10dd1f99b9fc9bb675`
Validated **source** SHA (merge / install / stage-7 candidate): `656d70e9f0dbcffb346c59112d6ae582f2d93fe7`
Content tree at validation commit (non-`.md`, matches Elite/Pro records): `1ce1c2155db1e1e90d47284899db8726e1686b2aa86891f750e709503cbd1407`
Firmware tree at validation commit: `3b10e0c6a5954846a354b1dda5d457828b0a7731`
Git `source_tree` (pre-repair package attribution): `a15afc439fc8b7bccd62d50403b925add45ae9e5`
Firmware tree (pre-repair): `5facb3b4405d119a269a05224b9268adfa1baa13`
Evidence-bearing tip (this branch; docs/logs/records only — **not** an install commit): stage-5 tip on `cursor/f14-bf013-stage5-237f` descended from stage-4 tip `53022082`.
BF-013 repair commit: `13b831e8` (`fxbus.c` blob `49a89483e1c777a46758ed370973afb85f15274d` + `stereo_test.c` blob `c8bc1092319f38a46d4f8d41ab5b2a4ef560ff39`)
Package validation commit: `656d70e9f0dbcffb346c59112d6ae582f2d93fe7`
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001), `ce690772` (BF-004), `94f4a012` (BF-005), `37bab7d1` (BF-006), `78a0452e` (BF-007), `de11b6d7` (BF-008), `6636cca1`/`b42bcba4` (BF-012)
DEV CHECKED records authenticate the tree of `656d70e9`, not a later tip that only adds evidence. Fast-forward and stage-6 install use the validated source SHA.

## Package identities (from stage-4 DEV CHECKED)

| Model | Result | Package | Package SHA-256 | Controller SHA-256 | pool_free |
| --- | --- | --- | --- | --- | --- |
| elite | DEV CHECKED | `x0x-smk37-elite.fwsc` | `af5cd709fb9ad235cb814b667e0eb5e99e01f9ced3d427c17a1704f628345247` | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` | 8324 |
| pro | DEV CHECKED | `x0x-smk37.fwsc` | `eba70777632b959a339d2f8a326a3d874c938627aeef96648b83d52091379161` | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` | 8324 |

Records: `evidence/f14-stage4/elite/record.json`, `evidence/f14-stage4/pro/record.json` (both `commit` = `656d70e9…`, `tree` = `1ce1c215…`). Local digest/record verify: trees MATCH validated; records DEV CHECKED; no package rebuild.
Host tests: `evidence/f14-stage3/CHECKS.md` (ALL_PASS); stage-5 reconfirm `blob-reconfirm.log` / `host-reconfirm.log` (PASS / `ok stereo`).
Package SHA-256 changed vs live nightly (BF-013 `fxbus.c` exact-20 BPM gates). Controller SHA-256 unchanged. Content/firmware trees differ from audited `b42bcba4` / `5facb3b4…` as required.

## Tree gate

`tools/dev_validate.py::tree` (same non-`.md` scope as `require_gate_pass.sh` / gate):

| Commit | Role | Tree matches records? |
| --- | --- | --- |
| `656d70e9f0dbcffb346c59112d6ae582f2d93fe7` | validated source | **ACCEPT** (`1ce1c215…`) |
| `13b831e8` | BF-013 repair | repair tree differs (pre-stage-2/3 evidence after repair) |
| `b42bcba46ee7255c18f18c10dd1f99b9fc9bb675` | audited pre-BF-013 | audited ref (not package-record tree) |
| stage-4 / stage-5 evidence tip | evidence-bearing tip | **REFUSED** (another source tree) |

Log: `evidence/f14-stage5/gate-tree-check.log`.

Package/controller bytes are intentionally out of Git. Full `tools/require_gate_pass.sh --dev-record … --fwsc … --ctl …` needs those local bytes with the recorded SHA-256 values; the install commit argument must be the validated source SHA above. Against an evidence-only tip, verify fails with `development evidence is for another source tree` before artifact digest.

## Nightly fast-forward

Attempted: `git push origin 656d70e9f0dbcffb346c59112d6ae582f2d93fe7:refs/heads/nightly` (no `--force`).

| Field | Value |
| --- | --- |
| Result | **REFUSED** non-fast-forward |
| `origin/nightly` before/after | `811d7dd2fd13d4ca13ac3d899fb202758ae997a7` (unchanged) |
| Merge-base with validated | `de11b6d7e3ac3322d2db689675d9ed27ed667785` |
| Commits on validated not in nightly | 13 |
| Commits on nightly not in validated | 404 |
| Force-push | **not performed** |

Logs: `evidence/f14-stage5/nightly-ff-refusal.log`, `nightly-ff-summary.txt`.

Stage 7 and the next audit still use validated source SHA `656d70e9…`, not the diverged nightly tip.

## Project PR / nightly tool

Stage 5 does **not** use `tools/merge_project_prs.py` for firmware `nightly`. No production `main` merge. Stage-4 PR #322 remains on its validate tip; this stage only records nightly refusal + stage-6 prompt + stage-7 handoff.

## Production

Production `main` and production GitHub Pages: **unchanged**. No device flash from this cloud thread. Deferred items in `evidence/f14-stage2/DEFERRED.md` / `evidence/f14-stage3/DEFERRED.md`: not implemented. BF-001–BF-008 / BF-012 / FP-001 not reopened. Arp/Fx #279/#281 leftovers not stolen.

## Successor create (API)

`POST https://api.cursor.com/v1/agents` not authenticated: pod has no `CURSOR_API_KEY` (see `api-create-stage7-attempt{1,2}.json`, `api-create-stage7-note.txt`). **Do not claim Task as Stage 7.** Coordinator CreateAgent for Stage 7 is **needed** with `startingRef=cursor/f14-bf013-stage5-237f` tip after push and prompt `evidence/f14-stage5/SUCCESSOR-STAGE7.txt`.

## Overall

Merge gate complete for nightly refusal + stage-6 prompt save. Stage-7 prompt pushed; owner-visible Stage 7 successor id **pending** coordinator CreateAgent (no API key). Validated source SHA and package/controller hashes recorded for stage 6; evidence tip kept distinct from the install/merge SHA.
