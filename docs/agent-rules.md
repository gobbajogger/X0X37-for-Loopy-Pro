# x0x37 skill triggers

Start with `x0x37-task`. Use `x0x37-writing` before output.
The task skill defines skill use and instruction priority. Read selected `SKILL.md` files, not old rule entries.

- [$x0x37-task](../skills/x0x37-task/SKILL.md): Task start, ownership, source protection, and completion.
- [$x0x37-writing](../skills/x0x37-writing/SKILL.md): All output and saved text.
- [$x0x37-workflow](../skills/x0x37-workflow/SKILL.md): Code stages and handoffs. Excludes docs-only tasks.
- [$x0x37-check](../skills/x0x37-check/SKILL.md): Scoped checks, tests, builds, commits, and handoffs.
- [$x0x37-integrate](../skills/x0x37-integrate/SKILL.md): Firmware integration, candidate validation, release builds, and full gates.
- [$x0x37-install](../skills/x0x37-install/SKILL.md): Device installation, artifact transfer, and requested release packages.
- [$x0x37-device-bug](../skills/x0x37-device-bug/SKILL.md): Device and controller-input diagnosis, captures, and regressions.
- [$x0x37-shared-work](../skills/x0x37-shared-work/SKILL.md): Concurrent edits, shared resources, difficult-to-undo actions, and independent review.
- [$x0x37-owner-decisions](../skills/x0x37-owner-decisions/SKILL.md): Authorization, owner reports, thread contact, and decision records.
- [$x0x37-docs](../skills/x0x37-docs/SKILL.md): Documentation, skill edits, discovery, and rollout.

Maintain procedures in `skills/`. `.agents/skills/` exposes them to discovery.
Running threads with an older skill list can use the paths above.
Inside this workspace, use the current workspace `AGENTS.md`. A standalone checkout uses this index and its own skills.
Read [source status](SKILLS.md) only for skill installation, source checks, or repair.
