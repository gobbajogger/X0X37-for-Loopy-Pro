# DnBOX d158 native evidence archive

This source/text-only archive preserves the independently reviewed native resource and package result for candidate `d158444101d569f9ee64ba05d7cb54f24de1be30` (tree `0b966725cb55713dcf8817156130cc3f57f61e06`, parent `3c41ef4bedf9701f549ce2752b73306e2fab1eb8`). It lets maintainers distinguish an established flash-reserve repair from unfinished sound, performance and user-workflow qualification. It does not change the instrument or distribute installable firmware.

## Observed native result

Both model builds exited 0 with SDK `AC79NN_SDK_V1.2.14_2026-10-13`, SMK-37 board and PADCAL 0. Elite used 543,860 app bytes with 8,812 free flash; Pro used 543,848 with 8,824 free flash. Both had 8,340 free pool bytes and 95,536 / 98,304 static RAM usage. Flash and pool exceed their separate 8,192-byte floors. The 2,800 app-byte saving per model is relative to the prior `3c41` result, not a new baseline build.

The independent receipt review accepted 41 consistency checks covering pinned source closure, compiler/SDK inputs, retained artifact identities, both-model resource arithmetic, package/model/CRC/controller pairing, expected invalid-package refusals and honest stage limits. This is consistency review of retained artifacts/receipts, not an independent rebuild or complete source/behavior acceptance.

## File provenance

`RECEIPT_REVIEW.json`, `source-delta.json`, `controller-source-scope.json` and `artifact-hashes.json` retain the eligible original technical JSON byte-for-byte. `MANIFEST.json` identifies their original hashes. `NATIVE-EVIDENCE.json` is a new, explicitly derived portable extraction from the original checkpoint. It omits operational metadata and absolute paths; it is not an original execution log or a standalone build reproducer. Original checkpoint SHA-256: `91289548b616fa9ddb49b34df96c7436d1c350993c05461d0e1782e9f19d0ae2`. Its original pending-review wording remains historical; the later exact-checkpoint independent review is separate.

Native command start/finish and model configuration in the extraction come from that checkpoint. The compiler and SDK hashes identify inputs without distributing them. The public source commit identifies all 5,412 tracked blobs; 5,399 verified baseline blobs and 13 changed blobs formed the exact build tree. Full original source/input manifests and raw command logs are not included here.

## Boundaries that remain open

The original idle fixture FAIL remains unresolved; required host Clang comparison was NOT_RUN; final idle-runner correction has link-only proof rather than a fresh execution PASS. Full-host suite, no-slowdown/performance, current emulator and device acceptance remain open. This native result does not qualify the historical `3c41` emulator archive as a `d158` run.

Controller `a334208b51f62bc7231ebb76a6de0dfa260033d7f7316e5b92053c540bc1d984` was reused exactly, not freshly compiled. The controller-scope receipt explicitly does not reestablish its complete original native build lineage. Model-specific complete package hashes identify retained local artifacts only; **no firmware/controller/ELF/SDK/toolchain bytes are included or published**.

This is an immutable evidence archive, not source integration, a release, owner-available installer, adoption of a completion policy or hardware proof. Original artifacts and failure history remain unchanged. The assigned firmware owner retains integration and disposition; the existing coordinator retains the canonical roadmap. No device action or build/test rerun accompanies preservation.
