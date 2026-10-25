# LoopyX0X37 optimization watch — 2026-10-23

> **Status: RESEARCH ONLY.** Verified source structure; performance impact remains unmeasured.
> No firmware changes, target builds, installation, or physical-device tests are claimed.

## Executive summary

**Source reviewed:** [`main` at `c9718ae`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/commit/c9718ae833eb64a5191c350922593df8a36017c9).
**Targets affected by future shared-firmware patches:** SMK-37 Pro and Elite.

The current code already has dual-core audio rendering, silence fast paths, an overload
guard and conditional LED flushes. Do not redo those optimizations before profiling.
These are **investigation candidates, not confirmed defects or proposed patches**:

| Order | Candidate | Source-backed observation | Next evidence needed |
| --- | --- | --- | --- |
| 1 | Audio command backlog | Render-time drain processes all queued commands; ring capacity is 1,024. | Measure worst-block time versus queue depth and command type. |
| 2 | Direct-SMC BLE output | Central emits at most one queued MIDI write per `blec_poll()` call. | Capture burst throughput, queue drops, latency and ordering. |
| 3 | SMC input bursts | SMC input ring has 32 entries with drop diagnostics. | Replay heavy fader/encoder bursts; inspect real drop counters. |
| 4 | Pad LED flush | LED transfer is blocking but already change-gated. | Correlate flush activity with audio peak and dropouts. |

## Findings and safe experiments

### 1. Audio command backlog — investigate first

