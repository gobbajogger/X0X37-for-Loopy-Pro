# Integration and gate

### Rule 11: branch order and formal validation

The assigned integrator owns firmware branch order and receives checked lane
commits. No second coordinator. Use history-preserving merges, fast forwards,
or cherry-picks in its controlled checkout; never `git update-ref` to evade checks.

Use focused tests during development and one formal gate for the finished batch.
Repeat for a changed validation tree, changed configuration/toolchain/dependencies,
a failed run needing repair proof, or an explicit force reason. Do not repeat
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
setup-only runs, VOID, and handwritten rows are not release PASS. Installation
requires the gate's exact validation tree key and artifact CRCs; record source
commit and configurations too. Missing setup leaves gate/release pending,
not independent local work. Docs edits create no device evidence.
