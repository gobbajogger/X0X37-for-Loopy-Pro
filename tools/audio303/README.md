# Chicago / production303 private offline experiments

These original host tools render the real `firmware/src/dsp/bass303.c` and the
installed Chicago3.1.6 AU through public AudioToolbox APIs. They preserve firmware
DSP/defaults. This is a limited experimental correspondence, not an emulation or
firmware correction. No vendor code, presets, assets, or binaries are included.

Source baseline: `bd489eda7c4ce4b56770fd24d6eec228d997c778`; bass303.c SHA256
`321bcc141e6f234d098eb5180f47eae1afc3b6195fd08ecb5dbe54ccb8cd0a1f`.
The source tree labels this Open303-derived DSP `bass303`; “production303” is our
host executable name, not proof of the running device's version.

Place the checkout at `WORK/repo`; tools resolve their private workspace as the
checkout's parent. Create `WORK/audio` and `WORK/evidence`. Run commands from WORK.

Importing `analyze.py`, `contour.py`, or `slide_experiment.py` does not read
those private files. Running the file, or calling `main`, is the entrypoint for
a private-reference workflow. `measure.py` holds the feature, distance, and
contour math. Check it with original synthetic tones, without a Chicago plugin
or a capture:

```sh
python3 repo/tools/audio303/test_synthetic_diagnostics.py
```

A passing synthetic run does not retune the instrument. Firmware DSP and the
default sound stay as they are. The private scripts remain the only path that
loads reference audio.
Dependencies already present in this test: Apple clang/AudioToolbox, Python,
NumPy/SciPy; Matplotlib only for plot.py. Do not install or alter permissions as
part of this workflow. Audio and reference manifests remain outside Git.

```sh
cc -std=c99 -O2 -ffp-contract=off -Irepo/firmware/src -Irepo/firmware/src/dsp repo/tools/audio303/render.c repo/firmware/src/dsp/bass303.c -lm -o production303
xcrun clang++ -std=c++17 -O2 -framework Foundation -framework AudioToolbox repo/tools/audio303/chicago.mm -o chicago_host
python3 repo/tools/audio303/prepare.py
```

`prepare.py` creates50 fresh reference trials/production controls and68 source
candidate clips. `extend_grid.py` adds90 production cutoff/envelope candidates.
`chicago_host MANIFEST AUDIO_DIR RECEIPT` runs only the requested AU manifest;
use the supported per-command approval route and stop on denial. No bundle,
editor, device, hardware MIDI, preset, or persistent-permission fallback exists.
The pinned host reads back44.1kHz and22 controls; every trial is a fresh AU.
Common block edges and exact original MIDI are explicit in source/manifests.

After successful approved reference rendering:

```sh
python3 repo/tools/audio303/extend_grid.py
OPENBLAS_NUM_THREADS=1 python3 repo/tools/audio303/analyze.py
python3 repo/tools/audio303/calibrated_render.py --cutoff .6 --note 48 audio/experimental-anchor-n48.f32
```

`analyze.py` fits normalized H2..H12 above−40dB, rejects fewer than3 partials,
selects on MIDI48, and evaluates MIDI60 independently. Gain, pitch, envelopes,
phase and rejected fits stay separate. Only cutoff.6 passed the exploratory
post-analysis screen (≤3dB shape RMSE train/holdout;≤1dB holdout gain error).
The wrapper refuses interpolation, rejected anchors, notes other than48/60 and
existing output files. It writes explicit gain and limitations to a sidecar.
The passed anchor is cutoff104/envmod0/saw/FENV1/sustain127/velocity100; this is a
constant-filter-envelope diagnostic patch, not preserved production presets.

For slide calibration, run `make_slide_holdout.py` to derive a fresh2-repeat held-out reference manifest from
the overlap_glide_mid trial transposed12semitones, IDs slide-holdout-n60-r0/r1,
then render it using the same approved AU route. `slide_experiment.py` trains9
host slew candidates on48→55, selecting120ms, and tests60→67 without retuning.
The optional C argument `--experimental-slide-tau-ms 120` changes only the host
instance's slew coefficient/powers. Without this flag the three independently
rerendered controls are byte-identical. `--experimental-gate-model GAIN_DB BETA SLOPE_DB_PER_S`
is a separate host-only gain on the rendered samples. It restarts on a new
non-slide note, holds through a slide, and keeps the last value after note-off.
Omit it and the file matches the default renderer. The measured coefficients and
the train/held-out scores are in `GATE_AMPLITUDE_RESULTS.md`. That flag does not
change firmware DSP. Output is native float32 mono44.1kHz.
Use `verify.py` for readback/finite/repeat/gain/default/rejection checks and
`contour.py`/`plot.py` for the recorded comparisons. These scripts do not execute
the AU. Calibration does not establish envelope, resonance, accent or general
waveform equivalence. A slide flag is an explicit DSP API input, not a claim
about the firmware MIDI allocator; old-note release is ignored in this harness.
