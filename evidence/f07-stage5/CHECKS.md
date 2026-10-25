# Stage 5 merge for F07 FP-001

Branch: `cursor/f07-fp-pin-stage5-fb90`
Stage-5 owner: `bc-6fe39901-28af-5ef9-8590-1ba47e4031bd` (desktop; owner-visible)
Audited firmware reference SHA: `1325f5d34340ea877e65ab86f3663f5dea052c0d`
Validated **source** SHA (merge / install / stage-7 candidate): `6df0f81fbbe0adc3207f67d912d2fe0ab676be04`
Content tree at validation commit (non-`.md`, matches Elite/Pro records): `69ccb3d77a718d32191cf17a75bce6454dad807ef9bb76f6050e6532a0bff3ad`
Audited firmware content tree (unchanged package bytes vs F06): `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`
Evidence-bearing tip (this branch; docs/logs/records only — **not** an install commit): stage-5 tip on `cursor/f07-fp-pin-stage5-fb90` descended from stage-4 tip `53ff69d8`.
FP-001 repair / package validation commit: `6df0f81f`
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003)
Codex P2 rule (carried forward; ignore Codex GitHub comments — local proof only): DEV CHECKED records authenticate the tree of `6df0f81f`, not a later tip that only adds evidence. Fast-forward and stage-6 install use the validated source SHA.

## Package identities (from stage-4 DEV CHECKED)

| Model | Result | Package | Package SHA-256 | Controller SHA-256 | pool_free |
| --- | --- | --- | --- | --- | --- |
| elite | DEV CHECKED | `x0x-smk37-elite.fwsc` | `d3b4cac3d7b7be7a72732d4dd00c3120c053dbffc9137ffb7c151a5fd5355f2b` | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` | 8324 |
| pro | DEV CHECKED | `x0x-smk37.fwsc` | `0b09605f357cd0accd5c5109dfb61138eb300945ffe05e6ff46942f5c5e076b9` | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` | 8324 |

Records: `evidence/f07-stage4/elite/record.json`, `evidence/f07-stage4/pro/record.json` (both `commit` = `6df0f81f…`, `tree` = `69ccb3d7…`).
Host tests: `evidence/f07-stage3/CHECKS.md` (ALL_PASS).
Package and controller SHA-256 match the F06/F05/F04 stage-4 attested bytes (host dual-digest tooling only; firmware application paths unchanged vs audited tip `1325f5d3`).

## Tree gate

`tools/dev_validate.py::tree` (same non-`.md` scope as `require_gate_pass.sh` / gate):

| Commit | Role | Tree matches records? |
| --- | --- | --- |
| `6df0f81fbbe0adc3207f67d912d2fe0ab676be04` | validated source | **ACCEPT** (`69ccb3d7…`) |
| `1325f5d34340ea877e65ab86f3663f5dea052c0d` | audited firmware reference | audited ref only (`b02108f7…`; not package-record tree) |
| `53ff69d8cde7325eac23776d4206a15df586cd56` | evidence-bearing stage-4 tip | **REFUSED** (another source tree `a7416da0…`) |

Log: `evidence/f07-stage5/gate-tree-check.log`.

Package/controller bytes are intentionally out of Git. Full `tools/require_gate_pass.sh --dev-record … --fwsc … --ctl …` needs those local bytes with the recorded SHA-256 values; the install commit argument must be the validated source SHA above. Against an evidence-only tip, verify fails with `development evidence is for another source tree` before artifact digest.

## Nightly fast-forward

Attempted: `git push origin 6df0f81fbbe0adc3207f67d912d2fe0ab676be04:refs/heads/nightly` (no `--force`).

| Field | Value |
| --- | --- |
| Result | **REFUSED** non-fast-forward |
| `origin/nightly` before/after | `08d3164a265123cb4c84e5104642fd93257d63d7` (unchanged) |
| Merge-base with validated | `c1e83ab9df8b1e64fc69829394e92dbae24bc80f` |
| Commits on validated not in nightly | 52 |
| Commits on nightly not in validated | 160 |
| Force-push | **not performed** |

Logs: `evidence/f07-stage5/nightly-ff-refusal.log`, `nightly-ff-summary.txt`.

Stage 7 and the next audit still use validated source SHA `6df0f81f…`, not the diverged nightly tip.

## Production

Production `main` and production GitHub Pages: **unchanged**. No device flash. Deferred items in `evidence/f07-stage2/DEFERRED.md`: not implemented.

## Successor create (API)

Pending after successor prompt push. Pod has no `CURSOR_API_KEY` expected; attempts and coordinator CreateAgent recorded when complete.

## Overall

Merge gate complete for this stage pending Stage-7 successor id: nightly refusal recorded without force-push; validated source SHA and package/controller hashes recorded for stage 6; evidence tip kept distinct from the install/merge SHA.
