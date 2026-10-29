# LX-REQ-020 workflow integrity maintenance, 2026-10-29

The owner asked for an end-to-end check of workflow, documents, agents, Dot roles and schedulers, with parallel execution and no duplicated work. This is a bounded continuation of LX-REQ-020. Existing product, ledger, integration and publication owners remain in place.

## Findings and concrete corrections

| Finding | Evidence and result | Correction or remaining owner action |
| --- | --- | --- |
| Prepared continuations bypass explicit dependencies | Native Python reproduces unfinished, missing, cancelled, unsupported-DONE and cyclic dependencies reaching a dependent action at coordination `57a692d79e1c1253af649b10eb4d5922566fa567`. The live LX-REQ-020 has no dependencies; no denied real action or duplicate dispatch was observed. | Require accepted dependency evidence before a prepared continuation reserves work areas or receives an executable next action. Blocked preparation gets read-only dependency reconciliation. Existing active-owner reservations and independent work are preserved. |
| Completed website publication remains a pending action in canonical task data | Source PR90/91 and Pages PR89/92 merged. Current source `a8205aff483dce09414093defdc19cf79d57b719`, Pages `821aa12e6625dd6e7b3c5202d7d0838a420d0bdc`. Source and Pages have the same 148-row history blob; nine versions and eighteen model records remain. Existing publisher actually verified HTTPS and all selections. | Reconcile only LX-REQ-012/014/020 and CATALOG-R48. Retain previous values in history. Mark the old publisher transition complete, reuse its evidence, and retain firmware snapshot/atomic-ownership qualification separately. Do not claim a new firmware or hardware result. |
| Request README presents an old transfer and focus as current | Its opening says Current owner transfer to Task-6, while current delivery records identify the existing Dot sole writer. The old focus also says the engine is unimplemented and website permission pending. | Add one current same-commit entrypoint and label old transfer/focus/table historical. Preserve the complete historical request table. |
| Runtime visibility is incomplete | Twenty-three exact registered/historical automation IDs were read. Two maintenance roles are accessible, enabled and have matching completed results; twenty-one return not-found-or-inaccessible in this context. Both visible prompts exactly match the canonical bootstrap. | No speculative prompt, cadence, lifecycle or ownership change. Existing Orchestrator/control owner must verify its authenticated result route and missing identities in its accessible context. Unknown does not mean deleted, paused or free capacity. |
| Event replacement acceptance is unproved | The three standalone replacements and their three predecessor timers are inaccessible. Prior registration is not exact subscription readback, event delivery or scoped receiver execution. | Existing migration owner retains one logical owner per stage and verifies a real matching event/consumer before any separately authorized predecessor retirement. No replacement scheduler is created by this maintenance. |
| Deadlines and sustained throughput cannot be established from the exposed evidence | Thirty-eight requests remain preserved; five carry unassigned ownership. The task reader supplies only one recent run per accessible task, with no admitted-capacity or heartbeat history. No task-level due-date fields were found in the canonical request records. | Existing coordinator records actual accepted due dates and responsible owners when known, plus expected next action/result in the existing plan. Do not invent dates, liveness or an SLA. Monitor actual handoff and result ages through supported existing roles. |

## Verification

The producer ran the real native Python priority suite: 35 tests pass. Four regression methods were added. Two expose 17 failing baseline subcases for unresolved preparation; two preserve accepted-dependency and ACTIVE behavior. Coverage includes 0/1/2 new-worker capacities, same-owner independent continuation and missing/cyclic/invalid completion evidence. The unchanged feature-intake suite has 26 passing tests. The continuity validator preserves all 38 request IDs and 79 recorded branch heads. The final combined candidate receives a separate independent review and native replay before delivery; its receipt is attached to the maintenance PR.

The helper only proposes work. It does not acquire an atomic remote lock, admit a worker, authenticate arbitrary supplied evidence or control the scheduler. Legacy ACTIVE reservation semantics deliberately remain unchanged. The change prevents the reproduced prepared-continuation error; it does not prove that all possible scheduling races are impossible.

## Reused source and acceptance

- [Existing adopted planner finding](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/676#issuecomment-6486687491).
- [Existing publisher's exact final website acceptance](https://github.com/gobbajogger/x0x37-for-Loopy-Pro_nightly/pull/92#issuecomment-6488870206).
- [Existing Throughput handoff awaiting canonical consumption](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/662#issuecomment-6489311630).
- [Remaining firmware snapshot dependency](https://github.com/gobbajogger/x0x37-for-Loopy-Pro_nightly/pull/58#issuecomment-6488921898).
- [Sanitized exact-ID scheduler evidence](workflow-audit-20261029-schedulers.json).
- [Current source/Pages readback and attributed publisher evidence](workflow-audit-20261029-publication.json).

Three mandatory nightly skill links were already repaired by PR680 before this audit; they were not repaired a second time. The working set is authenticated scoped coordination source, not a complete firmware checkout. No product build, device action, new recurring task, control restoration, forced write, protected firmware integration or binary publication was performed by this maintenance. Canonical adoption remains with the existing sole writer. The audit does not promise uninterrupted runtime or absence of all future regressions.
