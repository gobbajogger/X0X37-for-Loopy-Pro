# Workflow handoff to stage 2

Receiving stage: **2 Implement** (new cloud thread).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f03-stage1-fb90`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-1 owner: `bc-2e0f6b6e-50b2-506d-92f4-e19a81d6cfac`.
Audited / validated source SHA: `4ee8f37714b0892f2c70d157fda9328da393e987`.
Content tree: `7ad3a50da8abac890215dae3c28197a2fa90041b8615e828b6b3c6e18060fb83`.
Git tree: `c63af7972f3ca6e5d4e8ac3aac5e70abc8178265`.
Audit record: `evidence/f03-stage1/AUDIT.md`.
Checks: `evidence/f03-stage1/CHECKS.md`.
Prior publish evidence: `evidence/f02-stage7/CHECKS.md`, `HANDOFF.md`.
Deferred younger items: `evidence/f02-stage2/DEFERRED.md` (BF-001–003 and others remain).
Successor prompt: `evidence/f03-stage1/SUCCESSOR-STAGE2.txt`.
Successor agent id: none.
Successor URL: none.
Handoff status: **incomplete** until an owner-visible Stage 2 id is recorded on this remote tip. Pod has no `CURSOR_API_KEY`; coordinator CreateAgent is required.

Selected defect: **F03**. On the same-keyboard install path, a failed sample-slot layout probe currently skips the prewrite backup and continues the write. Approved repair: fail closed before write when the probe fails; keep `--no-slots` and successful empty/no-risk parses.

## Scope

Stage 2 only. Apply the approved F03 fix in `tools/fm1_install.py` and `web/x0x_breaks.js`. Do not implement BF-001–BF-003 or other deferred items. Do not merge production `main`, publish production Pages, or flash a device. Do not change firmware, controller, or package bytes.

When checked commits are on the remote, write the stage-3 successor prompt, push it, and start exactly one owner-visible stage-3 Cloud Agent under the same handoff rule (coordinator CreateAgent if no API key). An internal Task subagent does not complete the handoff.
