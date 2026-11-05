# Saved-role loader and save-guard host evidence

This evidence makes the existing saved-role test execution and separate actual `project_save` guard proof inspectable. It helps reviewers assess regressions before firmware reaches a player. It changes no firmware or existing tests and supplies no new original-reviewer acceptance.

## Exact source

All source below is from `gobbajogger/X0X37-for-Loopy-Pro` commit `8d43c53f6fc9d6aba749893c82d82a6e7409773a`, tree `8db6e029e34bbadaca0b1689b13cb556543f1f85`. The complete fixture closure is included under `work-a/closure/`.

| Path | Bytes | Git blob |
| --- | ---: | --- |
| `tests/host/test_smc_boot_saved_roles.py` | 12968 | `8ffe7511ee9373d00793a364de436d589379f6b5` |
| `firmware/src/app/project.c` | 30254 | `82cc9eba89884e4310b3273941fb9b6335611415` |
| `firmware/src/app/main_fm1.c` | 115127 | `2fa898c1c00ffceb0c0584fd0d504e5cb66af69d` |
| `firmware/src/app/x0x.h` | 8598 | `bf8c0f92eb167d2db3e54e18e736e0edbb3ff8ae` |

The [derived manifest](manifest.json) lists 91 selected technical files (438575 bytes) with exact sizes, SHA-256 and Git blob identities. These files were copied byte-for-byte from existing evidence. This is a filtered set, not the complete original archive or its original manifest. Empty stdout/stderr files are retained. This README, the manifest and [save-run record](save-run-record.json) are the only three derived files. The save-run record preserves the original technical scope, source, extraction, environment, commands, results and source readback values.

## Existing results

- The unchanged fixture passed 301 checks in strict plain and 301 in UBSan mode. Compilation and execution returned 0; stderr was empty. UBSan linkage to `libubsan.so.1` was verified. See [fixture process record](work-a/fixture-run.json), [stdout](work-a/fixture.stdout), [plain compile record](work-a/capture/plain/compile.json), [UBSan compile record](work-a/capture/ubsan/compile.json) and [linkage](work-a/capture/ubsan/ldd.stdout).
- The complete `read_project`/`project_load` callee side effect permits the 12 value-2 cases. All 20 diagnostic variants matched existing expectations. Deleting only that callee assignment in scratch C failed the existing assertion with exit 1. See [matrix output](work-a/scratch-controls/matrix-run.stdout), [control stderr](work-a/scratch-controls/no-callee-write-run.stderr) and [execution records](work-a/scratch-controls/execution-records.json).
- The separately extracted complete [project_save function](work-b/project_save.extracted.c), with deterministic layout/storage stubs in [actual.c](work-b/actual.c), passed 24 matrix cases and 205 assertions in both strict plain and UBSan runs. Each run had 20 returns of -9, three of -8 and one positive path returning 0. All 23 guarded rows made zero capture, measure, object-save and stream-save calls. Four post-guard failure checks are included in the 205 assertions. See [plain output](work-b/actual-plain-execute.stdout), [UBSan output](work-b/actual-ubsan-execute.stdout) and [save-run record](save-run-record.json) for compile/run exits.
- All three save-guard mutants failed as intended: omitted incompatible/layout guard, reordered settings fence and omitted settings fence. Their raw outputs and exits are included. Host macros were `X0X_HOST=1`, `X0X_BASS_TAGGED_SOUND=0`, `X0X_BASS_PRESET_PREVIEW=0`.

## Reproduction

Use Linux with Python 3, a C99 compiler, UBSan runtime, `ldd` and `readelf`. Run from a disposable writable copy of this evidence directory because the existing scripts regenerate logs, generated C and executables. Preserve the published raw records as original provenance. The old absolute temporary paths in raw commands describe the original execution; these relative commands reproduce the checks in a new location:

```sh
python3 work-a/run_fixture.py
python3 work-a/finish_checks.py
python3 work-b/run_guard_proof.py
```

The first command runs the unchanged fixture through its transparent compiler capture wrapper. The second executes the captured programs and reproduces the 20-row diagnostic and callee-deletion control. The third extracts the actual save function from the included public `work-b/project.c`, then compiles and executes the stubbed positive and mutant harnesses. Scripts can generate local bookkeeping and binaries; those generated outputs are not part of this evidence delivery. For a direct fixture run without capture, use `python3 work-a/closure/tests/host/test_smc_boot_saved_roles.py`.

## Scope and pending review

This is synthetic host control-flow evidence. Engine-specific branches, the device store-stage-held branch, complete storage/flash/firmware qualification, native target builds, packages and device tests are NOT_RUN. The earlier ten killed controls and one equivalent survivor were NOT_RERUN in this evidence run. Host results do not establish data safety on hardware, installability or release readiness.

The original reviewer corrected the earlier unreachability allegation and requested inspectable raw evidence in [the current review comment](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/789#issuecomment-6647553577). See also [the prior correction](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/789#issuecomment-6646596898) and [the producer summary](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/issues/832#issuecomment-6646012424). These public links establish provenance; this delivery does not reset that review. Original-reviewer consumption and final acceptance remain pending.

The #789/#698 source-closure mismatch remains unresolved by this host evidence. Combined candidate, NIGHTLY, #816, #822, native, package and device qualification remain on HOLD. This branch supplies evidence for the existing review and integration process.

Public project source retains the repository's GPL-3.0-only licence and applicable notices; see the root LICENSE and LICENSING.md. The included source closure and public GitHub response retain their original bytes and notices. No firmware, SDK or toolchain binaries are included.
