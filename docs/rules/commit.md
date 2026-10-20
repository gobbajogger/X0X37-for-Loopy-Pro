# Tests, commits, and handoffs

### Rule 9: supported checks

Use development configuration by default. Apply [rule 11](integration.md) before a release build or full gate.
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
Review each changed rule requirement. Run enforcement tests only when their implementation or contract changes. See [rule checks](SCRIPT-CHECKS.md).

For code changes, run affected tests and builds for affected supported targets. Shared code can affect both targets.
Add regression tests for meaningful defects.

Reuse results only when checked source, configuration, tools, and dependencies still match.
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
