# Workflow handoff to stage 3

Receiving stage: **3 Test** (new cloud thread; F13 after stage-2 BF-012 implement).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f13-bf012-stage2-237f`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-2 owner: `bc-a0181730-1daf-5e94-826a-e4339ed37abf`.
Audited / validated source SHA: `de11b6d7e3ac3322d2db689675d9ed27ed667785`.
Git `source_tree` (pre-repair package attribution): `c143a9f8f5b5d0f6804995829ccbe515c77c6103`.
Firmware tree (pre-repair): `9195dce5c4beb5fa8aa564504c8d1d114c8264cd`.
Implement tip (repair commit): `6636cca1778aa13dcb4f51f5f766a056c2b40698`; evidence and successor on branch tip.
Implement repair tree: `62c322a557605d2544a3447fd43f431308dc710c`.
Repair commits already in tip: `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001), `ce690772` (BF-004), `94f4a012` (BF-005), `37bab7d1` (BF-006), `78a0452e` (BF-007), `de11b6d7` (BF-008) — not re-opened.
BF-012 implement: clear look-ahead on LIMIT OFF→ON in `firmware/src/dsp/master.c` (blob `f91a9473f0ae1cdd5e037e521f39fdfcc2022395`) + host regressions in `tests/host/master_test.c` (blob `df2f13b35a6fc7b858b019a1950982c23f6386e3`) — two paths only from open draft PR #68 head `99543b84`.
Implement evidence: `evidence/f13-stage2/CHECKS.md`, `DEFERRED.md`, `bf012-red.log`, `bf012-green.log`, `host-unit.log`.
Audit record: `evidence/f13-stage1/AUDIT.md`, `bf012-definition.md`, `CHECKS.md`, `HANDOFF.md`.
Successor prompt: `evidence/f13-stage2/SUCCESSOR-STAGE3.txt`.
Successor agent id: none.
Successor URL: none.
Successor source: pending coordinator CreateAgent (pod has no `CURSOR_API_KEY`).
Handoff status: **incomplete** until an owner-visible Stage 3 id is recorded on this remote tip.

## Repair summary

BF-012 only. Bypass does not advance the 64-sample look-ahead rings; re-enabling LIMIT now clears `la`/`la_r` and resets hold/env/gain so silent input after OFF→ON yields peak 0.0. Host red on tip blob `b5557b6c…` (stale peak 0.500000); green after repair. Deferred items in `evidence/f13-stage2/DEFERRED.md`.

## Scope for stage 3

Run the BF-012 host checks on the implement tip (`X0X_TESTS=master`, `X0X_TEST_JOBS≥8`, `X0X_DEV_PADCAL=1`). Separate subagents for the test run and for failure investigation. Do not implement deferred items. Do not reopen BF-001–BF-008 or FP-001. Do not steal Arp/Fx #279/#281 leftovers. Do not merge production `main`, publish production Pages, or flash a device.

When checks pass, write the stage-4 successor prompt, push it, and start exactly one owner-visible stage-4 Cloud Agent under the same handoff rule. An internal Task subagent does not complete the handoff.
