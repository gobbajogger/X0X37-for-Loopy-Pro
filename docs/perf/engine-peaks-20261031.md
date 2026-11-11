# Chicago + Miami peak work, 2026-10-31

State: **partial candidate; dropout removal and AC79 worst-case improvement unproven**.
Base: `perf/engines-4` at `dffd05f0923304e26ac1e93b02fd640118e8d2a0`.
Owned lane: `perf/engines-5-codex`; review target: `perf/engines-4`.
The owner retains integration and device testing. The separate `perf/engines-5`
worker owns drive, FX/output conversion, shared output and compiler flags.

## Scoped plan and delivered change

Trace prepare/events/render and the DMA deadline; measure an inclusive host
baseline; remove a verified piece of repeated control work; compare exact PCM;
run affected checks and independent review; open one draft PR. Stop before merge,
flashing or firmware publication. Required acceptance still includes real target
timing and full affected native proof; a host result cannot close that acceptance.

Miami's `preparation_tail()` called `original_pw_width()` four times at each
active voice control update, including full-note preparation. The selected
profile always passes source, modulation intensity and modulation width as zero.
Consequently, the result depends only on the immutable compiled program controls.
The same four calls now run in `original_miami_voice_compile()`. Program edits
recompile that snapshot before publication. Every arithmetic argument is retained.
No tick, envelope, oscillator, filter, output or FX recurrence changes.

In the unedited 128-block Miami probe, 2,368 width calls after initialization
become zero. This is a fixed reduction in note/control preparation work, not proof
that this work alone caused the owner's underruns. Live macro edits retain their
four compilation calls. Compiled controls grow 16 bytes; host backend size grows
3,824 to 3,840 bytes and runtime part 5,288 to 5,304 bytes. Target layout, flash
and pool budgets require a native build. All consumers must rebuild together.

## Peak-path audit

| Path | Evidence and disposition |
| --- | --- |
| Note/voice start | `chicago_voice.c:full_note_mask`, `miami_selected_voice.c:full_mask` force a control update after resetting the counter. Two Miami unison lanes can do that work together. The probe includes frame-zero notes and repeated release/retrigger. Only program-only width work moves. |
| Control collisions | Both voices use `original_control_tick` at 690 Hz; full-note counters start at 44100. The CSV records control-update deltas. Offsetting counters would alter the time and duration of envelopes, MGs and filter ramps, so no offset was applied under the exact-sound requirement. |
| Coefficients/tables | Miami width conversion is hoisted. Chicago packed lookup already caches its last index; changing its decompression/cache policy is deferred. Filter/oscillator preparations still depend on live pitch, envelope and modulation and remain at their original samples. |
| FX reset/tail | `original_runtime_init` clears and initializes its owned graph; Chicago compact FX selection is destructive by policy. Cold adoption is measured separately. Existing tails, resets and output-chain work remain with their current owners. FX choices 0, 1 and 6 are exercised; this is not exhaustive FX coverage. |
| UI/display/flash | `main_fm1.c` runs UI/display and project/editor service in the main loop; `engine_render` handles queued edits and notes on the audio path. This synth-only probe does not exercise their contention, IRQ masking, flash stalls, or command bursts. No claim that they are ruled out. |
| Deadline | `main_fm1.c:render_block` renders two 256-frame stretches for each 512-frame DMA half. `fm1_alnk0_irq` checks whether DMA advanced during rendering and increments `late`/`audio_xruns`. TIMER5 can preempt audio. Existing `x0x_worst` and `x0x_dbg` are the target measurement route; no device was accessed. |
| Drive/shared output | Outside this lane. No changes to drive, FX/output conversions, shared output chain, compiler flags, `.github`, or private headers. |

The old `synth684_bench.c` starts its timer after prepare and note events. Its
render-only timing cannot answer this task's inclusive peak question. The new
probe times preparation, note/macro events and sequential synth renders. It
records cold initialization separately, preserving original render boundaries.

## Measured host results

Linux x86-64, GCC, `-O2`, unchanged base `ORIGINAL_SYNTH_FLAGS`, pinned real
tables, one CPU affinity, seven alternating candidate/reference rounds.
The timing executables have no call-count wrapper. A separate instrumented
executable checks that no width conversion occurs in note/control work.

Actual device deadline: **512 / 44100 = 11.609977 ms**. A 256-frame stretch
represents 5.804989 ms of audio; the IRQ's actual deadline covers the whole half.
The following are observed host maxima, not device WCET bounds. CPU maxima are
the largest observed thread CPU times; averages are medians of round averages.

