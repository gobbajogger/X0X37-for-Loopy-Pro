# x0x37 skills

Initialize the matching skill when its step begins. Read each selected skill once per task.
Load the skill body, not its old rule entry. Links do not request all linked skills.
Current owner instructions take priority over old checkout text and saved handoffs.

- [$x0x37-task](../skills/x0x37-task/SKILL.md): Start an x0x37 task or continuation with current ownership, isolated lanes, source protection, and verified results.
- [$x0x37-workflow](../skills/x0x37-workflow/SKILL.md): Run or hand off an authorized x0x37 code workflow across fresh stage threads, including successor startup and recovery. Excludes docs-only tasks.
- [$x0x37-shared-work](../skills/x0x37-shared-work/SKILL.md): Coordinate x0x37 concurrent edits, shared processes, device resources, hard-to-undo actions, or delegated independent review.
- [$x0x37-check](../skills/x0x37-check/SKILL.md): Select scoped checks, build, test, commit, or hand off x0x37 changes with matching validation evidence and lane ownership.
- [$x0x37-device-bug](../skills/x0x37-device-bug/SKILL.md): Diagnose an x0x37 device or controller-input bug using recorded captures, simulated sends, source evidence, and regression tests.
- [$x0x37-integrate](../skills/x0x37-integrate/SKILL.md): Integrate x0x37 firmware, validate a candidate, or select development versus release checks and full-gate admission.
- [$x0x37-install](../skills/x0x37-install/SKILL.md): Install validated x0x37 firmware, transfer install artifacts, or prepare an explicitly requested release package with data safeguards.
- [$x0x37-docs](../skills/x0x37-docs/SKILL.md): Edit x0x37 documentation, project skills, or instruction discovery and rollout. Preserve decisions, history, and firmware ownership.
- [$x0x37-owner-decisions](../skills/x0x37-owner-decisions/SKILL.md): Resolve x0x37 authorization, process an owner report, contact another thread, or record an owner decision and its source.
- [$x0x37-writing](../skills/x0x37-writing/SKILL.md): Write any x0x37 output or saved text using the owner's strict STE default, project terminology, and relevant writing skills.

Maintain skill sources in `skills/`. Expose them through `.agents/skills/`.
If a running thread has an older skill list, initialize the matching file by its path.
Claim skill initialization only after you read its `SKILL.md`.

Use the current workspace `AGENTS.md` inside the shared workspace.
A standalone checkout uses this index and its own skill files.
Old plans, rosters, logs, and archives provide evidence. They do not assign work or grant approval.
See [writing source status](SKILLS.md) only when source or installation details are needed.
