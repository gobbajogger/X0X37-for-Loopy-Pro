# B4 independent source review

Compound Engineering `ce-code-review` was delegated to an independent reviewer with correctness, testing, project-standards and adversarial source-only lenses. All four outcomes were collected. External model execution, tests, builds, linters, runtime validators and RAM measurements were not run.

The first frozen candidate received two P1 findings:

1. Consecutive foreground polls could confirm one cached mod ADC sample. The SMK producer now publishes a conversion sequence; learning consumes coherent fresh observations, never duplicate polls. The existing IIR update was factored without changing its arithmetic, so the regression can exercise the real raw-spike tail. Learning requires equal filtered values, excluding a changing tail from confirmation.
2. A new motion-restoration assertion read `perf_seen`, which is not updated by the exact-base restore path. The assertion now compares actual runtime bits with the independently converted control42 value and checks that the saved value remains42. Production diagnostics were not changed to satisfy the test.

The bounded changed-input review found both original P1s addressed in source and confirmed the publication ordering, bounded int16 fields, real-producer test seam and control58 calculation. It identified one further P1: distinct observed highs could straddle unobserved rest publications. The final source requires modulo-16-bit sequence distance one before endpoint growth while still refreshing history on gaps. Dedicated skipped-publication and UINT16_MAX-to-zero regression cases were added. This final narrow correction implements the reviewer's recommendation; it has not been runtime-validated or independently re-run.

No runtime red/green, full suite, Elite/Pro build, device result or free-RAM claim follows from this source review. The Orchestrator owns those checks on the exact PR head before any integration decision. Arbitrary multi-sample noise, first-read calibration and transient pitch/low-mod acquisition remain outside the specific cumulative high-outlier repair; the opt-in source trace helps distinguish them if the original symptom persists.
