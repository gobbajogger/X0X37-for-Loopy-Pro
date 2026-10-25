# Safety gates for F11 new package digests

**Applies to:** `backup-restore-readback`, `interruption-recovery`, and `ota-target-binding` for Elite/Pro page packages of `nightly-4f96b0d9-dev`.

BF-007 changed installable Elite/Pro package bytes vs F10 (`9cba1f3d…`/`5dda4eec…` → `4841110a…`/`b35d5875…`). F10 byte-identity carry-forward does **not** apply.

## Candidate-specific results (this tip)

| Identity | Digest |
| --- | --- |
| Page Elite (bundled) | `4841110a274e2212d56abf9234cf0d921b45d915d1ff25fad0727e52edb7d18a` |
| Page Pro (bundled) | `b35d587557d9f1d4c72b2c81bc944a1dd129bee625e98ae35f06fe7f653fc720` |
| Blank Elite / Pro | `829bb6da…` / `d2186cb0…` |
| Controller | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` |
| Source | `4f96b0d9f5f11f700ea3fc935800b8714e11f35d` |

Runner: `evidence/f11-stage7/check-safety-gates.py`  
Log: `evidence/f11-stage7/safety-gates-candidate.log` → **ALL_PASS**

| Gate | Result | Candidate-specific proof |
| --- | --- | --- |
| digest / join chain | PASS | blank+ctl SHA MATCH; `add_ctl` join == page Elite/Pro digests |
| `backup-restore-readback` | PASS | FakeX0X blank `.fwsc` + `--ctl` (candidate ctl `956e3e21…`): slots backup, install, ED_CTL_READ full-block CRC MATCH; both models |
| `interruption-recovery` | PASS | Same candidate install path; mid-restore SMP_WRITE fail-closed exit 7; backup retained; both models |
| `ota-target-binding` | PASS | Blank and page packages load as `SMK-37 Elite_900` / `SMK-37 Pro_900` with page SHA MATCH |
| host suites | PASS (supporting) | `install_slots_test` / `x0x_slots_test` / `install_f03_slot_probe_test` — **not** used alone as candidate QA |

## Also substantiated elsewhere

| Gate | Evidence |
| --- | --- |
| `package-integrity` / `model-controller` / `memory-flash` | `evidence/f11-stage4/CHECKS.md` |
| `host-regressions` | `evidence/f11-stage3/CHECKS.md` |
| `independent-qa` | `evidence/f11-stage7/independent-qa-local-review.md` + PR #264 local PASS |

**Not claimed:** hardware test, production promotion, or that nightly synthetic `npm test` alone is firmware device QA. Local-env review only.
