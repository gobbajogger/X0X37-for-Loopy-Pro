# Bounded original DSP refinement

2026-10-25. Host experiment only; separate integration owner. The immutable
predecessor tree is efff32ed2849161e8edfcbb50a99a1c55a69c72f (local9af8826,
remote2436c4a). Baseline numerical evidence is unchanged. No recording, binary,
proprietary source, preset or inferred private architecture enters this change.

The change adds a DC-corrected10% duty modulator pulse with the already audited
MIT DaisySP PolyBLEP helper at both edges. Carrier and LFO squares retain50% duty.
One shared pre-filter gain changes0.1→0.07725, the common exponential cutoff
base40→12Hz, and fixed release30→4.8ms. Attack remains2ms. No new state,
allocation, per-diagnostic correction, waveform sample table or history model
is added. PM, CRUSH, SVF, stereo and120BPM rate mapping remain approximations.

## Development boundary

`REFINEMENT-PLAN.md` was declared before fitting. Optimization used only fresh
MIDI48/velocity80/48kHz samples100–200ms: absolute H1/H3 for carrier open/filter,
and absolute H1/H2/H3 for isolated pulse. No phase or history fitting. The search
has one common gain/cutoff scale and pulse duty/level. The symmetric pulse,
asymmetric pulse and asymmetric triangle training RMS harmonic errors were
21.236,0.0188 and3.414dB. Missing symmetric-pulse H2 dominates its error; this
is evidence of a failed family, not a precise amplitude measurement of silence.
Rounded constants reduce incidental fit precision. Release duration comes from
the supplied228–234frame observation, not fitting release samples.

## Unfitted200–400ms and H4–H16

The same fixed theoretical frequency and union of partials stronger than−40dB
relative to H1 in either signal are used before/after. Absolute RMS is never
normalized away. Phase/correlation gets only one bounded constant lag, within
one period. Full arrays, hashes and exact events are in `evidence/refinement/`.

- Filtered carrier: RMS+2.501→+0.198dB; held harmonic maximum17.852→0.284dB;
  correlation0.999186→0.999856.
- Open carrier: RMS+2.231→−0.012dB; held harmonic maximum8.973→8.917dB.
- Square carrier: RMS+2.497→+0.233dB; held harmonic maximum20.730→20.598dB.
- Sub: RMS+2.163→−0.078dB; held harmonic maximum2.057→2.044dB.
- Triangle modulator: RMS+2.162→−0.080dB; held harmonic maximum2.005→1.992dB.
- Pulse modulator: RMS+7.100→+0.387dB; held harmonic maximum80.995→6.023dB;
  correlation0.361369→0.960651. Higher-partial mismatch still fails1dB.
- Mixed depth: RMS+3.839→+1.597dB; held harmonic maximum46.762→46.760dB;
  correlation0.829646→0.829629. PM architecture is unchanged and remains a mismatch.

All seven fresh release tails shorten1439→230frames above1e−7, compared with
228–234reference frames. This aligns duration, not the complete release shape.
The original100–400ms analyzer separately reports7/7 pitch goals (≤2cents),
6/7 steady-envelope goals (≤1dB), and1/7 spectrum goals (≤1dB). These are distinct
engineering targets, not a fidelity aggregate. The unfitted windows above can
produce different maxima from that broader window; neither is substituted for
the other. Fresh worst pitch error remains below0.334cents.

## Note and history holdouts

Manifest-verified MIDI55→48 sequences exist for open carrier and mixed depth;
no other velocity or isolated-pulse alternate-note reference exists. These
sequences were excluded from optimization. Carrier first/second RMS errors
improve+2.341/+3.173→+0.097/+0.914dB, but held harmonic maxima remain
14.223/3.545dB. Mix first/second errors improve+3.686/+5.094→+1.443/+2.798dB,
with harmonic maxima35.449/37.234dB. Thus alternate-note gain improves without
establishing spectral generalization.

Corresponding same-note history remains a failed goal. On the unfitted window,
reference carrier H3/H5 changes−4.708/−11.921dB while the candidate stays near0;
square reference−4.818/−13.254dB while candidate stays near0. Sub and mixed history
also fail. The broader baseline analyzer retains its own100–400ms history
measurements. Retaining phase is an explicit original policy, not an explanation
of reference history. The rejected equal-two-copy hypothesis remains excluded.

## Resources and qualification

Independent reviewer reran C99 host tests, ASan/UBSan without suppression, baseline
analysis regressions and new holdout regressions. Pulse tests detect even partials
and DC; release is checked at7sample rates. All pass. Apple Clang21 arm64 `-Os
-ffp-contract=off` object has3948B executable text and4648B total (baseline3656/
4344B), with no undefined symbols. State stays144B. Reviewer benchmark renders
480000frames in0.014764CPU seconds, consuming checksum. Host timing varies and
is not device timing, pi32v2 fit, target stack or native validation.

The48kHz versus192kHz antialias-decimated convergence diagnostic covers four
modes and MIDI48/84/108/127. It retains absolute gain and bounded lag. Residual
contains filter/sample-rate, phase and fractional-delay differences as well as
possible aliasing; it is not a pure alias-energy measure or an alias PASS.
At MIDI127 the48kHz pitch cap forces5760Hz carrier while192kHz allows the
nominal12543.85Hz, so that comparison includes a deliberate frequency mismatch.
Measured candidate open-carrier frequencies at MIDI48/84/108 are130.81227,
1046.50207 and4186.00868Hz, within0.007cents of nominal; MIDI127 measures
5759.99997Hz at the deliberate cap. Isolated pulse follows half carrier, with
measured errors below0.284cents over those tested points. This is selected-point
coverage, not full-range qualification. At MIDI108 full-depth PM rate residual
worsens−21.03→−18.47dB relative to the192kHz render, underscoring that this
refinement is not an anti-alias solution. High-note and full-depth PM remain
unqualified. See the exact rate and measured
pitch results and limitations in `evidence/refinement/rate-convergence.json` and
independent QA. The shared fastmath header and MIT helper bytes remain unchanged.

## Next concrete action

Accept this as an improved original host instrument candidate. Preserve the
immutable receipt for the separate combined integration/resource owner. A later
host stage should compare bounded original PM/anti-alias models using the same
frozen holdouts, with a newly declared development split; do not tune history by
copying reference samples. Full reference equivalence is not established. No
firmware, hardware, protected merge or public audio/binary was produced here.
