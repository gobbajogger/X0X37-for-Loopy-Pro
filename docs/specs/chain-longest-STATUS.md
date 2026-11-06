# chain-longest continuation status

Request t656u. Rebased the three chain-specific commits from `lane/dev-chain-longest-20261106` onto `wip/one-build` at `48e92b76c2f476cf40771e00a04e1838136bb55d`. The previous lane head was `3d470e158683af0fe7a5b2d8185af51e3209d371` and its original fork was `bdc4965e2c9b2025c2e2cf9ea5b4d4a6b03008ca`.

The chain slot remains the longest clamped 909 length in the selected range. Shorter patterns repeat, including a partial last pass. The rebase preserves the target's separate chain endpoint fields and registers only the chain test. No UI or project-format change.

Independent source review found two boundary defects. Turning REC off during song recording retained a chain window; playback now uses its natural 909 wrap. A bass-only cue at a partial boundary reset the unchanged 909; the drum now keeps its phase. Both added regressions failed before the fixes and pass afterward. Both original mutants are killed. Review found no new production regression.

## Evidence

See [the host receipt](../evidence/chain-longest-20261106/host-suite.json) and [native build receipt](../evidence/chain-longest-20261106/build-receipt.json).

- Full host coverage: 337 PASS, 0 unresolved FAIL, 0 SKIP. The complete run had 336 PASS and one missing-host-Clang failure. Installing Clang 19.1.7 and rerunning `optimization-equivalence` passed. The original full attempt and corrective run are retained separately; no failed or skipped check was removed.
- Root personally ran `sh tests/run_tests.sh`, the corrective runner entry, the regression/mutation checks and both native DEV builds in this Codex environment. The independent reviewer ran no builds or suites.
- Elite and Pro DEV: SDK `AC79NN_SDK_V1.2.1_2023-12-13`; 11 SDK/controller input pins match. Each app is 480,872 B, app free 71,800 B; RAM data+bss 95,784/98,304 B; pool 335,464/344,064 B, free 8,600 B. Package model and CRC checks pass. Controller build and integrity check pass.
- Host `sizeof(seq_t)` is 424 B before and after this rebase. Native build totals are measured; a native base-versus-tip delta was not measured.
- These are BUILD_ONLY results and host proof, not a `DEV CHECKED` runner record, installability, publication, release or hardware acceptance. Device checks are NOT_RUN.

Only source, tests and sanitized evidence are deliverable. No product binaries, SDK/toolchain bytes, private-header additions, credentials, `.github/` changes or device writes are included. Main and nightly remain unmodified. Integration remains with the assigned reviewer/integrator through the source PR.
