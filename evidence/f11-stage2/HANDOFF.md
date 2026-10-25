# Workflow handoff to stage 3

Receiving stage: **3 Test** (new cloud thread; F11 after stage-2 BF-007 implement).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f11-bf007-stage2-fb90`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-2 owner: `bc-1ef92313-0365-52c7-bb5d-751d2aef133c`.
Audited / validated source SHA: `af4947846df7233c68a6be72c4d0c634052a2752`.
Content tree: `1da3bbfbbf0df52256acceecbca2e064a1ef821478a33d78591e01813229643d`.
Git tree (pre-repair): `c12a7e747b79aa4619466764ecccc88285d4c10d`.
Firmware tree (pre-repair): `bcc21733ea77097a787cfa6cc617fbdf1dca5da1`.
Audited firmware content tree (pre-BF-005 package bytes baseline): `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`.
Audited firmware reference SHA: `1325f5d34340ea877e65ab86f3663f5dea052c0d`.
Repair commits already in tip: `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001), `ce690772` (BF-004), `94f4a012` (BF-005), `37bab7d1` (BF-006) — not re-opened.
BF-007 implement: split static vs lane BD PAN migration in `firmware/src/app/project.c` (blob `240a4b17…`) + `tests/host/bugfinder_legacy_bdpan_motion_test.c` + `legacy-bdpan-motion` in `tests/run_tests.sh` + `docs/audit/bugfinder-legacy-bdpan-motion-20261023.md`. Stage-1 tip also carries Codex P2 witness-runner path fix (`f625b57d`).
Implement evidence: `evidence/f11-stage2/CHECKS.md`, `DEFERRED.md`, `bf007-red.log`, `bf007-green.log`, `host-unit.log`, `project-runtime.log`.
Audit record: `evidence/f11-stage1/AUDIT.md`.
Successor prompt: `evidence/f11-stage2/SUCCESSOR-STAGE3.txt`.
Successor agent id: `bc-2bc7a358-dad4-5b70-b3e3-d4a3e7b43fef` (Name: F11 BF-007 tests; coordinator CreateAgent — pod had no `CURSOR_API_KEY`).
Successor URL: https://cursor.com/agents/bc-2bc7a358-dad4-5b70-b3e3-d4a3e7b43fef
Handoff status: **complete** — owner-visible Stage 3 id recorded on this remote tip. Local-env review for PR #260: `bc-4cf0f622-19bf-5b1f-b19b-2732fc96f5a3` (subscribed).

## Repair summary

BF-007 only. Static BD PAN still migrates at the post-OBJ_SOUND site; motion lane indices remap only after stream/arrange load via `sound_bd_sends_lanes`. Host regression fails on the unfixed tip (3/171) and passes after repair (171/171); project-stage, project-protocol, and project-runtime stay green. Deferred items in `evidence/f11-stage2/DEFERRED.md`.

## Scope for stage 3

Run the BF-007 host checks on the implement tip. Separate subagents for the test run and for failure investigation. Do not implement deferred items. Do not reopen BF-001–BF-006 or FP-001. Do not fold PR #28. Do not merge production `main`, publish production Pages, or flash a device.

When checks pass, write the stage-4 successor prompt, push it, and start exactly one owner-visible stage-4 Cloud Agent under the same handoff rule. An internal Task subagent does not complete the handoff.
