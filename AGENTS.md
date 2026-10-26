# x0x37 project instructions

Read [x0x37-task](.agents/skills/x0x37-task/SKILL.md) at task start and [x0x37-writing](.agents/skills/x0x37-writing/SKILL.md) for output. Load only the project skill relevant to the current step. [x0x37-workflow](.agents/skills/x0x37-workflow/SKILL.md) routes code and PR work through an explicitly selected tstack mode; installing a skill selects no mode. Current owner instructions take precedence over saved handoffs and older checkout text.

Keep firmware source integration with the assigned integrator. Firmware `nightly` is the development integration branch; `main` is production and requires same-byte, matching-model hardware validation plus current action-specific authority. Review exact source and artifact evidence and use [scoped checks](.agents/skills/x0x37-check/SKILL.md). A documentation-only workflow change does not start a firmware build or automatic successor loop.

Determine device permission for the exact model and action from the latest owner instruction. Do not infer a device write or recovery grant from a workflow mode, a connected device, or an old standing instruction. Do not force push, bypass a protected merge, or treat an incomplete check as PASS.

Optional host tooling gaps may be recorded as NOT_RUN while independent work continues. A check that actually runs and fails remains FAIL. GitHub Actions, billing errors, and review comments are evidence to inspect, not substitutes for local proof. Follow [local proof](docs/rules/local-proof.md) where it applies.
