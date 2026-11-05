# Build-25 tip recut onto a06e20189

The recut preserves the requested one-build base and carries the final M4 sound-reset and M8 PLAY-question helpers and their host checks. A future UI patch still owns their device gestures; this task does not wire either helper into the UI.

Base: `a06e20189141ff5fb8766e2c6152be460f4356c3`.
Donor: `lane/build-25-integration-20261106` at `2f451528dbf3d3f8a1f13c3b384dc155d12fa70b`.
Source-only candidate tree before evidence: `b1b41456069fbeddde88ad0c4264b0310d6d2190`.

The donor host-repair commits `43de100c`, `b9d97e10` and `ce278129` depend on the preceding lane stack. Their part-OFF implementation/tests, newer SMC and ADSR APIs, sixteen-pad rate ownership and designated stock pack are absent from the pinned base. The recut retains the base's matching checks. See the per-commit disposition. No preceding integration-stack content is copied.

Both Elite and Pro DEV builds ran locally with the genuine JieLi clang 4.0.1 compiler and AC79 SDK V1.2.1. All eleven SDK input hashes match. The source archive's 4,735 blobs and reconstructed root/commit match the pinned Git objects.

| Both models | Base | Recut | Delta |
| --- | ---: | ---: | ---: |
| App image | 474,840 B | 474,840 B | 0 B |
| App RAM budget use, including alignment | 95,780 B | 95,780 B | 0 B |
| .ram_text | 2,712 B | 2,712 B | 0 B |
| Audio pool use | 335,464 B | 335,464 B | 0 B |
| Package | 651,457 B | 651,457 B | 0 B |

App RAM headroom is 2,524 B against 98,304 B. Pool headroom is 8,600 B, which exceeds the 8,192 B build minimum by 408 B. App code headroom is 77,832 B. Each model's app and RAM-text bytes match its own baseline exactly. The build reports 844 RAM-text instructions and no calls, with no soft-double routines.

The audio path remains byte-identical. The 320 MHz / 44.1 kHz budget is about 7,256 cycles per sample; no hardware timing measurement or new target timing result is claimed.

The full host runner passed locally: **333 PASS, 0 FAIL, 0 SKIP, 0 NOT_RUN; exit 0**. Strict preflight passed 16/16. Every registered test has an aggregate result and log hash in host-suite.json. The first attempt was stopped incomplete after reproducing sandbox-only Node subprocess EPERM and LeakSanitizer/ptrace failures. The supported network-enabled execution profile passed both probes; the full suite then ran from the beginning. ASAN_OPTIONS was unset globally, and each test retained its normal sanitizer settings. No test was deleted, disabled, or converted to a skip.

RAM budget accounting is 428 B data + 20 B alignment + 95,332 B BSS = 95,780 B. The ELF section-size sum excludes alignment and is 95,760 B. Independent read-only source review found no actionable Critical, Important or Minor defects; it did not substitute for these agent-run builds/tests.

Delivery uses the requested base as first parent and the observed old lane tip as second parent. The resulting tree is built from the pinned base plus this recut and sanitized text evidence. This preserves existing lane history and permits a normal, nonforced update. No new PR is required. Source delivery is not a release, installation or hardware acceptance.

