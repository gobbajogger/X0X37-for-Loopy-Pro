# QB: HAL, platform, USB and BLE hygiene

Base: `build/0.0.23` at `bb1907494b8d1b52d52ac220dfeb9a0663ea3ee2`.
One branch: `chat/qb-hal-usb-hygiene`. Source delivery does not establish device acceptance.

## Changes and purpose

- Q2.1: Preserve the caller's complete ICFG when saving/restoring interrupts. The implementation follows SDK V1.2.1 `local_irq_save`/`local_irq_restore`: read ICFG, clear bit 8, then restore the saved ICFG. Input locks alias this primitive, so nested guards cannot enable interrupts before their caller restores them. The primitive is self-contained for both app and loader composition.
- Q2.3: Use the existing platform peer-count and name-length constants for copying and joining BLE names. A compile-time assertion checks the controller name field. Two maximum names still produce 42 characters plus NUL; no radio initialization or timing code changes.
- Q2.4: Name the existing outer-key wire format, preserve its bits, format the queue drain and derive its bound from `SMK_EXTQ`. `plat_outer_keys` transports the packed word unchanged; it does not repack it.
- Q2.5: A non-real-time status inside a malformed SysEx cancels collection immediately without dispatch. F0 restart and F7 completion remain intact. This prevents malformed input from retaining the OTA/editor collector until a later delimiter.
- Q2.6: Add C coverage for full-ring displacement order, wrap and explicit refusal. N2/N3 below extend the shared ring while preserving that ordering.

## Added archive fixes: N2–N4

| Item | Compared source | Result at the requested base and final change |
|---|---|---|
| N2: busy-input Note Off | PR #593, head `32fe2ea11d10aa0f521ad0c7262a8414a3c46d0a` | USB/TRS and BLE already route through the protected shared queue. Restore the missing archive predicate that protects source link-end markers from displacement; preserve the newer FIFO and IRQ handling. Add actual USB/TRS ingress coverage for Note Off and velocity-zero Note On under CC flood. |
| N3: Pitch Bend under CC flood | `e0ba1150408c036b461f671d15540084d4664d02` | Missing in USB input/output and BLE output. Port the priority/coalescing blocks only; keep all paths inside the existing TIMER5 guard and common restore exit. |
| N4: saturated APC40 handshake retry | `3a4e98f2b7d49558e3ddf3c2b04585bbcc54bd27` | Already implemented by `usb_sx_answer`, its EP1 retry call and existing reset handling; BLE already has the corresponding pending reply. No production change. Strengthen the test through actual EP1 draining/retry for both reply kinds and OTA configurations. |

The N3 port matches both cable and channel because the current input queue carries multiple sources. It updates the newest queued matching bend, so an older queued value cannot follow the replacement. Tests cover both adaptations. Pitch Bend can replace a CC but cannot evict a Note Off, transport event or link marker; Note Off/transport retain priority over bends. A queue containing only protected events still explicitly refuses an additional event, as required by Q2.6; this is not an unlimited-capacity guarantee.

PR #789 was open when ownership was checked. Therefore `ui.c` is untouched, including its decode line and the requested `live_outer` capacity assertion. Those UI-owned changes remain deferred. The base peer-name aggregation lines had not been rewritten; Q2.3 is limited to their constants and assertion.

No changes to engine/audio, graphics, mixer, USB audio/descriptors, MIDI connection/reset, Bluetooth initialization, bootloader, BTIF, USRTRIM, efuse, `key_mac`, `.github` or Actions. No FM-1 target was built. No flashing, installation or device operation was performed.

## Native verification

Genuine JieLi clang 4.0.1; SDK V1.2.1 commit `d179b4484759423312073f5fbb232501aa491047`. All 11 required SDK hashes were verified. Build configuration: SMK-37, `X0X_DEV_PADCAL=0`.

| Model | Base RAM free | Candidate RAM free | Pool free | Standard warnings, base → candidate | Extra warnings, base → candidate |
|---|---:|---:|---:|---:|---:|
| Elite | 2352 B | 2352 B | 8536 B | 1 → 1 | 22 → 22 |
| Pro | 2352 B | 2352 B | 8536 B | 1 → 1 | 22 → 22 |

Standard and extra-warning builds passed for both models. The temporary warning run adds `-Wextra -Wshadow -Wcast-align` to an imported build module; default build flags are unchanged. Initial candidate builds exposed an app-only helper dependency in the loader and a signed comparison warning; both were corrected before the final builds. Native source hashes were checked before and after each final run.

Commands, with the verified input paths supplied through the documented environment variables:

```sh
X0X_DEV_PADCAL=0 MODEL=elite ./build.sh --board smk37
X0X_DEV_PADCAL=0 MODEL=pro ./build.sh --board smk37
```

## Host verification

All nine new registered selectors pass focused verification: `irq-state`, `outer-pack`, `ble-peer-name-bounds`, `usb-sysex-parser`, `midi-ring-displace`, `midi-ingress-busy`, `midi-mq-pb-overflow`, `midi-out-mq-pb-overflow`, and `ble-out-pb-overflow`. The existing `usb-sxq-sticky-apc` selector now exercises actual endpoint draining/retry and complete response framing.

The IRQ regression first failed on premature enable after an inner restore. Its loader-oriented variant also failed on the initial helper dependency before the self-contained correction. The outer-format test covers 7680 combinations and single evaluation. The USB regression first failed immediately after the embedded status byte; it now passes with OTA enabled and disabled, through the production parser and USB receive path. The ring test exercises the production function, including physical and unsigned-index wrap. All three Pitch Bend paths and link-marker protection have observed failing and passing regressions. BLE name bounds, busy-ingress routing and APC retry characterize already-present behavior; scratch mutations verify that the ingress/retry tests reject regressions at runtime.

USB/ring and BLE bounds regressions also pass address/undefined sanitizers with leak detection disabled because LeakSanitizer cannot run under this executor's tracing. Existing MIDI overflow/APC SysEx checks pass. See `TESTS.md` for the full-suite result and baseline comparisons; a focused PASS is not a green full suite.

Independent source review inspected all product changes and new tests, independently reran the five new selectors and identified the corrected loader dependency. No actionable source findings remained. This was one independent native review using Compound Engineering, not a full cross-model/multi-persona review pipeline.

## Q2.2 and device acceptance

BLE timing remains measurement only. No timing result is inferred from source inspection or a host model.

| Device measurement | Status | Required evidence |
|---|---|---|
| Runtime Bluetooth toggle init time | NOT_RUN | Model, candidate SHA, measured `init_ms`, repetitions |
| TIMER5 watch over the runtime toggle | NOT_RUN | Before/after frame-watch values and any missed frames |
| Audio dropout during the toggle | NOT_RUN | Owner-observed duration and playback conditions |
| Ten-minute play plus USB MIDI soak | NOT_RUN | Matching model/SHA, duration, new-dropout count |

The owner authorized tests and Elite/Pro builds in this Codex thread, overriding the base's older execution-context restriction. Flashing remains explicitly prohibited. Hardware evidence and a green full suite are still required before declaring the brief accepted.

## Post-Deploy Monitoring & Validation

No deployment occurred. After a separately authorized matching-model installation, the hardware owner should capture the timing table and run the ten-minute play/USB MIDI soak. A new dropout, MIDI ordering error or interrupted editor-frame recovery blocks acceptance. Keep the current hardware state unchanged until installation has separate authority; this PR grants none.

Created in Codex with Superpowers and Compound Engineering workflows. Implementation and review agents used the session's inherited model; no distinct public model identity was reported by the harness.
