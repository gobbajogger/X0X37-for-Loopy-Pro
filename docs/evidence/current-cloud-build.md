# Current-source cloud validation

Source: `094bb1a55c159aefb0e2bfa39790a1b4b74cee40` (`wip/one-build`). GCC 14.2.0 on Linux. No firmware source changes, GitHub Actions or device operations.

## Actual build and execution

Reconstructed 400 exact public Git blobs from matching local files and authenticated GitHub API reads. Verified all 12 external inputs against the current committed manifest. The current profile additionally needs a locally available private state-codec header which is absent from that manifest; its provenance/hash are retained privately and its bytes are excluded from delivery.

1. `python3 tools/check_original_inputs.py --json`: exit 0, active manifest inputs verified (all 12 manifest entries also matched during recovery).
2. First `sh tests/host/original_release_ui_test.sh build/host/original-release-ui`: exit 1, compilation failed because `state_codec_private.h` is not supplied by the public tree or declared external-input manifest. This failure is preserved, not called a pass.
3. With that authentic locally pinned private execution-only header supplied, reran the **unmodified** command above: **exit 0**. Normal and ASan/UBSan executables each passed **22,949 checks** with **36 capture calls / 36 named PNG outputs (35 unique image hashes)**. Normal/sanitized logs and all generated PNGs match byte-for-byte. Leak checking is explicitly disabled by the committed runner.

This is exact public-source-plus-declared-private-input **host UI validation**, not a firmware/native/model/controller package acceptance. The additional private dependency remains a public build-contract gap.

## Other executed exact-source checks

- `python3 tests/host/smc_mapping_test.py`: exit 0.
- `python3 tests/host/smc_buttons_test.py`: exit 0.
- Same button fixture with only `smc.locked=1; smc.lock_kit=0; smc.lock_bank=0;` added at `main` entry: exit 0. This is a test-only variant of the committed extractor, not a production repair.
- `cc -std=c99 -O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all -Ifirmware/src/dsp -Ifirmware/src/app tests/host/smc_fx_test.c firmware/src/app/smc_fx_map_tables.c -lm -o smc_fx_test`, then `ASAN_OPTIONS=detect_leaks=0 ./smc_fx_test`: both exit 0.

## Coverage limits and owner-reported failures

The SMC checks use production-function extractors with fixture state. Button fixtures stub FX dispatch; passing them does not establish full locked FX behavior, two-controller hardware operation, or approved FX layout parity. Current `smc_lock_blocks()` already returns zero; the hardware report must be reconciled against the installed-source identity and remaining complete dispatch paths rather than blindly replaying a historical fix.

Current generated captures include Home, MIX, Chicago/Miami parameter and preset pages, and `chicago-fx0.png` through `chicago-fx6.png` (Chicago internal FX). They do **not** include the global/SMC FX surface or Settings. Image equality is normal-versus-sanitized determinism, not comparison with owner-approved UI references. FX mapping and page-lock hardware regressions remain must-fix acceptance items.

Reuse earlier exact-source page-wrap/rate-pad proof rather than duplicate it. Mac, full native firmware/controller, golden audio, performance, installation and hardware checks remain **NOT_RUN** here. An owner-reported successful one-SMC connection is separate hardware evidence, not proof of complete two-SMC integration.
