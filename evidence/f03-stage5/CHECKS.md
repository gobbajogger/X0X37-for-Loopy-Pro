# Stage 5 merge for F03

Branch: `cursor/f03-slot-probe-stage5-fb90`
Stage-5 owner: `bc-612598eb-d3fa-5382-8448-3bd7e18b0901` (desktop; CreateAgent by Project coordinator)
Audited SHA: `4ee8f37714b0892f2c70d157fda9328da393e987`
Validated **source** SHA (merge / install / stage-7 candidate): `a03cfab3a423ab5c65a6546afd4993fe51754ee7`
Content tree (non-`.md`, matches Elite/Pro records): `1755a2dd6d8b79e7405ab58919cc4d60a69ca11a4ae6601030b0f5de963f170e`
Evidence-bearing tip (this branch; docs/logs/records only — **not** an install commit): stage-5 tip on `cursor/f03-slot-probe-stage5-fb90` descended from `d4d083c8` / stage-4 handoff.
Repair commit: `4f18983a`
Codex P2 rule (carried from F02): DEV CHECKED records authenticate the tree of `a03cfab3`, not a later tip that only adds evidence. Fast-forward and stage-6 install use the validated source SHA.

## Package identities (from stage-4 DEV CHECKED)

| Model | Result | Package | Package SHA-256 | Controller SHA-256 | pool_free |
| --- | --- | --- | --- | --- | --- |
| elite | DEV CHECKED | `x0x-smk37-elite.fwsc` | `d3b4cac3d7b7be7a72732d4dd00c3120c053dbffc9137ffb7c151a5fd5355f2b` | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` | 8324 |
| pro | DEV CHECKED | `x0x-smk37.fwsc` | `0b09605f357cd0accd5c5109dfb61138eb300945ffe05e6ff46942f5c5e076b9` | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` | 8324 |

Records: `evidence/f03-stage4/elite/record.json`, `evidence/f03-stage4/pro/record.json` (both `commit` = `a03cfab3…`, `tree` = `1755a2dd…`).
Host tests: `evidence/f03-stage3/CHECKS.md` (ALL_PASS).

## Tree gate

`tools/dev_validate.py::tree` (same non-`.md` scope as `require_gate_pass.sh` / gate):

| Commit | Role | Tree matches records? |
| --- | --- | --- |
| `a03cfab3a423ab5c65a6546afd4993fe51754ee7` | validated source | **ACCEPT** (`1755a2dd…`) |
| `d4d083c89ec6e35a8eb4d0d48d8474b41e51314e` | evidence-bearing stage-4 tip | **REFUSED** (another source tree `6c8ff597…`) |

Log: `evidence/f03-stage5/gate-tree-check.log`.

Package/controller bytes are intentionally out of Git. Full `tools/require_gate_pass.sh --dev-record … --fwsc … --ctl …` needs those local bytes with the recorded SHA-256 values; the install commit argument must be the validated source SHA above. Against an evidence-only tip, verify fails with `development evidence is for another source tree` before artifact digest.

## Nightly fast-forward

Attempted: `git push origin a03cfab3a423ab5c65a6546afd4993fe51754ee7:refs/heads/nightly` (no `--force`).

| Field | Value |
| --- | --- |
| Result | **REFUSED** non-fast-forward |
| `origin/nightly` before/after | `08d3164a265123cb4c84e5104642fd93257d63d7` (unchanged) |
| Merge-base with validated | `c1e83ab9df8b1e64fc69829394e92dbae24bc80f` |
| Commits on validated not in nightly | 2 (`4f18983a`, `a03cfab3`) |
| Commits on nightly not in validated | 160 |
| Force-push | **not performed** |

Logs: `evidence/f03-stage5/nightly-ff-refusal.log`, `nightly-ff-summary.txt`.

Stage 7 and the next audit still use validated source SHA `a03cfab3…`, not the diverged nightly tip.

## Production

Production `main` and production GitHub Pages: **unchanged**. No device flash. Deferred items in `evidence/f03-stage2/DEFERRED.md`: not implemented.

## Overall

Merge gate complete for this stage: nightly refusal recorded without force-push; validated source SHA and package/controller hashes recorded for stage 6; evidence tip kept distinct from the install/merge SHA.
