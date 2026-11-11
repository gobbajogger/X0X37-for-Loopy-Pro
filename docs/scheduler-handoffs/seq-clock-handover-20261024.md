# Sequencer Clock BF-005 — producer closure and review readiness (2026-10-24 local)

## Consumed Git evidence
- Owned PR #35: `lane/safe-merge-clock-handover-20261023@32364a9524dd1ac6857d46b409af66029805d4ce`; Git tree `26277feee8efeb0acabe2836b1e6b53b35f69a33`.
- Firmware `nightly@58b5b45a2dd90581de7a674c331ca5cbe8a67c7e`; production `main@c9718ae833eb64a5191c350922593df8a36017c9`.
- Separate browser-only website `main@1a176e156b049f476074b77d7e6d54be03e8c9ad`, `gh-pages@9c5d3b6b0846f89436492851fef89ed3829b7ad7` (read only).
- Git blobs independently matched the remote PR: patched `sequencer.c@7345cab17c29ba3c26f58d1b2c042b05c1b55ef4`; parent `sequencer.c@83b03f73139d8e679c94526c66b390b6e5bc2f39`, `pattern.c@b6b667d023a912b13de8539168c61928182ffa23`, `tb3po.c@63018311bf3bb86d6a3638f4935ae9cc9dec0cb2`, `sequencer.h@9bbddbf55438247469816625bd0b07a865aa0dcb`, original regression `cf877263dc3ba8e987bdfa59a09ac7c1cf603b33`.
- Bootstrap `LoopyX0X37-Project-Sources-Bootstrap-20261023.zip` provided a **historical root** snapshot; only the listed affected production files and host test sources matched the exact current PR Git blobs. The historical root / `tests/run_tests.sh` do **not** equal the full current PR root.

## Reproduced and checked
Old source (GCC, with the extended native test): compile exit 0, regression **exit 1**. Three queued USB F8 clocks leaked into the new Bluetooth Start; the second step fired at fresh tick 4 instead of tick 7; old-owner drop failed to discard pending ticks.

Actual PR #35 patched sequencer source + source-matched `pattern.c`/`tb3po.c`: GCC and Clang plain, UBSan and ASan+UBSan each **compile exit 0 / run exit 0**. All three boundary groups passed: zero stale clocks; three stale USB clocks with seven fresh Bluetooth ticks; non-owner Bluetooth disconnect and subsequent repeated old USB disconnect do not erase legitimate new-owner ticks. The strengthened test Git blob is `19f7663c8ba37550f39b1049c0ddfc2a6644e187`.

Existing source-matched `tests/host/seq_test.c` and `clk_lock_test.c` each compiled and ran under GCC and Clang (UBSan for Clang), all exits 0. MIDI note/transport checks in those suites remain green in this **scoped** projection. This does not prove full-root, merged-tree, firmware-target or hardware behavior.

Raw local execution log hashes and literal commands are in the adjacent JSON evidence record and concise Git log index. No GitHub Actions, firmware build, flash, device I/O or Pages write occurred.

## Disposition and next gates
**PRODUCER SOURCE COMPLETE / READY FOR INDEPENDENT REVIEW** for this narrow BF-005 repair. This is **NOT** independent QA approval, Safe Merge integration, an Elite/Pro build, public NIGHTLY installability or device PASS.

Independent QA: consume the new exact producer HEAD, reproduce standalone handover + `sequencer` + `clk-lock` from an exact complete root, check MIDI Note Off, all owner disconnect and rapid Start/Stop cases, and assess #41/#48/#80/#86/#90 interplay. Leave exact SHA-linked QA disposition.

Hourly Build: immediately eligible to BUILD_ONLY exact new source SHA with pinned Elite/Pro/controller inputs, in parallel with QA. Sole Safe Merge alone integrates reviewed source to firmware NIGHTLY while preserving stacked dependencies. Complete prehardware model/controller, sample preservation, OTA recovery, integrity, license, immutable artifact-host and served-HTTP checks are still required before a publicly installable NIGHTLY DEV candidate; production requires separately authorized owner hardware evidence.
