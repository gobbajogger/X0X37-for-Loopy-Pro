# Optimization Watch — separate SMC User-mode MIDI queues can discard releases (2026-10-23)

**Executive summary.** A second SMC ingress path, for explicitly enabled User preset input forwarded through USB or Loopy-facing peripheral BLE, uses a separate 32-entry ring from the direct SMC-central path in earlier findings. When that ring is intentionally filled, an accepted channel-16 Solo Note Off is consumed upstream without being enqueued. A separate 16-command per-source MMC ring can also drop a command at capacity. Both were reproduced with production-matched C functions. Neither scenario has been reproduced on physical hardware or under normal traffic.

**Source checked:** public `gobbajogger/X0X37-for-Loopy-Pro` `main` at `c9718ae833eb64a5191c350922593df8a36017c9` (unchanged), relevant Git blobs:

- [`firmware/src/app/smc_mixer.c`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/c9718ae833eb64a5191c350922593df8a36017c9/firmware/src/app/smc_mixer.c) `b89e5c12062a3dc3459eeb660d205f607273aab6`: the complete historical file **does not** match live main. The two extracted functions **do** match live text: `smc_midi_host()` 519 characters / FNV-1a32 `82d4835c`; `smc_mmc_host()` 321 characters / FNV-1a32 `c4de5e76`.
- [`firmware/src/app/smc_protocol.h`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/c9718ae833eb64a5191c350922593df8a36017c9/firmware/src/app/smc_protocol.h) `03a642226aae01ffb9bc29888e0f5802d72ced26`: full-file match, only opted-in channel 16 qualifies for User-mode SMC input.
- [`firmware/src/app/engine.c`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/c9718ae833eb64a5191c350922593df8a36017c9/firmware/src/app/engine.c) `555480d9c81ca8b088842bfa6bac93e85865d49a`: full-file match; consumed SMC User-mode messages are not forwarded into the ordinary instrument path.

| Priority | Source-backed / tested behavior | Practical possible effect | Evidence classification |
| --- | --- | --- | --- |
| P1 research | Full `smc_host_q[32]` silently consumes a new channel-16 Note Off; no automatic retry. | The User-mode SMC could leave a part/voice Solo latched after heavy forwarded traffic, pending an independent clear. | Exact-function host **forced** reproduction, no device incidence evidence. |
| P2 research | Full `smc_mmc_q[src][16]` silently rejects another command for that source; the other source index remains independent. | A mapped MMC button/navigation/transport command could be missed under bursts. | Exact-function host **forced** reproduction only. |

## Executed local checks

Linux x86-64, GCC 14.2.0, Python 3.13.5. Used a privately materialized Project-bootstrap ZIP (6,323,764 bytes; SHA-256 `ea173ec3afd6adea9219622652ed0dcafe48f8a6cba43395c4dab62895928319`), independently checked embedded SHA256SUMS and ZIP integrity, extracted only safe regular tar entries and skipped 12 symlinks. Historical whole tree is **not current main**. The exact functions and header above were independently compared with live GitHub before testing.

```sh
python3 make_smc_user_queue_test.py
gcc -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer -I./source/firmware/src/app smc_user_queue_stress.c -o smc_user_queue_stress
ASAN_OPTIONS=detect_leaks=1 ./smc_user_queue_stress
# Executed separately in the historical source directory:
X0X_TEST_JOBS=1 sh tests/run_tests.sh smc-protocol smc-queue
```

| Check | Exact observed result | Exit |
| --- | --- | --- |
| Without SMC User opt-in / using channel 1 | Not redirected into SMC queue | 0 |
| Enqueue User-channel `9F 08 7F`, then 31 channel-16 fader CCs, then `8F 08 00` | 32 retained: 1 press, 31 CCs, 0 releases. Final rejected Note Off returned consumed=1 despite no queue advance. A **fresh** new release was accepted after simulated drain. | 0, expected loss reproduced under ASan/UBSan |
| Fill MMC src 0 to 16, then enqueue command 17 | 17th missing from src 0, accepted on independent src 1 | 0, expected loss reproduced |
| Original `smc-protocol` and `smc-queue` | Both explicitly `ok`; unrelated Pro package test explicitly skipped | 0, on historic runner; selected test inputs matched live main |

Fixture SHA-256 `eb67dfddce0961dee5e6ab41620bd120c4e2320624110a343e13ce7cb2ac585f`; host output SHA-256 `7a71bddbb4145d5ffe92318a14b58f358af1a971ed982ea7b3b9accfc7d8cef1`; selected regression output SHA-256 `cfc206aa2cf0c64969c3d839999a0d94879a906dbcf53b2060057dc18fc7798`. Test executable and logs remain scratch-only, not firmware release evidence. Historical GitHub Actions records were inspected read-only but are not treated as local test PASS.

## Disposition and unresolved risks

Add actual peak queue/drop diagnostics for *this specific* input path, capture host-USB/Loopy-forwarded MIDI while faders and Solo controls are used, and test release behavior across reset, mode switch, and reconnect. Consider a bounded, order-preserving critical-release recovery policy; do not forward otherwise-consumed User messages to the instrument engine as a workaround. MMC events require separate loss/replay testing. Implementation belongs to the active SMC/integration owner, not this documentation branch. Any firmware patch needs matched Elite and Pro DEV builds, RAM/flash/pool budget (>= 8192 bytes free pool), package/controller identity, installer/project/sample preservation, and physical checks.

**No hardware/USB/Loopy BLE measurements, real XRUN/latency, whole-current-source build, firmware edit, install, sample write, `main` merge, `gh-pages` update or stable release.** The user will not notice a software improvement until a separately validated fix is integrated.

**Git history:** The initial execution's GitHub connector write attempts were rejected; a subsequent independent check verified the code slices and this documentation was submitted to the existing draft research PR. No implementation was merged or released.

**Independent 2026-10-23 follow-up recheck:** A newly generated host harness from the historical archive's two production functions reproduced the same opt-in filter, full 32-entry queue consumed-but-dropped release, independent 16-entry MMC sources, and post-drain acceptance. Both source slices were separately compared to live `main` (lengths **519/321**, FNV-1a32 **`82d4835c`/`c4de5e76`**). Linux GCC 14.2.0 with ASan/UBSan `-Wall -Wextra -Werror`: compile exit `0`, execution exit `0`, no diagnostics. Harness SHA-256 **`10ca5d2aeef03cf1e6718669e88309ae4c4ecd5c11cb7da4c7f5f51e93c99766`**. This confirms only the source-slice forced condition; no hardware/real traffic measurement.