- **Observed:** [`engine.c`, command ring](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/c9718ae833eb64a5191c350922593df8a36017c9/firmware/src/app/engine.c#L130-L146)
  uses `CQ = 1024`. [`drain()`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/c9718ae833eb64a5191c350922593df8a36017c9/firmware/src/app/engine.c#L643-L668)
  consumes pending command entries until read catches up with write. The queue is finite;
  **this is not an infinite loop finding**. A large queued parameter burst may still
  consume variable render time.
- **Deadline context:** [`engine_render()`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/c9718ae833eb64a5191c350922593df8a36017c9/firmware/src/app/engine.c#L889-L927)
  calls `drain()`; [`render_block()`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/c9718ae833eb64a5191c350922593df8a36017c9/firmware/src/app/main_fm1.c#L9-L81)
  renders 512-frame audio DMA halves as two 256-frame calls.
- **Probe:** record max queue occupancy, command types, time spent draining,
  maximum render-half duration and audio XRUN count under realistic controller floods.
- **If proven necessary:** evaluate coalescing *superseded parameter writes only*,
  preserving triggers, note-off ordering, transport events, project guards and
  sample-accurate sequencing. Test equivalence and overloaded recovery before adopting.
- **Status:** SOURCE CONFIRMED / IMPACT UNTESTED. No code change authorized here.

### 2. BLE-central feedback batching — investigate after active SMC work

- **Observed:** [`blec_poll()`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/c9718ae833eb64a5191c350922593df8a36017c9/firmware/src/ble/ble_midi_central.c#L594-L655)
  sends one queued MIDI message per ATT Write Command when connected and credited.
  The [peripheral BLE-MIDI path](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/c9718ae833eb64a5191c350922593df8a36017c9/firmware/src/ble/ble_midi_host.c#L551-L589)
  already packs multiple MIDI messages into a notification, so that path must not
  be treated as missing batching.
- **Probe:** capture real SMC LED/fader feedback bursts, elapsed send time,
  out-of-order messages, lost Note Offs and queue drops with one SMC and two SMCs.
- **Possible experiment:** group complete timestamped BLE-MIDI messages within
  negotiated ATT/ACL and controller credit limits; preserve message ordering and
  correct parsing under fragmentation, reconnect, pause/page-lock and link loss.
- **Ownership guard:** [PR #18](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/18)
  already owns dual-SMC/page-lock changes; inspect its tip and coordinate with the
  designated integration owner. Do not modify that live branch from this report lane.
- **Status:** SOURCE CONFIRMED / BOTTLENECK UNTESTED.

### 3. SMC ingress and pad-feedback cost — measure, do not guess

- **SMC:** [`smc_mixer.c`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/c9718ae833eb64a5191c350922593df8a36017c9/firmware/src/app/smc_mixer.c#L7-L29)
  defines a 32-entry receive queue and enqueued/dropped/dispatched diagnostic bytes;
  [`smc_poll()`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/c9718ae833eb64a5191c350922593df8a36017c9/firmware/src/app/smc_mixer.c#L581-L653)
  consumes queued messages in a bounded batch. Use counters with actual MIDI captures.
- **Pads:** [`padled_flush()`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/c9718ae833eb64a5191c350922593df8a36017c9/firmware/src/lcd.c#L169-L218)
  is change-gated but performs a blocking LED transfer, described in the code as
  approximately 0.8 ms. Measure UI activity effects before altering the transfer path.
- **Status:** SOURCE CONFIRMED / DEVICE IMPACT UNTESTED.

## Verification before accepting any future optimization

1. **Rebase findings on evidence, not assumptions:** record the exact new commit SHA,
   relevant pending PR heads, target models, timestamps, and the test runner.
2. **Host regression selection:** inspect current `tests/run_tests.sh`, then run
   applicable tests such as `smc-protocol`, `smc-fader`, `smc-queue`, `smc-mapping`,
   `ble-midi-receive`, `ble-capture-replay` and `smc-reachability` for source patches.
   Verify exit codes and log contents. An Actions `startup_failure` with zero jobs
   is **not** executed test coverage.
3. **Real-time evidence:** compare baseline/candidate p50/p95/p99 and worst audio
   half durations, XRUNs, queue peaks/drops, ordered MIDI output, and reconnection.
   Include a dense 808 + 909 + both 303s + break scenario and simultaneous clip/SMC
   activity. Do not publish invented performance percentages.
4. **Budgets and fidelity:** maintain DSP reference tolerances, 44.1-kHz processing,
   documented app/RAM/flash limits, and the existing 8,192-byte minimum pool
   headroom. Target builds of **both Elite and Pro** are required for future shared
   firmware changes, not for this docs-only report.
5. **Hardware boundary:** only a matching-device test can establish physical BLE
   reachability, SMC usability and LED/audio behavior; host replay is not hardware PASS.

## Previously implemented work / coordination

- [`docs/plans/2026-10-04-performance.md`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/c9718ae833eb64a5191c350922593df8a36017c9/docs/plans/2026-10-04-performance.md#L72-L115)
  records earlier silent-part, master and DSP-maths optimizations.
- [`docs/plans/2026-10-04-dual-core-and-clock.md`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/c9718ae833eb64a5191c350922593df8a36017c9/docs/plans/2026-10-04-dual-core-and-clock.md#L1-L35)
  contains **historical FM-1** dual-core measurements; not a current Pro/Elite benchmark.
- Active owner work: [PR #18](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/18)
  (dual-SMC), [PR #19](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/19)
  (popup guard), [PR #20](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/20)
  (installer catalog). These features are not changed here.

## Report and merge contract for hourly updates

- Commit **only material new evidence** on the dedicated documentation branch.
  Do not create empty hourly commits. Keep one short dated Markdown report per
  independent research batch, with source SHA, severity, observations, hypotheses,
  reproducible checks, measured results, remaining risks and an explicit disposition.
- Keep the PR **draft** while evidence is incomplete. Make only ordinary fast-forward
  branch updates; never merge, force-push or modify firmware from this report lane.
- If this documentation PR is merged, start a **new** report branch from the then-current
  `main` for later findings. Do not push over a merged/closed PR or another author's lane.
- The repository is public. Exclude private samples, device backups, credentials,
  proprietary build inputs and unpublished binaries from all reports.

## Checks performed for this report

- GitHub source, repository visibility, live main SHA, active PRs and quoted
  source paths inspected on 2026-10-23.
- No host tests, device tests, target builds, firmware artifacts or CI passes
  are claimed by this research-only stage. No installation or publication occurred.
