# Data Safety — preserve preinstall sample archive on save failure (2026-10-23)

**SOURCE-EXTRACTED HOST CHECKED / FULL-TREE INDEPENDENT QA PENDING / DEVICE NOT TESTED / NOT INSTALLABLE.**

Owner: Data Safety. Depends on unmerged Data Safety PR #81, HEAD `057009814e63dbb34b8f7f3de03aa9391bd00306` (tree `2eeff330c66d3e3f8da0473eef026bc0fdcfe5d3`). Firmware nightly source `58b5b45a2dd90581de7a674c331ca5cbe8a67c7e`; the helper is not yet integrated there.

## Original data-loss defect

`tools/fm1_install.py` original Git blob `77dbe2cfab79a78c953abf7c273dbf6156a35f7e`, method `SlotMove.backup()` used `self.path.write_bytes(arch)`. A backup path holding a valid user sample archive was truncated **before** completion. Disk-full, partial writes or interruption could destroy the previous backup even though the installer never reached the firmware update stage. Reusing a named `--backup` destination, colliding generated timestamps, and symlink destinations were also unsafe.

## Minimal change

Use the nonclobber, same-directory staged and fsynced `xs.save_backup_archive(self.path, arch)` introduced by dependency PR #81. It refuses an existing name, symlink or concurrent creator and checks archive integrity/readback before publishing. No direct on-device sample command or firmware layout changed. Failed backup stops install via the existing `InstallError("slots")` path before calling `up.install`. Users who need another backup should select a fresh filename. Do **not** squash/merge this standalone: #81's helper must be included first.

## Executed scoped Linux host checks

Affected `SlotMove.backup` source-window exact-match to remote Git blob `77dbe2cfab79a78c953abf7c273dbf6156a35f7e`; helper `save_backup_archive` body matched PR #81 remote blob `fea90b39c93283bdeb31f238e3a9af5dc1e5f280`. Test fixtures contain actual extracted installer method and actual helper with unrelated imports/dependencies mocked; **not** a complete Git checkout.

- Baseline: `python3 /mnt/data/datasafety_run5/tests/fm1_install_slot_backup_atomic_test.py /mnt/data/datasafety_run5/fixtures/before` — exit **1**; 11 run, **8 failure assertions**. `/mnt/data/datasafety_run5/logs/baseline.log` SHA-256 `63c149a9cb82798807893204544742c6fb4318f867b8e6a108267410a1206ef4`.
- Patched: `python3 /mnt/data/datasafety_run5/tests/fm1_install_slot_backup_atomic_test.py /mnt/data/datasafety_run5/fixtures/after` — exit **0, 11/11 PASS**; committed `docs/scheduler-handoffs/data-safety-install-backup-noclobber-20261023-patched.log` SHA-256 `fa5ea0be9e846cd879683fbbc766c40ee48dd505dfbb927f317a1cd1358eaf1f`.
- Syntax: `python3 -m py_compile /mnt/data/datasafety_run5/fixtures/after/fm1_install.py /mnt/data/datasafety_run5/fixtures/after/x0x_slots.py /mnt/data/datasafety_run5/tests/fm1_install_slot_backup_atomic_test.py` — exit **0**.
- Coverage: new good backup; preexisting/same-name refusal; race; symlink; invalid CRC; sample read error; directory; disk full partial write; fsync failure; staged readback corruption.
- **NOT RUN**: exact full PR tree `python3 tests/fm1_install_slot_backup_atomic_test.py`, complete install/slot tests, Elite/Pro target native builds, controller readback, physical device. Local shell GitHub DNS unavailable; connected GitHub API source integrity read works. No claim of complete-tree PASS, rollback of a partially erased device, or crash-durable directory fsync.

Next **Independent QA**: reconstruct exact stacked PR tree; rerun this committed test plus host slot/install tests and check PR81 dependency and POSIX hardlink portability, before **sole Safe Merge** integrates source. Hourly Build separately qualifies model-specific firmware/ctl binaries for NIGHTLY; this patch is not a published or approved firmware artifact.
