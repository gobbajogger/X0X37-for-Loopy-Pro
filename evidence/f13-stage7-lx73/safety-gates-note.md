# Safety gates for F13 LX-73 page digests

**Applies to:** `backup-restore-readback`, `interruption-recovery`, and `ota-target-binding` for Elite/Pro page packages of `nightly-092589b8-dev`.

LX-73 packages for validated source `092589b8…` supersede the earlier F13 `b42bcba4…` picker entry. Prior byte-identity carry-forward does **not** apply.

## Candidate-specific results (this tip)

| Identity | Digest |
| --- | --- |
| Page Elite (bundled) | `3d107710cde229d7faf3e007ae14e324d7ea5ce209a802642dacf40608b2895e` |
| Page Pro (bundled) | `60dc7fda13fdaa3ee456736588e18b2d9e7e50cc34a800504bde678e14ba12d3` |
| Blank Elite / Pro | `9c1168cf…` / `3a29723e…` |
| Controller | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` |
| Source | `092589b84fd455355216cb0de93c4782236f7524` |

Runner: `evidence/f13-stage7-lx73/check-safety-gates.py`  
Log: `evidence/f13-stage7-lx73/safety-gates-candidate.log` → **ALL_PASS** (required candidate gates)

| Gate | Result | Candidate-specific proof |
| --- | --- | --- |
| digest / join chain | PASS | blank+ctl SHA MATCH; `add_ctl` join == page Elite/Pro digests |
| `backup-restore-readback` | PASS | FakeX0X blank `.fwsc` + `--ctl` (ctl `956e3e21…`): slots backup, install, ED_CTL_READ full-block CRC MATCH; both models |
| `interruption-recovery` | PASS | Same candidate install path; mid-restore SMP_WRITE fail-closed exit 7; backup retained; both models |
| `ota-target-binding` | PASS | Blank and page packages load as `SMK-37 Elite_900` / `SMK-37 Pro_900` with page SHA MATCH |
| host suites | installer-slots / slot-backup PASS; `install_f03_slot_probe_test` SUPPORTING_FAIL on this tip (empty FakeSMK `SlotError`) — recorded for Stage-1 audit; **not** used alone as candidate QA |

## Also substantiated elsewhere

| Gate | Evidence |
| --- | --- |
| `package-integrity` / `model-controller` / `memory-flash` | `evidence/f13-stage5/lx73-devcheck/CHECKS.md` |
| `host-regressions` | same (host master PASS on tip) |
| `independent-qa` | `evidence/f13-stage7-lx73/independent-qa-local-review.md` |

**Not claimed:** production promotion. Stage-6 Elite Mac install PASS exists; nightly page `hardware_tested` stays false until Stage 7 labeling policy says otherwise (stays false here).
