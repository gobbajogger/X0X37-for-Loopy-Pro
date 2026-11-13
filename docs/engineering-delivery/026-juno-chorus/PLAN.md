# DnBOX 0.0.26: Juno-60 slot engine + shared chorus

Base: `lane/025-zero-dropout` @ `78f81bad07b8e53161326c056a2111c663349772`; 25.1 firmware `79280e763`.
Scope: ROADMAP 26.4 and 26.5 only. This plan commit contains no firmware implementation.

## Goal

Add a six-voice Juno-60-style engine to an existing synth slot using the MIT-licensed Mutable Instruments Plaits analog oscillator/filter approach. Add a cheap shared chorus using a 512-byte, 16-bit short-delay buffer with a 1.7–5.4 ms delay range. The chorus must be shared across voices rather than duplicated per voice.

## Implementation sequence

1. Inspect the pinned engine/slot lifecycle, DSP interfaces, allocation rules, current tests, Plaits source/license, and the existing 25.1 emu360 harness. Record exact baseline inputs before changing behavior.
2. Add test-first coverage for six-voice allocation/stealing, note-on/off and panic/stop lifecycle, pitch and envelope/filter behavior, slot routing, chorus delay bounds, shared processing, and absence of audio-path allocation or added latency.
3. Implement fixed-allocation Juno DSP and one shared chorus pass. Preserve the Plaits MIT notice and keep all third-party code within the selected analog oscillator/filter scope.
4. Measure incremental engine and chorus CPU cost with the same `emu360 fm1-live` configuration as 25.1. Compare worst-block cost and latency against the pinned baseline; retain raw captures outside Git.
5. Run the full host suite (required: 0 failures, 0 skips), build Elite and Pro with the currently authorized pinned JieLi toolchain/AC79 SDK, and record exact source/tree, input identities, package identities, pool headroom, and emulator results.
6. Keep this PR draft until every requested gate is independently evidenced.

## Acceptance gates

- Six simultaneous voices; fixed, bounded memory; no heap allocation in the audio path.
- Shared 512-byte chorus delay, 1.7–5.4 ms range; measured CPU cost reported separately and at full six-voice load.
- `emu360 fm1-live`: worst block < 11,609 µs and 0 late blocks.
- No slower than 25.1 under identical measurement conditions; no added audio-path latency.
- Elite and Pro pool headroom each >= 8,192 bytes.
- Full host suite: 0 fail, 0 skip.

## Boundaries

Do not change `main`, `nightly`, `.github/`, or `bass303/wobble.c` unless an independently demonstrated dependency makes the latter unavoidable. Do not commit binaries, SDK/toolchains, credentials, private headers, or raw captures. No merge, publication, install, or device operation is included.
