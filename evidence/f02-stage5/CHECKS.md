# Stage 5 merge for F02

Branch: `cursor/f02-stage5-fb90`
Stage-5 owner: `bc-94bdcfbc-3579-555b-8b14-0f4431db7cd7` (desktop)
Audited SHA: `13d32e1dee37b6fb5a1e339aff499a75eecb97b5`
Validated **source** SHA (merge / install / stage-7 candidate): `4ee8f37714b0892f2c70d157fda9328da393e987`
Content tree (non-`.md`, matches Elite/Pro records): `7ad3a50da8abac890215dae3c28197a2fa90041b8615e828b6b3c6e18060fb83`
Evidence-bearing tip (this branch; docs/logs/records only — **not** an install commit): stage-5 tip on `cursor/f02-stage5-fb90` descended from `fbf0bd4` / stage-4 handoff.
Repair commit: `765695f`
Codex P2 (PR #169): DEV CHECKED records authenticate the tree of `4ee8f377`, not a later tip that only adds evidence. Fast-forward and stage-6 install use the validated source SHA.

## Package identities (from stage-4 DEV CHECKED)

| Model | Result | Package | Package SHA-256 | Controller SHA-256 | pool_free |
| --- | --- | --- | --- | --- | --- |
| elite | DEV CHECKED | `x0x-smk37-elite.fwsc` | `d3b4cac3d7b7be7a72732d4dd00c3120c053dbffc9137ffb7c151a5fd5355f2b` | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` | 8324 |
| pro | DEV CHECKED | `x0x-smk37.fwsc` | `0b09605f357cd0accd5c5109dfb61138eb300945ffe05e6ff46942f5c5e076b9` | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` | 8324 |

Records: `evidence/f02-stage4/elite/record.json`, `evidence/f02-stage4/pro/record.json` (both `commit` = `4ee8f377…`, `tree` = `7ad3a50…`).
Host tests: `evidence/f02-stage3/CHECKS.md` (ALL_PASS).

## Tree gate (Codex P2)

`tools/dev_validate.py::tree` (same non-`.md` scope as `require_gate_pass.sh` / gate):

| Commit | Role | Tree matches records? |
| --- | --- | --- |
| `4ee8f37714b0892f2c70d157fda9328da393e987` | validated source | **ACCEPT** (`7ad3a50…`) |
| `fbf0bd41545fe0c8fa194af467897fd2ce33eb36` | evidence-bearing stage-4 tip | **REFUSED** (another source tree `1848374…`) |

Log: `evidence/f02-stage5/gate-tree-check.log`.

Package/controller bytes are intentionally out of Git. Full `tools/require_gate_pass.sh --dev-record … --fwsc … --ctl …` needs those local bytes with the recorded SHA-256 values; the install commit argument must be the validated source SHA above. Against an evidence-only tip, verify fails with `development evidence is for another source tree` before artifact digest.

## Nightly fast-forward

Attempted: `git push origin 4ee8f37714b0892f2c70d157fda9328da393e987:refs/heads/nightly` (no `--force`).

| Field | Value |
| --- | --- |
| Result | **REFUSED** non-fast-forward |
| `origin/nightly` before/after | `58e2c5205a02836df678083eaa8db9994b6192e8` (unchanged) |
| Merge-base with validated | `c9718ae833eb64a5191c350922593df8a36017c9` |
| Commits on validated not in nightly | 18 |
| Commits on nightly not in validated | 155 |
| Force-push | **not performed** |

Logs: `evidence/f02-stage5/nightly-ff-refusal.log`, `nightly-ff-summary.txt`.

Stage 7 and the next audit still use validated source SHA `4ee8f377…`, not the diverged nightly tip.

## Production

Production `main` and production GitHub Pages: **unchanged**. No device flash. Deferred items in `evidence/f02-stage2/DEFERRED.md`: not implemented.

## Overall

Merge gate complete for this stage: nightly refusal recorded without force-push; validated source SHA and package/controller hashes recorded for stage 6; evidence tip kept distinct from the install/merge SHA.
