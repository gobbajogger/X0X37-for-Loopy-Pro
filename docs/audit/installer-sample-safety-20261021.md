# Installer and sample preservation repairs

Status: host checked. No target build, device installation, or physical-device test was performed in this lane.

Input: `7b0bce55c5446a709fc3eecdafb993181654dcab`, which includes the completed SMC connection repair at `e28ff9a` and the earlier integration history.

## Repairs

1. **Updater target selection.** Python and browser installers now require one update-compatible MIDI input before starting the update and during each reboot transition. The protocol reports model/version and has no per-unit serial. Consequently, two keyboards, including duplicate port names or a pre-existing loader, produce an explicit ambiguity error. The initial Python `--port` filter still selects the requested keyboard. A sole keyboard can change its port name on entering and leaving the loader. Loader model checks run before the write command, and later sample restore/controller readback retains the verified port name. The existing Pro/Elite alias policy remains unchanged.
2. **Uncertain sample layout.** A missing or malformed `SMP_INFO` reply no longer means there are no samples. Automatic installation stops before the update command when preservation is unknown. The documented stock `FM-1_015` and SMK version `016` identities can still install without replying to X0X sample commands. A malformed reply is rejected even for stock firmware. Python's explicit `--no-slots` opt-out remains available; `--force` does not disable preservation.
3. **Uploader capacity.** `fm1_sample_upload.py load` reads the actual layout and builds against the selected slot's capacity before sending `SMP_BEGIN`. The 55,125-byte replacement that overflows slot 3 is rejected without erasing or writing. A larger 88,200-byte sample fits the reported 120 KiB slot 2. Offline `build` retains its documented 80 KiB default.
4. **Exact sample headers.** Python restores, the browser sample library and the break page preserve the full 512-byte header area when archived bytes 480–511 are not erased. A read-only Projects QUERY v1 probe confirms the firmware generation that supports full headers before any erase. Archives with an erased tail still use the compatible 480-byte header command; a legacy device that cannot preserve a non-erased tail is rejected before the slot changes.

These repairs address the two installer findings in both audits, the uploader-capacity finding in `LoopyX0X37-Audit-2026-10-21.txt`, and the exact-header restore finding in both audits. The reproduced bystander write is prevented by refusing ambiguous hardware selection, rather than claiming a unique device identity that the MIDI protocol does not provide.

## Checks

| Command | Result |
| --- | --- |
| `python3 tests/installer_safety_test.py` | PASS: 11 tests, including Pro/Elite and enumeration-order subcases |
| `node web/test_installer_safety.mjs` | PASS: 19 test groups, including the actual break-page writer |
| `python3 tests/install_test.py` | PASS: existing update protocol and model guards |
| `python3 tests/install_slots_test.py` | PASS: existing backup, migration, restore failure and explicit opt-out cases |
| `python3 tests/install_ctl_test.py` | PASS: complete controller-block readback and corruption cases |
| `python3 tests/x0x_slots_test.py` | PASS: archive, restore, layout and extraction cases |
| `python3 tests/upload_tools_test.py` | PASS: existing loop upload and one-shot trimming cases |
| `X0X_TEST_TIME_SCALE=20 node web/test_install_slots.mjs` | PASS: browser installer, sample migration and actual installer-page flow |
| `node web/test_breaks.mjs` | PASS: Python/JS sample/archive compatibility |
| `node web/test_breaks_page.mjs` | PASS: actual page connect, backup, restore and upload |
| `git diff --check` | PASS |

The new regressions and exact tested-file SHA-256 values are recorded in `evidence/installer-sample-safety-20261021/`. Existing suite results above were observed in this lane on the same production changes. Their raw output remains in the execution record; it was not recreated by repeating passing checks.

An attempted `node web/test_web.mjs` could not initialize because this checkout does not contain its required `web/editor.html`. It supplied no test result. The changed updater's normal FM-1 reference path, Pro/Elite paths, and cross-model resume refusal are covered by the new focused JS suite. Package/image parity requiring a target build is left to the integration owner.

The first browser installer test attempt hit the runtime's Node child-process restriction while generating a Python fixture. The same offline command passed in the permitted execution environment. All devices in these checks were simulated; no MIDI ports were opened.

## Integration

The sole receiving owner is the current root integration task. Register `installer-safety` and `installer-safety-web` in `tests/run_tests.sh`; that shared file was intentionally left to its owner. Merge this checked source, then perform the applicable target, artifact and installer-package checks in the integration lane.
