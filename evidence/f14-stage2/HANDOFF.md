# Workflow handoff to stage 3

Receiving stage: **3 Test** (new cloud thread; F14 after stage-2 BF-013 implement).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f14-bf013-stage2-909f`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-2 owner: `bc-f2aab322-f2e4-5941-abce-ecc3fe5b909f`.
Audited / validated source SHA: `b42bcba46ee7255c18f18c10dd1f99b9fc9bb675`.
Git `source_tree` (pre-repair package attribution): `a15afc439fc8b7bccd62d50403b925add45ae9e5`.
Firmware tree (pre-repair): `5facb3b4405d119a269a05224b9268adfa1baa13`.
Content tree (non-`.md`, Stage-4/7 DEV CHECKED for BF-012): `519ccdf21b46db9198993587cb9334960e67ce483641c744c5caf71b1c769141`.
Implement tip: remote tip of `cursor/f14-bf013-stage2-909f` that contains `evidence/f14-stage2/SUCCESSOR-STAGE3.txt`, `HANDOFF.md`, `CHECKS.md`, `DEFERRED.md`, `bf013-red.log`, `bf013-green.log`, `host-unit.log`, and the BF-013 repair.
Repair blobs: `firmware/src/dsp/fxbus.c` `49a89483e1c777a46758ed370973afb85f15274d`; `tests/host/stereo_test.c` `c8bc1092319f38a46d4f8d41ab5b2a4ef560ff39`.
Repair commits already in tip (do not reopen): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001), `ce690772` (BF-004), `94f4a012` (BF-005), `37bab7d1` (BF-006), `78a0452e` (BF-007), `de11b6d7` (BF-008), `6636cca1`/`b42bcba4` (BF-012).
Implement evidence: `evidence/f14-stage2/CHECKS.md`, `DEFERRED.md`, red/green/host logs.
Audit record: `evidence/f14-stage1/AUDIT.md`, `bf013-definition.md`, `CHECKS.md`, `HANDOFF.md`.
Successor prompt: `evidence/f14-stage2/SUCCESSOR-STAGE3.txt`.
Successor agent id: none.
Successor URL: none.
Successor source: pending coordinator CreateAgent (pod has no `CURSOR_API_KEY`).
Handoff status: **incomplete** until an owner-visible Stage 3 id is recorded on this remote tip.

## Repair summary

BF-013 only. `fxbus_set_bpm` and `fx_retime` accept exact 20 BPM (`>= 20.0f`) so tempo-synced delay stays on the beat at the UI/sequencer minimum. Host red on tip blob `4c748539…`; green after repair. Deferred items in `evidence/f14-stage2/DEFERRED.md`.

## Scope for stage 3

Run the BF-013 host checks on the implement tip (`X0X_TESTS=stereo`, `X0X_TEST_JOBS≥8`, `X0X_DEV_PADCAL=1`). Separate subagents for the test run and for failure investigation. Do not implement deferred items. Do not reopen BF-001–BF-008, BF-012, or FP-001. Do not steal Arp/Fx #279/#281 leftovers. Do not merge production `main`, publish production Pages, or flash a device. Do not fast-forward firmware `nightly` (Stage 5).

When checks pass, write the stage-4 successor prompt, push it, and start exactly one owner-visible stage-4 Cloud Agent under the same handoff rule. An internal Task subagent does not complete the handoff.