| Workload | Max 256-frame CPU, before/after (us) | Max aligned 512-frame pair CPU, before/after (us) | Avg ns/output sample, before/after |
| --- | --- | --- | --- |
| Both, one requested note each | 158.296 / 478.150 | 202.722 / 523.468 | 109.403 / 110.971 |
| Both, four requested notes each | 224.282 / 168.650 | 271.522 / 217.773 | 110.292 / 109.463 |
| Both, Miami program 29 + Chicago chorus, four notes | 290.321 / 294.006 | 342.748 / 343.829 | 123.517 / 125.002 |
| Both, Miami program 18 + Chicago peaking, four notes | 286.555 / 739.549 | 337.111 / 798.768 | 115.423 / 112.986 |

There is **no demonstrated overall worst-case timing improvement** in this noisy
host environment. Wall maxima reach about 6 ms even for single synths; host
scheduling is observable. For the four-note plain pair, cold-adoption CPU maxima
are 13.641 / 30.285 us, also noisy. Do not infer AC79 headroom, underrun removal,
or a device speedup percentage from these values. The probe excludes drums,
mixer/master, USB, ISR entry latency and hardware contention. Paired values use
aligned adjacent stretches, corresponding to the current half-buffer geometry.

## Validation

| Check | Result |
| --- | --- |
| Exact source transport | PASS: 174 downloaded source blobs checked against the base Git blob IDs; no substitute DSP tables. |
| Active original inputs | PASS: all 10 pinned inputs; manifest SHA-256 `9c7ac9b0a71e775fb7009e2d4035fce827b2c56c4e8e692a7a8910c9c6c881bc`. |
| Regression red/green | PASS: baseline fails the no-hot-width-work assertion; candidate passes. |
| PCM A/B | PASS: byte-identical, one/four requested notes per engine, both engines, all 30 Miami factories, macro endpoints 0/.5/1, selected FX and tails. Each round compares 20,447,232 bytes; candidate SHA-256 `c8e87fdae7d743df3d25c8eb14cac10a5074ba3ffed3c91f558b3e65530dc721`. Requested chords use the engines' existing mono/unison note-allocation behavior. |
| Exact software FMA lane | PASS: byte-identical baseline/candidate PCM with `X0X_FAST_FMA` removed in the test only. Production flags unchanged. |
| ASan/UBSan | PASS: byte-identical A/B with `ASAN_OPTIONS=detect_leaks=0`; LeakSanitizer itself cannot run under this sandbox's ptrace. |
| Table interpolation regression | PASS: 408,412 exact cases at each of `-Os` and `-O2`. |
| Existing original-idle runner | FAIL: unmodified runner rejects pre-existing unused functions. With only that warning relaxed, both base and candidate fail the same line-74 `WORK_MASTER.frames` assertion. No unrelated fix included. |
| Legacy Miami stereo-split runner | FAIL: links removed `wobble_*` symbols. No legacy engine restored. |
| Independent review | PASS by source inspection: no blocking correctness finding. Initial measurement-instrumentation concern fixed before final timing run. Review did not independently rerun the tests. |
| Full host suite | NOT_RUN: only the pinned synth source/test closure was materialized; no whole-product host PASS claimed. |
| Native Elite/Pro / target RAM / AC79 timing | NOT_RUN: no JieLi compiler or pinned V1.2.1 SDK in the supplied execution environment. |
| Hardware/dropout acceptance | NOT_RUN: no flashing or device access authorized or performed. |

Reproduce the probe from a full candidate checkout with a separate exact-base
source directory (no network Git operation is performed by this test):

```sh
python3 tests/host/synth_peak_test.py --out build/peak-check --reference-source /path/to/exact-base
python3 tests/host/synth_peak_test.py --out build/peak-exact --reference-source /path/to/exact-base --exact --rounds 1
ASAN_OPTIONS=detect_leaks=0 python3 tests/host/synth_peak_test.py --out build/peak-sanitize --reference-source /path/to/exact-base --sanitize --rounds 1
```

Next owner action: review this isolated candidate, obtain full host and Elite/Pro
native validation on the combined source, then measure `x0x_worst` / `x0x_dbg`
with the owner's exact simultaneous-synth workload through a separately authorized
device-testing stage. Do not mark the dropout request complete until the real
512-frame worst time and underrun count meet acceptance. No automatic merge or
device operation follows from this draft.
