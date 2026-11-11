# N6: restore knob bases on pattern changes

Added to F2 / PR #791. Compared against candidate
`68b7ed8b1be942b05e5124b7fe55eedca6c86360`, based on `build/0.0.23`.

## Source decision

`e6c1cd1cc283c33cf8179360c649440767dcc9b9` and
`03b696f9e090599894aa14330962dad961f31028` have identical file patches.
The former is a republished copy of the latter, not a second implementation.

The N6 restore-before-incoming-apply behavior already exists in
`firmware/src/seq/motion.c:motion_step`: phase 0 restores departing patterns,
then phase 1 applies the incoming pattern. A destination without a lane leaves
the knob at its current base. `restore_base` preserves the optional engine
restore callback, which supports values beyond the integer UI grid.

No firmware hunk is ported. The old commits also retain retired targets in
runtime fields, pack ramp parity and change clear/reuse/Stop handling. Those
broader lifetime changes are not imported for N6: the current supported
`motion_clear(m, k)` API restores the target before freeing the slot and drops
queued requests. The older `motion_clear(lanes, k)` test calls cannot be copied
unchanged. This comparison does not certify arbitrary direct lane-pool replacement.
The historical target-width assertion is tied to the omitted packed fields.

## Regression and executed proof

Extend `tests/host/motion_pattern_handoff_test.c` and register it as
`motion-pattern-handoff` in `tests/run_tests.sh`. The existing four tests for
incoming automation, both allocation orders and tick/no-tick remain intact.
New N6 cases cover:

- Switching to a pattern with no lane restores the base changed since onset.
- Retirement clears the old runtime, and the old lock stays absent for a full pass.
- Another part retains its active automation.
- Returning to the old pattern reapplies its lock; departing again uses the latest base.
- Both fallback apply(base) and the supplied restore callback run; the callback is counted.

`X0X_TEST_JOBS=1 sh tests/run_tests.sh motion motion-pattern-handoff`:
PASS, both selected tests. The host runner prints its existing optional SMK package
skip notice; this is not a zero-skip firmware qualification.

The handoff executable passes 92 assertions (44 existing, 48 N6). A scratch
mutation removing departing-pattern `to_base` fails the N6 current-base
assertion. Production source was never mutated. This demonstrates test sensitivity
for behavior already present, not a new firmware fix.

AddressSanitizer/UBSan: PASS with `ASAN_OPTIONS=detect_leaks=0`.
LeakSanitizer's initial run failed because this environment uses ptrace; leak
detection is NOT_RUN, not passed. The neighboring pending-event clear and
pattern-handoff tests also pass with strict compiler warnings.

## Limits and remaining scope

Full host suite and SDK V1.2.1 Elite/Pro builds with `X0X_DEV_PADCAL=0`:
NOT_RUN; the sparse checkout lacks the complete app/private-synth build inputs.
Elite/Pro free static RAM: NOT_MEASURED. No firmware/layout/configuration changes
are made, so this test addition introduces no firmware RAM allocation.
No visual change or framebuffer/mockup is needed. The original six F2 rows
retain the open acceptance status documented in the recording comparison.
