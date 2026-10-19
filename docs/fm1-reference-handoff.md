# FM-1 reference-only handoff

Receiving owner: “Integrate and review x0x37 work”, the assigned firmware integration chat.

Owner decision: 2026-10-20, recorded in `docs/USER_PREFERENCES.md`. FM-1 is reference-only for porting. Elite and Pro remain build and test targets.

The inspected base is `cf83a97`. Its `tools/gate.sh` still builds FM-1 in dev and rel, checks its exit status and budgets, and reports its package. Remove those target requirements in the integration lane. Do not change a running gate script.

`tests/run_tests.sh` uses `build/x0x.fwsc` for `ota-entry`, `update-loader`, `installer`, and SMK package fixtures. Inspect and migrate coverage needed by SMK-37 to SMK build inputs before removing the FM-1 dependency. Update the gate’s expected-test list with the runner. Preserve model guards, SMK update coverage, shared flash-map checks, and memory budgets. Legacy `fm1` filenames alone do not identify FM-1-only code.

This commit records scope only. No gate script, firmware, or test runner changed. Documentation was checked for an exact owner quote and matching preference copies. No target build, formal gate, install, or device test ran.
