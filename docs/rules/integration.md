# Integration and gate

### Rule 11: branch order and formal validation

The assigned integrator owns firmware branch order and receives checked lane
commits. No second coordinator. Use history-preserving merges, fast forwards,
or cherry-picks in its controlled checkout; never `git update-ref` to evade checks.

Default every build and install to development (`X0X_DEV_PADCAL=1`). Build a
release configuration only when the user explicitly requests a release build
for the current work. Otherwise always skip the full gate, including trial,
force, and owner-validation runs. A request to build, validate, finish stage 4,
merge, install, push, or run full validation does not by itself request a release
build. Historical release instructions and successor prompts do not override
the current development default or a later owner correction.

Use [scoped checks](commit.md) and `tools/dev_validate.py` for development.
After an explicit release-build request, run the full gate only when the current
release step requires it under [install/release rules](install.md), or the user
also explicitly requests full validation of that release build. The request
permits the release configuration; it does not remove release/install guards
or authorize publication. At that step, reuse a matching PASS with a recorded
validation-input identity.
Missing identity in older records does not qualify for reuse. Repeat only for changed validation inputs
(source, configuration, toolchain, dependencies), repair of a failed run, or a
stated force reason. A changed input invalidates old evidence; it does not by
itself require running a gate before another development edit. Do not repeat
successful gates or reviews without cause.

For an authorized release build, every full-gate invocation must supply
`--purpose release-install|release-package|owner-validation`
and `--reason` naming the pending release step/artifact or explicit owner request.
A development install, changed input, failed old gate, `--trial`, or `--force`
is not a purpose by itself. Before starting, confirm the named step is current;
use scoped checks while development continues. `--force` additionally names why
matching passing evidence cannot be reused. The script rejects missing/invalid
purpose and blank reasons before setup or writes; it records the caller's reason,
but cannot prove owner intent. Trial runs require `owner-validation`.

Run maintained `tools/gate.sh` in supported Linux x86-64, including a local
container. Follow its header and `BUILDING.md` for accessible repository,
reference, work, lock, results, SDK, toolchain, and hook paths. The work path
must be writable through container mounts; do not fake the box with `HOME`.

Keep four DSP reference pins, dev and release checks, model guards, complete
tests, release tree checks, CRC evidence, and budgets. Static RAM free >= 0 B;
pool headroom >= 8192 B. SMK-37 Pro and Elite are maintained targets. Keep
FM-1 reference-only for porting; no FM-1 target builds or target tests are
required. Preserve shared SMK-37 checks, including tools with legacy fm1 names.

Only the gate writes its result. TRIAL PASS, focused tests, quick checks,
setup-only runs, VOID, and handwritten rows are not release PASS. Release
installation requires the gate's exact validation tree key and artifact CRCs;
development installation uses the scoped evidence in [rule 6](install.md).
Record source commit and configurations too. Missing setup leaves gate/release pending,
not independent local work. Docs edits create no device evidence.
