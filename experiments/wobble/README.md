# Original wobble engine: host experiment

This implements an original selectable-engine candidate for later evaluation.
It is a working DSP module and host renderer, **not a Miami clone or released
firmware feature**. No firmware build, GUI, device, flash or production branch
selects this code. Target integration has a separate owner.

## Build and exercise

From the repository root on macOS with Clang:

```sh
sh experiments/wobble/run.sh
clang -std=c99 -O1 -g -ffp-contract=off -fsanitize=address,undefined -fno-sanitize-recover=all experiments/wobble/wobble.c experiments/wobble/test_wobble.c -lm -o build/wobble-host/test_sanitized
build/wobble-host/test_sanitized
python3 experiments/wobble/test_compare.py
```

The DSP object has no libm or standard-library undefined references. Tests and
the file renderer use ordinary host I/O and diagnostic math outside the engine.
Analysis requires NumPy/SciPy. `run.sh` uses macOS `size -m`; an ELF host must
select its equivalent section-size command rather than interpreting Mach-O sizes.

To render privately, choose an output directory outside the repository:

```sh
build/wobble-host/render carrier_open /private/output/carrier_open.f32
build/wobble-host/render creative /private/output/creative.f32 8
build/wobble-host/render sweep /private/output/sweep.f32 8
python3 experiments/wobble/compare.py /private/reference-dir /private/output /private/comparison.json
```

The seven configuration names are `carrier_filter`, `carrier_open`,
`square_open`, `sub_open`, `mod_triangle`, `mod_pulse`, `mix_depth`. Generate each
before comparing. `compare.py` writes float WAVs of **original prototype** output
only. The public-API reference host and its diagnostic WAVs remain private.
Each diagnostic note is MIDI48/velocity80 at frame0, off at24000, total48000,
48kHz stereo. Reference repeats are asserted exact in both channels. The
manifest, full control readbacks and WAV hashes pin each comparison. Nothing is
normalized to hide level differences.

For history, render each of `carrier_open`, `square_open`, `sub_open`, `mix_depth`
with2 notes into `NAME-history.f32`, then run `compare_history.py` with the same
three directory/output arguments. The compared gap is24000 frames after note-off.

## State and signal contract

`wobble_t` owns all oscillator phases, LFO phase, filter states, allpass memory,
envelope, current note, gate/damper and parameters. There is no heap, global
mutable voice, wall clock, random source, unbounded collection or C++ runtime.
The engine writes caller-owned planar stereo buffers. Consumers must provide
distinct valid left/right buffers; `(L+R)/2` is its explicit mono contract.
Stereo0 produces exact L=R, which differs from the reference's small residual side.

Fresh initialization is deterministic. Ordinary note-on retains phase and
restarts attack from current envelope; note-off releases only the current note.
There is no held-note stack or fallback to older notes. Velocity0 is note-off.
Damper defers matching note-off; all-notes-off starts unconditional release.
After release, output and filter/allpass memory are exactly zero. Phases remain
retained and idle rendering freezes them. Reset restores fresh phase/state while
keeping parameters. These are explicit original policies, not discovered facts
about proprietary internals. In particular, retained phase does not reproduce
the measured Miami harmonic-history changes.

Controls are normalized0..1, clamp finite out-of-range values, and reject
NaN/Infinity/unknown indices. The first16 match documented functional names;
pitch bend and damper are auxiliary exposed MIDI controls from the AU manifest.
Exact response curves below are **experimental choices**:

| Control | Original implementation |
| --- | --- |
| Carrier wave, level, pitch, sub | Analytic triangle or MIT PolyBLEP square; linear level; pitch ±12 semitones; triangle sub one octave below |
| X-MOD wave, level, tune, depth | Triangle or symmetric PolyBLEP square, half carrier frequency at centered tuning; ±5 semitones; level-gated phase modulation |
| Cutoff | Original two-pass SVF, fixed damping; exponential40Hz mapping capped at0.18×sample rate |
| CRUSH | Saturation before filter; not asserted to be bit reduction, resonance or the reference algorithm |
| Wobble shape, depth, rate | Triangle/square filter modulation;7 fixed rates1–12Hz representing an original120-BPM mapping |
| Stereo | Fixed first-order allpass side component; mono sum preserved |
| Glide | Exponential pitch approach;0 changes pitch immediately |
| Output, bend, damper | Linear gain, ±2-semitone bend, deferred matching note-off |

The fixed2ms attack/30ms release are original choices; no public envelope
parameters are invented. The reference's measured release is much shorter.
Pitch caps at0.12×sample rate: nominal pitch accuracy is not promised above that
frequency. Analytic triangle and high-depth phase modulation remain susceptible
to aliasing. Finite-output tests do not establish low aliasing or audio quality.

## Evidence and limitations

See `MEASUREMENTS.md` for actual compiled-object, behavioral and reference
comparison results. Seven fresh-reference tuning goals pass; all envelope and
spectral goals fail. Phase correlation alone does not establish faithful sound.
Carrier and square first-to-second harmonic changes also fail the reference
history comparison. The rejected equal-two-copy interference hypothesis was not
used as the implementation architecture.

The engine reuses existing X0X float math and a small reviewed MIT DaisySP helper;
`LICENSE-AUDIT.md` records exact source/license/dependency decisions. A local
reciprocal wrapper avoids an inherited negative signed-shift path without editing
the shared firmware math header. Full ASan/UBSan runs pass without suppressions.

Host state/object/benchmark measurements are not pi32v2 code size, target stack,
flash fit, device cycle headroom, real-time performance or hardware validation.
No public binary or audio is part of this source change. A separately reviewed
integration, storage/lock migration, Elite/Pro native fit and device timing stage
must precede any released or hardware-selectable engine.
