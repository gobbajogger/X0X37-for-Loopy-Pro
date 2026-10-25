# Independent bounded DSP refinement QA

2026-10-25. A reviewer distinct from the DSP writer inspected the declared plan,
source diff, renderer, measurements and regressions, then compiled and executed
all checks below against frozen source SHA256
`22b310a6e5a2b24015c8934ad4a85fbb2bde487f48a9d98a66e7a4283a9d59d8`.
All reviewed source/dependency hashes are in `qa-source-hashes.json`.

Decision: ACCEPTED as a measured improvement to an original host instrument.
Reference equivalence, full spectral-history matching, low-alias qualification,
firmware integration, target fit/timing and hardware acceptance are NOT established.
The dependency closure retains the inherited fastmath header and pinned MIT
DaisySP BLEP helper. No production source/dependency or private recordings changed.

## Methodology and reproducibility

The writer declared fresh MIDI48 samples4800–9599/H1–H3 before fitting. The
reviewer evaluated unfitted samples9600–19199/H4–H16, corresponding second notes,
and additional existing MIDI55-first/MIDI48-second carrier and mixed-depth trials.
The latter sequence was discovered by checking BOTH manifest entries, not inferred
from filenames. Full parameter readbacks/requested values, MIDI events, block
size, sample rate, component version and exact reference repeats are asserted.
Reference/audio hashes are retained; audio stays outside source control.
No isolated pulse MIDI55 recording exists; velocity/general-note pulse coverage
is therefore limited. Window holdouts share an underlying note and are not
statistically independent instrument coverage.

The new evaluator keeps absolute RMS and per-harmonic errors separate from
relative spectrum, uses fixed common nominal frequencies and the union of strong
partials, and bounds constant lag to one period. Its fundamental-valid flag
rejects silent comparisons. The inherited fresh pitch fit uses100–400ms, overlapping
the training window; its window is explicitly labeled and is not a pitch holdout.
The separate pitch-range probe uses an FFT-initialized single-fundamental fit to
avoid broad local-fit minima at high notes. Neither fit implies perceived pitch
quality under arbitrary PM settings.

Regressions executed successfully for missing/added partial detection, silent
fundamental refusal, absolute gain separation, exclusion of the training window,
lag bounds, and high-note pitch-fit initialization. The baseline comparison
scripts and baseline evidence remain byte-identical.

## Observed improvement and failures

Unfitted200–400ms isolated pulse: absolute RMS error+7.100→+0.387dB;
H4–H16 union-strong maximum80.995→6.023dB; bounded-lag correlation
0.361369→0.960651. The remaining6.023dB spectrum mismatch FAILS the1dB goal.
Carrier-filter held partial maximum17.852→0.284dB and RMS+2.501→+0.198dB.
All seven release endpoints change1439→230frames; reference endpoints228–234.
Release duration informed the chosen4.8ms, so matching duration is not an
independent fit validation. No claim is made that the full release shape matches.

Other unfitted RMS errors after refinement: carrier−0.012dB, square+0.233dB,
sub−0.078dB, triangle modulator−0.080dB, mixed-depth+1.597dB. Mixed-depth
H4–H16 maximum remains46.760dB and correlation0.829629. The waveform refinement
only affects pulse mode; no improvement to PM matching is asserted.

The inherited100–400ms fresh goals now pass all7 pitch goals,6/7 absolute
envelope goals, and only1/7 relative-spectrum goals (carrier_filter). These
are separate engineering goals, not perceptual-equivalence acceptance.

MIDI55 carrier first-note RMS error+2.341→+0.097dB, second MIDI48
+3.173→+0.914dB; full union-spectrum maxima remain14.223/12.471dB.
MIDI55 mixed-depth first-note RMS+3.686→+1.443dB, second MIDI48
+5.094→+2.798dB; union-spectrum maxima35.449/37.234dB. This exposes remaining
mixed-depth/generalization mismatch rather than hiding it behind the fresh fit.

Reference carrier H3/H5 first→second changes remain−4.093/−19.782dB;
prototype changes stay approximately0dB. Square reference−4.184/−22.765dB
also remains unmatched. Retained phase is not accepted as spectral-history fidelity.
No two-copy interference architecture or private reference internals are inferred.

## Pitch range and alias-sensitive diagnostics

At48k, measured carrier pitches for MIDI48/84/108 are130.812/1046.502/4186.009Hz,
within0.008cents of capped expected pitch in this probe. MIDI127 measures5760Hz
because the deliberate0.12×sample-rate cap applies; this is−1347.408cents versus
its uncapped12543.854Hz target. This range check does not justify nominal pitch
accuracy for the capped range. Pulse/modulated base fundamentals are also
measured and recorded, with conservative single-fundamental fit limitations.

Sixteen before/after rate probes compare48k output with192k engine output
antialias-decimated using SciPy resample_poly, with no gain normalization.
These are alias-sensitive convergence diagnostics, NOT isolated alias energy:
filter response, numerical phase and integer-only alignment also affect residuals.
At MIDI48 isolated pulse, residual relative to the192k output changes
−31.89→−34.32dB. At MIDI108 pulse it worsens−18.01→−15.27dB;
high-depth mixed PM at MIDI108 worsens−21.03→−18.47dB. Thus no blanket alias
improvement/low-alias pass is warranted. MIDI127 frequencies differ across rates
because of the cap; those rows are explicitly excluded from rate-convergence
interpretation. True alias-only qualification remains open.

## Executed behavior, sanitizers and compiled resources

`run.sh` compiled with Apple Clang21.0.0 arm64, C99/Os/ffp-contract=off and ran
all host tests. Deterministic/reset/block sizes1–256, last-note ownership,
damper, cleanup, exact silence, mono/stereo/downmix, level nulls/control
sensitivity, glide and finite extrema passed. New tests detect missing pulse
even harmonics/DC and bound short release at7 sample rates8–192kHz.
A separate O1 AddressSanitizer+UndefinedBehaviorSanitizer executable ran the
same behavioral tests with no suppressions and exited0. Analyzer regressions
also exited0. Source inspection found no new allocation, global mutable state,
unsupported module or target-selection path.

Actual object measurement:144-byte state,3948B executable text,4648B Mach-O
segment total (section sum4644B, including352B compact unwind); no undefined
symbols. The consumed-checksum host benchmark renders480000frames in0.014764
CPU seconds in the retained run. Host data is not pi32v2 flash/RAM, target stack,
CPU deadline or device timing evidence. Logs retain compiler and checks.

## Stop and next action

This bounded stage has working reviewed source, exact measurements and private
listenable outputs. Continue mixed-depth PM/generalization and spectral-history
work only in a fresh scoped stage with the immutable fit/holdout evidence retained.
A separately owned integration/resource stage must consume the exact reviewed
source/dependency hashes. No GUI, AU execution, hardware, flashing, firmware
integration, Actions build, protected merge or binary/recording publication ran.
