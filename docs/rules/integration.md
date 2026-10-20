# Integration and gate

### Rule 11: branch order and formal validation

The assigned integrator owns firmware branch order and receives checked commits from contributing lanes.
Do not create a second coordinator. Use history-preserving merges, fast-forwards, or cherry-picks in the controlled checkout.

Never use `git update-ref` to avoid checks.

Use development configuration for every build and install by default: `X0X_DEV_PADCAL=1`.
Build release configuration only when the user explicitly requests a release build for the current work.

Until then, skip the full gate, including trial, force, and owner-validation runs.
A build, validation, stage-4, merge, install, push, or full-validation request alone is not a release-build request.

Historical release instructions and handoff prompts cannot override this default or a later owner correction.

For development work, use [scoped checks](commit.md) and `tools/dev_validate.py`.
After an explicit release-build request, run the full gate only for either condition:

- The current release step requires it under [rule 6](install.md).
- The user also requests full validation of that release build.

A release-build request permits that configuration. It does not remove install or release guards or permit publication.
Reuse a matching PASS with a recorded validation-input identity.

An older record without that identity cannot qualify.
Rerun only after changed validation inputs, repair of a failed run, or a stated force reason.

Validation inputs include source, configuration, toolchain, and dependencies.
A changed input invalidates old evidence. It does not itself require a gate before another development edit.

Do not repeat passing gates or reviews without cause.

For each authorized full-gate run:

1. Supply `--purpose release-install|release-package|owner-validation`.
2. Supply `--reason` with the pending release step, artifact, or explicit owner request.
3. Verify that the named step is current.

A development install, changed input, old failure, `--trial`, or `--force` alone is not a purpose.
Use scoped checks during development. With `--force`, also state why you cannot reuse matching passing evidence.

The script rejects an invalid purpose or blank reason before setup or writes. Its recorded reason does not prove owner intent.
Trial runs require `owner-validation`.

Run maintained `tools/gate.sh` on supported Linux x86-64. You can use a local container.
Use its header and `BUILDING.md` for repository, reference, work, lock, results, SDK, toolchain, and hook paths.

The mounted work path must be writable. Never change `HOME` to imitate another machine.

Preserve the four DSP reference pins, development and release checks, model guards, complete tests, release-tree checks, and CRC evidence.
Keep static RAM free >= 0 B and pool headroom >= 8192 B.

Maintain SMK-37 Pro and Elite targets.
Keep FM-1 as a porting reference only. Do not require FM-1 target builds or target tests.

Preserve shared SMK-37 checks, including tools with legacy `fm1` names.

Only the gate writes its result. TRIAL PASS, focused tests, quick checks, setup-only runs, VOID, and handwritten rows are not release PASS.
A release install needs the gate's exact validation tree key and artifact CRCs.

A development install uses [rule 6](install.md). Record the source commit and configurations too.
Missing setup blocks dependent gate or release work. Continue independent local work.

Documentation changes do not create device evidence.
