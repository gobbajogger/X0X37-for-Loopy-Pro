# B2 audio headroom implementation plan

> Execute with Superpowers test-first discipline and Compound Engineering review.

**Goal:** Zero observed audio dropouts during the owner's combined heavy workload, with the worst 512-frame audio half at or below 60% of its deadline and each supported synth at or below 15%.

**Architecture:** Preserve the existing Chicago/Miami arithmetic, allocation and sample order. Reuse the existing profiler and synth peak probe; add a repeatable whole-app workload and inclusive engine measurements before selecting an optimization from observed peaks. Keep host scheduling measurements separate from real AC79 ISR evidence.

**Tech stack:** C99 host simulator, Python orchestration, JieLi clang 4.0.1, AC79 SDK V1.2.1.

**Spec:** Owner's attached `cx-B2.txt`, subsequent stress requirement, and 2026-11-01 owner closeout: host evidence is sufficient now; hardware timing moves to the owner's later box check. Product contract unchanged.

## Constraints

- Base `build/0.0.23` at `bb1907494b8d1b52d52ac220dfeb9a0663ea3ee2`; one isolated branch and one PR into that base. Author gobbajogger <255244738+gobbajogger@users.noreply.github.com>.
- B2 owns engine work before F1. Preserve other owners and verify overlap before shared source writes.
- Elite and Pro: SDK V1.2.1, `X0X_DEV_PADCAL=0`; report measured static RAM free against the supplied 2352 B baseline. Explain any loss.
- No main/nightly merge, `.github`/Actions edits, FM-1 target, Miami ADSR, bootloader, BTIF, USRTRIM, efuse or key_mac operation. Exclude feature rows 73 and 120.
- No sound change. Any proposed inaudible difference requires proof and explicit disclosure; prefer byte-identical output.
- No device commands or firmware installation are authorized. Hardware validation is deferred to the owner's later box check; no device route or discovery approval is needed for host completion.
- At 44100 Hz, 512 frames allow 11.609977 ms: owner's conservative 11.6 ms goals: 60% = 6.960 ms; 15% = 1.740 ms. Report observed maximum first, mean second, and remaining deadline margin.

## Review focus

Simultaneous note starts; edited preset adoption; FX state clearing and tails; queue bursts with active UI/mixer; underruns hidden by host simulation or profiler averaging. Supported maximum allocation is one Chicago voice and two Miami unison voices; sending chords must not silently claim unsupported polyphony.

## Tasks

- [x] Authenticate named prior implementations and actual base; distinguish ancestry from equivalent code. Inspect `fa67f70d1` before choosing its probe.
- [x] Add a whole-app stress test using production engine, sequencer, SMC mixer and UI paths. Drive dense note bursts, all drum tracks, BRK, FX/DJF, USB/BLE-origin packets, mixer movement and redraws. Assert active workload coverage, no silently dropped input, and correct block accounting. Register the test in the host suite.
- [x] Record baseline per-half CPU/wall maximum, mean, margin, inclusive per-synth maximum, and coverage counters. Use a separate baseline checkout with the identical test driver. Host transport substitutes are not hardware ISR or USB/BLE proof.
- [x] Remove verified redundant control-compilation work after observing the failing operation-count regression; preserve exact snapshots/PCM and record reused prior work. No causal target peak-time fix is claimed.
- [x] Rerun the same workload and compare exact PCM, selected synth checks, sanitizers and the host suite as required. Record all failures and unrun checks without weakening checks.
- [x] Build actual Elite/Pro candidates with pinned inputs and requested pad calibration setting; record RAM, code/pool budgets and source/input identity.
- [x] Record owner disposition: target half-block, per-synth timing and dropouts are NOT_RUN and deferred to the owner's later box check; device commands are prohibited.
- [x] Run independent `ce-code-review`, resolve verified findings, open one PR, read back remote full HEAD SHA and report PR, RAM, tests, timing table and mockup paths (not applicable: no visual change).

## Definition of done

Per the owner's 2026-11-01 closeout, complete the host stress/evidence, exact-sound optimization, native build/RAM checks, source review and PR delivery. Retain recorded failures and unrun checks accurately. Hardware timing and zero-dropout validation are deferred to the owner's later box check, outside this task's completion gate. No device command or merge is authorized.

## Current outcome

**Complete for the owner-accepted host scope.** Incremental bend/cutoff compilation removes verified redundant work with byte-identical snapshots and PCM. Fast/exact/sanitizer and selected integration checks pass; both native models retain 2352 B static RAM free. M4 peak goals pass across 36,864 halves per side. Shared Linux failures remain recorded. Hardware timing/dropouts are NOT_RUN and deferred to the owner. See `docs/perf/b2-stress-023/README.md` and `owner-acceptance.json` alongside it.
