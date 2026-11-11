# B4: synth parameter write ownership

Source-only delivery, per the owner's 2026-11-01 instruction. Codex writes source and tests; the Orchestrator runs all tests and SDK V1.2.1 Elite/Pro builds on the box. No runtime reproduction, red/green result, RAM measurement or hardware result is claimed here.

Base: `build/0.0.23@bb1907494b8d1b52d52ac220dfeb9a0663ea3ee2`.

- [x] Inspect #766, current scanner, UI/SMC edits, motion callbacks and preset adoption; preserve the other briefs' owners.
- [x] Write the recurrent-outlier regression before changing production code. Existing `perf-wheel-noise` already owns this contract and is registered.
- [x] Author endpoint confirmation on two equal, adjacent ADC publications. Add a mod publication sequence in the SMK ADC producer; foreground polls and missed samples cannot manufacture confirmation. Preserve immediate deliberate wheel writes, engine routing and gesture ownership. Narrow bounded calibration/history fields to offset sequence storage; the box must verify linked RAM.
- [x] Author opt-in host diagnostic write-source logging. Cover wheel pitch/mod, panel/SMC knobs, accepted SMC faders, motion application/restoration, preset adoption and MIDI. Disabled tracing adds no target storage or work.
- [x] Author and register fresh-process coverage for both synths/voices, repeated positive spikes with/without REC, interleaved jitter, duplicate foreground polls, real HAL IIR spike tails, sustained range expansion, stationary knob ownership and source labels. Execution remains NOT_RUN.
- [x] Perform independent Compound Engineering source review without executing tests/builds; address relevant findings. See REVIEW.md for findings and execution limits.
- [ ] Push one draft PR into `build/0.0.23`, verify its full remote head, and hand off exact commands to the Orchestrator. Do not merge.

The source hypothesis is cumulative `mhi` growth on isolated above-range samples, followed by a false nonzero mod value at the unchanged physical rest. The existing test supplies one spike, which does not expose cumulative range poisoning. Other write sources are instrumented for discrimination, not declared faulty without runtime evidence.

Independent source review found that repeated foreground calls were not distinct ADC samples, and that one new restoration assertion read a stale diagnostic cache. The candidate now tracks actual mod conversions and the test checks actual runtime bits. No production diagnostic behavior was changed to satisfy a test. Review also required the real IIR producer-chain case; equal filtered-value confirmation excludes its changing tail from endpoint learning.

The bounded follow-up required adjacency, not only distinct sequence IDs: missed rest publications must break confirmation. The final guard admits only modulo-16-bit distance one, updates history on larger gaps, and adds dedicated skipped-publication and wrap regressions.

Preserve pitch centre 64, engine-specific `engine_bass_mod`, external MIDI, motion REC/playback, preset admission and exact sound representation. No new screen or renderer; mockup paths N/A. Main, `.github`/Actions, bootloader/protected device data, FM-1 targets, Miami ADSR and rows 73/120 remain out of scope.
