# B2 audio headroom implementation plan

> Execute with Superpowers test-first discipline and Compound Engineering review.

**Goal:** Zero observed audio dropouts during the owner's combined heavy workload, with the worst 512-frame audio half at or below 60% of its deadline and each supported synth at or below 15%.

**Architecture:** Preserve the existing Chicago/Miami arithmetic, allocation and sample order. Reuse the existing profiler and synth peak probe; add a repeatable whole-app workload and inclusive engine measurements before selecting an optimization from observed peaks. Keep host scheduling measurements separate from real AC79 ISR evidence.

**Tech stack:** C99 host simulator, Python orchestration, JieLi clang 4.0.1, AC79 SDK V1.2.1.

**Spec:** Owner's attached `cx-B2.txt` plus the subsequent worst-case stress requirement. Product contract unchanged.

## Constraints

- Base `build/0.0.23` at `bb1907494b8d1b52d52ac220dfeb9a0663ea3ee2`; one isolated branch and one PR into that base. Author gobbajogger <255244738+gobbajogger@users.noreply.github.com>.
- B2 owns engine work before F1. Preserve other owners and verify overlap before shared source writes.
- Elite and Pro: SDK V1.2.1, `X0X_DEV_PADCAL=0`; report measured static RAM free against the supplied 2352 B baseline. Explain any loss.
- No main/nightly merge, `.github`/Actions edits, FM-1 target, Miami ADSR, bootloader, BTIF, USRTRIM, efuse or key_mac operation. Exclude feature rows 73 and 120.
- No sound change. Any proposed inaudible difference requires proof and explicit disclosure; prefer byte-identical output.
- No firmware flashing is authorized by this task. Target acceptance requires exact installed candidate identity and a qualified, available test route.
- At 44100 Hz, 512 frames allow 11.609977 ms: owner's conservative 11.6 ms goals: 60% = 6.960 ms; 15% = 1.740 ms. Report observed maximum first, mean second, and remaining deadline margin.

## Review focus

Simultaneous note starts; edited preset adoption; FX state clearing and tails; queue bursts with active UI/mixer; underruns hidden by host simulation or profiler averaging. Supported maximum allocation is one Chicago voice and two Miami unison voices; sending chords must not silently claim unsupported polyphony.

## Tasks

- [x] Authenticate named prior implementations and actual base; distinguish ancestry from equivalent code. Inspect `fa67f70d1` before choosing its probe.
- [x] Add a whole-app stress test using production engine, sequencer, SMC mixer and UI paths. Drive dense note bursts, all drum tracks, BRK, FX/DJF, USB/BLE-origin packets, mixer movement and redraws. Assert active workload coverage, no silently dropped input, and correct block accounting. Register the test in the host suite.
- [x] Record baseline per-half CPU/wall maximum, mean, margin, inclusive per-synth maximum, and coverage counters. Use a separate baseline checkout with the identical test driver. Host transport substitutes are not hardware ISR or USB/BLE proof.
- [ ] From the measured hotspot, write and observe a failing regression before the smallest exact-sound source optimization. Reuse already included work; record ported/already included/dropped commits and reasons.
- [ ] Rerun the same workload and compare exact PCM, selected synth checks, sanitizers and the host suite as required. Record all failures and unrun checks without weakening checks.
- [x] Build actual Elite/Pro candidates with pinned inputs and requested pad calibration setting; record RAM, code/pool budgets and source/input identity.
- [ ] Obtain exact-candidate target half-block and synth peak/dropout evidence through an available authorized route. If unavailable, keep acceptance pending and name the missing dependency.
- [ ] Run independent `ce-code-review`, resolve verified findings, open one PR, read back remote full HEAD SHA and report PR, RAM, tests, timing table and mockup paths (not applicable: no visual change).

## Definition of done

The implementation and evidence satisfy the owner's combined workload, unchanged sound, target CPU limits, zero observed target dropouts, native budgets and review requirements. A host pass, build-only result, or open draft PR alone does not close B2.

## Current outcome

Incremental bend/cutoff compilation removes verified redundant work with byte-identical snapshots and PCM. Baseline regression fails; current fast/exact/sanitizer and selected integration tests pass. Both native model builds retain 2352 B static RAM free. Shared Linux maxima still fail; an isolated Apple M4 host comparison passes strict owner limits across 36,864 halves per side with identical PCM. Exact-target acceptance remains blocked. See `docs/perf/b2-stress-023/README.md`.
