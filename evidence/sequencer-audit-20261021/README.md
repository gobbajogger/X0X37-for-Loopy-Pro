# Sequencer, motion and project audit repairs

Base: `7b0bce55c5446a709fc3eecdafb993181654dcab`.
Lane: `lane/sb-sequencer-audit-20261021`.
The checked production and test file identities are in `checked-files.sha256`.

## Changes

- The audio owner retains the target of each applied motion lane. Deleting or
  reusing its slot restores that target's current base at the next audio block,
  step, or Stop. Probability returns to its base of 100. Lane allocation and
  persistence remain unchanged.
- Pattern transitions restore outgoing lanes before applying incoming lanes.
  The result no longer depends on allocation order or equal cached values.
- Ping-pong look-ahead and playback normalize a position outside a shortened
  pattern. Length one and both travel directions remain inside the new length.
- Reverse playback counts a pass only when it wraps from the first step to the
  last. Conditional probability and automatic mutation use complete traversals.
- Historical kick-pan motion indices migrate after arrangement decoding, using
  the saved sound revision. Static sound migration keeps its existing position
  before pan-default repairs. A save and reload does not repeat migration.

The motion runtime target/index share one byte; its voice retains all eight bits
for probability's encoded part. Ramp parity shares the existing ramp flags.
`lane_rt_t` changes from 11 to 12 bytes, so 160 runtime lanes add 160 bytes.
The host `motion_t` changes from 1864 to 2024 bytes. The saved `lane_t` and
project layout do not change. The engine has a compile-time bounds check for
the target/index encoding. Exact target RAM/code budgets remain the integrator's
check; this helper did not build firmware.

## Checks and results

| Scope | Result | Evidence |
| --- | --- | --- |
| Existing motion suite plus pattern order, deletion, slot reuse, probability and unequal ramp timing | PASS | `motion-final.log` |
| Same motion scope, ASan and UBSan | PASS, no diagnostics | `motion-sanitizers.log` |
| Sequencer suite plus live ping-pong bounds and direction/pass/mutation checks | PASS | `sequencer-normal.log` |
| Same sequencer scope, ASan and UBSan | PASS, no diagnostics | `sequencer-sanitizers.log` |
| Project stage suite, real A/B NOR and historical sound revisions 2, 3 and 4 through load/save/reload | 24,857 checks, zero failures | `project-stage-normal.log` |
| Same project stage scope, ASan and UBSan | 24,857 checks, zero failures, no diagnostics | `project-stage-sanitizers.log` |
| Production UI, engine and DSP: pattern switch, Clear Part, final lock removal, probability reset and reverse 1:2 conditions | 54 checks, zero failures | `motion-ui.log` |
| Same production UI/engine scope, ASan and UBSan | 54 checks pass; existing UBSan diagnostic below | `motion-ui-sanitizers.log` |
| New tests against the unchanged base | Expected failures in all four scopes | `baseline-tests.log`, `baseline-motion.log`, `baseline-sequencer.log`, `baseline-project-stage-full.log`, `baseline-motion-ui.log` |

The baseline UI/engine test fails 20 of 54 assertions. Historical pan migration
fails six assertions, including the resave cases. The sequencer log excerpt
records the full raw log hash and shows reverse pass/mutation errors and the
out-of-range ping-pong positions. Baseline files were tested in an isolated
worktree. The original audit reproductions supplied the UI and legacy stream
fixtures; they were converted into positive assertions against production code.

The initial ordinary host run used the first three-byte motion target snapshot.
The final compact snapshot was checked by the final motion test, production UI
test and sanitizer runs. An intermediate odd-ramp test incorrectly assumed the
earlier gesture had measured a 1000-sample odd step; it had measured 500 samples.
That fixture was replaced by explicit unequal 1000/2000-sample steps, and the
final motion checks passed without a further production change.

## Commands

From this checkout:

```sh
X0X_TEST_JOBS=1 sh tests/run_tests.sh motion sequencer project-stage
X0X_HOST_LOAD=0 sh tests/host/motion_ui_test.sh
ASAN_OPTIONS=detect_leaks=0 SAN_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' sh tests/host/run_project_stage.sh
X0X_HOST_LOAD=0 ASAN_OPTIONS=detect_leaks=0 SAN_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' sh tests/host/motion_ui_test.sh
```

The standalone motion and sequencer sanitizer compile commands are recorded at
the start of their logs. LeakSanitizer is disabled because these checks target
memory access and undefined arithmetic, without relying on sandbox `/proc`
inspection. The optional external TB-3PO reference comparison did not run.

## Existing diagnostic and integration follow-up

The full DSP sanitizer run reports a left shift of negative values in
`firmware/src/dsp/fastmath.h:57`, at `b.i += n << 23`. That file is unchanged from
the base. The result is not a clean whole-engine UBSan result. The integrator
accepted ownership of the separate arithmetic correction and its numerical/UI
checks. This lane does not modify that file.

Register the new UI test in `tests/run_tests.sh`:

```sh
run motion-ui sh tests/host/motion_ui_test.sh
```

Add `motion-ui` to the runner's `G` class because it generates shared host
headers. The root integrator owns that runner and the final source merge,
target build, package verification and authorized push. This helper performed
no target build, device test, installation or push.
