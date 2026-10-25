# Stage 5 merge for F13 BF-012

Branch: `cursor/f13-bf012-stage5-237f`
Stage-5 owner: `bc-77b4a99f-7d71-5957-ac89-ccaa9428016a` (desktop; owner-visible)
Audited / validated source SHA (pre-BF-012): `de11b6d7e3ac3322d2db689675d9ed27ed667785`
Validated **source** SHA (merge / install / stage-7 candidate): `b42bcba46ee7255c18f18c10dd1f99b9fc9bb675`
Content tree at validation commit (non-`.md`, matches Elite/Pro records): `519ccdf21b46db9198993587cb9334960e67ce483641c744c5caf71b1c769141`
Firmware tree at validation commit: `5facb3b4405d119a269a05224b9268adfa1baa13`
Evidence-bearing tip (this branch; docs/logs/records only — **not** an install commit): stage-5 tip on `cursor/f13-bf012-stage5-237f` descended from Stage-4 clarify tip `28aed5c7`.
BF-012 repair commit: `6636cca1` (`master.c` blob `f91a9473…`, `master_test.c` blob `df2f13b3…`)
Package validation commit (= install/merge SHA): `b42bcba46ee7255c18f18c10dd1f99b9fc9bb675`
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001), `ce690772` (BF-004), `94f4a012` (BF-005), `37bab7d1` (BF-006), `78a0452e` (BF-007), `de11b6d7` (BF-008)
DEV CHECKED records authenticate the tree of `b42bcba4`, not a later tip that only adds evidence. Fast-forward and stage-6 install use the validated source SHA only.

Stage-4 clarify `28aed5c7` + Codex on [#302](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/302#discussion_r4372024606): do **not** treat the Stage-4 evidence tip (`d344ad12` / `28aed5c7`, trees `92197ef2…` / `0e58aab3…`) as the validated install SHA. Evidence commits change the source fingerprint vs recorded `519ccdf2…`.

## Package identities (from stage-4 DEV CHECKED)

| Model | Result | Package | Package SHA-256 | Controller SHA-256 | pool_free |
| --- | --- | --- | --- | --- | --- |
| elite | DEV CHECKED | `x0x-smk37-elite.fwsc` | `d9a9ccf4698d9519316d90b36f6158101f7f1c5cd96ba50f6c1be6ef8693ebcc` | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` | 8324 |
| pro | DEV CHECKED | `x0x-smk37.fwsc` | `a6a2387c712e95772ab1ff2f184f6b21357f71da08f0414f8d698b5f94309498` | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` | 8324 |

Records: `evidence/f13-stage4/elite/record.json`, `evidence/f13-stage4/pro/record.json` (both `commit` = `b42bcba4…`, `tree` = `519ccdf2…`). Local digest/record verify exited 0 (minimum local proof; no package rebuild).
Host tests: `evidence/f13-stage3/CHECKS.md` (ALL_PASS).
Package SHA-256 changed vs live nightly (BF-012 `master.c` look-ahead clear). Controller SHA-256 unchanged.

## Tree gate

`tools/dev_validate.py::tree` (same non-`.md` scope as `require_gate_pass.sh` / gate):

| Commit | Role | Tree matches records? |
| --- | --- | --- |
| `b42bcba46ee7255c18f18c10dd1f99b9fc9bb675` | validated source | **ACCEPT** (`519ccdf2…`) |
| `6636cca1` | BF-012 repair | repair tree differs (pre-stage-2/3 evidence after repair) |
| `de11b6d7…` | audited pre-BF-012 | audited ref (not package-record tree) |
| stage-4 evidence tip `28aed5c7` (incl. clarify) | evidence-bearing tip | **REFUSED** (tree `0e58aab3…` — another source tree) |

Log: `evidence/f13-stage5/gate-tree-check.log`.

Package/controller bytes are intentionally out of Git. Full `tools/require_gate_pass.sh --dev-record … --fwsc … --ctl …` needs those local bytes with the recorded SHA-256 values; the install commit argument must be the validated source SHA above. Against an evidence-only tip, verify fails with `development evidence is for another source tree` before artifact digest.

## Nightly fast-forward

Attempted: `git push origin b42bcba46ee7255c18f18c10dd1f99b9fc9bb675:refs/heads/nightly` (no `--force`).
Stage-4 evidence tip `28aed5c7` (and earlier `d344ad12`) was **not** pushed to `nightly`.

| Field | Value |
| --- | --- |
| Result | **REFUSED** non-fast-forward |
| `origin/nightly` before attempt | `dcd288a4cdf96e71aa45fdc72994283c8fb173a4` |
| `origin/nightly` after (fetched) | `bd2d99332e1f7abc3db3c8dd5839132d99e21e8f` (unchanged by this push; tip advanced elsewhere) |
| Merge-base with validated | `de11b6d7e3ac3322d2db689675d9ed27ed667785` |
| Commits on validated not in nightly | 5 |
| Commits on nightly not in validated | 393 |
| Force-push | **not performed** |

Logs: `evidence/f13-stage5/nightly-ff-refusal.log`, `nightly-ff-summary.txt`.

**Nightly did not land `b42bcba4`.** Stage 7 and the next audit still use validated source SHA `b42bcba4…`, not the diverged nightly tip.

## Project PR merge

Proven stack: Stage-2 [#294](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/294) (firmware; base `nightly` — merge tool forbids nightly base; left open), Stage-3 [#298](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/298), Stage-4 [#302](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/302).
`tools/merge_project_prs.py --pr 298 --pr 302` failed retarget to `main` (`Resource not accessible by integration`). Merged into existing stack bases without retarget: `#298` → `a57b3626…`, `#302` → `257fab87…`. Logs: `merge-prs-dry-run.log`, `merge-prs.log`, `merge-prs-direct.log`.

## Production

Production `main` and production GitHub Pages: **unchanged**. No device flash from this cloud thread. Deferred items in `evidence/f13-stage2/DEFERRED.md` / `evidence/f13-stage3/DEFERRED.md`: not implemented. BF-001–BF-008 and FP-001 not reopened.

## Successor create (API)

`POST https://api.cursor.com/v1/agents` not available (`CURSOR_API_KEY` unset). Attempts recorded as `api-create-stage7-attempt{1,2}.json` (`NO_CURSOR_API_KEY`). Ask Project coordinator to CreateAgent Stage 7 with `startingRef=cursor/f13-bf012-stage5-237f` and `evidence/f13-stage5/SUCCESSOR-STAGE7.txt`. Stage 6 on Mac separately (LaunchAgent / rtmidi). Do not use Task as Stage 7.

## Overall

Merge gate complete for nightly refusal + stage-6 prompt save. Validated package/install SHA is `b42bcba4…` only; Stage-4/5 evidence tips kept distinct. Owner-visible Stage 7 successor pending coordinator CreateAgent. **Nightly did not receive `b42bcba4`.**
