# Core rules

Read at task start. These rules apply on the Mac, in a local Linux container, and on the box.

### Rule 1: protected data and sources

Keep project pushes on the owner's machines and approved local repositories. Do not push to an outside remote without the owner's explicit request. Never push, merge, or package `refs-oscuro`. Keep proprietary JieLi SDK and toolchain files out of project commits and bundles. Preserve licensing and target model checks.

### Rule 2: process ownership

Stop only your own processes, by PID. Never use `pkill -f` or `killall` on a shared script.

### Rule 4: shared actions

Before a shared or hard to undo action, use the Decision check in [shared-work.md](shared-work.md). Work in an isolated lane. Preserve other agents' edits.

### Rule 6A: completion and install scope

Finish the work the owner authorized. A local code or docs task can finish after its changes, relevant checks, review, commit, and requested handoff are complete. An unavailable device, release service, old role, or box is not a reason to keep completed local work open.

State each evidence level: source review, host tests, target build, formal gate, install, and device test. Give the commit or tree, configuration, checks, and any unrun checks. Never turn a focused PASS into a full gate PASS or device PASS. If a required dependency is absent, finish independent work and name the exact remaining dependency and next step. Do not repeat the same request or poll forever.

Install only when the task includes an install, the finished set is ready, and [install.md](install.md) passes. Keep all queued fixes in that set. Do not install a fallback build to make the keyboard playable. The designated installer owns device writes. Local task completion does not permit an install or release.

### Rule 12A: rule changes

Treat rule changes as code changes. Follow [docs.md](docs.md). Do not let an obsolete role handoff prevent an authorized repair.

### Rule 16: bounded delegation

Use subagents when the owner asks or the applicable task needs an independent review. Give each agent a bounded scope, inputs, current rules, writing skills, and done criteria. Reuse an agent for a follow-up in its scope. Do not create a helper for every small edit. The main task agent remains responsible for completion.

Read the rule for the step:

- [shared-work.md](shared-work.md): shared files, hardware, waits, subagents.
- [device-bug.md](device-bug.md): device reports and regressions.
- [commit.md](commit.md): environments, tests, commits, handoffs.
- [integration.md](integration.md): integration and formal gates.
- [install.md](install.md): backup, install, release evidence.
- [docs.md](docs.md): docs, logs, preferences, rule repair.
- [owner-messages.md](owner-messages.md): decisions and owner messages.
- [style.md](style.md): all output, including subagent prompts.
