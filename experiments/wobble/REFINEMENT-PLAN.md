# Declared bounded refinement plan

Declared before candidate fitting on 2026-10-25. Original host instrument only.
One writer owns DSP source. Baseline source/renders remain immutable in the
completed predecessor stage.

Development set: fresh MIDI 48, velocity 80, 48 kHz diagnostics; samples
4800–9599 (100–200 ms), absolute RMS and fixed theoretical-frequency H1/H2/H3
only. Fit at most one common gain, one fixed pulse duty and its level, and one
common cutoff scale. Compare symmetric pulse, asymmetric pulse, and a shaped
triangle/pulse family. Use supplied reference release duration (228–234 frames)
to choose a fixed 4.8 ms release; release shape is not fit to waveform samples.
Do not fit per-configuration corrections, diagnostic sample tables, history,
phase reset policy, or modulation algorithm to the reference.

Holdouts: fresh samples 9600–19199 (200–400 ms), H4–H16 at the same nominal
frequency, onset/release shape, and available corresponding second-note history.
These same-note windows are not independent note/velocity holdouts; no new AU
execution or broad-note fidelity claim is planned. Keep union strong-partial
masks, absolute gain/envelope, pitch, and bounded constant-lag metrics separate.
Report all mismatches; do not widen acceptance thresholds.

Pitch/alias diagnostics: measure original engine output at selected higher MIDI
notes against an oversampled rendering of the same algorithm, with declared
limitations (numerical sample-rate dependence and filter differences). These are
host-only diagnostic measurements, not target fit or audio-quality acceptance.

Stop after one small general implementation with actual compiled resources,
host behavior/sanitizer tests, independent methodology/QA, and measured holdouts.
PM, CRUSH, SVF, stereo and rate mapping remain explicit approximations. Do not
revive equal-two-copy interference or infer proprietary internal architecture.

## Search declaration and result

Objective: least squares in absolute dB, common gain/cutoff fitted jointly to
carrier_open and carrier_filter H1/H3; pulse family duty/level fitted to isolated
mod_pulse H1/H2/H3. H2 of carrier triangle is excluded from fitting because it is
below -60dB. No phase fitting. Common bounds: gain0.05–0.1, cutoff scale0.1–1.5.
Pulse/triangle bounds: duty0.015–0.49, level0.1–2; symmetric pulse duty constrained
0.49999–0.50001 (numerical search only). Fit uses SciPy bounded least squares.
Symmetric pulse training error21.236dB RMS; asymmetric triangle3.414dB;
asymmetric pulse0.0188dB. The selected pulse is a DC-corrected 10% duty analytic
pulse with existing PolyBLEP at both edges. No waveform samples are retained.

Fitted common gain0.07724615, cutoff scale0.30069631; pulse duty0.10147830,
level0.97457452. Implement rounded gain0.07725, cutoff scale0.3, duty0.1 and
level1, reducing incidental parameter precision. Fixed release4.8ms derives
from supplied prior duration; attack2ms remains unchanged. The fitting script
requires the immutable baseline source hash and private reference/work paths.
Actual holdout results are recorded separately by the independent reviewer.
