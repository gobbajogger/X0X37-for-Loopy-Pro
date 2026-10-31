# B2 diagnostic stage — acceptance pending

B2 is not fixed or accepted. This change adds repeatable host stress and peak reporting. No firmware/DSP source changed. Target audio ISR time, per-synth CPU, dropouts and installed candidate identity are NOT_RUN. Do not merge as the dropout fix.

## Peak results

512 frames at 44100 Hz allow **11.609977 ms**. The owner’s 60% whole-half limit is **6.965986 ms**; each synth’s 15% limit is **1.741497 ms**. Negative margins below mean the observed host execution exceeded the audio deadline.

Three rounds of 1024 halves per case per side (9216 halves per side), serial execution, Linux CPU affinity set to logical CPU 0. Before and after use identical firmware and identical final test drivers in separate checkouts. This is an **unchanged-firmware control**, not an optimization comparison. Large variation, including the outliers below, prevents a speedup claim. No observed peaks were discarded.

| Control | Case | Audio CPU peak ms (% deadline) | Audio mean ms | Deadline margin ms | Wall peak ms | Chicago peak ms | Miami peak ms |
|---|---|---:|---:|---:|---:|---:|---:|
| before | plain | 97.527921 (840.04%) | 2.243304 | -85.917944 | 97.555349 | 16.913602 | 5.020280 |
| before | chorus | 12.700908 (109.40%) | 2.046235 | -1.090931 | 12.700656 | 4.297283 | 6.164043 |
| before | peaking | 127.324552 (1096.68%) | 2.005117 | -115.714575 | 127.323222 | 4.722635 | 7.832770 |
| after | plain | 10.522858 (90.64%) | 2.019996 | 1.087119 | 10.522701 | 6.683118 | 7.954701 |
| after | chorus | 22.084361 (190.22%) | 2.122709 | -10.474384 | 22.083933 | 7.455802 | 6.646209 |
| after | peaking | 26.939666 (232.04%) | 1.888276 | -15.329689 | 26.939339 | 14.945756 | 5.862479 |

All six aggregate host cases fail at least one requested peak limit. Means are not acceptance evidence. The JSON preserves per-round and aggregate maxima, means, margins, over-limit counts, worst block indices, CSV hashes and PCM hashes. Aggregate block indices concatenate rounds in order.

PCM is byte-identical across both checkouts and all three rounds within each case. No sound change was made or proposed. This is deterministic host sample evidence, not target listening or sound acceptance.

## Repeatable workload and timing scope

Run `X0X_STRESS_BLOCKS=1024 sh tests/host/audio_stress_test.sh` from the repository root. Output defaults to `build/host/audio-stress`. The ordinary host suite registers it as `audio-stress`. To enforce observed host budgets after generating a report:

```sh
python3 tools/audio_stress_report.py --csv build/host/audio-stress/plain/blocks.csv --pcm build/host/audio-stress/plain/audio.s32 --out build/host/audio-stress/plain/report.json --require-host-limits
```

Functional coverage success and timing status are separate: the stress shell succeeds only when workload assertions pass, but prints an explicit timing FAIL when limits fail. `--require-host-limits` exits nonzero on that failure. Target status always remains NOT_RUN in this host reporter.

The workload drives Chicago and Miami simultaneously with four-note USB- and BLE-origin chord requests every half, and both dense sequencer basslines at 400 BPM / RATE_32. Supported saturation is **one Chicago voice and two Miami unison voices**; four-note requests do not create unsupported four-voice polyphony. All 11 tracks in both 909/808 kits play dense 64-step patterns; BRK complexity/roll, sends, reverb, delay/tape, distortion, DJF, master compressor and momentary bus effects are enabled. Eight valid SMC pitch-bend-format faders move every half. The three fixed cases use Miami factory 0/29/18 and Chicago FX 0/1/6.

Assertions verify both synth voice maxima, all five parts audible, all 22 drum tracks hit, queue exhaustion, exact successful note-on/off counts per synth/source/half, eight SMC faders accepted and dispatched without drops, nonzero audio, and actual redraws. A negative control drops one injected note and must fail consumer accounting. All 18 measured runs passed workload assertions and redrew on every half.

The timed audio half consists of two production `engine_render(256)` and `engine_volume(256)` pairs. Inclusive synth runtime wrappers cover prepare, key, param, init, render, release_source, cut_source, all_off and retire; internal calls are charged once. Shared mixing and BFX are outside per-synth runtime attribution. UI CPU is separate, between audio halves. CSV also records 909/808/BRK/FX stage deltas. Assertions and instrumentation add host overhead; target production uses different execution conditions.

