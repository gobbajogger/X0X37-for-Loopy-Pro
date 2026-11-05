Ownership and qualification bars have one home: [current owner contract](../../../docs/rules/current-owner-contract.md). Do not fork them here.

# Task ownership and shared work

- Keep each task in its assigned isolated lane with one accountable owner.
  Transfer at meaningful stage boundaries through a qualified receiver under the
  current owner contract; do not migrate active work merely to change chat type
  or create another copy of the task.

  Apply [chat routing](routing.md) before choosing its destination.
  Use new threads at meaningful stage handoffs, not for each task; follow the [current workflow](../../x0x37-workflow/SKILL.md) and its [handoff procedure](../../x0x37-workflow/references/handoff.md).
- Use old chats for context and handoff details. The owner permits new agents to request that information.
  The owner permits requests for a completion notice from a working chat. The owner also permits that reply.

  Read the actual result through a supported reader or requested completion notice.
  Keep independent ready work moving. Do not create a new timer job merely to wait;
  retain existing authorized checkup/watchdog ownership and verify actual handoffs.
  Do not assign new execution work to an old chat.
- At task start, check the merge-queue/ledger record for the scope you will touch.
  Read other project chats only when that record is missing or stale for the scope.
  Re-check the ledger before a shared change if it may have changed since task start.

  Do not duplicate work or compete for ownership.
- Use a separate lane for each task. Preserve other work.
  Stop only your own processes by PID. Never use `pkill -f` or `killall` on shared scripts.
