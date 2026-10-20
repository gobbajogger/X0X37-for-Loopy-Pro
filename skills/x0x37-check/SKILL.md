---
name: x0x37-check
description: "Select scoped checks, build, test, commit, or hand off x0x37 changes with matching validation evidence and lane ownership."
---

# Select checks

### Rule 9: supported checks

Select checks from the diff: changed behavior, direct consumers, shared code, configuration, and build inputs.
For each check, state what the change can break.
Use the smallest set that covers those risks and the step's explicit requirements.
Add coverage only for a concrete dependency, unexplained failure, or applicable formal gate.
An available check is not automatically required. A requested formal gate retains its defined coverage.

- Documentation: relevant content and links; no firmware build.
- Web or host tools: no firmware build unless firmware inputs or package compatibility can change.
- Rules: review each changed requirement. Use [rule checks](../x0x37-docs/references/checks.md) only for affected content or enforcement contracts.
- Shared development: Elite only. After an Elite PASS, do not repeat Pro builds or target checks.
- Pro-specific code, model selection, identity, loader, or package behavior: do the applicable Pro checks.
- Installation or distribution: check every selected model package. An Elite PASS gives no Pro artifact or device PASS.

Obey an explicit request for Pro checks. Do model-independent host checks one time for the same checked inputs.
Before execution, read [validation and evidence](references/validation.md).
Before target builds, read [build setup](references/build-environment.md). For host checks, use it only when setup is needed.
Read a script's header before execution. Read `BUILDING.md` when setup is needed.
Use development configuration by default; apply [rule 11](../x0x37-integrate/SKILL.md) before release work.

### Rule 10: lanes and handoffs

Before a commit, fetch, rebase, push, or lane handoff, read [commit and handoff requirements](references/commits.md).
