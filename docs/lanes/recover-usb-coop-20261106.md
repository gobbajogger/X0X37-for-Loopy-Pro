# USB cooperative reset recovery

Branch: `lane/recover-usb-coop-20261106`.
Base: `a06e20189141ff5fb8766e2c6152be460f4356c3` (`wip/one-build`).
Recovery source: `handoff/local-codex-recovery-20261106` at
`2abf039d503e77c0c15ee71188a471e019fc3bb7`,
`docs/handoff/local-codex-recovery/LOST-WORK.md` and
`patches/lost-02-usb-retry-cooperative-reset.patch` / `recovered-usb.patch`.
The production additions/removals match `lost-02` exactly.

## Behavior change

An admitted USB retry now resets the controller and returns to the foreground.
It attaches on a later call after at least 25 ms of TIMER4 ticks. This lets
main-loop input processing continue during the reset settle interval instead of
waiting in `fm1_delay_ms(25)`. Boot bring-up still blocks for 25 ms. One-second
retry admission, cancellation when detached or up, EP0 DMA setup, SIE writes,
endpoint enabling and dead-SIE timeout handling retain their original behavior.

The two new retry state variables are foreground-only. `usb.up` stays false
through reset/settle/attach, so TIMER5 and nested UAC servicing cannot access the
SIE during the recovery. The loader initializes TIMER4 before USB and uses the
same sequence from its foreground loop.

## Verification

Checked source/test tree before these evidence files:
`9321e0d7e08e1545327357351bf1cc1051de1cf7`. All reviewed input hashes
remained unchanged through the completed build/test run.

- New real-`usb.c` host regression: **27 checks, 0 failures**. Before recovery,
  the same test produced **10 failures** on the blocking base implementation.
  Covers boot, retry admission, exact 25 ms boundary, repeated foreground calls,
  attach ordering, detach/up cancellation, both timer wraps, dead SIE and retry
  recovery. Register and clock fakes stay in tests.
- Existing UAC, TRS, USB SysEx queue, clip USB clear and clip knob tests pass.
  Their host fixtures now supply the TIMER4 interface needed by real `usb.c`.
- Full unfiltered host suite: **333 PASS, 0 FAIL, 0 SKIP, 0 NOT_RUN**, exit 0.
  Includes all 58 simulator scenarios; strict preflight: **16 PASS, 0 FAIL**.
  Counts and exact selector results are in
  [the machine-readable receipt](recover-usb-coop-20261106.json).
- Independent read-only review: **PASS**, no critical or important findings;
  reviewer checked the diff, ISR/loader interaction and regression coverage.
  Builds and host tests were run by the primary task agent in this Linux x86-64
  environment, not delegated and not inferred from GitHub Actions.
- Elite and Pro native development builds: **PASS**, `X0X_DEV_PADCAL=1`, default
  Chicago/Miami configuration, no release build.
- Matching controller: **PASS**, native build, 61,088 / 65,536 B image and
  26,336 / 26,368 B controller RAM. Kept local.

SDK: `AC79NN_SDK_V1.2.1_2023-12-13`, commit
`d179b4484759423312073f5fbb232501aa491047`; all 11 app/controller input pins
verified. Toolchain: native JieLi Clang 4.0.1,
`jieli-linux-toolchains-20260730.1`; downloaded archive SHA256
`a1545cddbc451a06bac43eb5cd96d831470c42a97c5c0b1f98879a20fc7647bd`.

External host references:

- 9W9: `10cbe5c1e1c9e65b0818bb34644ab1a13092082c`.
- 8W8: `94aa2716d4c2e5851fc7c5dbb4c17adfd133c997`.
- TB3PO: `f49d81d05d1e29ae37728a08e30f193892be3bc9`.

The documented 9W9/8W8 commits could not be retrieved from the upstream refs
checked; the exact commits were recovered from the retained `charlesvestal` forks.

## Measured native deltas

Both baseline builds used the exact base and the same toolchain, SDK and DEV
configuration as the candidate. Values come from successful build receipts and
ELF sections, not host executable sizes.

| Metric | Base (both models) | Candidate (both models) | Delta |
| --- | ---: | ---: | ---: |
| App image | 474,840 B | 474,936 B | +96 B |
| App `.text` | 471,088 B | 471,184 B | +96 B |
| App RAM (builder region usage) | 95,780 B | 95,780 B | 0 B |
| App `.ram_text` | 2,712 B | 2,712 B | 0 B |
| Pool usage | 335,464 B | 335,464 B | 0 B |

The candidate leaves **2,524 B** below the 98,304 B app RAM limit,
**8,600 B** pool headroom (above the 8,192 B minimum), and **77,736 B** of app
code space. `.ram_text` remains 844 disassembled instructions with no calls and
has identical bytes in baseline/candidate for both models (SHA256
`52705d9683efd8ba671487161c9f1d73a5305bfe5a66d3dd0570b9432f33a00c`).

Elite compressed loader: 7,341 → 7,475 B (+134 B); package:
651,457 → 651,591 B (+134 B). Pro compressed loader: 7,341 → 7,471 B (+130 B);
package: 651,457 → 651,587 B (+130 B). The shared USB change also affects the
loader; app and loader identities remain model-specific.

This change adds no work to the audio render path. Hardware cycles/sample and
physical USB enumeration were not measured; no claim is made about measured
utilization of the approximately 7,256 cycles/sample budget at 44.1 kHz/320 MHz.
The 25 ms improvement is removal of the explicit foreground wait, not a hardware
latency benchmark. Build/test evidence is BUILD_ONLY, not DEV CHECKED or device
validation.

## Reproduction

Set `JIELI_TOOLCHAIN`, `AC79_SDK`, `SDK_LOCAL`, `NINEW9_DIR`, `EIGHTW8_DIR` and
`TB3PO_REF` to the verified inputs above. Use a writable `X0X_GEN_CACHE` directory.

```sh
X0X_DEV_PADCAL=1 python3 tools/build.py --board smk37 --model elite
X0X_DEV_PADCAL=1 python3 tools/build.py --board smk37 --model pro
CTL_NATIVE=1 CTL_NO_CACHE=1 sh ble/ctl/build_ctl.sh
X0X_STRICT_PREFLIGHT=1 X0X_TEST_JOBS=8 sh tests/run_tests.sh
```

The first suite attempt was interrupted after reproducing Node `spawnSync`
`EPERM` and LeakSanitizer runtime failures in the default sandbox profile.
The network-enabled execution profile resolved these local runtime restrictions:
an identical minimal piped child and sanitizer probe then passed. This is an
observed execution-profile workaround, not a network dependency of local tests.
No sanitizer, assertion or test was disabled. A separate initial build failed because its default cache
directory was read-only; setting `X0X_GEN_CACHE` to the task's writable directory
resolved it.

Only source, tests and sanitized evidence are delivered. Firmware, controller,
SDK and toolchain bytes remain local and untracked. No protected refs, workflow
files, flash areas or devices were changed. There is no existing PR for this
lane; the requested delivery is a branch push.
