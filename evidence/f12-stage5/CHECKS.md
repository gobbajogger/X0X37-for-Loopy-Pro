# Stage 5 merge for F12 BF-008

Branch: `cursor/f12-stage5-bf008-1207`
Stage-5 owner: `bc-2ced8972-1537-5ad0-91cf-84177aa15c17` (desktop / owner-visible cloud thread)
Audited / validated source SHA (pre-BF-008): `4f96b0d9f5f11f700ea3fc935800b8714e11f35d`
Audited firmware reference SHA: `1325f5d34340ea877e65ab86f3663f5dea052c0d`
Validated **source** SHA (merge / install / stage-7 candidate): `de11b6d7e3ac3322d2db689675d9ed27ed667785`
Content tree at validation commit (non-`.md`, matches Elite/Pro records): `43e97f34ca00d32570c831ea4ec2bf56f697341cf8881018476cc67c9d76d5a9`
Firmware tree at validation commit: `9195dce5c4beb5fa8aa564504c8d1d114c8264cd`
Audited firmware content tree (pre-BF-005 package bytes baseline): `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`
Content tree (pre-BF-008 package baseline): `f27c89e1ac8e11ab57c5ae7e49145ce19b964082bf3ca9a700a99bfa8a97091e`
Firmware tree (pre-BF-008): `5b1a21f07563b0262ece6728051cb0436bd4bfa1`
Evidence-bearing tip (this branch; docs/logs/records only — **not** an install commit): stage-5 tip on `cursor/f12-stage5-bf008-1207` descended from stage-4 tip `729f7e5d`.
BF-008 repair commit: `de11b6d7` (`fastmath.h` blob `a5c26c95cad0d91a548a8db073f59777e36d4e86`)
Package validation commit: `de11b6d7e3ac3322d2db689675d9ed27ed667785`
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001), `ce690772` (BF-004), `94f4a012` (BF-005), `37bab7d1` (BF-006), `78a0452e` (BF-007), `de11b6d7` (BF-008)
DEV CHECKED records authenticate the tree of `de11b6d7`, not a later tip that only adds evidence. Fast-forward and stage-6 install use the validated source SHA.

## Package identities (from stage-4 DEV CHECKED)

| Model | Result | Package | Package SHA-256 | Controller SHA-256 | pool_free |
| --- | --- | --- | --- | --- | --- |
| elite | DEV CHECKED | `x0x-smk37-elite.fwsc` | `829bb6da6eb0042af0731a236d6238e50cc13581ce059ebbe02882534a94b056` | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` | 8324 |
| pro | DEV CHECKED | `x0x-smk37.fwsc` | `d2186cb0f3fe74376651cb2804973c8255780532a8cd54c33d2bac09aeda60ec` | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` | 8324 |

Records: `evidence/f12-stage4/elite/record.json`, `evidence/f12-stage4/pro/record.json` (both `commit` = `de11b6d7…`, `tree` = `43e97f34…`). Local digest/record verify exited 0 (minimum local proof; no package rebuild). Helper: internal Task `bc-c46d7d32-8669-5cfd-96e8-1e9bb43d898e` (not a successor).
Host tests: `evidence/f12-stage3/CHECKS.md` (ALL_PASS).
Package SHA-256 match F11 blank-slot bytes (defined unsigned cast yields identical Clang 4.0.1 object code). Controller SHA-256 unchanged. Content/firmware trees differ from `4f96b0d9` / `5b1a21f0…` as required.

## Tree gate

`tools/dev_validate.py::tree` (same non-`.md` scope as `require_gate_pass.sh` / gate):

| Commit | Role | Tree matches records? |
| --- | --- | --- |
| `de11b6d7e3ac3322d2db689675d9ed27ed667785` | validated source | **ACCEPT** (`43e97f34…`) |
| `729f7e5d8e0b2e371d4ed522182728a181938bad` | stage-4 evidence tip | **REFUSED** (tree `349eee8d…`; another source tree) |
| `4f96b0d9f5f11f700ea3fc935800b8714e11f35d` | audited pre-BF-008 | audited ref (`f27c89e1…`; not package-record tree) |

Log: `evidence/f12-stage5/gate-tree-check.log`.

Package/controller bytes are intentionally out of Git. Full `tools/require_gate_pass.sh --dev-record … --fwsc … --ctl …` needs those local bytes with the recorded SHA-256 values; the install commit argument must be the validated source SHA above. Against an evidence-only tip, verify fails with `development evidence is for another source tree` before artifact digest.

## Nightly fast-forward

Attempted: `git push origin de11b6d7e3ac3322d2db689675d9ed27ed667785:refs/heads/nightly` (no `--force`).

| Field | Value |
| --- | --- |
| Result | **REFUSED** non-fast-forward |
| `origin/nightly` before/after | `08d3164a265123cb4c84e5104642fd93257d63d7` (unchanged) |
| Merge-base with validated | `c1e83ab9df8b1e64fc69829394e92dbae24bc80f` |
| Commits on validated not in nightly | 138 |
| Commits on nightly not in validated | 160 |
| Force-push | **not performed** |

Logs: `evidence/f12-stage5/nightly-ff-refusal.log`, `nightly-ff-summary.txt`.

Stage 7 and the next audit still use validated source SHA `de11b6d7…`, not the diverged nightly tip.

## Project PR merge

Proven stage-4 PR [#272](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/272): local-env review **PASS**. Tip landed on stage-3 branch `cursor/f12-stage3-bf008-1207` (merge `403ae0a8…` includes stage-4 tip `f99d5601…` and validated source `de11b6d7…`).
A brief mistaken merge of #272 into production `main` (`11604ab7…`) was **reverted** via [#273](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/273); production `main` tip is `4e12d022…`. Stage 5 continues on **stage bases only** — no production `main`. Prefer `tools/merge_project_prs.py` / direct stage-branch merge when `gh pr edit --base` fails. Logs: `merge-pr272-dry-run.log`, `merge-pr272-result.json`.
Stage-5 PR [#274](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/274) on base `cursor/f12-stage4-bf008-1207`; subscribed; local-env review `bc-936118da-9d30-5f96-9e5e-56433edb6ae7`.

## Production

Production `main` tip `4e12d022…` (post-#273 revert) and production GitHub Pages: **unchanged by Stage 5**. No device flash from this cloud thread. Deferred items in `evidence/f12-stage2/DEFERRED.md`: not implemented. BF-001–BF-008 and FP-001 not reopened. Do not merge firmware stage tips into production `main` from this stage.

## Successor create (API)

`POST https://api.cursor.com/v1/agents` not callable: pod has no `CURSOR_API_KEY` (see `api-create-stage7-attempt{1,2}.json`, `api-create-stage7-note.txt`). Coordinator CreateAgent launched owner-visible Stage 7 `bc-45a62e39-2e34-5bca-b974-4df019080a51` (https://cursor.com/agents/bc-45a62e39-2e34-5bca-b974-4df019080a51; name `F12 nightly publish`; startingRef `cursor/f12-stage5-bf008-1207`). Do not create another Stage 7. PR #274 subscribed; local-env review `bc-936118da-9d30-5f96-9e5e-56433edb6ae7`.

## Overall

Merge gate complete for nightly refusal + stage-6 prompt save. Owner-visible Stage 7 successor id recorded on the handoff. Validated source SHA and package/controller hashes recorded for stage 6; evidence tip kept distinct from the install/merge SHA.
