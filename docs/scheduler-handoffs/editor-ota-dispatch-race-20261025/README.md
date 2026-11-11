# Editor/OTA dispatch race source handoff

Draft [PR #141](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/141), isolated branch `lane/editor-ota-dispatch-race-20261025`, fixes independently reproduced source routing loss. It does **not** establish the cause of the historical hardware timeouts.

Source/test commit: `a8113c2a0036be76069fb6431454650ee84f6e73`; root `3c09d2698059657b3a0005bac944284970b56a1e`; changed firmware subtree `b7a004e4d37b95b4efc8def3842c9f41589cda06`. Base is current `nightly@54f2ffbc3124f7449a69067dc8215a2d91a2e675`. The later evidence commit changes only this documentation directory; the reviewed source/test blobs remain identical.

## Behavior and ownership

Previously, an editor request arriving after the editor check could be released by OTA before OTA rejected its envelope. `ed_service` now returns eligibility only when its existing header check leaves a non-editor frame pending. The normal loop polls OTA only for that eligibility and with the existing project guard. A pending frame stays owned until release, so OTA reads the frame already classified; a later editor request waits for the next editor pass.

Ordinary editor commands retain copy-before-release behavior; sample/project handlers retain the frame until handled. OTA parser/session, USB hooks, safe-mode polling and loader paths are unchanged. This avoids pinning unrelated traffic in safe mode or an active session, where no normal editor service can drain it. No new queues, persistent state, protocol/header duplication, instrumentation or packet contents are added.

## Demonstrated results

- Same actual-source regression: original **22/34, exit 1** (12 expected routing failures); candidate **34/34, exit 0**, also under ASan/UBSan. Independent reviewer reproduced each result from immutable Git archives.
- Registered `editor-ota-dispatch`, full `project-protocol`, and `project-runtime`: all pass with `X0X_HOST_DEFS=-D_DARWIN_C_SOURCE=1 X0X_TEST_JOBS=1 sh tests/run_tests.sh editor-ota-dispatch project-protocol project-runtime`. That existing host option resolves the macOS libproc declarations; no host source change was made.
- Independent sanitized project protocol passes. Broader runtime sanitizer exits **134 on both baseline and candidate** at existing `firmware/src/dsp/fastmath.h:57`, left shift of negative -1. Complete sanitizer qualification is not claimed.

The regression extracts the actual editor service and main update block, reuses the unchanged OTA fixture and implementation, and uses an explicit editor dispatch/ownership sentinel. It covers before/between/after arrivals, arrival after an earlier editor release, PING, addressed controller read 63392/224, sample/project commands, unknown editor commands, OTA identity, malformed/unknown drain, project/recovery holds, safe-mode drain and session-consumer progress. It does not exercise physical IRQ timing, USB, real editor reply generation or complete update sessions.

See [independent review](independent-review.md), [exact independent commands](independent-runs.json), [reviewed blobs/log hashes](independent-review-receipt.json), and [structured handoff](handoff.json). Logs in this directory contain only synthetic host test output.

## Remaining qualification

Independent review **accepts the scoped source/host handoff** with no corrective findings. PR stays draft as requested. New Elite/Pro target builds, code/RAM budgets, package/controller compatibility, complete combined-source QA, integration and owner-operated hardware testing remain downstream. Old firmware artifacts do not qualify this changed firmware tree. The broader pre-existing fastmath sanitizer failure remains for its owner to assess.

No hardware operations, captures, reset/reflash, GUI, full firmware builds, merges, releases, artifact publication, credential handling, SDK/toolchain files, installer changes or concurrent SMC worktree edits occurred. Local changes in the original task8 checkout were preserved by cloning only committed Git objects into this isolated task checkout. The source commit was pushed normally and independently read back from remote; final evidence HEAD is recorded in the PR and delivery receipt.
