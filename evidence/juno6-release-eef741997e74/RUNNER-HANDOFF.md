# Existing Runner owner: PR #895 CPU measurement handoff

**Action:** consume this record in the already-owned Runner work. Reuse existing exact artifacts/results where their hashes and configuration match. Return matched target-counter evidence, or one precise missing-input/instrumentation result, to [PR #895](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/895). This is a proposed procedure, not a completed device measurement or a new Runner dispatch.

## Exact inputs

| Role | Source commit | Tree |
|---|---|---|
| Candidate | `eef741997e743189f17df7e46bf881e472ad254c` | `f0e8b7d282fc57cfcae18f6f02727f7455df98e8` |
| Actual parent | `1322293e89c1582eecb3d9b7950e14db01722065` | `ee09c9302ba8d1883677596ee996731b8d316a7c` |

**Source-proven candidate artifacts:** all names below refer to the producer's existing `build/` directory, not a Git download or a verified shared Runner mount. Complete path/size/hash records, source-file hashes, SDK/toolchain identity, and build settings are in [runner-manifest.json](runner-manifest.json).

| Model / file | Bytes | SHA-256 |
|---|---:|---|
| Elite application `smk37-elite/x0x.bin` | 544264 | `23b6680ab999762024485fc21cc8d62c312ca8785d61dddd535bda010cf7b056` |
| Elite ELF `smk37-elite/x0x.elf` | 628872 | `a1bc8b6066392e91f714784c9be54caf93a6bf4f23ea714488179c491e6abbda` |
| Elite package `x0x-smk37-elite.fwsc` | 651732 | `40c142cfd0e738fa339905957270a799fb132227e61aa54aa31ecfb553f5e4f1` |
| Pro application `smk37/x0x.bin` | 544256 | `97c07d3d1d7a7d9571ffc5cc5a778104605777561027a466bc82d9c017296544` |
| Pro ELF `smk37/x0x.elf` | 628864 | `d47026ba03b2b6facb5ece839206518a0a87a996ddbcc07ecd099b6e1e053855` |
| Pro package `x0x-smk37.fwsc` | 651730 | `3327d05e77e126a40e365fbea8b85bb30c3d8e04085b62ac41b524fb0f2cdd54` |

**Source-proven missing input:** this lane has not verified a native application/ELF for parent `1322293e89c1`, nor access to these candidate bytes from Runner. First resolve the already-existing parent artifacts and candidate access through the existing Runner owner/cache. Record hashes. Do not substitute `94197fc8`, another hoist revision, or a later integration build. If an exact parent artifact is absent, return that specific missing artifact/configuration and stop the target comparison; no candidate rebuild or device write is requested here.

## Minimal matched workload

Use the internal DSP state/workload in [juno6_matched_bench.c](../../tests/host/juno6_matched_bench.c), pinned by [protocol.json](host-matched/protocol.json). This is not a J6/plugin preset adapter.

1. Initialize synth and chorus at **44,100 Hz** with their source defaults. Set control 8 to **1.0f**. For N voices, note-on notes **48 + 3*i** with velocity **100**, then set each envelope to **0.8f**; held is stage 3/gate 1, release is stage 4/gate 0. Keep the approved gain and the default chorus settings.
2. Cover **idle, held-1, held-6, release-1, release-6**. Reset the complete seed outside each timed leg. Each leg performs **16 blocks × 256 frames** of render → mono-to-right copy → chorus. Warm both variants identically. Keep logging, checksum/state copies, and allocations outside the measured region.
3. Use the existing queued matched job if it already covers these exact inputs. Otherwise the fixed host schedule is **15 trials per case, 16 alternating ABBA/BAAB comparison quartets and 16 interleaved same-artifact A/A quartets per trial**, with rotated case order. Preserve all ordered samples and record controls before interpreting A/B. Do not repeat until a favorable result appears.
4. Use Runner's existing validated counter/instrumentation and document its boundaries. If it exposes only `juno6_render`, label the result render-only; do not compare it directly with the host render/copy/chorus aggregate. Do not modify the native artifacts merely to match the host link-order control.

The native clock must come from the actual configuration/counter record. A branch name containing “360 MHz” is not clock verification. An emulator's host wall time is not device CPU. If Runner only has emulator timing, return that method and its calibration/limitations explicitly; it cannot silently satisfy a physical-device requirement. This handoff authorizes no device write or new emu360 duplication.

## Return record and decision

Attach a compact result plus the ordered raw samples to the same PR. Include:

- Candidate and parent source commits and application/ELF SHA-256s, model, SDK/compiler/settings, actual clock, counter/timebase, instrumentation version and measurement scope.
- Physical target versus emulator identity, interrupt policy, warmup/reset procedure, voice/block counts, counter resolution/wrap handling, raw A/B order, and same-artifact A/A results.
- Per-case paired median/tail summaries with variability and resolution. Include deadline/XRUN information only when captured by the same run; missing telemetry is not a zero count.
- **Decision:** reject an established matched regression. Keep the draft if controls or resolution are insufficient. State exactly what the workload supports; do not claim universal never-slower behavior.

**Current source-proven state:** the native Elite/Pro flash/pool and focused PCM/state results are already qualified for `eef741997e74` and must be reused. Host CPU remains **INCONCLUSIVE** after all 20 same-function controls failed. Target CPU is **NOT RUN** by this owner. A target result does not close the unresolved host gate. Source and CPU evidence remain separate from UI frames and unverified J6/plugin preset formats.

**Delivery versus execution:** this packet is attached to #895 for the existing Runner owner; Runner receipt, artifact access, and execution are not asserted. No new owner, job, queue entry, merge, publication, or install is created by this packet.
