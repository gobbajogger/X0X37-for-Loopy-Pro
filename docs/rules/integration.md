# Integration and gate

### Rule 11: branch order and formal validation

The assigned integrator owns firmware branch order and receives checked lane
commits. No second coordinator. Use history-preserving merges, fast forwards,
or cherry-picks in its controlled checkout; never `git update-ref` to evade checks.

Use [scoped checks](commit.md) during development. An edit, commit, merge,
handoff, or finished development batch does not by itself require a formal gate.
Run the full gate only when the next step requires it under [install/release
rules](install.md), or the owner explicitly requests full validation.
At that step, reuse a matching PASS. Repeat only for changed validation inputs
(source, configuration, toolchain, dependencies), repair of a failed run, or a
stated force reason. A changed input invalidates old evidence; it does not by
itself require running a gate before another development edit. Do not repeat
successful gates or reviews without cause.

Run maintained `tools/gate.sh` in supported Linux x86-64, including a local
container. Follow its header and `BUILDING.md` for accessible repository,
reference, work, lock, results, SDK, toolchain, and hook paths. The work path
must be writable through container mounts; do not fake the box with `HOME`.

Keep four DSP reference pins, dev and release checks, model guards, complete
tests, release tree checks, CRC evidence, and budgets. Static RAM free >= 0 B;
pool headroom >= 8192 B. SMK-37 Pro and Elite are maintained targets; keep the
documented FM-1 allowance separate.

Only the gate writes its result. TRIAL PASS, focused tests, quick checks,
setup-only runs, VOID, and handwritten rows are not release PASS. Release
installation requires the gate's exact validation tree key and artifact CRCs;
development installation uses the scoped evidence in [rule 6](install.md).
Record source commit and configurations too. Missing setup leaves gate/release pending,
not independent local work. Docs edits create no device evidence.
