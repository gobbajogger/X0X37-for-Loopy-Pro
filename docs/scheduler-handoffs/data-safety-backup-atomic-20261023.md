# Data Safety: non-destructive sample-backup file publication (2026-10-23)

Status: **SOURCE ONLY / TARGETED PYTHON HOST CHECKED / FULL-TREE QA PENDING / BUILD PENDING / DEVICE NOT TESTED**.
Producer: Data Safety. Next: Independent QA, then sole Safe Merge.

## Before/after
At firmware `nightly` baseline `c9718ae833eb64a5191c350922593df8a36017c9` (`tools/x0x_slots.py` Git blob `699096ff6a8b92054c0eb292f8f2ef25791063ac`), `backup FILE --force` ran `Path.write_bytes(archive)`: this truncates an existing good sample backup *before* it knows the replacement can be written. A partial-write/disk-full/interrupt exception can destroy the previous archive. The initial `Path.exists()` check also has a no-force concurrent-creator race.

The patch pre-validates the archive, writes to a private same-directory temporary file, flushes and fsyncs it, reads back its exact bytes, then atomically substitutes the destination only with `--force`. Without `--force`, hard-link publication fails closed if another file appeared during backup. Temporary files are removed after errors; an existing destination remains intact for simulated failing writes/rename/sync. The symlink destination points to a separate file left untouched by replacement.

Changed scope: `tools/x0x_slots.py`, `tests/x0x_slots_backup_atomic_test.py`; no device commands, UI, browser, SMC, sequencer, model/flash layouts, firmware binary or controller bytes. This is **not** crash/power-loss-durable directory metadata assurance; directory fsync is not implemented, and hard-link support on non-POSIX filesystems deserves independent QA portability review.

## Actual host regression evidence
Local runner: Linux x86_64 / Python 3; independent baseline Library file `/mnt/data/files/QA-NIGHTLY-PR71-tools--x0x_slots.py` Git blob `b0f1bc700e8774d712a0ed5bab0754159f48837b`. The complete affected original `backup` and CLI `main` function windows were independently text-compared equal to firmware `nightly` blob `699096ff6a8b92054c0eb292f8f2ef25791063ac`; unrelated sections differ. The local modified source was made from that Library file with the exact same patch as the actual remote `nightly` branch; remote patched blob `fea90b39c93283bdeb31f238e3a9af5dc1e5f280` was independently re-fetched.

- `python3 /mnt/data/datasafety_atomic_run4/x0x_slots_backup_atomic_test.py /mnt/data/files/QA-NIGHTLY-PR71-tools--x0x_slots.py` — **exit 1 (expected RED)**: 11 attempted, 10 errors due missing helper, 1 true destructive CLI partial-write failure. Log SHA256 `4dfc81ec3456aabfa6bad54839686a25d6d65c8cef23fabd303e8c3bcd84dbcf`. The observed pre-fix CLI destination differs from its formerly good backup after simulated write failure.
- `python3 /mnt/data/datasafety_atomic_run4/x0x_slots_backup_atomic_test.py /mnt/data/datasafety_atomic_run4/x0x_slots.py` — **exit 0, 11/11 PASS**, log Git path `docs/scheduler-handoffs/data-safety-backup-atomic-20261023-patched.log`, SHA256 `928458a5133e1c155207a114c461aca8e126cf95c1f5c1c369a6881a56fbe42f`.
- `python3 -m py_compile /mnt/data/datasafety_atomic_run4/x0x_slots.py /mnt/data/datasafety_atomic_run4/x0x_slots_backup_atomic_test.py` — **exit 0**.
- Regression positive/negative: valid new and forced replacement, default no-clobber, racing creator, fsync failure, os.replace failure, short temp write, readback mismatch, corrupt CRC, symlink target preservation and CLI interrupted/disk-full scenario.
- No physical keyboard or firmware build. Complete exact PR-tree native test still **NOT RUN** because native Git DNS clone failed; connected GitHub verified source/test blobs, but executable local file used a byte-identical affected-function subset of PR71 rather than complete current `nightly` checkout.

## Handoff / unrun gates
Independent QA: clone/reconstruct *the final remote PR head*, rerun `python3 tests/x0x_slots_backup_atomic_test.py` and `python3 tests/x0x_slots_test.py`, examine staging durability and no-clobber hard-link compatibility, then check interplay with data-safety PRs #56/#66/#75/#76 and controller-atomic PRs #42/#49. Only sole Safe Merge integrates firmware `nightly`; Hourly Build qualifies real Elite/Pro+ctl native DEV artifacts if/when build-eligible. Neither source tests nor this draft constitute installable firmware. Production `main`, all Pages, catalog, controller and device remain untouched.
