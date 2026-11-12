# Task ownership and shared work

- Keep each task in its assigned isolated lane with one accountable owner.
  Transfer only when execution needs a qualified receiver; do not migrate active
  work merely to change chat type or create another copy of the task.

  Apply [chat routing](routing.md) before choosing its destination.
  Logical workflow stages do not each need another chat; use the [current workflow](../../x0x37-workflow/SKILL.md) and its [handoff procedure](../../x0x37-workflow/references/handoff.md).
- Use old chats for context and handoff details. The owner permits new agents to request that information.
  The owner permits requests for a completion notice from a working chat. The owner also permits that reply.

  Read the actual result through a supported reader or requested completion notice.
  Keep independent ready work moving. Do not create a new timer job merely to wait;
  retain existing authorized checkup/watchdog ownership and verify actual handoffs.
  Do not assign new execution work to an old chat.
- At task start, check other project chats. Read their current status and scope before shared work.
  Check again before shared changes if that information can be out of date.

  Do not duplicate work or compete for ownership.
- Use a separate lane for each task. Preserve other work.
  Stop only your own processes by PID. Never use `pkill -f` or `killall` on shared scripts.
