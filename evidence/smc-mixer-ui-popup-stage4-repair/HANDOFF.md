# Stage-4 handoff → stage 5 merge (post-repair)

Receiving stage: **5 Merge** (fresh cloud / integrator thread).
Audit: [docs/audit/smc-mixer-ui-popup-2026-10-22.md](../../docs/audit/smc-mixer-ui-popup-2026-10-22.md).
Validated firmware: `828298d9db1a403919e7e4c8176db83512281bb2` (cursor/smc-mixer-ui-popup-stage2-repair-5686 tip).
Stage-4 evidence branch: `cursor/smc-mixer-ui-popup-stage4-repair-5686`.
Stage-4 owner: `bc-ab3a29e8-bb90-5a5c-81a6-88320d3b99a3`.
Prior Stage-4 FAIL: `evidence/smc-mixer-ui-popup-stage4/DEFECT.txt` (ad4f311 XIP).

## Verdict

**PASS** — Elite and Pro DEV packages link and meet budgets on the repaired tip.

| Model | Identity | code_free | ram_free | pool_free | package SHA256 (prefix) |
| --- | --- | --- | --- | --- | --- |
| Elite DEV | SMK-37 Elite_900 | 56 | 508 | 8364 | dfe17da6a21075ca… |
| Pro DEV | SMK-37 Pro_900 | 56 | 508 | 8364 | 7081aee83af0d923… |

Tree: `d006ab424770b3afb13523bb54e360ee13c1b1e89b7c3e58dafa0ab6c88424ab`.
DEV records: `elite/record.json`, `pro/record.json`.

## Do (stage 5)

1. Merge the validated firmware lane (`828298d9db1a403919e7e4c8176db83512281bb2` / stage2-repair branch history) toward main per integrator rules.
2. Do **not** install on device in stage 5. Stage 6 install remains a **local** thread.
3. Do **not** implement page-lock / multi-SMC.
4. Carry Stage-3 host PASS and this Stage-4 DEV CHECKED evidence.

## Do not

- Re-validate `ad4f311` / `5109ef6`.
- Treat host suite alone as package PASS (already closed by this stage).
- Flash firmware from this Stage-4 cloud thread.

## Evidence

`evidence/smc-mixer-ui-popup-stage4-repair/` — RESULT.txt, environment.json, elite|pro records and logs, stage5-prompt.txt.
