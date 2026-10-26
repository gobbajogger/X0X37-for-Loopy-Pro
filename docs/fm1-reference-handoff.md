# SMK-only gate fix

Receiving owner: “Integrate and review x0x37 work”. Base: `53d5654`.

The owner requested FM-1 reference-only scope on 2026-10-20, then explicitly requested this fix. This implementation supersedes the scope-only handoff in `5ca84bd`.

The gate now builds Elite and Pro in dev and release configurations. It no longer builds FM-1 or requires its budgets or package. Its CRC manifest lists only the required SMK artifacts and fails if one is missing. Stale FM-1 files cannot enter that manifest.

The runner retains SMK storage, package, OTA, model, and loader tests. Package fixtures use `build/smk37/x0x.bin` and `build/smk37/loader/ota.bin`. Installer protocol tests use SMK identities and `build/x0x-smk37.fwsc`; dependent slot and Bluetooth mocks use the renamed class. Wrong-model rejection remains covered. Quickcheck no longer selects the removed FM-1 storage test.

## Validation

Focused checks ran in a private Linux amd64 container from `x0x37-final-cache:dev-532d91f`, with the current lane sources and the local SDK. Existing SMK build artifacts supplied test inputs; no fresh target build is claimed.

`AC79_SDK=/sdk X0X_GATE_LOCK=/tmp/smk-only-gate.lock X0X_TEST_JOBS=1 sh tests/run_tests.sh gate-configs smk-package smk-ota-entry smk-ota-entry-model1 smk-ota-entry-model2 smk-update-loader smk-build-package smk-build-package-elite installer installer-slots installer-ctl smk-storage` passed all 12 selected tests.

After deleting the container’s `build/x0x.fwsc`, `build/x0x.bin`, and `build/loader/ota.bin`, the nine gate, installer, package, and update tests passed again. Gate regression checks execute both build configurations, reject FM-1 manifest entries, require missing SMK artifacts to fail, and retain expected SMK tests. The complete gate-configs check passed 19 of 19 checks.

Shell syntax, `git diff --check`, and `python3 tools/prefs_lint.py docs/USER_PREFERENCES.md` passed. Manual review checked mock subclasses, package paths, expected-test discovery, budgets, and manifest errors. Logs are in the workspace folder `smk_only_gate_evidence/`.

## Integration

Apply this lane commit to the integration branch through its assigned chat, then run the formal gate for the finished batch. Do not replace a script during an active run. No active integration checkout or running gate was changed. No full formal gate, installation, or device test ran for this lane.
