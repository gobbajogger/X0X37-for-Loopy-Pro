# Stage 5 merge for F06 BF-003

Branch: `cursor/f06-bf003-stage5-fb90`
Stage-5 owner: `bc-79dd773c-ae89-569f-8782-477f0086ede5` (desktop; owner-visible)
Audited SHA (source under audit): `ab90f47ccad0f213114d6a98c1bc6179be2ebb35`
Validated **source** SHA (merge / install / stage-7 candidate): `1325f5d34340ea877e65ab86f3663f5dea052c0d`
Content tree (non-`.md`, matches Elite/Pro records): `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`
Audited firmware reference content tree (unchanged package bytes): `0ee03318a0b64a1e535b36fad284558529ff7e3f95ff069efed86bbcf308518c`
Evidence-bearing tip (this branch; docs/logs/records only — **not** an install commit): stage-5 tip on `cursor/f06-bf003-stage5-fb90` descended from stage-4 tip `c1fcebd5`.
BF-003 repair commit: `1325f5d3`
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03)
Codex P2 rule (carried forward; ignore Codex GitHub comments — local proof only): DEV CHECKED records authenticate the tree of `1325f5d3`, not a later tip that only adds evidence. Fast-forward and stage-6 install use the validated source SHA.

## Package identities (from stage-4 DEV CHECKED)

| Model | Result | Package | Package SHA-256 | Controller SHA-256 | pool_free |
| --- | --- | --- | --- | --- | --- |
| elite | DEV CHECKED | `x0x-smk37-elite.fwsc` | `d3b4cac3d7b7be7a72732d4dd00c3120c053dbffc9137ffb7c151a5fd5355f2b` | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` | 8324 |
| pro | DEV CHECKED | `x0x-smk37.fwsc` | `0b09605f357cd0accd5c5109dfb61138eb300945ffe05e6ff46942f5c5e076b9` | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` | 8324 |

Records: `evidence/f06-stage4/elite/record.json`, `evidence/f06-stage4/pro/record.json` (both `commit` = `1325f5d3…`, `tree` = `b02108f7…`).
Host tests: `evidence/f06-stage3/CHECKS.md` (ALL_PASS).
Package and controller SHA-256 match the F05/F04 stage-4 attested bytes (web harness / site-builder-only BF-003 repair; firmware application paths unchanged).

## Tree gate

`tools/dev_validate.py::tree` (same non-`.md` scope as `require_gate_pass.sh` / gate):

| Commit | Role | Tree matches records? |
| --- | --- | --- |
| `1325f5d34340ea877e65ab86f3663f5dea052c0d` | validated source | **ACCEPT** (`b02108f7…`) |
| `ab90f47ccad0f213114d6a98c1bc6179be2ebb35` | audited firmware reference | audited ref only (`0ee03318…`; not package-record tree) |
| `c1fcebd5890c97f420757ac4fcd6393aa97d31b7` | evidence-bearing stage-4 tip | **REFUSED** (another source tree `bf182012…`) |

Log: `evidence/f06-stage5/gate-tree-check.log`.

Package/controller bytes are intentionally out of Git. Full `tools/require_gate_pass.sh --dev-record … --fwsc … --ctl …` needs those local bytes with the recorded SHA-256 values; the install commit argument must be the validated source SHA above. Against an evidence-only tip, verify fails with `development evidence is for another source tree` before artifact digest.

## Nightly fast-forward

Attempted: `git push origin 1325f5d34340ea877e65ab86f3663f5dea052c0d:refs/heads/nightly` (no `--force`).

| Field | Value |
| --- | --- |
| Result | **REFUSED** non-fast-forward |
| `origin/nightly` before/after | `08d3164a265123cb4c84e5104642fd93257d63d7` (unchanged) |
| Merge-base with validated | `c1e83ab9df8b1e64fc69829394e92dbae24bc80f` |
| Commits on validated not in nightly | 39 |
| Commits on nightly not in validated | 160 |
| Force-push | **not performed** |

Logs: `evidence/f06-stage5/nightly-ff-refusal.log`, `nightly-ff-summary.txt`.

Stage 7 and the next audit still use validated source SHA `1325f5d3…`, not the diverged nightly tip.

## Production

Production `main` and production GitHub Pages: **unchanged**. No device flash. Deferred items in `evidence/f06-stage2/DEFERRED.md`: not implemented.

## Successor create (API)

`POST https://api.cursor.com/v1/agents` attempted twice with no `CURSOR_API_KEY`.

| Attempt | HTTP | Body |
| --- | --- | --- |
| 1 | 401 | Invalid User API Key |
| 2 | 401 | Invalid User API Key |

Logs: `api-create-stage7-attempt1.json`, `api-create-stage7-attempt2.json`.
Coordinator CreateAgent launched owner-visible Stage 7: `bc-f7783059-a0d9-5c29-a373-763ffd739822` (desktop; https://cursor.com/agents/bc-f7783059-a0d9-5c29-a373-763ffd739822). Handoff complete on the remote tip.

## Overall

Merge gate complete for this stage: nightly refusal recorded without force-push; validated source SHA and package/controller hashes recorded for stage 6; evidence tip kept distinct from the install/merge SHA; owner-visible Stage 7 successor id on the remote.
