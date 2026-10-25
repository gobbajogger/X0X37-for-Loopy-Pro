# Performance Engineering R22: BREAK dual-voice reverse callback work (2026-10-25)

**Musician-facing behavior:** Rapid live-pad retriggers and release during reverse BREAK playback must preserve exact sound and STOP silence, even while two voices crossfade. Decoder-work spikes at those moments are a potential underrun risk worth measuring, not a demonstrated device failure.

## Immutable source and scope

Consumed current open non-draft PR #93 at `847f3aea5d3cd0b71583b5a9920d35236e441d5d` (whole ROOT tree `f26f68197e39d9057289cc2a6524b9299f66c40a`; compiled firmware subtree `ee863f9cba6cbfb4e471d199ae408c7694acbac5`), against accepted protected NIGHTLY `54f2ffbc3124f7449a69067dc8215a2d91a2e675` (firmware subtree `5c209609b770fd13949230aeb80956594fd6b741`). The host runner validates all BREAK C/H and dependent Git blob IDs, plus the inherited original C test fixture. It injects live keys and an IMA operation observer into **throwaway copies only**. The actual run used source-projection files pinned to those Git blobs, **not a complete ROOT checkout**.

This R22 test adds no compiled target inputs, controller, settings, DSP vendor data, audio samples, or firmware binary. The comparison is a complete native production BREAK C renderer linkage, but not a device deadline measurement.

## New failure stimulus and actual observations

Sizes: 705,600, 1,411,200, 2,116,800 ADPCM samples; source-advance factors: 2, 4, 8, 12; forward/reverse; steady versus live-pad repeated changes. In the latter reverse condition a held slice is changed every four 64-frame callbacks. The reverse key releases midway. The harness checks actual overlap of both active voice slots, not merely synthetic events, and always confirms audible finite PCM and post-STOP silence.

For each GCC 14.2 / Clang 17 source pair, both baseline and candidate compiled and ran EXIT 0. Each executable rendered 48 scenarios × 80 blocks × 64 frames: 3,840 genuine BREAK audio callbacks and 245,760 PCM frames. All **four traces matched** SHA-256 `b176bc84b11fad61010d715bc30d0ec5235953e0bc916290ba49630e95497cf7`.

| Stimulus | NIGHTLY calls | PR93 calls | Change | Worst NIGHTLY / PR93 calls in one 64-frame callback | Observed dual-voice overlap blocks |
|---|---:|---:|---:|---:|---:|
| Reverse steady (12 scenarios) | 2,055,168 | 1,323,008 | -35.63% | 15,872 / 8,704 | 0 |
| Reverse live retriggers (12) | 2,085,540 | 1,610,916 | -22.76% | 28,160 / 20,937 | 192 |
| Forward steady (12) | 399,360 | 399,360 | 0% | 768 / 768 | 0 |
| Forward key events (12) | 406,248 | 406,248 | 0% | 1,538 / 1,538 | 12 |

Crossfades measurably raise the worst reverse-block decoder work even with PR93. This is a valuable **real-time review target**, not an XRUN/click proof or assurance that every possible audio block improves.

**Fail-closed negatives executed:** all four invalid executable invocations returned expected EXIT 2; the runner rejected missing args (EXIT 2), existing output directory without overwrite (EXIT 2), altered production BREAK Git blob (EXIT 1 before output creation), and altered inherited R21 test blob (EXIT 1 before output creation). Source/fixture validation and PCM comparisons do not silently accept corrupted input.

## Reproduction / role handoff

From the matching PR #93 ROOT checkout and the protected NIGHTLY ROOT checkout:

```sh
python3 -B tests/host/run_breaks_dual_voice_decoder_compare.py --nightly-root /path/to/verified-nightly --pr93-root . --output-dir /path/to/new-output
```

A separate source-only private ZIP includes already blob-matched projections and GCC/Clang logs. Original PR93 source sanitizer is **NOT RUN** for R22; the known inherited `fastmath.h:57` UBSan issue remains with draft PR #28 and is **not** waived by prior corrected-header projections.

**Only existing Independent QA** can independently accept/reject this optional PR on an exact full HEAD. **Existing Hourly Build** owns genuine Elite/Pro/controller BUILD_ONLY provenance for the distinct compiled `ee863f9...` inputs; old 8,308 B free-pool evidence on `a87a...` cannot qualify new code. **Only Safe Merge** integrates firmware. Already accepted first NIGHTLY is separate. No hardware, protected refs, production, installer, private firmware or public hosting touched.
