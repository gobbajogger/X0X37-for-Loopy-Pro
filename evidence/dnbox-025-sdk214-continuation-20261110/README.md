# DnBOX 0.0.25 continuation: completed host-fixture repairs

This is source preservation for the existing PR #698 integration, not a new source owner, candidate selection, release approval or NIGHTLY publication.

## Exact source delta

`host-fixture-repairs.patch` is the complete test-source delta from local firmware build commit `5ebad0e14f46baf4767a88f6e0b129f99b85d67d` through local test commit `072fc5d60b1841ecdf6918252882e57cd250b956`, including predecessor `0e5b6a5ddf525b5043963b70d43779b3ef8ebcfb`.

- Patch Git blob: `bca2d162fbb43ba04fdd4f3279fdab48303fcf83`
- Patch SHA-256: `d8d650f0b5da968b47d9a7ac0497f8a8138a8e47304271b26af7d4ba3f431af2`
- Patch size: 9,277 bytes
- Composed source tree at local test commit: `08d3701bb42020a242df3107e76d74c4c072babb`

These are not applied production changes on this evidence tree. Apply the patch only to its exact source base, or reconcile its preimages against a separately selected combined source. The complete source build commit is not claimed pushed by this preservation step.

## Changes and verification

The completed five-fixture change preserves independent-peer FX, solo overflow, remainder glyph, sound-matrix and peer-label checks after the existing API and HOME+SELECT sound-reset changes. FX remapping is driven through the actual arrow gesture rather than the chord now owned by sound reset. No player-facing firmware source changed in this unit.

Fresh command: `X0X_TEST_JOBS=4 sh tests/run_tests.sh smc-remainder smc-solo-overflow smc-multi-peer sound-matrix ui-peer-names`

Result: all five selected groups PASS, exit 0. The runner separately printed its auxiliary SMK-37 Pro package-test SKIP because this is a host-only workspace. This is not full-host or native/release qualification. The preceding reconnect fixture has its prior scoped evidence; it was not rerun in this five-selector command.

Two unfinished fixture edits, original-build composition and glyph-polish, are excluded from this completed patch. Other previously recorded full-host, performance, M12 and independent-review gates remain open. The existing 5ebad0e1 Elite/Pro packages remain BUILD_ONLY and are not promoted by this evidence.

## Remote reconciliation and delivery rule

Observed shared integration head: `wip/one-build@337a02fa2a7bbb03a6996bdc10e91a2d7f737b42`. Preserve its newer merges. Observed separate SDK pin lane: `lane/build-sdk214-pins-20261110@71c0a932bbbe7b22dd5b22ddcf9cfe223e4a1403`; do not start another SDK writer.

The owner reiterated: "push to git always!!" Completed source work is not delivered until its authorized branch commit is present remotely and read back. A local commit, ZIP, PR comment or unattached blob is not equivalent to a completed branch push. No main/nightly/.github/device or automation mutation belongs to this source-preservation step.
