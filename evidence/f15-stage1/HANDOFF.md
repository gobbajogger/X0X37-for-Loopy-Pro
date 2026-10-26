# Workflow handoff to stage 2

Receiving stage: **2 Implement** (new cloud thread; F15 after F13 LX-73 LIVE_PAGE_MATCH).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f15-f03-slotprobe-stage1-237f`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-1 owner: `bc-6c51c5fd-d995-54b7-baf7-b5033919009d`.
Validated **source** SHA (audit / publish tip): `092589b84fd455355216cb0de93c4782236f7524`.
Content tree: `25ada1f02e2e289072fab8db464b715e09f9c0712bded561af008bc88d8c54af`.
Git source_tree: `9382fb1276e320d9f9d290dbad187fa08cdd224c`.
Firmware tree: `a5ba08046700be5cdf2751b7afc1197411b24121`.
Selected defect: **BF-034** (F03 / installer slot-probe fail-closed host negatives; FakeSMK SMP_INFO mute/empty broken).
Live at audit: **LIVE_PAGE_MATCH** `nightly-092589b8-dev` (parallel F14 may later move page to `b48cd800`; Stage 2 repairs against this audited SHA / audit branch tip).
Elite page sha256: `3d107710cde229d7faf3e007ae14e324d7ea5ce209a802642dacf40608b2895e`.
Pro page sha256: `60dc7fda13fdaa3ee456736588e18b2d9e7e50cc34a800504bde678e14ba12d3`.
Evidence: `evidence/f15-stage1/AUDIT.md`, `CHECKS.md`, `bf034-definition.md`, `live-verify.log`, `bf034-f03-slotprobe-witness.log`, `install-test-supporting.log`.
Successor prompt: `evidence/f15-stage1/SUCCESSOR-STAGE2.txt`.
Successor agent id: none.
Successor URL: none.
Successor source: pending coordinator CreateAgent (pod has no `CURSOR_API_KEY`).
Handoff status: **incomplete** until an owner-visible Stage 2 id is recorded on this remote tip. Do not claim a Task subagent as Stage 2. Stage 1 keeps ownership until the id is pushed.

## Scope for stage 2

Implement BF-034 only: repair FakeSMK SMP_INFO mute/empty answering so `install_f03_slot_probe_test.py` and affected install_test fail-closed negatives pass on a stage-2 branch from this audit tip. Do not reopen BF-001–BF-013 or FP-001. Do not steal Arp/Fx sole-owner leftovers. Do not flash. Leave production unchanged. Do not claim Stage 7.
