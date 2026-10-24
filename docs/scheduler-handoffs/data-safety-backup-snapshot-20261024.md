# Data Safety: refresh backup-derived migration decisions before OTA

**Consuming source**: firmware PR #103, `5493044eb663b931fddd13f847c370b6971df623`; **complete root** `1ca400ea04a7369c5e056ec80d35dc42c4a42855`; separately compiled `firmware/` subtree `d3b67bf127121acdbb2d056890b8c6987ffb363e`.

At `tools/fm1_install.py::SlotMove.backup`, the second SMP_INFO probe (inside `xs.backup`) and the actual backed-up images replaced `self.lay`, but the earlier `self.old_map` and `self.risk` remained cached from initial preflight. If a sample slot becomes populated or its flash layout changes between those probes, a valid archive can contain newly endangered samples that the post-OTA restore never selects. Conversely, if a previously endangered slot becomes empty, the stale plan can attempt to process `None`. This is a *possible software hazard*, not observed user hardware loss.

The minimal patch refreshes the old flash map and risk list from the exact layout whose sample images are archived, before any OTA write. It does not alter the firmware subtree or loosen archive validation, backups, wrong-model refusal or recovery guards. It cannot eliminate every possible hardware race after the verified snapshot.

## Reproduction — executed in local Linux/Python 3 source-matched fixture

| Command | Outcome |
|---|---|
| `python3 /mnt/data/x0x37-data-safety-snapshot/tests/fm1_install_backup_snapshot_test.py` against original `SlotMove` and exact original `flash_map/at_risk/plan_migration` windows | **exit 1, 4/7 fail** (expected fail-before) |
| Same command against patched `SlotMove` | **exit 0, 7/7 pass** |
| `python3 -O /mnt/data/x0x37-data-safety-snapshot/tests/fm1_install_backup_snapshot_test.py` against patched source | **exit 0, 7/7 pass** |
| `python3 -m py_compile /mnt/data/x0x37-data-safety-snapshot/tests/fm1_install_backup_snapshot_test.py` | **exit 0** |

Exact stdout/stderr log SHA256:
- before (exit 1): `253a983c8f11cb65839446ce382b93c240ea9608e5b235b8d546f8ec2cf28531`
- after (exit 0): `a324a31b7dce1460baeef4fbec4a60f47cb891938d280152deb264d3c6a43cea`
- optimized (exit 0): `2e7542c0ce5c053879460e4b6cec8d02bc1e4bef9e4cbce3d9f2a48cc554774f`

New test is registered as `installer-backup-snapshot`; it uses the real source AST windows with simulated SMP_INFO changes and restoration, not a complete linked install or physical device. Full-root native regression, independent QA, target binaries, hardware and HTTP were **NOT RUN** by this producer.

**Next**: Independent QA validates same full-root source and backup/restore/wrong-model/OTA/interruption negatives; Hourly Build can BUILD_ONLY Elite/Pro/controller immediately from the new exact head (confirm unchanged firmware subtree and external inputs); Safe Merge owns reconciliation into #103 through approved reviews. Catalog and publisher continue to block selection until prehardware safety/model/controller/integrity/source/license/HTTP gates pass. Prior safe live builds are retained.
