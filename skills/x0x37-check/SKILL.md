---
name: x0x37-check
description: "Select scoped checks, build, test, commit, or hand off x0x37 changes with matching validation evidence and lane ownership."
---

# Select checks

### Rule 9: supported checks

Select checks from committed edits and their affected consumers. Name the changed path and the failure each check covers.
Use the smallest set that covers those risks. Reuse matching evidence for unchanged inputs.
A stage change, new commit ID, available suite, or install alone does not trigger unrelated tests.
Expand only for an affected dependency, unexplained failure, or applicable explicitly requested formal gate.
For install-time readbacks and unchanged packages, use [the install scope](../x0x37-install/references/artifacts.md).

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

Before a commit, fetch, rebase, push, or lane handoff, read [commit and handoff requirements](references/commits.md). Apply the current authorized destination and actual Git identity; complete the declared outcome under the [continuation contract](../x0x37-workflow/references/handoff.md).