The host serializes CPU0/CPU1 work and substitutes USB/BLE transports. It has no actual radio/USB interrupts, DMA deadline, SPI display contention or AC79 XIP/cache behavior. The host xrun substitute cannot prove zero dropouts. These finite samples are not a mathematical worst-case bound. Only three preset/FX combinations are covered: live preset adoption, exhaustive preset/FX selection and cold starts under the full combined workload remain gaps. The existing separate synth peak test exercises cold/edited adoption but does not close those combined-load gaps.

## Native build and memory evidence

Exact base: `bb1907494b8d1b52d52ac220dfeb9a0663ea3ee2` (`build/0.0.23`). Actual JieLi native compile/link/package, SDK V1.2.1 consumed inputs verified by SHA-256; both builds use `X0X_DEV_PADCAL=0`. No firmware source or native build input differs in this diagnostic change, so these are unchanged-source build results, not optimized candidates.

| Model | Result | Image bytes | Static RAM used / capacity | Static RAM free | Delta from baseline | Pool free |
|---|---|---:|---:|---:|---:|---:|
| SMK-37 Elite | PASS | 499004 | 95952 / 98304 | 2352 B | 0 B | 8536 B |
| SMK-37 Pro | PASS | 498988 | 95952 / 98304 | 2352 B | 0 B | 8536 B |

See `build-inputs.json`, `elite-build.txt` and `pro-build.txt`. Device identity comes from package identities SMK-37 Elite_900 / SMK-37 Pro_900, never USB name. No device writes or firmware installation occurred.

## Prior-work audit

- `c8e1599b3`: already an ancestor of the base; production fast FMA is present. No duplicate port.
- `f055d79fdc8813a495cf82fb46e6295f909330df` (#692): equivalent Chicago clock-span change already present, verified identical blob `43cbd15867bc602d93f6bc542dad2df367e5119a` and reverse-apply check. No duplicate port.
- `fa67f70d1616cd434f27e42ba9d8e296e5e7a880`: inspected; a BLE core-1 idle-gap probe, not inclusive per-synth audio CPU timing. Not ported because it touches BLE/controller scope and cannot establish the requested engine limit.
- `perf/engines-20261031` and the named historical result paths were unavailable in authenticated source / connected project searches. Existing base `tests/host/synth_peak_test.*` and `docs/perf/engine-peaks-20261031.md` were reused as references. No old measurements were invented or treated as current target data.
- No engine edits were made, avoiding concurrent F1 ownership changes. B2 still precedes F1.

## Verification and review

- Test-first report checks observed red before implementation; five current report unit/CLI tests pass, including peaks hidden by averages, engine half-budget accounting, invalid rows, failing budget exit and malformed PCM rejection.
- Current-driver ASan + UBSan: all three 64-half cases and missing-note negative control pass; this is functional checking, not timing evidence.
- Existing synth peak probe: three baseline rounds pass; original engine inputs validated.
- Selected registered host suite: `audio-stress runner-admission`, 64-half smoke; see verification receipt. Full host suite and Apple toolchain execution were not run.
- Compound Engineering simplification and independent five-persona code review completed. Three validated instrumentation/coverage findings were fixed and reviewed again; review receipt retains target acceptance as not ready. No cross-provider review was available.

## Remaining blocker and required continuation

The connected Mac’s earlier read-only discovery returned `MIDI inputs: (none)`. A fresh invocation of the shared `tools/fm1_install.py --info` utility was rejected by automatic approval review because its name targets the prohibited FM-1 family. It was not retried or bypassed. No exact installed SMK-37 identity or target timing stream is available.

Before selecting a source optimization, obtain an owner-approved SMK-37 profiling route and exact installed source/package identity. Measure inclusive per-half audio ISR and per-synth peaks, underrun counters, starts and live preset/FX transitions under the same combined load; then isolate the actual target hotspot, add a failing regression, make the smallest exact-sound fix, and rerun identical host and target before/after checks. Require worst observed audio half <=6.965986 ms, each synth <=1.741497 ms and zero observed target underruns. Report the test duration and any residual untested cases. Hardware headroom and zero dropouts cannot be certified by this draft.

Visual changes: none. Framebuffer mockups: N/A. No main, nightly, .github/Actions, FM-1 target, Miami ADSR, bootloader, BTIF, USRTRIM, efuse, key_mac, row 73 or row 120 changes.
