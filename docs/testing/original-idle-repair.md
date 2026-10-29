# Original-engine idle work repair

The owner reported 113% idle audio load with peaks to 120% after activating the
original Chicago/Miami profile. The installed device bytes and device timing
were not independently read back in this execution. The measurements below
identify and remove unnecessary work in the code; they are not a new device
CPU percentage.

## Cause and change

The native compiler uses `-Os -fno-builtin`. Plain four-byte `memcpy` calls used
to reinterpret float/sample bits therefore became actual calls to a byte-copy
routine. A default Chicago/Miami pair executed 981,240 such calls per second in
cold or settled silence. Output conversion also performed 176,400 native
floating-point divisions per second. This was not a software-division helper.

The repair reuses the existing C99 binary32 representation helpers in six hot
units. Bulk/state copies are retained. Output conversion now constructs the
exact round-to-nearest-even binary32 result of `int16 /32767` using integer
arithmetic and a short zero path. It does not multiply by an approximate
reciprocal. Every signed 16-bit input agrees with the independent Fraction oracle,
including the known positive/negative 513 rounding witnesses.

The runtime continues advancing clocks, modulators, envelopes and effects.
An activity flag does not prove that effect history or phase can be frozen:
the silent 256-frame probe advances both clock accumulators, the Chorus history
head and phase, and the Ring/Phaser phases. Those recorded states are unchanged
by the repair.

## Comparative host measurement

Production runtime/backends were compiled at `-Os -fno-builtin`, with strict
floating-point settings and `ORIGINAL_USE_SOFT_FMA`. The host's fortify wrapper
was disabled explicitly because it otherwise inlined small copies despite
`-fno-builtin`. Timing and instrumented call-count builds were separate. There
were 594 serial timing trials: 22 case/phase combinations, three code variants,
and nine trials per combination.

| Complete requested part set | Baseline median host ns/frame | Repaired median host ns/frame | Reduction |
| --- | ---: | ---: | ---: |
| Miami, cold silence |117.03|75.47|35.52%|
| Chicago, cold silence |138.90|91.19|34.35%|
| Default pair, cold silence |252.69|165.15|34.64%|
| Default pair, settled after release |254.73|163.31|35.89%|
| Default pair, active notes |682.26|515.95|24.38%|
| Two Miami parts, active notes |882.20|656.06|25.63%|
| Two Chicago Chorus parts, active notes |517.54|421.34|18.59%|

For settled default-pair silence, trial ranges were 242.83–324.80 and
159.24–171.04 ns/frame respectively. These host ratios cannot be converted
directly into JieLi load percentages.

For the default pair, four-byte copies fall from 981,240/s to zero in silence
and from 2,209,540/s to 14,184/s during the active-note case. Remaining active copies
are control-rate operations. The 692 remaining silence `memcpy` calls/s copy
complete output buffers. Output conversion invocation counts, scheduler entries,
voice/effect/master calls and software-FMA invocation counts are unchanged.

All compared output-word hashes, sample statistics and recorded states match.
All seven compact Chicago effects were included in the cold-silence matrix.
The matrix is not a worst-case sweep across every program and concurrent action.

## Independent correctness and target evidence

- Existing independent rational oracle: all 65,536 int16 outputs, normal and
  ASan/UBSan; maintained 257×4096-frame stereo composition also passes.
- Finite FMA adapter: 3,375 edge triples and 1,000,000 random triples agree with
  host `fmaf`, normal and ASan/UBSan.
- Chicago compact suite:four arithmetic/sanitizer variants retain exact
  kernel hash `12029e7d37ee5bf4` and backend hash `2d3a2467a4153b7c`, covering
  1,408,157 kernel frames and 63,140 real backend frames.
- All six changed units compile with the pinned JieLi target commands. Native
  output conversion is call-free, including native `clz`; the changed hot
  bitcast calls disappear. Allocated object sections total 474 bytes less.
  That object delta is not a final linked application-size claim.

The local performance commit is `baa6b6c2bcc84de57d93564966921391efa11080`,
tree `54dc7b032fad8249b47db9a1cc68f70743e2380c`. All 118 compiler-derived production
inputs in the comparative harness match that commit. A later publication tree
must retain that exact DSP closure or provide its own changed-input evidence.
Whole-app UI/ingress/project proof, final model packages and physical-device
timing are separate checks with their own source/input pins.

## Subsequent zero-input fast path

The final source also short-circuits positive and negative binary32 zero in
`miami_fx_input_rne`, after the existing null-output/invalid-FZ guard. Both
valid FZ modes produce the exact integer zero. No clock, effect, envelope or
runtime work is skipped. Invalid arguments retain their failure result and
leave the output sentinel unchanged. The normal and ASan/UBSan conversion
checks cover both zeros, both valid FZ modes, null outputs and invalid FZ
values, alongside the exhaustive output oracle and stereo composition checks.

This is a separate delta from the 594-trial comparison above: 117 of its 118
production inputs are identical, and only the conversion C file changes.
Its SHA256 is
`b9ae9a070509cea7ba0f3d292279aebf6f4d2ba640ecca4a26c264dec4e2e8bd`.

The pinned native compiler emits a call-free zero path: three nested calls
and one floating multiply disappear. The default mixed pair makes 88,200
Miami input-wrapper calls per audio second, so cold/settled silence avoids
264,600 actual nested native calls per second. The 529,200-call saving applies
to two Miami parts. Chicago uses a separate input-conversion path. The
wrapper grows by 14 bytes; the nonzero path adds three instruction operations
and retains its existing callees. These are instruction/call observations,
not cycle measurements or a final linked size result.

A bounded follow-up used six counter runs and 54 serial elapsed trials
(cold, settled and active mixed-pair cases, two variants, nine trials each).
PCM hashes, sample summaries, recorded states and every non-conversion call
count remained identical. Active-note conversion counts were unchanged.
Median host changes were -7.95%, +2.03% and -1.72% respectively, with broad
overlapping ranges; this sample does not establish an additional latency
improvement. The deterministic reduction of work in silence is established.
The original 594-trial packet remains unchanged.

The microcheck receipt SHA256 is
`bbc7de07d12eee7bb236e1a163b69e2c82796c892d2fc0cdcb80fb30e2202d06`;
the target zero-path receipt SHA256 is
`6303db652451cb87e877c92dcc15f94f33239595e72065fa852eb44ded8aefde`.
Whole-device idle load, worst-case load and deadline behavior still require
the exact resulting firmware on the owner's hardware.
