# Stage-5 handoff → stage 6 LOCAL install

Receiving stage: **6 Install** — must be a **local** (hardware) thread.
Do **not** start Stage 6 as a cloud agent. Do **not** flash from cloud.

Audit: [docs/audit/smc-mixer-ui-popup-2026-10-22.md](../../docs/audit/smc-mixer-ui-popup-2026-10-22.md).
Validated firmware: `828298d9db1a403919e7e4c8176db83512281bb2`.
Merge vehicle: PR #13 (`cursor/smc-mixer-ui-popup-stage2-repair-5686`).
Stage-5 owner: `bc-9c0a7607-f7c1-569a-b2d1-b9f35d58cdc1`.
Stage-5 evidence branch: `cursor/smc-mixer-ui-popup-stage5-5686`.

## Stage-5 verdict

**MERGE-PREP COMPLETE** — PR #13 is the correct vehicle; tip preserved; conflicts
with `main` (#9) documented. Cloud tools cannot land or undraft the PR. Exact
integrator steps: `MERGE-INSTRUCTIONS.md`.

## Stage-6 do

1. Parent/owner starts **one local** install thread using `stage6-prompt.txt`.
2. Flash Elite DEV package identity below (or Pro if that is the attached device).
3. Device-check peek / hold / restore per the checklist in `stage6-prompt.txt`.
4. Do not implement page-lock / multi-SMC.

## Validated package identities (Stage-4 PASS on 828298d9)

| Model | Identity | package SHA256 |
| --- | --- | --- |
| Elite DEV | SMK-37 Elite_900 | `dfe17da6a21075caaf2661c6e7701e1c94761357d53e6456cebde7c3d1afe6d7` |
| Pro DEV | SMK-37 Pro_900 | `7081aee83af0d923ff7b502f3f40e95898703334c821544244a9e21c4ab6d7a9` |

Controller SHA256: `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a`.
Tree (Stage-4): `d006ab424770b3afb13523bb54e360ee13c1b1e89b7c3e58dafa0ab6c88424ab`.

## Integrator parallel track

Landing on `main` still needs conflict resolution on PR #13 (`MERGE-INSTRUCTIONS.md`).
If firmware sources change in that merge, re-run Stage-4 on the merge tip before
treating merged packages as install candidates. Stage-6 device check of the
**pre-merge** validated packages remains valid for feature UX.
