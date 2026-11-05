# Lane A engines/DSP — 0.0.25

Completed the supplied bus-FX source optimization and added a runnable differential host test. No extra optimization met a sufficiently clear safety/value bar during the scoped search. No push, remote branch creation, merge, device action, build or host-test execution occurred.

## Source and local commits

Branch: `lane/optimizer-engines-20261106`.
Starting commit: `b2b929f46f1b7868dfa569d0d0989f5ab91aa16e`.
Its Git tree is exactly the supplied `1e9a4c0f83a5ecfd529e4e5cb2dba772efa9441b`; there is no source-base discrepancy. `ce278129` is an ancestor of this lane.

- `f8a015ae6f853c195cdb989eaa25f633d90c9e71`: `firmware/src/dsp/busfx.h` only. Applied `03-busfx-call-invariants.patch` with `patch --batch --fuzz=0 -p1`; no offsets, fuzzy hunks or rejects reported. Hoists exact cutoff target and rhythmic tempo period calculations; retains standalone rhythm helper and sample/state recurrence. Commit records NOT_RUN.
- `ea0d18b69038e2decbf0f54ab524e82e48dbeb88`: `tests/busfx_equivalence_test.py` only. Generates an isolated, symbol-renamed reference from the pinned starting Git blob, then compares all audio buffers, filter/gate state and stutter-ring bytes. Covers all 10 effects, 1–4 channels, cutoff threshold, clamped/fractional BPM, empty/negative lengths, press/release coincidence, null/out-of-range stutter channels, release tails and uint32 sample wrap. Runs at O0/O2 with FMA and fast-math disabled. It requires the pinned Git object and a C99 compiler. No reference firmware bytes or binaries committed separately.

## Gains and resource impact

ESTIMATE, source operation counts only: for a 256-sample call below cutoff 126.5, exp2 and exp evaluations fall from 256 each to 1 each. Rhythmic period work becomes one computation per nonempty call, instead of up to 1,536 source period evaluations with held four-channel stutter. Compiler optimization may already eliminate some repeated work; actual CPU gain is unmeasured. An unheld rhythmic call now calculates one period even if no sample ultimately uses it.

ESTIMATE persistent RAM/sample-pool impact: 0 B. No new fields, arrays, heap allocations or audio pools. No BLE source changed; ESTIMATE BLE RAM impact 0 B. Native code/flash size, stack/register pressure and linker placement are NOT_RUN/unmeasured; no flash-shrink claim or measured no-growth claim. New scalars are automatic locals. The test has no firmware footprint.

## Search and conflicts

Read PERF-REPORT section 3 and FEATURE-REGISTRY. Reviewed existing `bb33a8fe` perf(engines) changes to avoid repeating already-landed master flag snapshots, output snapshots, FX inlining, Chicago drive caching and 808/break inlining. No Miami implementation, MIAMI-OPT, #814 work, ui.c or .github files changed.

Inspected call-invariant candidates in busfx.h, fxbus.c, master.c, breaks.c, drum808.c, drum909.c and engine.c hot loops. Delay/reverb coefficient setup is already outside sample loops. Master detector coefficients are cheap arithmetic and helpers/state require care; no unvalidated change added. Break pitch must refresh at in-block clock/retrigger/grain events. 909 kick frequency and 808 tail thresholds depend on evolving sample state. Engine glide arithmetic is recurrent and must retain float update order. No further new optimization committed.

Potential integration conflict: busfx.h overlaps any concurrent bus-FX edit. The test is a new standalone file; it intentionally does not edit the shared host-suite runner. No overlap with UI or project-stream/undo patches. No claim of exhaustive independent review.

## Validation and next action

Patch application: successful. `git diff --check HEAD~2 HEAD`: exit 0. Starting source identity: exact Git tree match. Host equivalence, full host suite, Elite/Pro builds, timing, code size, pool/link-map measurements and device tests: **NOT_RUN**. Owner AGENTS.md reserves builds and host tests for classic ChatGPT threads and prohibits Codex execution; the added test was not compiled or run here.

Next authorized verification stage: run `python3 tests/busfx_equivalence_test.py` in a classic host-test thread, then the applicable host/native gates and native size checks. Source review and test availability do not establish output equivalence or release qualification.