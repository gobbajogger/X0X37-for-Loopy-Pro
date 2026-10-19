# Tests, commits, and handoffs

### Rule 9: supported checks

Read `BUILDING.md` and the relevant script header. Supported host tests can run
on macOS or Linux. JieLi target tools need Linux x86-64; a working local
container is valid. Use actual mounted paths, including linked-worktree Git
metadata. Never change `HOME` to impersonate another machine.

Set `JIELI_TOOLCHAIN` or `X0X_TOOLCHAIN`, `AC79_SDK`, and reference variables as
needed. Keep reference pins exact. `CC` and `CXX` name single executables
(e.g. `CC=cc CXX=c++`), not flags. A missing optional reference blocks only its
dependent check.

Run affected tests and required checks. Add regression tests for meaningful
faults; docs-only edits need docs checks, not target builds. Reuse a result only
when its checked source, configuration, tools, and dependencies still match.
Rerun affected checks after changes or failures. Record commands, inputs, result,
and log path once; link that evidence in handoffs. New required coverage still runs.
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
