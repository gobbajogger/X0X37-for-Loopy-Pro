# Immutable baseline host prototype results

2026-10-25. Baseline tree efff32ed2849161e8edfcbb50a99a1c55a69c72f.
These retained results describe the original prototype. Current refinement results
are in `REFINEMENT.md`; no firmware integration or release qualification.

| Configuration | Pitch error cents | Note RMS error dB | H3/H5 relative error dB | Strong-spectrum maximum dB | Lag correlation |
| --- | ---: | ---: | --- | ---: | ---: |
| carrier_filter | -0.031 | +2.513 | +2.08 / +6.25 | 15.07 | 0.999238 |
| carrier_open | -0.031 | +2.231 | +0.30 / +1.61 | 6.00 | 0.999816 |
| square_open | -0.025 | +2.447 | +0.46 / +1.43 | 30.31 | 0.974708 |
| sub_open | -0.334 | +2.177 | -0.04 / +0.52 | 1.62 | 0.999939 |
| mod_triangle | -0.034 | +2.176 | -0.04 / +0.51 | 1.56 | 0.999967 |
| mod_pulse | -0.028 | +7.190 | -8.26 / -9.92 | 88.35 | 0.362343 |
| mix_depth | -0.030 | +3.838 | -18.92 / -14.11 | 42.96 | 0.832139 |

All seven fresh stereo reference repeats are exact. All seven provisional pitch goals
(≤2 cents) pass. All seven absolute steady-envelope (≤1dB) and relative strong-spectrum
(≤1dB) goals fail. These goals are engineering targets, not perceptual equivalence limits.

Spectrum uses16 harmonics at a fixed common theoretical frequency,100–400ms.
The spectral error mask is the union of partials above−40dB relative to the
fundamental in either signal; phase uses only the common strong partials.
Large errors at missing/effectively cancelled partials indicate mismatch, not
precise physical harmonic amplitudes. In particular the symmetric prototype
modulator pulse misses strong even harmonics in the reference. Its correlation
is only0.362 and gain-aligned residual RMS is about0.932 of prototype RMS.
The mixed-depth case also has unexpected strong4th/8th/12th/16th partials.

No gain normalization is applied to envelope or absolute harmonic measurements.
Constant-lag correlation is reported separately within one fundamental period.
Full per-harmonic, envelope, gain, residual, stereo and control/event data are in
[comparison.json](evidence/comparison.json). Fresh carrier/sub/triangle-modulator
waveform correlation is high, but their absolute gain, higher harmonics, release
and stereo still differ. Reference release remains above1e-7 for228–234 frames
after note-off; prototype1439 frames. Stereo0 has exact mono output in the
prototype versus a small nonzero reference side component.

## Active history fails spectral matching

| Configuration | Reference H3/H5 first→second dB | Prototype H3/H5 first→second dB |
| --- | --- | --- |
| carrier_open | -4.093 / -19.782 | +0.000 / +0.000 |
| square_open | -4.184 / -22.765 | +0.001 / +0.001 |
| sub_open | -0.870 / -3.028 | -0.001 / -0.001 |
| mix_depth | -0.864 / -2.878 | -0.004 / -0.010 |

The implementation retains deterministic phase and changes raw samples across
notes. It does not reproduce the reference carrier/square harmonic cancellations.
The equal-two-copy interference hypothesis was not adopted. No oscillator count,
randomness or proprietary modulation/filter architecture is inferred from these results.
See [history-comparison.json](evidence/history-comparison.json).

## Actual compiled resources and checks

- Apple Clang21.0.0, arm64-apple-darwin27.2.0; C99, `-Os -ffp-contract=off`.
- Engine state:144 bytes; no engine heap, global voice state or table bank.
- Compiled object:3656 bytes text,328 bytes literals/constants,352 bytes compact unwind;
  Mach-O size reports4344 bytes total including section padding. This is neither source
  size nor a pi32v2/firmware flash estimate.
- Undefined symbols: none. No libm/stdlib/C++ runtime dependency in the engine object.
- Uninstrumented host benchmark:480000 frames (10s audio) in0.014535 CPU seconds in
  the retained run. Checksum is consumed. Host timing does not predict target deadlines.
- All block sizes1–256 are sample-identical; reset/fresh determinism, note ownership,
  gate/damper/all-notes-off cleanup, exact tail silence, mono/stereo/downmix,
  zero-level nulls, positive controls, glide and finite swept/extreme controls pass.
- ASan+UBSan pass without suppression. A local reciprocal wrapper avoids inherited
  negative-shift UB in the reused fastmath path without changing firmware source.
- Methodology regression: missing/added partials fail spectrum, silence fails pitch,
  gain changes remain separate from normalized spectrum.

Tests are agent executed. Logs and compiler/section evidence are retained under
`evidence/`. Pitch validation applies to the tested nominal note, not full MIDI range:
the engine deliberately clamps above0.12×sample rate. High-note/PM aliasing,
target stack, pi32v2 code/RAM fit, target CPU and hardware are NOT TESTED.

## Next concrete refinement

Keep this baseline immutable. Refine isolated pulse/modulator shape and common
gain/release/filter mapping as original candidate algorithms; fit only a declared
development window and evaluate unfitted windows/harmonics plus second-note
history. Add anti-alias qualification. Continue documenting deterministic spectral
history as a failed target until a model passes holdouts. Do not compensate with
a broad20dB tolerance or assert fidelity from fresh tuning/correlation.
The separate integration owner retains storage/locks/engine selection work.
