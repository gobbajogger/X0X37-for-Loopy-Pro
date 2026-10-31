# B2 incremental control compilation — peak acceptance pending

**The M4 host stress passes the requested limits; SMK-37 target acceptance remains unverified.** The shared Linux host still records failures, preserved below. This candidate removes redundant audio-path control compilation with byte-identical samples. No installed target identity, target ISR/per-synth timing or hardware dropout count is available. Keep PR #796 draft.

## Retained change

Source commit: `e6aefdf78317a0a27ce32d5137d4b8f55479fac1`, based on `build/0.0.23@bb1907494b8d1b52d52ac220dfeb9a0663ea3ee2`.

Every pitch bend previously rebuilt the complete voice-control snapshot. The same occurred for the cutoff control used by CC1. The serial owners now update only the mapped field for Chicago macros 20/2 and Miami macros 16/12. Mapping, arithmetic, validity checks, revisions, output callbacks, damper behavior and sample/event boundaries remain the same. Other macros, unready snapshots and revision gaps use full compilation. No persistent state is added.

The workload sends two bends and two CC1 edits per synth per half. Each formerly invoked four waveform-width calculations and unrelated control conversions. The fast path invokes zero width calculations: **32 unnecessary width calculations per combined half are removed**. The test checks the actual runtime/backend/control-owner path so reverting an owner to full compilation is detected. This is a proven operation-count reduction, not proof of a peak-time or dropout fix.

The new compiler differential covers all 81 Chicago and 30 Miami factories, mixed edits, every 14-bit bend value, revision wrap/gaps, unready snapshots, invalid values and unsupported Miami shapes. Baseline failed the zero-width-work assertion; current fast, exact and ASan/UBSan runs pass **379,667 checks**. Whole compiled snapshots and published programs match full compilation exactly.

## Apple M4 host peak acceptance

Apple M4, macOS 27.2 arm64, Apple clang 21.0.0, Python 3.12.15. Exact base/candidate source and driver hashes are verified in `mac-timings.json`. Three alternating rounds of **4096 halves per case/side** give **36,864 halves per side**, equivalent to **427.990 seconds of audio per side**. Execution is accelerated, not real-time DMA. All 18 run logs verify every-half redraw, all five audible parts, all 22 drum tracks, supported voice saturation and consumer coverage. No run or peak is discarded.

The stricter owner goals are **6.960 ms for the audio half and 1.740 ms per synth**. Every CPU and wall peak in every measured M4 case passes. Baseline also passes on this host. This does not erase Linux failures or establish a target speedup.

| Case | Audio CPU peak before / after ms | Audio mean before / after ms | Wall peak before / after ms | Chicago CPU peak before / after ms | Miami CPU peak before / after ms | After physical deadline margin ms |
|---|---:|---:|---:|---:|---:|---:|
| plain | **1.101875 / 0.605500** | 0.330634 / 0.325113 | 1.119000 / 0.669000 | 0.105499 / 0.061752 | 0.089667 / 0.061669 | 11.004477 |
| chorus | **0.991792 / 1.006708** | 0.332352 / 0.326503 | 0.992000 / 1.029000 | 0.088209 / 0.070540 | 0.075749 / 0.095249 | 10.603269 |
| peaking | **1.139833 / 0.557375** | 0.329832 / 0.325860 | 1.156000 / 0.605000 | 0.089417 / 0.050998 | 0.086081 / 0.089375 | 11.052602 |

Worst candidate CPU peaks: **audio 1.006708 ms (8.679% of 11.6 ms), Chicago 0.070540 ms, Miami 0.095249 ms**. Margin to the requested goal: **5.953292 ms whole half, 1.669460 ms Chicago, 1.644751 ms Miami**. Worst wall peak: **1.029000 ms**. PCM matches byte-for-byte between baseline/candidate and all rounds in each case.

Both Mac builds and missing-note negative controls passed; the production control regression also passes all 379,667 checks with Apple clang. Six strict report tests pass on macOS with Python 3.12. The compiler emits an existing `smc_states[1]` bounds warning in both base and candidate for the single-peer fixture; it was recorded, not fixed outside this scope. Full host suite is not claimed.

## Shared Linux host failures retained

The physical 512-frame deadline at 44100 Hz is **11.609977 ms**. The owner explicitly specifies fractions of **11.6 ms**, so this report enforces the stricter goals: **whole half <=6.960 ms; each synth <=1.740 ms**. It retains physical-deadline margin separately from goal margin. A regression test rejects values only 1 ns above these goals.

