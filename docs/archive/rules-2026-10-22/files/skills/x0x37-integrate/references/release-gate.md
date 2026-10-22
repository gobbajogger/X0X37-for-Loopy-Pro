# Release validation and full gate

After an explicit release-build request, run the full gate only for either condition:

- The current release step requires it under [rule 6](../../x0x37-install/SKILL.md).
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

Only the gate writes its result. TRIAL PASS, focused tests, quick checks, setup-only runs, VOID, and handwritten rows are not release PASS.
A release install needs the gate's exact validation tree key and artifact CRCs.
