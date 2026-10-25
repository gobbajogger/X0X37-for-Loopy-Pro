# Workflow handoff to stage 4

Receiving stage: **4 Validate** (new cloud thread; F11 after stage-3 BF-007 test).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f11-bf007-stage3-fb90`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-3 owner: `bc-2bc7a358-dad4-5b70-b3e3-d4a3e7b43fef`.
Audited / validated source SHA: `af4947846df7233c68a6be72c4d0c634052a2752`.
Content tree: `1da3bbfbbf0df52256acceecbca2e064a1ef821478a33d78591e01813229643d`.
Git tree (pre-repair): `c12a7e747b79aa4619466764ecccc88285d4c10d`.
Firmware tree (pre-repair): `bcc21733ea77097a787cfa6cc617fbdf1dca5da1`.
Audited firmware content tree (pre-BF-005 package bytes baseline): `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`.
Audited firmware reference SHA: `1325f5d34340ea877e65ab86f3663f5dea052c0d`.
Tested implement tip: `4f96b0d9f5f11f700ea3fc935800b8714e11f35d` plus this stage-3 evidence tip on `cursor/f11-bf007-stage3-fb90`.
BF-007 repair commit: `78a0452e` (`project.c` blob `240a4b17…` + `tests/host/bugfinder_legacy_bdpan_motion_test.c` + `legacy-bdpan-motion` in `tests/run_tests.sh` + `docs/audit/bugfinder-legacy-bdpan-motion-20261023.md`).
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001), `ce690772` (BF-004), `94f4a012` (BF-005), `37bab7d1` (BF-006).
Test evidence: `evidence/f11-stage3/CHECKS.md`.
Implement evidence: `evidence/f11-stage2/CHECKS.md`, `evidence/f11-stage2/DEFERRED.md`.
Audit record: `evidence/f11-stage1/AUDIT.md`.
Successor prompt: `evidence/f11-stage3/SUCCESSOR-STAGE4.txt`.
Successor agent id: none.
Successor URL: none.
Handoff status: **incomplete** until an owner-visible Stage 4 id is recorded on this remote tip. Pod has no `CURSOR_API_KEY` (API create expected → 401); coordinator CreateAgent is required.

## Test summary

BF-007 only. Host regressions ALL_PASS under dedicated test-run subagent `bc-27ab5431-773a-5a81-8b8a-c9c13fbc45c9`: `sh tests/host/run_bugfinder_legacy_bdpan_motion.sh` exit 0 — 171/171; `X0X_TESTS="legacy-bdpan-motion project-stage project-protocol project-runtime" sh tests/run_tests.sh` exit 0 (Pillow 13.0.0 installed for project-runtime); migration-order review PASS (`sound_bd_sends` static PAN post-OBJ_SOUND; `sound_bd_sends_lanes` after stream/arrange; blob `240a4b17…`). No failing required check, so no failure-investigation subagent and no return to stage 2.

## Scope for stage 4

Build Elite and Pro development packages (`X0X_DEV_PADCAL=1`) from the tested candidate on `cursor/f11-bf007-stage3-fb90`. Record exact package and controller identities, budgets, checks, and logs. Host PASS is not a package PASS. Do not implement deferred items. Do not reopen BF-001–BF-006 or FP-001. Do not fold PR #28. Do not merge production `main`, publish production Pages, or flash a device from this stage. Owner advance-approved flash + nightly publish apply later (Stage 5/6/7); still no production main.

When the affected development packages are DEV CHECKED, write the stage-5 successor prompt, push it, and start exactly one owner-visible stage-5 Cloud Agent under the same handoff rule. An internal Task subagent does not complete the handoff.
