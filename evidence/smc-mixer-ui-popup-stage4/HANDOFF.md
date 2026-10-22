# Stage-4 handoff → stage 2 repair (not stage 5)

Receiving stage: **2 Repair** (fresh cloud thread), then fresh **3 Test**, then **4 Validate** again.
Audit: [docs/audit/smc-mixer-ui-popup-2026-10-22.md](../../docs/audit/smc-mixer-ui-popup-2026-10-22.md).
Stage-4 branch: `cursor/smc-mixer-ui-popup-stage4-5686`.
Stage-4 owner: `bc-03fb7159-af1e-5d80-837b-f4bec84f0cb4`.

## Verdict

**FAIL** — toolchain present; Elite and Pro DEV packages do not link (XIP overflow).

| Model | Baseline e00741e | Candidate ad4f311 / 5109ef6 |
| --- | --- | --- |
| Elite DEV | 488 B free | overflow 248 B |
| Pro DEV | 504 B free | overflow 232 B |

Host suite remains PASS (stage 3). Do **not** treat host PASS as package PASS.

## Do next

1. Launch exactly one Stage-2 repair thread with `stage2-repair-prompt.txt`.
2. After repair + stage 3 PASS, run a new Stage 4 Validate on the repaired SHA.
3. Only then Stage 5 Merge. Stage 6 install remains local.

## Do not

- Merge to main or install firmware from this FAIL.
- Advance to Stage 5 on 5109ef6 / ad4f311.
- Implement page-lock / multi-SMC in the repair.
