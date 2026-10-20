---
name: x0x37-check
description: "Select scoped checks, build, test, commit, or hand off x0x37 changes with matching validation evidence and lane ownership."
---

Initialize this skill when its description matches the current step. Read it once per task.
Load linked skills only when their described step applies. Current owner instructions take priority.

# Check and hand off x0x37 changes

### Rule 9: supported checks

Use development configuration by default. Apply [rule 11](../x0x37-integrate/SKILL.md) before a release build or full gate.
Read a script's header before running it. Read `BUILDING.md` when build or test setup is needed.

Host tests can run on supported macOS or Linux. JieLi target tools require Linux x86-64. You can use a working local container.
Use actual mounted paths, including linked-worktree Git metadata. Never change `HOME` to imitate another machine.

Set `JIELI_TOOLCHAIN` or `X0X_TOOLCHAIN`, `AC79_SDK`, and reference variables as required. Keep exact reference pins.
Set `CC` and `CXX` to single executable names, such as `CC=cc CXX=c++`. Do not put flags in them.

A missing optional reference blocks only its dependent check.

Select checks from the diff and its dependencies: changed behavior, direct consumers, shared code, configuration, and build inputs.
For each selected check, state what the change can break.

Use the smallest set that covers those risks and the step's explicit requirements.
An available check is not automatically required.

Expand coverage only for a concrete dependency, unexplained failure, or applicable formal gate.
A requested formal gate retains its defined coverage.

For documentation-only changes, check relevant links and content. Do not build firmware.
For web-only or host-tool changes, do not build firmware unless firmware inputs or package compatibility can change.
Review each changed rule requirement. Run enforcement tests only when their implementation or contract changes. See [rule checks](../x0x37-docs/references/checks.md).

For shared development changes, do the Elite build and checks only.
If Elite has a PASS result, do not do duplicate Pro builds or target checks.
This result is sufficient for shared checks and memory budgets. It does not give a Pro artifact or device PASS.
Do Pro checks when the change affects Pro-specific code, model selection, identity, loader, or package behavior.
Do checks of each model package that you will install or distribute.
Obey an explicit request for Pro checks and the defined coverage of an authorized full gate.
Do model-independent host checks one time for the same checked inputs.
Add regression tests for meaningful defects.

Use results again in other workflow stages when the checked source, configuration, tools, dependencies, and check scope are the same.
A new stage or commit ID alone does not require another test or build.
Rerun invalidated checks and failed checks after repair. Do not rerun unrelated passing checks.

Record scope, commands, inputs, result, and log path once. Link that evidence in handoffs.
State material coverage gaps. Do not list every unrelated omitted check. Run new required coverage.

Use `X0X_TEST_TIME_SCALE` only where supported. Formal gates use fixed real time.
Run tests in scratch folders and repositories. Never use live queues, results, or status files as test fixtures.

Preserve required code, static RAM, and pool margins. Do not remove features or reduce sample quality for a PASS.
Use commit author `genkigenki <3080081+genkigenki@users.noreply.github.com>`.

Group related repairs into coherent commits with relevant validation.

### Rule 10: lanes and handoffs

Check the base and existing edits. Use `lane/<agent>-<topic>` with `mb`, `sb`, `opt`, `btr`, or `dk`.
Use `lane/dk-rules-<topic>` for rules.

Fetch or rebase only when needed and when the approved source is reachable. Never rebase another agent's work.

The assigned integrator owns firmware branch order. Do not push to its active branch.
Push to approved local repositories only when needed. A checked local commit and requested handoff can complete a lane.

Include the commit, files, checks, limits, next action, and one receiving owner in each handoff.
