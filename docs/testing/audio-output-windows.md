# Audio output window reuse

This change frees 2048 bytes of allocated audio-engine RAM without changing the
render order, quality, polyphony, sample memory or reserve limits. It targets
public nightly `7e4bbfd95da1d9ad86c80541b9bceae48231d006` independently of the
unpublished common-base candidate. No common-only MIDI/live-key/tempo/tape
features are imported.

## Lifetime rule

With WOBBLE enabled, the existing 808 stereo scratch arrays hold one current
subblock until CPU2 completion and 808 mixing. Bass and FX then replace that same
window with final output. Completed earlier windows remain untouched until the
full-block master pass. CPU2 argument bits20..27 carry the window offset; all
808 producers and consumers use it. Send scratch stays separate. Legacy
non-WOBBLE still keeps separate output arrays because bass runs before CPU2 join.
The existing public-base wait/fallback and MIDI behavior is unchanged.

## Reproduction

Generate unchanged host assets, then run:

```
sh host/build_host.sh --generate-only
python3 tests/host/engine_output_window_test.py --output /tmp/audio-window-proof
```

The actual host engine and DSP compare baseline and candidate PCM for WOBBLE
on/off, normally and with ASan/UBSan. Each run covers 800 blocks, uneven sequencer
splits, zero/one/255/256 frames and a 300-frame request capped at256. It simulates
synchronous fallback, deferred CPU2 completion, failed completion and partial-job
fallback; output guards and unwritten tails are checked. A deliberately raw alias
must differ from baseline. LeakSanitizer alone is disabled because of ptrace
incompatibility; address and undefined-behavior checks remain enabled.

The first transferred harness failed to compile because it referred to a
common-only host CPU2 context flag. Removing those two harness assignments
matches the public simulator; no firmware fallback behavior changed.

## Native measurements

Actual baseline/candidate builds use the same pinned JieLi4.0.1 and AC79V1.2.1
inputs, DEV_PADCAL=1, BASS_PRESET_PREVIEW=1, BASS_TAGGED_SOUND=1,
BASS_NOTE_SOUND=0 and fresh generated assets. Both Elite and Pro builds pass.

| Measurement | Baseline | Candidate |
|---|---:|---:|
| Main RAM, both models | 98224 / 98304 | 96176 / 98304 |
| Pool, both models | 323028 / 344064 | 323028 / 344064 |
| Elite app bytes | 354392 | 354392 |
| Pro app bytes | 354392 | 354376 |

RAM saving is actual allocated2048 bytes; free main RAM rises80→2128 bytes.
These are changed-base app build measurements, not the separate common-base
owner-test packages. No controller rebuild, full-runtime synth-fit, CPU-speed,
hardware, installation or publication claim. Existing SMC-B connectivity issue
is not fixed by this change. Proprietary inputs and generated binaries stay private.