Three alternating rounds of 1024 halves per case and side, serial measured execution on Linux CPU affinity 0. Each side has 9216 measured halves. Before uses the exact base product code; after uses the source commit above. Test drivers match. No measured peaks are removed, normalized or replaced by averages.

| Side | Case | Audio CPU peak ms | Audio mean ms | Physical deadline margin ms | Wall peak ms |
|---|---|---:|---:|---:|---:|
| before | plain | **193.832364** | 1.907730 | -182.222387 | 193.948862 |
| before | chorus | **113.671571** | 1.622465 | -102.061594 | 113.670406 |
| before | peaking | **193.833035** | 1.573857 | -182.223058 | 193.930657 |
| after | plain | **211.252258** | 1.955785 | -199.642281 | 211.250043 |
| after | chorus | **29.145135** | 1.682119 | -17.535158 | 29.144808 |
| after | peaking | **21.548294** | 1.376989 | -9.938317 | 21.547798 |

| Side | Case | Chicago peak ms | Chicago mean ms | Margin to 1.740 ms | Miami peak ms | Miami mean ms | Margin to 1.740 ms |
|---|---|---:|---:|---:|---:|---:|---:|
| before | plain | **30.376690** | 0.150959 | -28.636690 | **74.251517** | 0.184060 | -72.511517 |
| before | chorus | **3.905832** | 0.115315 | -2.165832 | **76.016794** | 0.171078 | -74.276794 |
| before | peaking | **3.330021** | 0.111952 | -1.590021 | **4.801833** | 0.134571 | -3.061833 |
| after | plain | **12.592495** | 0.125955 | -10.852495 | **13.186460** | 0.159651 | -11.446460 |
| after | chorus | **4.484318** | 0.111098 | -2.744318 | **14.826125** | 0.147117 | -13.086125 |
| after | peaking | **2.415017** | 0.086907 | -0.675017 | **3.579124** | 0.118905 | -1.839124 |

All six aggregate cases fail at least one requested limit. No overall peak-time improvement is established. `incremental-timings.json` preserves round/aggregate maxima, averages, physical/goal margins, worst-block indices, over-limit counts and CSV/PCM SHA-256 values. Aggregate block indices concatenate rounds in order. `host-control.json` preserves the earlier unchanged-firmware control at commit 949d42d.

A separate fixed arithmetic kernel, with no synth or sequencer, repeated the same 100000 dependent unsigned multiply/add operations 4096 times. Thread CPU times ranged around a median of **0.132000 ms**, with a **6.868692 ms maximum**; that worst sample had no faults or reported context switches. This confirms large timing variation exists independently of DSP workload changes; it does not prove the cause of every audio spike. See `host-environment-probe.json`. The host remains FAIL.

## Sound and functional proof

- All 18 fast-profile whole-app runs: identical PCM hashes across both source versions and all rounds within each case. All coverage assertions pass.
- Exact software-FMA synth comparison: 72 scenarios per round, three rounds, **20,447,232 PCM bytes per round**, byte-identical to base. Includes factory/cold/edited, note and FX cases, but is not a whole-app target test.
- Selected registered suite: `original-release-integration original-stale-release perf-wheel-motion perf-wheel-noise control-recompile audio-stress` passes. The release integration includes normal/sanitized PCM/state/framebuffer checks.
- Focused fast/exact compiler checks, ASan/UBSan, six report unit/CLI checks and the missing-note negative control pass.
- Full host suite: NOT_RUN. Apple toolchain stress execution and consumer coverage PASS; no zero-skip integration qualification is claimed.

## Workload and reproduction

```sh
X0X_STRESS_BLOCKS=1024 sh tests/host/audio_stress_test.sh
sh tests/host/control_recompile_test.sh
X0X_RECOMPILE_EXACT=1 sh tests/host/control_recompile_test.sh
python3 tools/audio_stress_report.py --csv build/host/audio-stress/plain/blocks.csv --pcm build/host/audio-stress/plain/audio.s32 --out build/host/audio-stress/plain/report.json --require-host-limits
```

Functional coverage and timing status are separate. The stress shell fails on workload errors and prints explicit timing FAIL for over-budget runs; `--require-host-limits` makes budget failure exit nonzero. The host reporter always leaves target status NOT_RUN.

