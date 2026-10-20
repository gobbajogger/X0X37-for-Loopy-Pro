# Tests, commits, and handoffs

### Rule 9: supported checks

Use the development build by default; apply [release authorization](integration.md)
before selecting a release configuration or the full gate.

Read the relevant script header before running it; read `BUILDING.md` when
build or test environment setup is needed. Supported host tests can run
on macOS or Linux. JieLi target tools need Linux x86-64; a working local
container is valid. Use actual mounted paths, including linked-worktree Git
metadata. Never change `HOME` to impersonate another machine.

Set `JIELI_TOOLCHAIN` or `X0X_TOOLCHAIN`, `AC79_SDK`, and reference variables as
needed. Keep reference pins exact. `CC` and `CXX` name single executables
(e.g. `CC=cc CXX=c++`), not flags. A missing optional reference blocks only its
dependent check.

Select checks from the diff: changed behavior, direct consumers, shared code,
configuration, and build dependencies. For each selected check, name what the
edit could break. Run the smallest set that covers those risks and any explicit
requirement for the current step; a check's existence does not make it required.
Broaden only for a concrete dependency, unexplained failure, or applicable formal
gate. Formal gates retain their defined coverage when that step is requested.

Docs-only edits need relevant link/content checks, not target builds. Rule text
needs review of the changed requirement; enforcement tests apply when its
implementation or contract changes (see [rule checks](SCRIPT-CHECKS.md)). Code
edits need affected tests and builds for affected supported targets; shared code
can affect more than one target. Add regression tests for meaningful faults.

Reuse a result only when its checked source, configuration, tools, and dependencies
still match. Rerun checks invalidated by later edits or failed checks after repair;
do not rerun unrelated passes. Record scope, commands, inputs, result, and log path
once; link that evidence in handoffs. Explain material coverage gaps, not every
unrelated check omitted. New required coverage still runs.
Use `X0X_TEST_TIME_SCALE` only where supported; formal gates use fixed real time.
Tests use scratch folders/repos, never live queues, results, or status files.

Keep required code, static RAM, and pool margins. Do not cut features or sample
quality for a PASS. Use commit author
`genkigenki <3080081+genkigenki@users.noreply.github.com>`.
Group related repairs in coherent commits with relevant validation.

### Rule 10: lanes and handoffs

Check the base and existing edits. Use `lane/<agent>-<topic>` (`mb`, `sb`, `opt`,
`btr`, `dk`); rules use `lane/dk-rules-<topic>`. Fetch/rebase only when needed
and the approved source is reachable; never rebase another agent's work.

The assigned integrator owns firmware branch order. Do not push to its active
branch. Push to approved local repositories only when needed. A checked local
commit and requested handoff can finish a lane. Handoffs give commit, files,
checks, limits, next action, and one receiving owner.
