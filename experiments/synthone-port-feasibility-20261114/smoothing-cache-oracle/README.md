# Synth One smoothing-cache source oracle

This isolated host test extends the earlier scalar recurrence experiment to the
actual 150-parameter metadata and cached traversal of the pinned public Synth One
sources. It changes no firmware, parameter mapping, saved project or sound bank.
The candidate adapts the upstream develop index traversal and coefficient/recurrence
expressions. The reference compiles the actual pinned AudioKit `sp_port_compute`.

## Inputs and run

Use separately obtained public source checkouts at these exact commits:

- AudioKit/AudioKitSynthOne master `6466a3715c96b7ecf1dd255a218cbd571408a314`.
- AudioKit/AudioKitSynthOne develop `627744f2495863483f95f88887c40a13c2ce0ed2`.
- AudioKit/AudioKit v4.9.2 `03fecf80a66be88f6ac0356cd532c72dbe09922d`.

Set `MASTER_ROOT`, `DEVELOP_ROOT`, and `AUDIOKIT_ROOT` to the corresponding checkout
roots. Set `SOUNDPIPE_ROOT` to `$AUDIOKIT_ROOT/AudioKit/Core/Soundpipe`, then run
`sh run.sh`. A C99 compiler with ASan/UBSan and Python 3 are required. The runner
verifies seven source hashes, the actual metadata flags and the upstream code
expressions before compiling, and binds the compiled Soundpipe directory to the
verified AudioKit subtree. A negative fixture rejects an independently selected
Soundpipe directory. Generated flags contain only public metadata.

## Evidence and scope

The deterministic workload changes targets every seven frames, half-times every
64 frames, and resets every 257 frames over 8,192 frames at each of 44.1, 48 and
96 kHz. It compares all 150 outputs bitwise and the six relevant smoothing-state
fields for all 46 selected parameters. Unsmoothed controls are checked too.

Each of `-Os`, `-O2`, and their ASan/UBSan variants compares 3,686,400 output values,
1,130,496 state tuples and 4,416 reset applications with 384 half-time update
batches. Floating-point contraction and fast-math are disabled. LeakSanitizer is
disabled because this stack-allocated test has no heap ownership under test.
Upstream unused `sp` parameters are the only suppressed strict-build warning.
Expected-failure mutation checks ensure dropped/duplicate selections, stale
coefficients, corrupted unsmoothed outputs, omitted resets and missing output
assignment are detected. Compile errors do not count as successful detection.

The flag lists agree across the pinned master/develop versions. Their metadata
is not wholly identical: index 136 has a different compressor makeup-gain name,
though it is unsmoothed in both. This experiment does not import other develop
changes or validate that parameter's semantics.

This is a limited arithmetic/list-boundary qualification, not a measured CPU
saving, integrated engine test, device result or AudioKit acoustic equivalence.
Actual integration must preserve all initialization, half-time updates, resets,
sample-rate changes, setter cadence and floating-point/compiler behavior. It
must ensure the index list is built exactly once or cleared before rebuild.
Zero/negative half-times, non-finite controls and concurrent setter races are not
qualified here. The unrelated develop silence-horizon FX approximation is not
included. No user-supplied bank values, audio, binaries or toolchain are included.

Public upstream licensing notices are retained in `notices/`.