Both synths receive four-note requests from each USB/BLE source every half while dense bass patterns run at 400 BPM / RATE_32. Actual supported saturation is **one Chicago voice and two Miami unison voices**, not unsupported four-voice polyphony. All 22 tracks in both 909/808 kits, BRK complexity/roll, sends, reverb, delay/tape, distortion, DJF, compressor and momentary bus effects run. Eight valid SMC faders move per half; UI redraws occur every half. Cases use Miami factory 0/29/18 and Chicago FX 0/1/6.

Assertions check supported voice maxima, all five audible parts, all 22 drum tracks, exact consumer note counts per half/source/synth, accepted/dispatched SMC input with no drops, queue exhaustion, nonzero audio and redraws. A dropped-note negative control must fail.

The timed half contains two production `engine_render(256)` / `engine_volume(256)` pairs. Synth attribution includes runtime prepare/key/param/init/render/release/cut/all-off/retire; nested runtime calls count once. Shared mixing and BFX are outside per-synth attribution. UI runs between halves and is reported separately. The host serializes CPU0/CPU1 and substitutes transports; there are no actual USB/radio IRQs, DMA, SPI or AC79 XIP contention. Host xruns do not establish hardware dropouts. Only three static preset/FX combinations are measured under full load. Full-load live adoption, exhaustive FX/presets and target WCET remain gaps.

## Native build and memory

Both isolated source-pinned native compile/link/package runs pass with SDK V1.2.1 and `X0X_DEV_PADCAL=0`.

| Model | Before image B | After image B | Static RAM used / capacity | Free RAM before / after | Pool free |
|---|---:|---:|---:|---:|---:|
| SMK-37 Elite | 499004 | 499628 | 95952 / 98304 | **2352 / 2352 B** | 8536 B |
| SMK-37 Pro | 498988 | 499612 | 95952 / 98304 | **2352 / 2352 B** | 8536 B |

Code grows 624 B per model. RAM and pool reserve do not fall. Build input hashes, source file hashes, baseline/current model receipts and selected real logs are saved alongside this report. Identity is the package identity SMK-37 Elite_900 / SMK-37 Pro_900. No device write, installation or binary publication occurred.

## Prior-work audit

- `c8e1599b3`: already an ancestor of the base; production fast FMA is present. No duplicate port.
- `f055d79fdc8813a495cf82fb46e6295f909330df` (#692): equivalent Chicago clock-span change already present, verified identical blob `43cbd15867bc602d93f6bc542dad2df367e5119a` and reverse-apply check. No duplicate port.
- `fa67f70d1616cd434f27e42ba9d8e296e5e7a880`: inspected; a BLE core-1 idle-gap probe, not inclusive per-synth audio CPU timing. Not ported because it touches BLE/controller scope and cannot establish the requested engine limit.
- `perf/engines-20261031` and the named historical result paths were unavailable in authenticated source / connected project searches. Existing base `tests/host/synth_peak_test.*` and `docs/perf/engine-peaks-20261031.md` were reused as references. No old measurements were invented or treated as current target data.
- Current changes stay in voice/control-owner compilation. F1 compact-FX/factory files and A3 master-glide files are untouched. The shared test-runner registration is additive; no scope is cherry-picked from either PR.

## Review and remaining blocker

Superpowers debugging, test-first and verification workflows and Compound Engineering were used. Five independent source-review personas completed; the one test-coverage finding was corrected and independently rechecked. Simplification review found no further changes warranted. Cross-provider review was unavailable. See `review-receipt.json`.

No matching target is available: earlier connected-Mac discovery returned `MIDI inputs: (none)`. Automatic approval review then rejected the shared `tools/fm1_install.py --info` command because its FM-1 name conflicts with the brief. It was not retried or bypassed. The existing read-only discovery approval question remains pending.

The next acceptance step requires a connected SMK-37 and an approved profiling route with exact installed source/package identity. Obtain actual per-half ISR and inclusive per-synth maxima, averages, goal/deadline margins, workload duration and underrun counters. Use the same combined workload including starts and live preset/FX transitions, isolate any target hotspot, then rerun before/after. Neither source review, operation-count reduction, host PCM identity nor native builds establish **zero target dropouts / <=6.960 ms whole-half / <=1.740 ms per synth**.

Visual changes: none; mockup paths N/A. No F1/compact-FX/factory work or A3/master-glide work duplicated. No main/nightly, .github/Actions, FM-1 target, Miami ADSR, bootloader, BTIF, USRTRIM, efuse, key_mac, row 73 or row 120 changes.
