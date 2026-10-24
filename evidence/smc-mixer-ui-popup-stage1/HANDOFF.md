# Stage-1 handoff → stage 2 implement

Receiving stage: **2 Implement approved fixes** (fresh thread).
Audit: [docs/audit/smc-mixer-ui-popup-2026-10-22.md](../../docs/audit/smc-mixer-ui-popup-2026-10-22.md).
Base: `e00741e2c1e8a018e7a7229dfd279e26aec34126`.
Stage-1 branch: `cursor/smc-mixer-ui-popup-stage1-5686`.
Stage-1 owner: `bc-01a16423-4043-76bd-8bc4-e110c6d15686`.
Stage-2 successor started: `bc-14736e19-2d5a-5b06-97af-527185058605`.

## Do

1. Branch from the stage-1 tip (or `main` plus this audit commit): `cursor/smc-mixer-ui-popup-stage2-5686`.
2. Implement temporary mapped-page peek / hold / restore in `firmware/src/app/smc_mixer.c` per the audit map and defaults (1200 ms hold; peek on PICK and SMC; no clip peek; no `set_view` for peek/restore).
3. Document in `docs/SMC_MIXER.md` and `BEHAVIOR.md` (SMC-05).
4. Add host tests that assert view/page/`mixsel` peek, timer refresh, restore, and help/ASK/clip guards. Run them. Record logs under `evidence/smc-mixer-ui-popup-stage2/`.
5. Commit and open/update the PR. Create exactly one fresh **stage 3 Test** successor thread with this handoff plus your commit SHA.
6. Do **not** install firmware. Install is stage 6 on a **local** thread only.

## Do not

- Implement SMC page-lock or multi-SMC (owned by `bc-9d689865-6ced-488b-a8e7-31248e12a0aa`).
- Call `set_view` for peek/restore (held-step cancel / `prev_view` corruption).
- Advance to merge, install, or push from stage 2.

## Successor requirement

Before finishing stage 2, start one new thread for stage 3 (test and investigate) with the candidate SHA, test commands, and evidence paths. Stage 6 must be local.
