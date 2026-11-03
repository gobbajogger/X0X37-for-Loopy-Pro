# Project context and execution

Use fresh ChatGPT Work threads for distinct **LoopyX0X37** tasks. Continue the
same task in its assigned thread and lane through the authorized checks and
delivery. A new logical stage does not require another thread. Preserve an active
owner; use an explicit scoped handoff before another worker takes its files.

## Execution and checks

Use the Work task's actual native or headless tools for authorized repository
inspection, edits, synthetic tests, host tests, builds and artifact verification.
Verify the needed capabilities before assigning work. A task title, saved prompt
or enabled job does not prove a worker started or a check ran. Coordination and
handoff reports do not require another owner reply before authorized work continues.

Run the applicable synthetic checks before asking the owner to test hardware.
The owner performs hardware checks when synthetic testing is smooth. Agents do
not access, probe, install, flash or recover a device under this routing rule.
Required firmware proof and the documentation-only exception remain in
[AGENTS.md](../../../AGENTS.md); required failed, skipped or unrun checks do not pass.

Do not use or control the owner's Safari. Use an authorized isolated agent
browser or headless checks when a synthetic test needs a browser. A missing
browser capability blocks only the dependent check; it does not authorize use
of the owner's browser or device.

Rule 1m remains: no Git over the network in chats. Use the GitHub plugin or
GitHub API to retrieve source, and SDK V1.2.1 from the project files for applicable
firmware builds. Never change `.github/` or add or edit GitHub Actions.
`x0x37-capability-first` is a ChatGPT plugin, not a repository script.

## Handoffs and delivery

Give a distinct Work task a compact handoff: project, owned paths, isolated lane,
exact source revision, requested operations, current authority, relevant skill
paths, completed proof, remaining checks and acceptance condition. Verify the
newest GitHub head before resuming from a chat that fails to load. Read the actual
result through a supported reader; a sent pointer is not receiver consumption.

For code work, select the explicit tstack mode in
[x0x37-workflow](../../x0x37-workflow/SKILL.md). Use the maintained development
workflow for continuation and proof, while preserving the actual target's
integration destination and current action-specific authority. Reading `nightly`
does not retarget a PR or authorize a merge. Preserve one writer per scope,
independent review, required checks, licensing and data safeguards. Keep source
delivery, review, build, integration, publication and hardware acceptance separate.

This supersedes the classic-only execution and fresh-classic-thread-per-stage
routing introduced in PR #720, including its Work test/build prohibition and
fixed model choices. Older handoffs remain historical evidence. Automation
settings, private rules and security, review, merge, publication and device
permissions retain their separate owners and action-specific safeguards.
