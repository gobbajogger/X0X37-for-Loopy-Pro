# Quickcheck scope and controller compile jobs

Quickcheck records its changed paths, target decision and selected tests in
`build/qc/plan.json` before it starts a target build. Proven documentation,
project skills (`.cursor/skills/`, `.agents/skills/`, and `skills/`), rule
entries, web, host and test changes do not need an Elite
build. Firmware, assets, build or package tools, unknown paths, mixed changes
and empty scope retain target coverage. An unreadable base, diff or test index
fails the check. Folder mode includes staged, unstaged and non-ignored untracked
files. Rename detection is disabled for scope so both original and new paths count.

Documentation and rule scope keeps applicable preference pointers and commit
Trigger/Check evidence checks. Other routes run selected tests and require a
reported PASS for every selected name. Test source bytes are indexed once per
invocation. Simulator scenarios keep host-build. Upload generation runs for
selected scenarios with slot inputs or unknown/unreadable commands; ordinary
scenarios avoid it. The upload and upload44 scenarios retain all upload formats.
The index is not saved for reuse by later invocations.

Ordinary Quickcheck skips STE. `X0X_QUICK_ONLY=ste` requests advisory STE plus
rule guards. `X0X_STE_STRICT=1` requests strict STE and fails if the checker
cannot produce a valid result. `X0X_QUICK_ONLY=rules` runs rule guards alone.
`X0X_QUICK_ONLY=devbug` keeps the existing regression and parent negative control.
The fixed-tree regression still runs separately: the normal runner can change
per-test isolation, so its result is not yet an equivalent reusable record.

Controller compilation uses four independent jobs by default. Set `X0X_JOBS=1`
for serial compilation, or select an integer from 1 through 64. Both native and
Docker entry paths carry this setting. Compilation keeps the original flags,
object/link order, final LTO link and image checks. Diagnostics are collected
per source unit and merged in source order. Failures and signals stop and join
owned compiler process groups before deleting scratch output. A failed run
removes old controller output so it cannot appear to be a new successful build.
